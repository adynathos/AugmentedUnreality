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

#include "AugmentedUnreality.h"
#include "AURMarkerBoardDefinitionBase.h"
#include "AURMarkerComponentBase.h"
#include "AURDriver.h"

const float INCH = 2.54f;

bool FArucoDictionaryDefinition::operator==(FArucoDictionaryDefinition const & other) const
{
	return GetUniqueId() == other.GetUniqueId();

	/*
	if (UseCustomDictionary != other.UseCustomDictionary) return false;

	if (UseCustomDictionary)
	{
		return PredefinedDictionaryId == other.PredefinedDictionaryId;
	}
	else
	{
		return (CustomDictionaryCount == other.CustomDictionaryCount 
			&& CustomDictionaryMarkerSize == other.CustomDictionaryMarkerSize);
	}
	*/
}

bool FArucoDictionaryDefinition::operator!=(FArucoDictionaryDefinition const & other) const
{
	return !(*this == other);
}

int32 FArucoDictionaryDefinition::GetUniqueId() const
{
	if (!UseCustomDictionary)
	{
		return PredefinedDictionaryId;
	}
	else
	{
		// the size should be limited (in the editor UI) to 64, so will only take the lowest bytes
		return - (CustomDictionaryCount * 64 + CustomDictionaryMarkerSize);
	}
}

FString FArucoDictionaryDefinition::GetName() const
{
	if (!UseCustomDictionary)
	{
		return FString::FromInt(PredefinedDictionaryId);
	}
	else
	{
		return FString::Printf(TEXT("%dx%dx%d"), CustomDictionaryMarkerSize, CustomDictionaryMarkerSize, CustomDictionaryCount);
	}
}

cv::aruco::Dictionary FArucoDictionaryDefinition::GetDictionary() const
{
	if (!UseCustomDictionary)
	{
		return cv::aruco::getPredefinedDictionary(cv::aruco::PREDEFINED_DICTIONARY_NAME(PredefinedDictionaryId));
	}
	else
	{
		return cv::aruco::generateCustomDictionary(CustomDictionaryCount, CustomDictionaryMarkerSize);
	}
}


FFreeFormBoardData::FFreeFormBoardData(AAURMarkerBoardDefinitionBase* board_actor)
	: BoardActor(board_actor)
{
}

void FFreeFormBoardData::Clear()
{
	ArucoBoard.ids.clear();
	ArucoBoard.objPoints.clear();
}

void FFreeFormBoardData::SetDictionaryDefinition(FArucoDictionaryDefinition const & dict_def)
{
	DictionaryDefinition = dict_def;
	ArucoDictionary = dict_def.GetDictionary();
}

void FFreeFormBoardData::AddMarker(FMarkerDefinitionData const & marker_def)
{
	ArucoBoard.ids.push_back(marker_def.MarkerId);

	//UE_LOG(LogAUR, Log, TEXT("Marker %d"), marker_def.MarkerId);

	std::vector<cv::Point3f> corners(4);
	for (int idx = 0; idx < 4; idx++)
	{
		corners[idx] = cv::Point3f(FAUROpenCV::ConvertUnrealVectorToOpenCv(marker_def.Corners[idx]));
		//UE_LOG(LogAUR, Log, TEXT("	C %s -> (%f %f %f)"), *marker_def.Corners[idx].ToString(), corners[idx].x, corners[idx].y, corners[idx].z);
	}
	ArucoBoard.objPoints.push_back(corners);
}

AAURMarkerBoardDefinitionBase::AAURMarkerBoardDefinitionBase()
	: MarkerFileDir("AugmentedUnreality/Markers")
	, ActorToMove(nullptr)
	, AutomaticMarkerIds(false)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickEnabled(false);
}

void AAURMarkerBoardDefinitionBase::BuildBoardData()
{
	if (!BoardData.IsValid())
	{
		BoardData = TSharedPtr<FFreeFormBoardData>(new FFreeFormBoardData(this));
	}

	BoardData->Clear();
	BoardData->SetDictionaryDefinition(DictionaryDefinition);

	TInlineComponentArray<UAURMarkerComponentBase*, 32> marker_components;
	GetComponents(marker_components);

	UE_LOG(LogAUR, Log, TEXT("Board def %s: found %d markers"), *GetName(), marker_components.Num());

	for (auto & marker : marker_components)
	{
		BoardData->AddMarker(marker->GetDefinition());
	}
}

TSharedPtr<FFreeFormBoardData> AAURMarkerBoardDefinitionBase::GetBoardData()
{
	if (!BoardData.IsValid())
	{
		BuildBoardData();
	}

	return BoardData;
}

void AAURMarkerBoardDefinitionBase::EndPlay(const EEndPlayReason::Type reason)
{
	Super::EndPlay(reason);

	// Unregister to be sure we don't leave invalid pointers in the list
	UAURDriver::UnregisterBoardForTracking(this);
}

void AAURMarkerBoardDefinitionBase::SaveMarkerFiles(FString output_dir, int32 dpi)
{
	if (output_dir.IsEmpty())
	{
		output_dir = FPaths::GameSavedDir() / MarkerFileDir / GetName();
	}

	BuildBoardData();
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*output_dir);

	float pixels_per_cm = float(dpi) / INCH;

	FString dict_name = DictionaryDefinition.GetName();
