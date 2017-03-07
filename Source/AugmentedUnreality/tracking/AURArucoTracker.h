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

#pragma once

#include "AUROpenCVCalibration.h"
#include "AUROpenCV.h"
#include "AURFiducialPattern.h"
#include "AURDriver.h"

#include "AURArucoTracker.generated.h"

USTRUCT(BlueprintType)
struct FArucoTrackerSettings
{
	GENERATED_BODY()

	/**
	*	Scale of the tracking coordinates:
	*	unreal coords / tracking coords
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	float TranslationScale;

	// Value in range (0, 1), the higher, the more smoothed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	float SmoothingStrength;

	FArucoTrackerSettings()
		: TranslationScale(1.0)
		, SmoothingStrength(0.5)
	{
	}
};

/*
	Tracking of Aruco markers:
	http://docs.opencv.org/3.1.0/db/da9/tutorial_aruco_board_detection.html
*/
class FAURArucoTracker
{
public:

	struct TrackedBoardInfo {
		// Tracked boards are identified by the lowest ID of their markers (marker IDs are unique)
		int32 Id;

		AAURFiducialPattern* BoardActor;

		cv::aur::TrackedPose* PoseHandle;

		FTransform CurrentTransform;
		//
		bool UseAsViewpointOrigin;

		TrackedBoardInfo(AAURFiducialPattern* board_actor, cv::aur::TrackedPose* pose)
			: Id(pose->getPoseId())
			, BoardActor(board_actor)
			, PoseHandle(pose)
			, CurrentTransform(FTransform::Identity)
			, UseAsViewpointOrigin(false)
		{
		}
	};

	FAURArucoTracker();

	FArucoTrackerSettings const& GetSettings()
	{
		return this->Settings;
	}

	void SetSettings(FArucoTrackerSettings const& settings);

	void SetCameraProperties(FOpenCVCameraProperties const& camera_properties);

	// Returns the position of the camera (but with Z axis pointing toward the markers)
	FTransform const& GetViewpointTransform() const
	{
		return ViewpointTransformCamera;
	}

	/*
		Calculate camera's position/rotation relative to the markers
		Returns true if any markers were detected
	*/
	bool DetectMarkers(cv::Mat_<cv::Vec3b>& image, bool draw_found_markers = false);

	// Start tracking a board
	bool RegisterBoard(AAURFiducialPattern* board_actor, bool use_as_viewpoint_origin = false);

	// Stop tracking a board
	void UnregisterBoard(AAURFiducialPattern* board_actor);

	/*
		Tell the board actors about newly detected positions.
		Should run in game thread.
	*/
	void PublishTransformUpdatesOnTick(UAURDriver* driver_instance);

	void SetDiagnosticInfoLevel(EAURDiagnosticInfoLevel NewLevel);
	void SetBoardVisibility(bool NewBoardVisibility);

private:
	FArucoTrackerSettings Settings;

	cv::aur::FiducialTracker TrackerModule;

	bool BoardVisibility;

	FCriticalSection PoseLock;

	// Marker information
	// Collection of all boards to track
	TMap<int, TUniquePtr<TrackedBoardInfo>> TrackedBoardsById;

	// Set of boards for which a new position was measured
	TArray<TrackedBoardInfo*> DetectedBoards;
	bool ViewpointPoseDetectedOnLastTick;

	FTransform ViewpointTransform;
	FTransform ViewpointTransformCamera;

	FOpenCVCameraProperties CameraProperties;

	void PublishTransformUpdate(TrackedBoardInfo* tracking_info);

	/*
	OpenCV's rotation is
	from a coord system with XY on the marker plane and Z upwards from the table
	to a coord system such that camera is looking forward along the Z axis.

	But UE's cameras look towards the X axis.
	So after the main rotation we apply a rotation that moves
	the Z axis onto the X axis.
	(-90 deg around Y axis)
	*/
	static const FTransform CameraAdditionalRotation;
};
