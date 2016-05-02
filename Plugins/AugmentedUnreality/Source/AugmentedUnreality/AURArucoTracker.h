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
#include "OpenCV_includes.h"
#include <OpenCVWrapper.h>

#include "AURArucoTracker.generated.h"

USTRUCT(BlueprintType)
struct FArucoGridBoardDefinition
{
	GENERATED_BODY()

	// Where to store the marker image, relative to FPaths::GameSavedDir()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	FString SavedFileName;

	// Size of the grid in X direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	int32 GridWidth;

	// Size of the grid in Y direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	int32 GridHeight;

	// Size of the marker in pixels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	int32 MarkerSize;
	
	// Space between markers in pixels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	int32 SeparationSize;

	/**
	Id of the predefined marker dictionary. Choices:
	DICT_4X4_50 = 0,
	DICT_4X4_100,
	DICT_4X4_250,
	DICT_4X4_1000,
	DICT_5X5_50,
	DICT_5X5_100,
	DICT_5X5_250,
	DICT_5X5_1000,
	DICT_6X6_50,
	DICT_6X6_100,
	DICT_6X6_250,
	DICT_6X6_1000,
	DICT_7X7_50,
	DICT_7X7_100,
	DICT_7X7_250,
	DICT_7X7_1000,
	DICT_ARUCO_ORIGINAL
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	int32 DictionaryId;

	FArucoGridBoardDefinition()
		: SavedFileName("AugmentedUnreality/Markers/marker.png")
		, GridWidth(1)
		, GridHeight(2)
		, MarkerSize(400)
		, SeparationSize(100)
		, DictionaryId(1)
	{
	}
};

USTRUCT(BlueprintType)
struct FArucoTrackerSettings
{
	GENERATED_BODY()

	// Parameters of the marker image to use.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	FArucoGridBoardDefinition BoardDefinition;

	/**
	*	Scale of the tracking coordinates:
	*	unreal coords / tracking coords
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	float TranslationScale;

	/**
	*	Desired center of scene in tracker coorindates.
	*	Will be subtracted from the measured position.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	FVector SceneCenterInTrackerCoordinates;

	// Whether the marker outlines should be displayed on the screen
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	uint32 bDisplayMarkers : 1;

	FArucoTrackerSettings()
		: bDisplayMarkers(false)
		, SceneCenterInTrackerCoordinates(0., 0., 0.)
		, TranslationScale(1.)
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
	FAURArucoTracker();
	FArucoTrackerSettings const& GetSettings() 
	{
		return this->Settings;
	}
	void SetSettings(FArucoTrackerSettings const& settings);
	void SetCameraProperties(FOpenCVCameraProperties const& camera_properties);

	/* 
		Calculate camera's position/rotation relative to the markers
		Returns true if markers found.
	*/
	bool DetectMarkers(cv::Mat& image, FTransform & out_camera_transform);

	// OpenCV vectors use different handedness.
	static FVector ConvertOpenCvVectorToUnreal(cv::Vec3f const& cv_vector)
	{
		// UE.x = CV.y
		// UE.y = CV.x
		// UE.z = CV.z
		return FVector(cv_vector[1], cv_vector[0], cv_vector[2]);
	}

protected:
	FArucoTrackerSettings Settings;

	// Camera information
	//FOpenCVCameraProperties CameraProperties;

	// Marker information
	ArucoWrapper ArucoAPI;

	void ConvertTransformToUnreal(cv::Vec3d const& opencv_translation, cv::Vec3d const& opencv_rotation, FTransform & out_transform) const;

	// Creates a default aruco board and saves a copy to a file.
	void UpdateMarkerDefinition(FArucoGridBoardDefinition const & BoardDefinition);
};
