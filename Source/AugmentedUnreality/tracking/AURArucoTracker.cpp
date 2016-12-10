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

const FTransform FAURArucoTracker::CameraAdditionalRotation = FTransform(FQuat(FVector(0, 1, 0), -M_PI / 2), FVector(0, 0, 0), FVector(1, 1, 1));

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

bool FAURArucoTracker::DetectMarkers(cv::Mat& image, bool draw_found_markers)
{
	// http://docs.opencv.org/3.1.0/db/da9/tutorial_aruco_board_detection.html
#if !PLATFORM_ANDROID
	try
	{
#endif
		cv::aruco::detectMarkers(image, ArucoDictionary,
			*FoundMarkerCorners, *FoundMarkerIds);

		// we cannot see any markers at all
		const size_t found_marker_count = FoundMarkerIds->size();
		if (found_marker_count <= 0)
		{
			return false;
		}

		if (draw_found_markers)
		{
			cv::aruco::drawDetectedMarkers(image, *FoundMarkerCorners, *FoundMarkerIds);
		}

		DetectedBoards.Empty();
		for(size_t idx = 0; idx < found_marker_count; idx++)
		{
			int found_marker_id = FoundMarkerIds->at(idx);

			TrackedBoardInfo* tracker_info = TrackedBoardsByMarker.FindRef(found_marker_id);
			if (tracker_info)
			{
				DetectedBoards.Add(tracker_info);
				tracker_info->FoundMarkerIds.push_back(found_marker_id);
				tracker_info->FoundMarkerCorners.push_back(FoundMarkerCorners->at(idx));
			}
		}

		for (TrackedBoardInfo* detected_board_info : DetectedBoards)
		{
			if (detected_board_info != nullptr)
			{
				DetermineBoardPosition(detected_board_info);
			}
			else
			{
				UE_LOG(LogAUR, Error, TEXT("detected_board_info is null"))
			}
		}

		/*
		if (draw_found_markers)
		{
			cv::aruco::drawAxis(image, CameraProperties.CameraMatrix, CameraProperties.DistortionCoefficients,
				rotation_raw, translation_raw, 10);
		}
		*/
#if !PLATFORM_ANDROID
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("Exception in ArucoWrapper::DetectMarkers:\n	%s\n"), UTF8_TO_TCHAR(exc.what()))
		return false;
	}
#endif

	return true;
}

void FAURArucoTracker::DetermineBoardPosition(TrackedBoardInfo * tracking_info)
{
	//UE_LOG(LogAUR, Log, TEXT("Determine %d"), tracking_info->Id);

	// Translation and rotation reported by detector
	cv::Vec3d rotation_raw, translation_raw;

	int number_of_markers = cv::aruco::estimatePoseBoard(tracking_info->FoundMarkerCorners, tracking_info->FoundMarkerIds,
		tracking_info->BoardData->GetArucoBoard(), CameraProperties.CameraMatrix, CameraProperties.DistortionCoefficients,
		rotation_raw, translation_raw);

	tracking_info->FoundMarkerIds.clear();
	tracking_info->FoundMarkerCorners.clear();
	
	// if markers fit
	if (number_of_markers > 0)
	{
		FTransform measured_transform;
		ConvertTransformToUnreal(translation_raw, rotation_raw, measured_transform, tracking_info->UseAsViewpointOrigin);

		// Smoothly merge measured transform with current transform
		tracking_info->CurrentTransform.BlendWith(measured_transform, (1.0 - Settings.SmoothingStrength));

		// This board provides the location of the camera
		if (tracking_info->UseAsViewpointOrigin)
		{
			ViewpointTransform = tracking_info->CurrentTransform;
			//ViewpointTransformChanged = true;
		}
	}
}

void FAURArucoTracker::PublishTransformUpdate(TrackedBoardInfo * tracking_info)
{
	auto board_actor = tracking_info->BoardData->GetBoardActor();

	if (board_actor)
	{
		if (tracking_info->UseAsViewpointOrigin)
		{
			/*
			OpenCV's rotation is
			from a coord system with XY on the marker plane and Z upwards from the table
			to a coord system such that camera is looking forward along the Z axis.

			But UE's cameras look towards the X axis.
			So after the main rotation we apply a rotation that moves
			the Z axis onto the X axis.
			(-90 deg around Y axis)
			*/
			// Apply the rotation first, so that it does not apply to the translation in CurrentTransform
			board_actor->TransformMeasured(
				CameraAdditionalRotation * tracking_info->CurrentTransform,
				tracking_info->UseAsViewpointOrigin);
		}
		else
		{
			// Transforms measured by the tracker = positions of the camera looking at the board:
			// transform_tracker_viewpoint = camera pos 1
			// transform_tracker_object = camera pos 2

			// The camera is in the same place - but the object is moved:
			// transform_tracker_object(object_pos) = camera pos 1
			// object_pos = transform_tracker_object.inverse (camera pos 1)
			// object_pos = transform_tracker_object.inverse (transform_tracker_viewpoint)

			// "GetRelativeTransformReverse returns this(-1)*Other, and parameter is Other."

			board_actor->TransformMeasured(
				tracking_info->CurrentTransform.GetRelativeTransformReverse(ViewpointTransform),
				tracking_info->UseAsViewpointOrigin);
		}
	}
}

