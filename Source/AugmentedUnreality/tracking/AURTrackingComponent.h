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

#include "Components/ChildActorComponent.h"
#include "AURTrackingComponent.generated.h"

/**
 * Moves the actor to the location of the specified ArUco marker configuration.
 * Set the ChildActorClass property to a subclass of AURMarkerBoardDefinition.
 */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class UAURTrackingComponent : public UChildActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
};
