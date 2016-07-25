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

#pragma once

#include "AUROpenCVCalibration.h"
#include "AUROpenCV.h"
#include "AURMarkerBoardDefinitionBase.h"

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

	// Whether the marker outlines should be displayed on the screen
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	uint32 bDisplayDetectedMarkers : 1;

	// Value in range (0, 1), the higher, the more smoothed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	float SmoothingStrength;

	FArucoTrackerSettings()
		: TranslationScale(1.0)
		, bDisplayDetectedMarkers(false)
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
		
		// 
		bool UseAsViewpointOrigin;

		TSharedPtr<FFreeFormBoardData> BoardData;

		FTransform CurrentTransform;

		std::vector<int> FoundMarkerIds;
		std::vector< std::vector<cv::Point2f> > FoundMarkerCorners;

		TrackedBoardInfo(TSharedPtr<FFreeFormBoardData> board_data) 
			: Id(board_data->GetMinMarkerId())
			, UseAsViewpointOrigin(false)
			, BoardData(board_data)
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
		return ViewpointTransform;
	}

	/* 
		Calculate camera's position/rotation relative to the markers
		Returns true if any markers were detected
	*/
	bool DetectMarkers(cv::Mat& image);

	// Start tracking a board
	bool RegisterBoard(AAURMarkerBoardDefinitionBase* board_actor, bool use_as_viewpoint_origin = false);

	// Stop tracking a board
	void UnregisterBoard(AAURMarkerBoardDefinitionBase* board_actor);

	/*
		Tell the board actors about newly detected positions.
		Should run in game thread.
	*/
	void PublishTransformUpdatesOnTick();

private:
	FArucoTrackerSettings Settings;

	// Marker information
	// Collection of all boards to track
	TMap<int, TUniquePtr<TrackedBoardInfo>> TrackedBoardsById;

	TMap<int, TrackedBoardInfo*> TrackedBoardsByMarker;

	// Set of boards for which a new position was measured
	TSet<TrackedBoardInfo*> DetectedBoards;

	TSet<int> DetectedBoardIds;

	FTransform ViewpointTransform;
	//bool ViewpointTransformChanged;

	//TSharedPtr<FFreeFormBoardData> BoardData;
	FOpenCVCameraProperties CameraProperties;

	// Dictionary - set of markers to be detected (all boards must use markers from the same dictionary)
	uint32_t DictionaryId;

	/**
	A full copy of the dictionary returned by cv::ArUco is stored here
	so that we avoid the crash-inducing Ptr<Dictionary>
	**/
	cv::aruco::Dictionary ArucoDictionary;

	void DetermineBoardPosition(TrackedBoardInfo* tracking_info);

	void PublishTransformUpdate(TrackedBoardInfo* tracking_info);

	void ConvertTransformToUnreal(cv::Vec3d const& opencv_translation, cv::Vec3d const& opencv_rotation, FTransform & out_transform, bool camera_viewpoint) const;

	// OpenCV writes directoy to those vectors, so they need to be allocated/deleted outside AUR binary
	cv::aur_allocator::OpenCvWrapper< std::vector<int> > FoundMarkerIds;
	// OpenCV writes directoy to those vectors, so they need to be allocated/deleted outside AUR binary
	cv::aur_allocator::OpenCvWrapper< std::vector< std::vector<cv::Point2f> > > FoundMarkerCorners;

	// Creates a default aruco board and saves a copy to a file.
	//void UpdateMarkerDefinition(FArucoGridBoardDefinition const & BoardDefinition);

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
