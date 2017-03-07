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
#include "AURFiducialPattern.h"
#include "AURMarkerComponentBase.h"
#include "AURTrackingComponent.h"
#include "AURDriver.h"

AAURFiducialPattern::AAURFiducialPattern()
	: PatternFileDir("AugmentedUnreality/Patterns")
	, PredefinedDictionaryId(cv::aruco::DICT_4X4_100)
	, AutomaticallyUseForCameraPose(true)
	, ActorToMove(nullptr)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickEnabled(false);
}

bool AAURFiducialPattern::IsInTrackingComponent() const
{
	auto const root_component = GetRootComponent();
	if (root_component)
	{
		auto const external_attachment = root_component->GetAttachParent();
		if (external_attachment && external_attachment->IsA(UAURTrackingComponent::StaticClass()))
		{
			return true;
		}
	}
	return false;
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

void AAURFiducialPattern::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogAUR, Log, TEXT("AAURFiducialPattern %s: board transform %s"), *GetName(), *GetActorTransform().ToHumanReadableString());

	// Patterns which are placed in the world should automatically be used for camera pose tracking
	if (AutomaticallyUseForCameraPose && (! IsInTrackingComponent()))
	{
		UE_LOG(LogAUR, Log, TEXT("AAURFiducialPattern %s: register as camera tracker "), *GetName());

		UAURDriver::RegisterBoardForTracking(this, true);
	}
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

void AAURFiducialPattern::TransformMeasured(FTransform const & new_transform)
{
	if (ActorToMove)
	{
		ActorToMove->SetActorTransform(new_transform, false);
	}

	OnTransformUpdate.Broadcast(new_transform);
}

cv::Ptr< cv::aruco::Dictionary > AAURFiducialPattern::GetArucoDictionary() const
{
	return cv::aruco::getPredefinedDictionary(PredefinedDictionaryId);
}
