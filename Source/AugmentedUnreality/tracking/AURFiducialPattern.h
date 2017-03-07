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

#include "AUROpenCV.h"
#include "AURFiducialPattern.generated.h"

/**
 * Actor blueprint representing a fiducial pattern used for pose tracking.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class AAURFiducialPattern : public AActor
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAURFiducialTransformUpdate, FTransform, MeasuredTransform);

	// Where to store the marker image, relative to FPaths::GameSavedDir()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	FString PatternFileDir;

	/** Use one of the predefined dictionaries Choices:
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
	int32 PredefinedDictionaryId;

	/*
		If this is set to true and the pattern Actor is placed in the world,
		it will be automatically used to find the camera position.
		Does not apply to patterns in AURTrackingComponent
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	bool AutomaticallyUseForCameraPose;

	// Event fired when this board is detected by the tracker and provides the location of the board.
	UPROPERTY(BlueprintAssignable, Category = AugmentedReality)
	FAURFiducialTransformUpdate OnTransformUpdate;

	// This actor will be moved to match the detected position of this board.
	// Or if this was registered with use_as_viewpoint_origin, it will be placed in the determined camera position.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	AActor* ActorToMove;

	/**
		Save all markers to image files.
		By default saves to FPaths::GameSavedDir()/this->MarkerFileDir/this->GetName()
	**/
	UFUNCTION(BlueprintCallable, Category = ArucoTracking)
	virtual void SaveMarkerFiles(FString output_dir = "", int32 dpi=150);

	UFUNCTION(BlueprintCallable, Category = ArucoTracking)
	bool IsInTrackingComponent() const;

	AAURFiducialPattern();

	virtual void BuildPatternData();

	virtual cv::Ptr<cv::aur::FiducialPattern> GetPatternDefinition();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type reason) override;

	// Called by AURArucoTracker when a new transform is measured
	void TransformMeasured(FTransform const& new_transform);

protected:
	cv::Ptr< cv::aruco::Dictionary > GetArucoDictionary() const;
};
