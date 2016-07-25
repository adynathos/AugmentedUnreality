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

const float INCH = 2.54f;

FFreeFormBoardData::FFreeFormBoardData(AAURMarkerBoardDefinitionBase* board_actor)
	: BoardActor(board_actor)
{
}

void FFreeFormBoardData::Clear()
{
	ArucoBoard.ids.clear();
	ArucoBoard.objPoints.clear();
}

void FFreeFormBoardData::SetDictionaryId(uint32 dict_id)
{
	DictionaryId = dict_id;
	ArucoDictionary = cv::aruco::getPredefinedDictionary(cv::aruco::PREDEFINED_DICTIONARY_NAME(DictionaryId));
}

void FFreeFormBoardData::AddMarker(FMarkerDefinitionData const & marker_def)
{
	ArucoBoard.ids.push_back(marker_def.MarkerId);

	UE_LOG(LogAUR, Log, TEXT("Marker %d"), marker_def.MarkerId);

	std::vector<cv::Point3f> corners(4);
	for (int idx = 0; idx < 4; idx++)
	{
		corners[idx] = cv::Point3f(FAUROpenCV::ConvertUnrealVectorToOpenCv(marker_def.Corners[idx]));
		UE_LOG(LogAUR, Log, TEXT("	C %s -> (%f %f %f)"), *marker_def.Corners[idx].ToString(), corners[idx].x, corners[idx].y, corners[idx].z);
	}
	ArucoBoard.objPoints.push_back(corners);
}

/*
void FFreeFormBoardData::DrawMarkers(float marker_size_cm, uint32 dpi, float margin_cm, cv::Size2f page_size_cm)
{
	float pixels_per_cm = float(dpi) / INCH;
	cv::Size page_size_pix(
		(int)std::round(pixels_per_cm*(page_size_cm.width - margin_cm)),
		(int)std::round(pixels_per_cm*(page_size_cm.height - margin_cm))
	);

	uint32 marker_pixels = (uint32)std::round(pixels_per_cm*marker_size_cm);
	uint32 margin_pixels = (uint32)std::round(pixels_per_cm*margin_cm);

	Pages.clear();
	for (int id = 0; id < 10; id++)
	{
		Pages.push_back(RenderMarker(id, marker_pixels, margin_pixels));
	}
}
*/

AAURMarkerBoardDefinitionBase::AAURMarkerBoardDefinitionBase()
	: MarkerFileDir("AugmentedUnreality/Markers")
	, DictionaryId(cv::aruco::DICT_4X4_100)
	, ActorToMove(nullptr)
	//, UsedAsViewpointOrigin(false)
	//, AutomaticMarkerIds(true)
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
	BoardData->SetDictionaryId(DictionaryId);

	TInlineComponentArray<UAURMarkerComponentBase*, 32> marker_components;
	GetComponents(marker_components);

	UE_LOG(LogAUR, Log, TEXT("Board def %s found %d markers"), *GetName(), marker_components.Num());

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

void AAURMarkerBoardDefinitionBase::SaveMarkerFiles(FString output_dir, int32 dpi)
{
	if (output_dir.IsEmpty())
	{
		output_dir = FPaths::GameSavedDir() / MarkerFileDir / GetName();
	}

	BuildBoardData();
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*output_dir);

	float pixels_per_cm = float(dpi) / INCH;

	try
	{
		TInlineComponentArray<UAURMarkerComponentBase*, 32> marker_components;
		GetComponents(marker_components);

		UE_LOG(LogAUR, Log, TEXT("Board def %s found %d markers"), *GetName(), marker_components.Num());

		for (auto & marker : marker_components)
		{
			FString filename = output_dir / FString::Printf(TEXT("marker_%d_%02d.png"), DictionaryId, marker->Id);
				
			int32 canvas_pixels = (int32)std::round(pixels_per_cm*marker->BoardSizeCm);
			int32 margin_pixels = (int32)std::round(pixels_per_cm*marker->MarginCm);

			cv::imwrite(TCHAR_TO_UTF8(*filename), RenderMarker(marker->Id, canvas_pixels, margin_pixels));

			UE_LOG(LogAUR, Log, TEXT("AAURMarkerBoardDefinitionBase::SaveMarkerFiles: Saved marker image to: %s"), *filename);
		}
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("AAURMarkerBoardDefinitionBase::SaveMarkerFiles: exception when saving\n    %s"), UTF8_TO_TCHAR(exc.what()))
	}
}

//cv::Mat AAURMarkerBoardDefinitionBase::RenderMarker(int32 id, int32 canvas_side, int32 margin, float size_cm)
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

	FString name = FString::Printf(TEXT("%d.%02d"), DictionaryId, id);

	cv::rectangle(canvas, cv::Point(0, 0), cv::Point(canvas_side - 1, canvas_side - 1), cv::Scalar(200), 4);

	// Texts
	const int text_margin = 8;
	const double font_size = margin / 30.0;
	const int font_line_width = std::round(font_size);
	const int font_index = cv::FONT_HERSHEY_SCRIPT_SIMPLEX;
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
