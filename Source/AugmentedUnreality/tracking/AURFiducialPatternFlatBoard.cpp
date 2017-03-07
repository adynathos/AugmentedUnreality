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
#include "AURFiducialPatternFlatBoard.h"
#include "AURMarkerComponentBase.h"
#include "AURDriver.h"

AAURFiducialPatternFlatBoard::AAURFiducialPatternFlatBoard()
	: BoardWidth(3)
	, BoardHeight(4)
	, SquareSize(7.0)
	, MarkerMargin(1.0)
	, MarkerIdSequenceStart(0)
{
}

void AAURFiducialPatternFlatBoard::BuildPatternData()
{
	PatternData = cv::aur::FiducialPatternChArUcoBoard::build(
		BoardWidth, BoardHeight, SquareSize, MarkerMargin,
		MarkerIdSequenceStart, PredefinedDictionaryId
	);
}

cv::Ptr<cv::aur::FiducialPattern> AAURFiducialPatternFlatBoard::GetPatternDefinition()
{
	if (!PatternData)
	{
		BuildPatternData();
	}

	return PatternData;
}

void AAURFiducialPatternFlatBoard::SaveMarkerFiles(FString output_dir, int32 dpi)
{
#if !PLATFORM_ANDROID
	if (output_dir.IsEmpty())
	{
		output_dir = FPaths::GameSavedDir() / PatternFileDir;
	}

	BuildPatternData();
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*output_dir);

	try
	{
		FString const filename = output_dir / GetClass()->GetName() + ".png";

		UE_LOG(LogAUR, Log, TEXT("AAURFiducialPatternFlatBoard::SaveMarkerFiles: saving to %s"), *filename);

		cv::imwrite(TCHAR_TO_UTF8(*filename), PatternData->drawPattern());
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("AAURFiducialPatternFlatBoard::SaveMarkerFiles: exception when saving\n    %s"), UTF8_TO_TCHAR(exc.what()))
	}
#endif
}