#ifndef __ANDROID__
	try
	{
#endif
		TInlineComponentArray<UAURMarkerComponentBase*, 32> marker_components;
		GetComponents(marker_components);

		UE_LOG(LogAUR, Log, TEXT("Board def %s: saving marker images to %s"), *GetName(), *output_dir);

		for (auto & marker : marker_components)
		{
			FString filename = output_dir / FString::Printf(TEXT("marker_%s_%02d.png"), *dict_name, marker->Id);
				
			int32 canvas_pixels = FMath::RoundToInt(pixels_per_cm*marker->BoardSizeCm);
			int32 margin_pixels = FMath::RoundToInt(pixels_per_cm*marker->MarginCm);

			cv::imwrite(TCHAR_TO_UTF8(*filename), RenderMarker(marker->Id, canvas_pixels, margin_pixels));

			//UE_LOG(LogAUR, Log, TEXT("AAURMarkerBoardDefinitionBase::SaveMarkerFiles: Saved marker image to: %s"), *filename);
		}
#ifndef __ANDROID__
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("AAURMarkerBoardDefinitionBase::SaveMarkerFiles: exception when saving\n    %s"), UTF8_TO_TCHAR(exc.what()))
	}
#endif
}

cv::Mat AAURMarkerBoardDefinitionBase::RenderMarker(int32 id, int32 canvas_side, int32 margin)
{
	int32 marker_side = canvas_side - 2 * margin;

	if (marker_side <= 0)
	{
		UE_LOG(LogAUR, Error, TEXT("AAURMarkerBoardDefinitionBase::RenderMarker: margin is bigger than half of square size"))
		return cv::Mat(canvas_side, canvas_side, CV_8UC1, cv::Scalar(255));
	}
	
	cv::Mat canvas(cv::Size(canvas_side, canvas_side), CV_8UC1, cv::Scalar(255));

	cv::Mat img_marker;
	BoardData->GetArucoDictionary().drawMarker(id, marker_side, img_marker, 1);
	img_marker.copyTo(canvas.rowRange(margin, marker_side + margin).colRange(margin, marker_side + margin));

	FString name = FString::Printf(TEXT("%s.%02d"), *DictionaryDefinition.GetName(), id);

	cv::rectangle(canvas, cv::Point(0, 0), cv::Point(canvas_side - 1, canvas_side - 1), cv::Scalar(200), 4);

	// Texts
	const int text_margin = 8;
	const double font_size = margin / 30.0;
	const int font_line_width = FMath::RoundToInt(font_size);
	const int font_index = cv::FONT_HERSHEY_DUPLEX;
	const cv::Scalar font_color(175);

	// putText takes bottom-left corner of text
	// text: id
	cv::putText(canvas, TCHAR_TO_UTF8(*name), cv::Point(6, canvas_side - 8),
		font_index, font_size, font_color, font_line_width);

	// text: size
	/*
	if (size_cm > 0.0)
	{
		std::string description_size = "size " + std::to_string(size_cm) + " cm";
		cv::putText(canvas, description_size, cv::Point(canvas_side-50, canvas_side - 8),
			font_index, font_size, font_color, font_line_width);	
	}
	*/

	return canvas;
}

/*
void AAURMarkerBoardDefinitionBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	UE_LOG(LogAUR, Warning, TEXT("AAURMarkerBoardDefinitionBase::PostInitializeComponents"));


	if (AutomaticMarkerIds)
	{
		AssignAutomaticMarkerIds();
	}
}
*/

void AAURMarkerBoardDefinitionBase::TransformMeasured(FTransform const & new_transform, bool used_as_viewpoint_origin)
{
	if (!used_as_viewpoint_origin)
	{
		this->SetActorTransform(new_transform, false);
	}

	if (ActorToMove)
	{
		ActorToMove->SetActorTransform(new_transform, false);
	}

	OnTransformUpdate.Broadcast(new_transform);

	//UE_LOG(LogAUR, Log, TEXT("TransformMeasured: %s"), *new_transform.ToString())
}

void AAURMarkerBoardDefinitionBase::RefreshAfterEdit()
{
	if (AutomaticMarkerIds)
	{
		AssignAutomaticMarkerIds();
	}

	TInlineComponentArray<UAURMarkerComponentBase*, 32> marker_components;
	GetComponents(marker_components);

	for (auto & marker : marker_components)
	{
		marker->RedrawSurface();
	}
}

void AAURMarkerBoardDefinitionBase::AssignAutomaticMarkerIds()
{
	UE_LOG(LogAUR, Log, TEXT("AAURMarkerBoardDefinitionBase::AssignAutomaticMarkerIds"));

	// Get components of type UAURMarkerComponentBase
	// https://docs.unrealengine.com/latest/INT/API/Runtime/Engine/GameFramework/AActor/GetComponents/3/index.html
	TInlineComponentArray<UAURMarkerComponentBase*, 32> marker_components;
	GetComponents(marker_components);

	// Sort by name
	marker_components.Sort(
		[](UAURMarkerComponentBase const & left, UAURMarkerComponentBase const & right)
		{
			return left.GetFName() < right.GetFName();
		}
	);

	// Assign consecutive ids
	for (int idx = 0; idx < marker_components.Num(); idx++)
	{
		UE_LOG(LogAUR, Log, TEXT("M %s %d"), *marker_components[idx]->GetName(), idx);

		marker_components[idx]->SetId(idx);
	}
}
