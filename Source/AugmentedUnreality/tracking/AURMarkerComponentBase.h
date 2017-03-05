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
#include "AURMarkerComponentBase.generated.h"

/*
struct FMarkerDefinitionData {
	int32 MarkerId;

	FVector Corners[4];

	FMarkerDefinitionData(int id)
		: MarkerId(id)
	{
		for (int idx = 0; idx < 4; idx++)
		{
			Corners[idx].Set(0, 0, 0);
		}
	}
};*/

/**
 * Actor blueprint representing a spatial configuration
 * of ArUco markers.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class UAURMarkerComponentBase : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	static const float MARKER_DEFAULT_SIZE;
	static const float MARKER_TEXT_RELATIVE_SCALE;
	static const std::vector<FVector> LOCAL_CORNERS;

	/**
		Unique id encoded into the pattern of the marker.
		Each marker used should have different Id.
	**/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AUR Marker")
	int32 Id;

	/**
		Total length of the square marker's side.
		The size of actual marker pattern is (BoardSideCm - 2*MarginCm)
	**/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AUR Marker")
	float BoardSizeCm;

	/**
		Length of the margin added to each marker's side.
		Total size = MarkerSideCm + 2*MarginCm
	**/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AUR Marker")
	float MarginCm;

	UFUNCTION(BlueprintCallable, Category = "AUR Marker")
	void SetBoardSize(float new_border_size);

	UFUNCTION(BlueprintCallable, Category = "AUR Marker")
	void SetId(int32 new_id);

	UAURMarkerComponentBase();

	//FMarkerDefinitionData GetDefinition() const;
	void AppendToBoardDefinition(cv::Ptr<cv::aur::FiducialPatternArUco::Builder> & pattern_builder);

	/* UActorComponent */
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& property_change_event) override;
#endif
	/* end UActorComponent */

protected:
	UPROPERTY(Transient)
	UTextRenderComponent* MarkerText;

	/* UActorComponent */
	virtual void OnRegister() override;
	/* end UActorComponent */
};
