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

#include "AURVideoScreenBase.h"
#include "AURVideoScreenBackground.generated.h"

/**
 * Screen displaying the camera stream behind the scene,
 * has additional functions for setting proper size of the screen.
 */
UCLASS(Blueprintable, BlueprintType)
class UAURVideoScreenBackground : public UAURVideoScreenBase
{
	GENERATED_BODY()
	
public:
	/**
	 *	Set the size (scale) of this component based on:
	 *	- distance to (0, 0, 0) of actor-space where we assume the camera is
	 *	- FOV of the camera
	 *	- aspect ratio
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	void SetSizeForFOV(float FOV_Horizontal);
	
	UAURVideoScreenBackground();

	virtual void OnCameraPropertiesChange(UAURDriver* Driver) override;

protected:
	FIntPoint Resolution;

	/**
	 * Initializes the mechanisms related to updating a dynamic texture with a given resolution
	 */
	void SetResolution(FIntPoint resolution);
};
