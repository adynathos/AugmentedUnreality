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

#include "AURSmoothingFilter.h"
#include "AURSmoothingFilterBlend.generated.h"

/**
 * A simple smoothing filter such that:
 *	new_transform = measurement * (1 - SmoothingStrength) + old_transform * SmoothingStrength
 */
UCLASS(Blueprintable, BlueprintType)
class UAURSmoothingFilterBlend : public UAURSmoothingFilter
{
	GENERATED_BODY()

public:
	// Value in range (0, 1), the higher, the more smoothed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	float SmoothingStrength;

	UAURSmoothingFilterBlend();

	virtual void Measurement(FTransform const & MeasuredTransform) override;
	virtual FTransform GetCurrentTransform() const override;

protected:
	FTransform CurrentTransform;
};
