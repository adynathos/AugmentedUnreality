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
	this->SetSettings(this->Settings);
}

void FAURArucoTracker::SetSettings(FArucoTrackerSettings const& settings)
{
	this->Settings = settings;
	this->UpdateMarkerDefinition(this->Settings.BoardDefinition);
	this->ArucoAPI.SetDisplayMarkers(settings.bDisplayMarkers);
}

void FAURArucoTracker::SetCameraProperties(FOpenCVCameraProperties const & camera_properties)
{
	//this->CameraProperties = camera_properties;
	this->ArucoAPI.SetCameraProperties((camera_properties.CameraMatrix), (camera_properties.DistortionCoefficients));
}

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
}

void FAURArucoTracker::ConvertTransformToUnreal(cv::Vec3d const& opencv_translation, cv::Vec3d const& opencv_rotation, FTransform & out_transform) const
{
	FVector rotation_axis = ConvertOpenCvVectorToUnreal(opencv_rotation);
	FVector cv_translation = ConvertOpenCvVectorToUnreal(opencv_translation);

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

void FAURArucoTracker::UpdateMarkerDefinition(FArucoGridBoardDefinition const & definition)
{
	ArucoAPI.SetMarkerDefinition(definition.DictionaryId, definition.GridWidth, definition.GridHeight, definition.MarkerSize, definition.SeparationSize);

	// save image to file
	FString board_image_filename = FPaths::GameSavedDir() / definition.SavedFileName;

	// ensure directory exists
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FPaths::GetPath(board_image_filename));

	cv::imwrite(TCHAR_TO_UTF8(*board_image_filename), ArucoAPI.GetMarkerImage());
	UE_LOG(LogAUR, Log, TEXT("AURArucoTracker: Saved marker image to: %s"), *board_image_filename);
}
