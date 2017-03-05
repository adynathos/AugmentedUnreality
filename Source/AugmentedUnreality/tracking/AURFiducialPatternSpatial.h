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

#include "AURFiducialPattern.h"
#include "AURFiducialPatternSpatial.generated.h"

/**
 * Actor blueprint representing a spatial configuration of ArUco markers.
 *
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class AAURFiducialPatternSpatial : public AAURFiducialPattern
{
	GENERATED_BODY()

public:
	/*
		Each marker must have a different Id.
		Set this to true to automatically set different ids to child markers.
		Generated ids are consecutive integers ordered by component names.
	*/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ArucoTracking)
	//bool AutomaticMarkerIds;

	AAURFiducialPatternSpatial();

	virtual void SaveMarkerFiles(FString output_dir = "", int32 dpi=150) override;
	virtual void BuildPatternData() override;
	virtual cv::Ptr<cv::aur::FiducialPattern> GetPatternDefinition() override;

protected:
	cv::Ptr<cv::aur::FiducialPatternArUco> PatternData;

	// size_cm is used only for displaying the text showing
	//cv::Mat RenderMarker(int32 id, int32 canvas_side, int32 margin, float size_cm = 0.0);
	cv::Mat RenderMarker(int32 id, int32 canvas_side, int32 margin);

	void AssignAutomaticMarkerIds();
};
