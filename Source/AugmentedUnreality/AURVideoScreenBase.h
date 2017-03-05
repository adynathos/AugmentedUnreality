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
	/** Use the driver globally published by AURDriver static delegate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AugmentedReality)
	bool UseGlobalDriver;

	// Material displayed when there is no signal from AR
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AugmentedReality)
	UMaterial* ReplacementMaterial;

	/** Start receiving video from this given AURDriver */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void UseDriver(UAURDriver* Driver);

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void OnCameraPropertiesChange(UAURDriver* Driver);

	UAURVideoScreenBase();

	/* UActorComponent */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/* end UActorComponent */

protected:
	UPROPERTY(BlueprintReadOnly, Transient, Category = AugmentedReality)
	UAURDriver* VideoDriver;

	// Material used to display video stream through dynamic texture parameter
	UPROPERTY(BlueprintReadOnly, Transient, Category = AugmentedReality)
	UMaterialInstanceDynamic* VideoMaterial;

	/**
	* Finds the material instance with texture parameter "VideoTexture"
	* on this actor's component
	*/
	void InitVideoMaterial();

	/* true - switch to material displaying video, false - switch to empty material*/
	void SetVideoMaterialActive(bool new_active);

};
