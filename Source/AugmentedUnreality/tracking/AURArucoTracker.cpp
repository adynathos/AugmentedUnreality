/*
Copyright 2016-2017 Krzysztof Lis

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
#include <functional>
#define _USE_MATH_DEFINES
#include <math.h>

/*
OpenCV's rotation is
from a coord system with XY on the marker plane and Z upwards from the table
to a coord system such that camera is looking forward along the Z axis.

But UE's cameras look towards the X axis.
So after the main rotation we apply a rotation that moves
the Z axis onto the X axis.
(-90 deg around Y axis)
*/
const FTransform FAURArucoTracker::CameraAdditionalRotation = FTransform(FQuat(FVector(0, 1, 0), -M_PI / 2), FVector(0, 0, 0), FVector(1, 1, 1));

FAURArucoTracker::FAURArucoTracker()
	: ViewpointPoseDetectedOnLastTick(false)
	, ViewpointTransform(FTransform::Identity)
{
	ViewpointTransformCamera = CameraAdditionalRotation * ViewpointTransform;

	cv::aur::setLogCallback([](cv::aur::LogLevel level, std::string const& msg) {
		switch(level)
		{
			case cv::aur::LogLevel::Log:
				UE_LOG(LogAUR, Log, TEXT("cv::aur log: %s"), UTF8_TO_TCHAR(msg.c_str()));
				break;
			case cv::aur::LogLevel::Warning:
				UE_LOG(LogAUR, Warning, TEXT("cv::aur warning: %s"), UTF8_TO_TCHAR(msg.c_str()));
				break;
			case cv::aur::LogLevel::Error:
				UE_LOG(LogAUR, Error, TEXT("cv::aur error: %s"), UTF8_TO_TCHAR(msg.c_str()));
				break;
			default:
				;
		}
	});

}

void FAURArucoTracker::SetSettings(FArucoTrackerSettings const& settings)
{
	this->Settings = settings;
}

void FAURArucoTracker::SetCameraProperties(FOpenCVCameraProperties const & camera_properties)
{
	this->CameraProperties = camera_properties;
	TrackerModule.setCameraInfo(camera_properties.CameraMatrix, camera_properties.DistortionCoefficients);
}

bool FAURArucoTracker::DetectMarkers(cv::Mat_<cv::Vec3b>& image, bool draw_found_markers)
{
	// this outside of lock so doesn't block
	TrackerModule.processFrame(image);

	{
		FScopeLock lock(&PoseLock);

		DetectedBoards.Empty();
		for (auto detected_pose : TrackerModule.getDetectedPoses())
		{
			TrackedBoardInfo* tbi = (TrackedBoardInfo*)detected_pose->userObject;

			// Write the projection matrix to Unreal's datastructures
			FMatrix t_mat;
			t_mat.SetIdentity();

			// Unreal's projection matrices are for some reason transposed from the traditional representation
			// so we index [c][r] to contruct that transposed mat

			cv::Mat_<double> const& detected_rot = detected_pose->getRotationCameraUnreal();
			for (int32 r : {0, 1, 2}) for (int32 c : {0, 1, 2})
			{
				t_mat.M[c][r] = detected_rot(r, c);
			}

			cv::Mat_<double> const& detected_trans = detected_pose->getTranslationCameraUnreal();
			for (int32 r : {0, 1, 2})
			{
				t_mat.M[3][r] = detected_trans(r);
			}

			if (!t_mat.ContainsNaN())
			{
				const float blend_factor = (1.0 - Settings.SmoothingStrength);
				FTransform detected_transform(t_mat);

				if (tbi->UseAsViewpointOrigin)
				{
					
					if (tbi->BoardActor)
					{
						// Transforms are from right to left:
						// - outer: board actor transform => but board actor in center
						// - middle: camera transform from AR marker => move from board to camera looking at board
						// - inner: rotate to look forward 
						detected_transform *= tbi->BoardActor->GetActorTransform();
					}

					// Blend the viewport transform
					ViewpointTransform.BlendWith(detected_transform, blend_factor);
					ViewpointPoseDetectedOnLastTick = true;
				}
				else
				{
					// Smoothly merge measured transform with current transform
					tbi->CurrentTransform.BlendWith(detected_transform, blend_factor);
					DetectedBoards.Add(tbi);
				}			
			}
			else
			{
				UE_LOG(LogAUR, Warning, TEXT("Wrong transform matrix %s"), *t_mat.ToString());
			}
		}
	}

	return true;
}

