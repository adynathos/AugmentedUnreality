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

#include "AUROpenCV.h"
#include "AURMarkerComponentBase.h"
#include "AURMarkerBoardDefinitionBase.generated.h"

class FFreeFormBoardData
{
public:
	FFreeFormBoardData();
	~FFreeFormBoardData();
	void Clear();
	void SetDictionaryId(uint32_t dict_id);
	void AddMarker(FMarkerDefinitionData const & marker_def);

	cv::aruco::Board const& GetArucoBoard() const
	{
		return *ArucoBoard;
	}

	cv::aruco::Dictionary const& GetArucoDictionary() const
	{
		return *Dictionary;
	}

	std::vector<cv::Mat> const & GetMarkerImages() const
	{
		return Pages;
	}

	// Default size is A4 landscape.
	void DrawMarkers(float marker_size_cm = 8.0f, uint32_t dpi = 300, float margin_cm = 2.0f, cv::Size2f page_size_cm = cv::Size2f(29.7f, 21.0f));

	cv::Mat RenderMarker(int id, uint32_t marker_side, uint32_t margin);

	uint32_t DictionaryId;

protected:

	std::vector<cv::Mat> Pages;

	cv::aruco::Board* ArucoBoard;
	cv::aruco::Dictionary* Dictionary;
};


/**
 * Actor blueprint representing a spatial configuration
 * of ArUco markers.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class AAURMarkerBoardDefinitionBase : public AActor
{
	GENERATED_BODY()

public:

	// Where to store the marker image, relative to FPaths::GameSavedDir()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	FString SavedFileDir;

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

	/*
		Each marker must have a different Id.
		Set this to true to automatically set different ids to child markers.
		Generated ids are consecutive integers ordered by component names.
	*/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ArucoTracking)
	//bool AutomaticMarkerIds;

	AAURMarkerBoardDefinitionBase();

	void WriteToBoard(FFreeFormBoardData* board) const;

	//virtual void PostInitializeComponents() override;

protected:
	void AssignAutomaticMarkerIds();
};
