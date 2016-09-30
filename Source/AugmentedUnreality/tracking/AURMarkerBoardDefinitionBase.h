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
#include <algorithm>

#include "AURMarkerBoardDefinitionBase.generated.h"
class AAURMarkerBoardDefinitionBase;

/**
	Represents an set of markers.
*/
USTRUCT(BlueprintType)
struct FArucoDictionaryDefinition
{
	GENERATED_BODY()

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

	// False: Use PredefinedDictionaryId, True: build custom dict with CustomDictionary* properties.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	bool UseCustomDictionary;

	// Number of markers in the custom dictionary.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	int32 CustomDictionaryCount;

	// Marker is made of [size x size] squares
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking, meta = (ClampMin = "0", ClampMax = "64", UIMin = "0.0", UIMax = "64"))
	int32 CustomDictionaryMarkerSize;

	FArucoDictionaryDefinition()
		: PredefinedDictionaryId(cv::aruco::DICT_4X4_100)
		, UseCustomDictionary(false)
		, CustomDictionaryCount(0)
		, CustomDictionaryMarkerSize(3)
	{}

	bool operator==(FArucoDictionaryDefinition const & other) const;
	bool operator!=(FArucoDictionaryDefinition const & other) const;

	// Id for comparing if 2 dictionaries are the same.
	int32 GetUniqueId() const;
	FString GetName() const;
	cv::aruco::Dictionary GetDictionary() const;
};

/**
	Description of a set of AR markers, in a format needed by the tracker library.
	Usually created from a board definition BP.
**/
class FFreeFormBoardData
{
public:
	FFreeFormBoardData(AAURMarkerBoardDefinitionBase* board_actor);
	void Clear();
	void SetDictionaryDefinition(FArucoDictionaryDefinition const & dict_def);
	void AddMarker(FMarkerDefinitionData const & marker_def);

	cv::aruco::Board const& GetArucoBoard() const
	{
		return ArucoBoard;
	}

	std::vector<int> const& GetMarkerIds() const
	{
		return ArucoBoard.ids;
	}

	int GetMinMarkerId() const
	{
		return *std::min_element(std::begin(GetMarkerIds()), std::end(GetMarkerIds()));
	}

	int GetArucoDictionaryId() const
	{
		return DictionaryDefinition.GetUniqueId();
	}

	cv::aruco::Dictionary const& GetArucoDictionary() const
	{
		return ArucoDictionary;
	}

	AAURMarkerBoardDefinitionBase* GetBoardActor() const
	{
		return BoardActor;
	}

protected:
	AAURMarkerBoardDefinitionBase* BoardActor;

	/**
		The board is constructed in AUR binary, without GridBoard::create,
		so it can be deleted in AUR binary too.
	**/
	cv::aruco::Board ArucoBoard;
	
	/**
		A full copy of the dictionary returned by cv::ArUco is stored here
		so that we avoid the crash-inducing Ptr<Dictionary>
	**/
	cv::aruco::Dictionary ArucoDictionary;
	FArucoDictionaryDefinition DictionaryDefinition;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAURBoardTransformUpdate, FTransform, MeasuredTransform);

/**
 * Actor blueprint representing a spatial configuration of ArUco markers.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class AAURMarkerBoardDefinitionBase : public AActor
{
	GENERATED_BODY()

public:
	// Where to store the marker image, relative to FPaths::GameSavedDir()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	FString MarkerFileDir;

	// Set of markers to use
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	FArucoDictionaryDefinition DictionaryDefinition;

	// Event fired when this board is detected by the tracker and provides the location of the board.
	UPROPERTY(BlueprintAssignable, Category = AugmentedReality)
	FAURBoardTransformUpdate OnTransformUpdate;

	// This actor will be moved to match the detected position of this board.
	// Or if this was registered with use_as_viewpoint_origin, it will be placed in the determined camera position.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoTracking)
	AActor* ActorToMove;

	/*
		Each marker must have a different Id.
		Set this to true to automatically set different ids to child markers.
		Generated ids are consecutive integers ordered by component names.
	*/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ArucoTracking)
	//bool AutomaticMarkerIds;

	/**
		Save all markers to image files.
		By default saves to FPaths::GameSavedDir()/this->MarkerFileDir/this->GetName()
	**/
	UFUNCTION(BlueprintCallable, Category = ArucoTracking)
	void SaveMarkerFiles(FString output_dir = "", int32 dpi=150);

	AAURMarkerBoardDefinitionBase();

	void BuildBoardData();

	TSharedPtr<FFreeFormBoardData> GetBoardData();

	virtual void EndPlay(const EEndPlayReason::Type reason) override;

	//virtual void PostInitializeComponents() override;

	// Called by AURArucoTracker when a new transform is measured
	// @param used_as_viewpoint_origin Is this board used to position the camera (as opposed to moving some object in scene)
	void TransformMeasured(FTransform const& new_transform, bool used_as_viewpoint_origin);

protected:
	TSharedPtr<FFreeFormBoardData> BoardData;

	void AssignAutomaticMarkerIds();

	// size_cm is used only for displaying the text showing 
	//cv::Mat RenderMarker(int32 id, int32 canvas_side, int32 margin, float size_cm = 0.0);
	cv::Mat RenderMarker(int32 id, int32 canvas_side, int32 margin);
};
