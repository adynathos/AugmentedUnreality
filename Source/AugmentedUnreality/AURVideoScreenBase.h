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

#include "Components/StaticMeshComponent.h"
#include "AURDriver.h"
#include "AURVideoScreenBase.generated.h"

/**
 * Base class for a static mesh that displays video on its surface
 * The defaults for mesh and texture are set in the corresponding blueprint AURVideoScreen
 * The video will be displayed on a dynamic texture with "VideoTexture" texture parameter.
 */
UCLASS(Blueprintable, BlueprintType)
class UAURVideoScreenBase : public UStaticMeshComponent
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

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	void Initialize(UAURDriver* Driver);

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	void OnCameraPropertiesChange(UAURDriver* Driver);

	UAURVideoScreenBase();

	/* UActorComponent */
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
	/* end UActorComponent */

protected:
	UPROPERTY(BlueprintReadOnly, Transient, Category = AugmentedReality)
	UAURDriver* VideoDriver;

	UPROPERTY(BlueprintReadOnly, Transient, Category = AugmentedReality)
	UMaterialInstanceDynamic* ScreenMaterial;

	FIntPoint Resolution;

	/**
	 * Initializes the mechanisms related to updating a dynamic texture with a given resolution
	 */
	void SetResolution(FIntPoint resolution);

	/**
	* Finds the material instance with texture parameter "VideoTexture"
	* on this actor's component
	*/
	UMaterialInstanceDynamic* FindScreenMaterial();
};
