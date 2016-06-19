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
}

void FAURArucoTracker::SetSettings(FArucoTrackerSettings const& settings)
{
	this->Settings = settings;
}

void FAURArucoTracker::SetCameraProperties(FOpenCVCameraProperties const & camera_properties)
{
	this->CameraProperties = camera_properties;
}

bool FAURArucoTracker::DetectMarkers(cv::Mat& image, FTransform & out_camera_transform)
{
	if(!BoardData.IsValid())
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::DetectMarkers: no board definition, call UpdateBoardDefinition"));
		return false;
	}

	// Translation and rotation reported by detector
	cv::Vec3d rotation_raw, translation_raw;

	// http://docs.opencv.org/3.1.0/db/da9/tutorial_aruco_board_detection.html
	try
	{
		cv::aruco::detectMarkers(image, BoardData->GetArucoDictionary(),
			*FoundMarkerCorners, *FoundMarkerIds);

		// we cannot see any markers at all
		if (FoundMarkerIds->size() <= 0)
		{
			return false;
		}

		if (Settings.bDisplayMarkers)
		{
			cv::aruco::drawDetectedMarkers(image, *FoundMarkerCorners, *FoundMarkerIds);
		}

		// determine translation and rotation + write to output
		int number_of_markers = cv::aruco::estimatePoseBoard(*FoundMarkerCorners, *FoundMarkerIds,
			BoardData->GetArucoBoard(), CameraProperties.CameraMatrix, CameraProperties.DistortionCoefficients,
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
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("Exception in ArucoWrapper::DetectMarkers:\n	%s\n"), UTF8_TO_TCHAR(exc.what()))
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
	ue_translation *= -1.0 / Settings.TranslationScale;

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
	if (!board_definition_object)
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::UpdateBoardDefinition board_definition_object is null"));
		return;
	}

	BoardData = board_definition_object->GetBoardData();
}