void FAURArucoTracker::PublishTransformUpdate(TrackedBoardInfo * tracking_info)
{
	auto board_actor = tracking_info->BoardActor;

	if (board_actor)
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
			tracking_info->CurrentTransform.GetRelativeTransformReverse(ViewpointTransform)
		);
	}
}

bool FAURArucoTracker::RegisterBoard(AAURFiducialPattern* board_actor, bool use_as_viewpoint_origin)
{
	if (!board_actor)
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::RegisterBoard board_actor is null"));
		return false;
	}

	// Save marker images
	board_actor->SaveMarkerFiles();

	UE_LOG(LogAUR, Log, TEXT("AURArucoTracker::RegisterBoard %s"), *AActor::GetDebugName(board_actor));
	cv::aur::TrackedPose* pose_handle = TrackerModule.registerPoseToTrack(board_actor->GetPatternDefinition());

	if(!pose_handle)
	{
		return false;
	}

	// Construct and add to map
	TrackedBoardInfo* tracker_info = new TrackedBoardInfo(board_actor, pose_handle);
	pose_handle->userObject = tracker_info;
	tracker_info->UseAsViewpointOrigin = use_as_viewpoint_origin;

	// keep the shared ptr here
	TrackedBoardsById.Emplace(tracker_info->Id, tracker_info);

	board_actor->SetActorHiddenInGame(!BoardVisibility);

	return true;
}

void FAURArucoTracker::UnregisterBoard(AAURFiducialPattern* board_actor)
{
	int board_id = board_actor->GetPatternDefinition()->getMinMarkerId();

	if (!TrackedBoardsById.Contains(board_id))
	{
		UE_LOG(LogAUR, Error, TEXT("AURArucoTracker::UnregisterBoard Board with id %d is not registered"),
			board_id);
		return;
	}

	TrackedBoardInfo* tracking_info = TrackedBoardsById[board_id].Get();

	if(tracking_info->PoseHandle)
	{
		tracking_info->PoseHandle->unregister();
	}

	// Since this object will be deleted, remove it from being potentially used in Publish
	DetectedBoards.Remove(tracking_info);

	// Remove the unique ptr and also delete object
	TrackedBoardsById.Remove(board_id);
}

void FAURArucoTracker::PublishTransformUpdatesOnTick(UAURDriver* driver_instance)
{
	FScopeLock lock(&PoseLock);

	if (ViewpointPoseDetectedOnLastTick)
	{
		if (driver_instance)
		{
			driver_instance->OnViewpointTransformUpdate.Broadcast(
				driver_instance, 
				CameraAdditionalRotation * ViewpointTransform // rotate so camera looks forward
			);
		}

		ViewpointPoseDetectedOnLastTick = false;
	}

	for (auto detected_board_info : DetectedBoards)
	{
		PublishTransformUpdate(detected_board_info);
	}

	DetectedBoards.Empty();
}

void FAURArucoTracker::SetDiagnosticInfoLevel(EAURDiagnosticInfoLevel NewLevel)
{
	/**
	Diagnostic levels
		0 - nothing
		1 - show boards
		2 - show boards and positions of detected markers
	*/
	SetBoardVisibility(NewLevel >= EAURDiagnosticInfoLevel::AURD_Basic);

	// Pass to cv::aur module
	cv::aur::DiagnosticLevel lvl = cv::aur::DiagnosticLevel::Silent;
	if(NewLevel == EAURDiagnosticInfoLevel::AURD_Basic) lvl = cv::aur::DiagnosticLevel::Basic;
	else if (NewLevel == EAURDiagnosticInfoLevel::AURD_Advanced) lvl = cv::aur::DiagnosticLevel::Full;

	TrackerModule.setDiagnosticLevel(lvl);
}

void FAURArucoTracker::SetBoardVisibility(bool NewBoardVisibility)
{
	BoardVisibility = NewBoardVisibility;

	for (auto& bi : TrackedBoardsById)
	{
		bi.Value->BoardActor->SetActorHiddenInGame(!BoardVisibility);
	}
}
