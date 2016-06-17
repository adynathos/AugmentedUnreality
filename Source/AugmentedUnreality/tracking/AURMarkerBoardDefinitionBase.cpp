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

FFreeFormBoardData::FFreeFormBoardData()
{
}

FFreeFormBoardData::~FFreeFormBoardData()
{
}

void FFreeFormBoardData::Clear()
{
	ArucoBoard.ids.clear();
	ArucoBoard.objPoints.clear();
}

void FFreeFormBoardData::SetDictionaryId(uint32_t dict_id)
{
	DictionaryId = dict_id;
	ArucoDictionary = cv::aruco::getPredefinedDictionary(cv::aruco::PREDEFINED_DICTIONARY_NAME(DictionaryId));
}

void FFreeFormBoardData::AddMarker(FMarkerDefinitionData const & marker_def)
{
	ArucoBoard.ids.push_back(marker_def.MarkerId);

	std::vector<cv::Point3f> corners(4);
	for (int idx = 0; idx < 4; idx++)
	{
		corners[idx] = cv::Point3f(FAUROpenCV::ConvertUnrealVectorToOpenCv(marker_def.Corners[idx]));
	}
	ArucoBoard.objPoints.push_back(corners);
}

void FFreeFormBoardData::DrawMarkers(float marker_size_cm, uint32_t dpi, float margin_cm, cv::Size2f page_size_cm)
{
	float pixels_per_cm = float(dpi) / INCH;
	cv::Size page_size_pix(
		(int)std::round(pixels_per_cm*(page_size_cm.width - margin_cm)),
		(int)std::round(pixels_per_cm*(page_size_cm.height - margin_cm))
	);

	uint32_t marker_pixels = (uint32_t)std::round(pixels_per_cm*marker_size_cm);
	uint32_t margin_pixels = (uint32_t)std::round(pixels_per_cm*margin_cm);

	//cv::Mat img_out(page_size_pix, CV_8UC1);
	//img_out.setTo(255);

	//int marker_side = std::min(page_size_pix.width, page_size_pix.height) - 2*margin_pixels;

	//cv::Mat img_marker;
	//dictionary.drawMarker(ids[0], marker_side, img_marker, 1);
	//cv::putText(img_marker, "1", cv::Point(10, 10), cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 10.0, cv::Scalar::all(125));

	//img_marker.copyTo(img_out.rowRange(margin_pixels, margin_pixels+marker_side).colRange(margin_pixels, margin_pixels+marker_side));

	//cv::putText(img_out, "1", cv::Point(margin_pixels, margin_pixels + marker_side + 10), 
	//	cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 4.0, cv::Scalar(125), 3);

	//Pages.resize(1);
	//Pages[0] = img_marker;
	//putText(_image, s.str(), cent, FONT_HERSHEY_SIMPLEX, 0.5, textColor, 2);

	Pages.clear();
	for (int id = 0; id < 10; id++)
	{
		Pages.push_back(RenderMarker(id, marker_pixels, margin_pixels));
	}
}

cv::Mat FFreeFormBoardData::RenderMarker(int id, uint32_t marker_side, uint32_t margin)
{
	uint32_t canvas_side = marker_side + 2 * margin;
	cv::Mat canvas(cv::Size(canvas_side, canvas_side), CV_8UC1, cv::Scalar(255));

	cv::Mat img_marker;
	GetArucoDictionary().drawMarker(id, marker_side, img_marker, 1);
	img_marker.copyTo(canvas.rowRange(margin, marker_side + margin).colRange(margin, marker_side + margin));

	std::string name = std::to_string(DictionaryId) + '.' + std::to_string(id);

	cv::rectangle(canvas, cv::Point(0, 0), cv::Point(canvas_side - 1, canvas_side - 1), cv::Scalar(125), 4);

	// putText takes bottom-left corner of text
	cv::putText(canvas, name, cv::Point(6, canvas_side - 8),
		cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 4.0, cv::Scalar(125), 3);

	return canvas;
}

AAURMarkerBoardDefinitionBase::AAURMarkerBoardDefinitionBase()
	: SavedFileDir("AugmentedUnreality/Markers")
	, DictionaryId(cv::aruco::DICT_4X4_100)
	//, AutomaticMarkerIds(true)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickEnabled(false);
}

void AAURMarkerBoardDefinitionBase::WriteToBoard(FFreeFormBoardData * board) const
{
	board->Clear();
	board->SetDictionaryId(DictionaryId);

	TInlineComponentArray<UAURMarkerComponentBase*, 32> marker_components;
	GetComponents(marker_components);

	UE_LOG(LogAUR, Log, TEXT("Board def %s found %d markers"), *GetName(), marker_components.Num());

	for (auto & marker : marker_components)
	{
		UE_LOG(LogAUR, Log, TEXT("Adding marker %s"), *marker->GetName());
		board->AddMarker(marker->GetDefinition());
	}
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
