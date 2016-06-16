/*
Copyright 2016 Krzysztof Lis

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "AugmentedUnreality.h"
#include "AURArucoTracker.h"

#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

FAURArucoTracker::FAURArucoTracker()
{
	//this->SetSettings(this->Settings);
}

void FAURArucoTracker::SetSettings(FArucoTrackerSettings const& settings)
{
	this->Settings = settings;
	//this->UpdateMarkerDefinition(this->Settings.BoardDefinition);

	/*
	if (Settings.BoardDefinitionClass)
	{
		this->UpdateBoardDefinition(Settings.BoardDefinitionClass.GetDefaultObject());
	}
	else
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker No board definition"));
	}
	*/
}

void FAURArucoTracker::SetCameraProperties(FOpenCVCameraProperties const & camera_properties)
{
	this->CameraProperties = camera_properties;
	//this->ArucoAPI.SetCameraProperties((camera_properties.CameraMatrix), (camera_properties.DistortionCoefficients));
}

/**
bool FAURArucoTracker::DetectMarkers(cv::Mat& image, FTransform & out_camera_transform)
{
	// Translation and rotation reported by detector
	cv::Vec3d rotation_raw, translation_raw;

	bool found = ArucoAPI.DetectMarkers(image, translation_raw, rotation_raw);
	if(!found)
	{
		return false;
	}

	// calculate camera translation
	ConvertTransformToUnreal(translation_raw, rotation_raw, out_camera_transform);

	return true;
}*/

bool FAURArucoTracker::DetectMarkers(cv::Mat& image, FTransform & out_camera_transform)
{
/*
	if(!Board)
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::DetectMarkers: no board definition"));
		return false;
	}
*/
	// Translation and rotation reported by detector
	cv::Vec3d rotation_raw, translation_raw;

	// http://docs.opencv.org/3.1.0/db/da9/tutorial_aruco_board_detection.html
	try
	{
		auto MarkerCorners = cv::aur_allocator::allocate_vector2_Point2f();
		auto MarkerIds = cv::aur_allocator::allocate_vector_int();

		cv::aruco::detectMarkers(image, cv::aruco::getPredefinedDictionary(cv::aruco::PREDEFINED_DICTIONARY_NAME(Board.DictionaryId)),
			*MarkerCorners, *MarkerIds);

		// we cannot see any markers at all
		if (MarkerIds->size() <= 0)
		{
			return false;
		}

		if (Settings.bDisplayMarkers)
		{
			cv::aruco::drawDetectedMarkers(image, *MarkerCorners, *MarkerIds);
		}

		// determine translation and rotation + write to output
		int number_of_markers = cv::aruco::estimatePoseBoard(*MarkerCorners, *MarkerIds,
			Board.GetArucoBoard(), CameraProperties.CameraMatrix, CameraProperties.DistortionCoefficients,
			rotation_raw, translation_raw);

		// the markers do not fit
		if (number_of_markers <= 0)
		{
			return false;
		}

		if (Settings.bDisplayMarkers)
		{
			cv::aruco::drawAxis(image, CameraProperties.CameraMatrix, CameraProperties.DistortionCoefficients,
				rotation_raw, translation_raw, 10);
		}

		cv::aur_allocator::external_delete(MarkerCorners);
		cv::aur_allocator::external_delete(MarkerIds);
	}
	catch (std::exception& exc)
	{
		std::cout << "Exception in ArucoWrapper::DetectMarkers:\n" << exc.what() << '\n';
		return false;
	}

	// calculate camera translation
	ConvertTransformToUnreal(translation_raw, rotation_raw, out_camera_transform);

	return true;
}

void FAURArucoTracker::ConvertTransformToUnreal(cv::Vec3d const& opencv_translation, cv::Vec3d const& opencv_rotation, FTransform & out_transform) const
{
	FVector rotation_axis = FAUROpenCV::ConvertOpenCvVectorToUnreal(opencv_rotation);
	FVector cv_translation = FAUROpenCV::ConvertOpenCvVectorToUnreal(opencv_translation);

	float angle = rotation_axis.Size();
	rotation_axis.Normalize();
	FQuat cv_rotation(rotation_axis, angle);

	// The reported translation vector "translation_vec" is the marker location
	// relative to camera in camera's coordinate system.
	// To obtain camera position in 3D, rotate by the provided rotation.
	FVector ue_translation = cv_rotation.RotateVector(cv_translation);
	ue_translation *= -1;
	ue_translation -= Settings.SceneCenterInTrackerCoordinates;
	ue_translation /= Settings.TranslationScale;

	/*
	OpenCV's rotation is
	from a coord system with XY on the marker plane and Z upwards from the table
	to a coord system such that camera is looking forward along the Z axis.

	But UE's cameras look towards the X axis.
	So after the main rotation we apply a rotation that moves
	the Z axis onto the X axis.
	(-90 deg around Y axis)
	*/
	FQuat ue_rotation = cv_rotation * FQuat(FVector(0, 1, 0), -M_PI / 2);

	// return the calculated transform
	out_transform.SetComponents(ue_rotation, ue_translation, FVector(1, 1, 1));
}

/*
void FAURArucoTracker::UpdateMarkerDefinition(FArucoGridBoardDefinition const & definition)
{
	ArucoAPI.SetMarkerDefinition(definition.DictionaryId, definition.GridWidth, definition.GridHeight, definition.MarkerSize, definition.SeparationSize);

	// save image to file
	FString board_image_dir = FPaths::GameSavedDir() / definition.SavedFileDir;

	// ensure directory exists
	//FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FPaths::GetPath(board_image_filename));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*board_image_dir);

	size_t marker_count = ArucoAPI.GetMarkerCount();
	for (int idx = 0; idx < marker_count; idx++)
	{
		FString filename = board_image_dir / "marker_" + FString::FromInt(idx) + ".png";
		cv::imwrite(TCHAR_TO_UTF8(*filename), ArucoAPI.GetMarkerImage(idx));
		UE_LOG(LogAUR, Log, TEXT("AURArucoTracker: Saved marker image to: %s"), *filename);
	}
}
*/

void FAURArucoTracker::UpdateBoardDefinition(AAURMarkerBoardDefinitionBase * board_definition_object)
{
	UE_LOG(LogAUR, Log, TEXT("AURArucoTracker::UpdateBoardDefinition"));

	if (!board_definition_object)
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::UpdateBoardDefinition board_definition_object is null"));
		return;
	}

	board_definition_object->WriteToBoard(&Board);

	FString board_image_dir = FPaths::GameSavedDir() / board_definition_object->SavedFileDir;
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*board_image_dir);

	/*
	try
	{
		size_t marker_count = board->GetMarkerImages().size();
		for (int idx = 0; idx < marker_count; idx++)
		{
			FString filename = board_image_dir / "marker_" + FString::FromInt(idx) + ".png";
			cv::imwrite(TCHAR_TO_UTF8(*filename), board->GetMarkerImages()[idx]);
			UE_LOG(LogAUR, Log, TEXT("AURArucoTracker: Saved marker image to: %s"), *filename);
		}
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::UpdateBoardDefinition: exception when saving\n    %s"), UTF8_TO_TCHAR(exc.what()))
	}
	*/
}
