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
#include <algorithm>

#include "AURFiducialPatternFlatBoard.generated.h"

/**
 * Actor blueprint representing a spatial configuration of ArUco markers.
 *
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class AAURFiducialPatternFlatBoard : public AAURFiducialPattern
{
	GENERATED_BODY()

public:
	// Numbers of squares in the horizontal direction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ArucoTracking)
	int32 BoardWidth;

	// Numbers of squares in the vertical direction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ArucoTracking)
	int32 BoardHeight;

	// Length of a square's side
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ArucoTracking)
	float SquareSize;

	// For markers placed on white fields of chessboard: distance to chessboard field sides.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ArucoTracking)
	float MarkerMargin;

	// The board will contain w*h/2 marker ids starting from this one.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ArucoTracking)
	int32 MarkerIdSequenceStart;

	AAURFiducialPatternFlatBoard();

	virtual void SaveMarkerFiles(FString output_dir = "", int32 dpi=150) override;
	virtual void BuildPatternData() override;
	virtual cv::Ptr<cv::aur::FiducialPattern> GetPatternDefinition() override;

protected:

	cv::Ptr<cv::aur::FiducialPatternChArUcoBoard> PatternData;
};