void FAURArucoTracker::ConvertTransformToUnreal(cv::Vec3d const& opencv_translation, cv::Vec3d const& opencv_rotation, FTransform & out_transform, bool camera_viewpoint) const
{
	FVector rotation_axis = FAUROpenCV::ConvertOpenCvVectorToUnreal(opencv_rotation);
	FVector cv_translation = FAUROpenCV::ConvertOpenCvVectorToUnreal(opencv_translation);

	// cv_rotation = R^-1? because other direction of angle in UE4?
	float angle = rotation_axis.Size();
	rotation_axis.Normalize();
	FQuat cv_rotation(rotation_axis, angle);

	// The reported translation vector "translation_vec" is the marker location
	// relative to camera in camera's coordinate system.
	// To obtain camera position in 3D, rotate by the provided rotation.
	FVector ue_translation = cv_rotation.RotateVector(cv_translation);
	ue_translation *= -1.0 / Settings.TranslationScale;

	out_transform.SetComponents(cv_rotation, ue_translation, FVector(1, 1, 1));
}

bool FAURArucoTracker::RegisterBoard(AAURMarkerBoardDefinitionBase* board_actor, bool use_as_viewpoint_origin)
{
	if (!board_actor)
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::RegisterBoard board_actor is null"));
		return false;
	}

	TSharedPtr<FFreeFormBoardData> board_data = board_actor->GetBoardData();

	// Save marker images
	board_actor->SaveMarkerFiles();

	if (TrackedBoardsById.Num() > 0)
	{
		// If we already have boards registered, the new board's dictionary must be the same
		if(board_data->GetArucoDictionaryId() != DictionaryId)
		{
			UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::RegisterBoard New board uses dictionary %d but existing boards use dictionary %d"),
				board_data->GetArucoDictionaryId(), DictionaryId);
			return false;
		}

		// The marker IDs must be kept unique - if we detect a marker we want to know which board it is from
		for (const int marker_id : board_data->GetMarkerIds())
		{
			if (TrackedBoardsByMarker.Contains(marker_id))
			{
				UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::RegisterBoard Marker Id %d is used by another registered board"),
					marker_id);
				return false;
			}
		}
	}
	else
	{
		// That is the first board so set the dictionary to match it
		DictionaryId = board_data->GetArucoDictionaryId();
		ArucoDictionary = board_data->GetArucoDictionary();
	}

	// Construct and add to map
	TrackedBoardInfo* tracker_info = new TrackedBoardInfo(board_data);
	tracker_info->UseAsViewpointOrigin = use_as_viewpoint_origin;

	TrackedBoardsById.Emplace(tracker_info->Id, tracker_info);

	for (const int marker_id : board_data->GetMarkerIds())
	{
		TrackedBoardsByMarker.Add(marker_id, tracker_info);
	}

	board_actor->SetActorHiddenInGame(!BoardVisibility);

	return true;
}

void FAURArucoTracker::UnregisterBoard(AAURMarkerBoardDefinitionBase * board_actor)
{
	int board_id = board_actor->GetBoardData()->GetMinMarkerId();

	if (!TrackedBoardsById.Contains(board_id))
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::UnregisterBoard Board with id %d is not registered"),
			board_id);
		return;
	}

	TrackedBoardInfo* tracking_info = TrackedBoardsById[board_id].Get();

	// unregister all marker ids
	for (const int marker_id : tracking_info->BoardData->GetMarkerIds())
	{
		TrackedBoardsByMarker.Remove(marker_id);
	}

	// Since this object will be deleted, remove it from being potentially used in Publish
	DetectedBoards.Remove(tracking_info);

	// Remove the unique ptr and also delete object
	TrackedBoardsByMarker.Remove(board_id);
}

void FAURArucoTracker::PublishTransformUpdatesOnTick()
{
	for (auto detected_board_info : DetectedBoards)
	{
		PublishTransformUpdate(detected_board_info);
	}

	DetectedBoards.Empty();
}

void FAURArucoTracker::SetBoardVisibility(bool NewBoardVisibility)
{
	BoardVisibility = NewBoardVisibility;

	for (auto& bi : TrackedBoardsByMarker)
	{
		bi.Value->BoardData->GetBoardActor()->SetActorHiddenInGame(!BoardVisibility);
	}
}
