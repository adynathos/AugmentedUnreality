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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AugmentedReality)
	bool bSetSizeAutomatically;

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	void Initialize(UAURDriver* Driver);

	UAURVideoScreenBase(const FObjectInitializer & ObjectInitializer);

	/* UActorComponent */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;
	/* end UActorComponent */

protected:
	UPROPERTY(BlueprintReadOnly, Transient, Category = AugmentedReality)
	UAURDriver* VideoDriver;

	UPROPERTY(BlueprintReadOnly, Transient, Category = AugmentedReality)
	UTexture2D* DynamicTexture;

	/**
	 * Initializes the mechanisms related to updating a dynamic texture.
	 */
	void InitDynamicTexture();

	/**
	 * Calculate the mesh scale based on the distance to camera.
	 */
	void InitScreenSize();

	/**
	 * Display the frame received from the driver on the dynamic texture
	 */
	void UpdateDynamicTexture();

	/**
	* Finds the material instance with texture parameter "VideoTexture"
	* on this actor's component
	*/
	UMaterialInstanceDynamic* FindVideoMaterial();
	
	// The rendering scheduled by ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER 
	// takes place in the rendering thread:
	// https://docs.unrealengine.com/latest/INT/Programming/Rendering/ThreadedRendering/index.html
	// so data has to be passed through a struct.
	struct FTextureUpdateParameters
	{
		FTexture2DResource*	Texture2DResource;
		FUpdateTextureRegion2D RegionDefinition;
		UAURDriver* Driver;
	};
	// The constant values of these parameters:
	FTextureUpdateParameters TextureUpdateParameters;
};
