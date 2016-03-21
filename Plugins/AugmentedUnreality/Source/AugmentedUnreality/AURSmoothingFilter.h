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

#include "Object.h"
#include "AURSmoothingFilter.generated.h"

/**
 * A filter which smooths out the jitter in the camera position/rotation
 * reported by marker tracking.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class UAURSmoothingFilter : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void Init();

	/**
	 * Report a new measured value of the camera position/rotation.
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void Measurement(FTransform const & MeasuredTransform);
	
	/**
	 * Get the current transform derived by smoothing the measurements.
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual FTransform GetCurrentTransform() const;
};
