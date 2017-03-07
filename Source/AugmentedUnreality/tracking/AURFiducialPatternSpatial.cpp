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

#include "AugmentedUnreality.h"
#include "AURFiducialPatternSpatial.h"
#include "AURMarkerComponentBase.h"
#include "AURDriver.h"

const float INCH = 2.54f;

AAURFiducialPatternSpatial::AAURFiducialPatternSpatial()
//, AutomaticMarkerIds(true)
{
}

void AAURFiducialPatternSpatial::BuildPatternData()
{
	TInlineComponentArray<UAURMarkerComponentBase*, 32> marker_components;
	GetComponents(marker_components);

	UE_LOG(LogAUR, Log, TEXT("Pattern %s: found %d markers"), *GetClass()->GetName(), marker_components.Num());

	auto builder = cv::aur::FiducialPatternArUco::builder();
	builder->dictionary(PredefinedDictionaryId);
	for (auto & marker : marker_components)
	{
		marker->AppendToBoardDefinition(builder);
	}
	PatternData = builder->build();
}

cv::Ptr<cv::aur::FiducialPattern> AAURFiducialPatternSpatial::GetPatternDefinition()
{
	if (!PatternData)
	{
		BuildPatternData();
	}

	return PatternData;
}

void AAURFiducialPatternSpatial::SaveMarkerFiles(FString output_dir, int32 dpi)
{
#if !PLATFORM_ANDROID
	// ensure pattern data exists
	GetPatternDefinition();

	if (output_dir.IsEmpty())
	{
		output_dir = FPaths::GameSavedDir() / PatternFileDir / GetClass()->GetName();
	}
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*output_dir);

	float const pixels_per_cm = float(dpi) / INCH;
	FString const dict_name = FString::FromInt(PredefinedDictionaryId);

	try
	{
		TInlineComponentArray<UAURMarkerComponentBase*, 32> marker_components;
		GetComponents(marker_components);

		UE_LOG(LogAUR, Log, TEXT("AAURFiducialPatternSpatial::SaveMarkerFiles: Pattern %s, saving marker images to %s"), *GetName(), *output_dir);

		for (auto & marker : marker_components)
		{
			FString filename = output_dir / FString::Printf(TEXT("marker_%s_%02d.png"), *dict_name, marker->Id);

			int32 canvas_pixels = FMath::RoundToInt(pixels_per_cm*marker->BoardSizeCm);
			int32 margin_pixels = FMath::RoundToInt(pixels_per_cm*marker->MarginCm);

			cv::imwrite(TCHAR_TO_UTF8(*filename), RenderMarker(marker->Id, canvas_pixels, margin_pixels));

			//UE_LOG(LogAUR, Log, TEXT("AAURFiducialPatternSpatial::SaveMarkerFiles: Saved marker image to: %s"), *filename);
		}
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("AAURFiducialPatternSpatial::SaveMarkerFiles: exception when saving\n    %s"), UTF8_TO_TCHAR(exc.what()))
	}
#endif
}

cv::Mat AAURFiducialPatternSpatial::RenderMarker(int32 id, int32 canvas_side, int32 margin)
{
	int32 marker_side = canvas_side - 2 * margin;

	if (marker_side <= 0)
	{
		UE_LOG(LogAUR, Error, TEXT("AAURFiducialPatternSpatial::RenderMarker: margin is bigger than half of square size"))
		return cv::Mat(canvas_side, canvas_side, CV_8UC1, cv::Scalar(255));
	}

	cv::Mat canvas(cv::Size(canvas_side, canvas_side), CV_8UC1, cv::Scalar(255));

	cv::Mat img_marker;
	PatternData->getArucoDictionary()->drawMarker(id, marker_side, img_marker, 1);
	img_marker.copyTo(canvas.rowRange(margin, marker_side + margin).colRange(margin, marker_side + margin));

	FString name = FString::Printf(TEXT("%d.%02d"), PredefinedDictionaryId, id);

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
void AAURFiducialPatternSpatial::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	UE_LOG(LogAUR, Warning, TEXT("AAURFiducialPatternSpatial::PostInitializeComponents"));


	if (AutomaticMarkerIds)
	{
		AssignAutomaticMarkerIds();
	}
}
*/

void AAURFiducialPatternSpatial::AssignAutomaticMarkerIds()
{
	UE_LOG(LogAUR, Log, TEXT("AAURFiducialPatternSpatial::AssignAutomaticMarkerIds"));

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
