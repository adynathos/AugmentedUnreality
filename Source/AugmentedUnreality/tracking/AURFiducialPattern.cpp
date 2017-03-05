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
#include "AURFiducialPattern.h"
#include "AURMarkerComponentBase.h"
#include "AURDriver.h"

AAURFiducialPattern::AAURFiducialPattern()
	: PredefinedDictionaryId(cv::aruco::DICT_4X4_100)
	, PatternFileDir("AugmentedUnreality/Patterns")
	, ActorToMove(nullptr)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickEnabled(false);
}

void AAURFiducialPattern::BuildPatternData()
{
	UE_LOG(LogAUR, Warning, TEXT("AAURFiducialPattern::BuildPatternData: not implemented"));
}

cv::Ptr<cv::aur::FiducialPattern> AAURFiducialPattern::GetPatternDefinition()
{
	UE_LOG(LogAUR, Error, TEXT("AAURFiducialPattern::GetPatternDefinition: not implemented, returning NULL"));
	return cv::Ptr<cv::aur::FiducialPattern>();
}

void AAURFiducialPattern::EndPlay(const EEndPlayReason::Type reason)
{
	// Unregister to be sure we don't leave invalid pointers in the list
	UAURDriver::UnregisterBoardForTracking(this);

	Super::EndPlay(reason);
}

void AAURFiducialPattern::SaveMarkerFiles(FString output_dir, int32 dpi)
{
	UE_LOG(LogAUR, Warning, TEXT("AAURFiducialPattern::SaveMarkerFiles: not implemented"));
}

void AAURFiducialPattern::TransformMeasured(FTransform const & new_transform, bool used_as_viewpoint_origin)
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

cv::Ptr< cv::aruco::Dictionary > AAURFiducialPattern::GetArucoDictionary() const
{
	return cv::aruco::getPredefinedDictionary(PredefinedDictionaryId);
}
