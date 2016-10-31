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
#include "Engine.h"
#include "AURVideoScreenBase.h"

UAURVideoScreenBase::UAURVideoScreenBase()
	: Resolution(100, 100)
{
	PrimaryComponentTick.bCanEverTick = true;
	this->bTickInEditor = false;
	this->bAutoRegister = true;
	this->bAutoActivate = false; // activates when Initialize is called

	this->SetEnableGravity(false);
	this->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	this->bGenerateOverlapEvents = false;
}

void UAURVideoScreenBase::Initialize(UAURDriver* Driver)
{
	if (Driver)
	{
		this->VideoDriver = Driver;

		if (GetWorld())
		{
			this->VideoDriver->SetWorld(this->GetWorld());
		}
		else
		{
			UE_LOG(LogAUR, Error, TEXT("UAURVideoScreenBase::Initialize VideoDriver->GetWorld is null"));
		}

		this->ScreenMaterial = this->FindScreenMaterial();

		if (!this->ScreenMaterial)
		{
			UE_LOG(LogAUR, Error, TEXT("AVideoDisplaySurface::InitDynamicMaterial(): cannot find the material with proper texture parameter"));
			return;
		}

		// React to Driver setting the video parameters
		VideoDriver->OnCameraParametersChange.AddUniqueDynamic(this, &UAURVideoScreenBase::OnCameraPropertiesChange);

		UE_LOG(LogAUR, Log, TEXT("UAURVideoScreenBase initialized"));

		this->Activate();
	}
	else
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoScreenBase::Initialize The driver passed is null"));
	}
}

void UAURVideoScreenBase::Activate(bool bReset)
{
	if (this->VideoDriver)
	{
		UE_LOG(LogAUR, Log, TEXT("UAURVideoScreenBase activates"));
		Super::Activate(bReset);
	}
	else
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoScreenBase::Activate: Have no Driver to display, will not activate"));
	}
}

void UAURVideoScreenBase::Deactivate()
{
	this->VideoDriver = nullptr;
	Super::Deactivate();
}

UMaterialInstanceDynamic* UAURVideoScreenBase::FindScreenMaterial()
{
	// Iterate over materials to find the reference to the dynamic texture
	// so that its content can be written to later.

	for (int32 material_idx = 0; material_idx < this->GetNumMaterials(); material_idx++)
	{
		UMaterialInterface* material = this->GetMaterial(material_idx);
		UTexture* texture_param_value = nullptr;
		if (material->GetTextureParameterValue("VideoTexture", texture_param_value))
		{
			UMaterialInstanceDynamic* dynamic_material_instance = Cast<UMaterialInstanceDynamic>(material);
			if (!dynamic_material_instance)
			{
				dynamic_material_instance = UMaterialInstanceDynamic::Create(material, this);
				this->SetMaterial(material_idx, dynamic_material_instance);
			}

			return dynamic_material_instance;
		}
	}
	
	return nullptr;
}

void UAURVideoScreenBase::OnCameraPropertiesChange(UAURDriver* Driver)
{
	if (VideoDriver)
	{
		SetResolution(VideoDriver->GetResolution());
		SetSizeForFOV(VideoDriver->GetFieldOfView().X);

		if (this->ScreenMaterial)
		{
			this->ScreenMaterial->SetTextureParameterValue(FName("VideoTexture"), VideoDriver->GetOutputTexture());
		}
	}
}

void UAURVideoScreenBase::SetResolution(FIntPoint resolution)
{
	Resolution = resolution;
	// the X size is tied to FOV, so scale Y with respect to X
	// With the new texture, the XY placement of texture is swapped
	//this->SetRelativeScale3D(FVector(RelativeScale3D.Y * (float)Resolution.Y / (float)Resolution.X, RelativeScale3D.Y, 1));
}

void UAURVideoScreenBase::SetSizeForFOV(float FOV_Horizontal)
{
	// Get camera parameters
	//FIntPoint cam_resolution;
	//float cam_fov, cam_aspect_ratio;
	//this->VideoDriver->GetCameraParameters(cam_resolution, cam_fov, cam_aspect_ratio);

	// Get local position
	float distance_to_origin = this->GetRelativeTransform().GetLocation().Size();

	// Try to adjust size so that it fills the whole screen
	float width = distance_to_origin * 2.0 * FMath::Tan(FMath::DegreesToRadians(0.5 * FOV_Horizontal));
	float height = width * (float)Resolution.Y / (float)Resolution.X;

	// The texture size is 100x100
	//this->SetRelativeScale3D(FVector(width / 100.0, height / 100.0, 1));
	// With the new texture, the XY placement of texture is swapped
	this->SetRelativeScale3D(FVector(height / 100.0, width / 100.0, 1));


	FString msg = "UAURVideoScreenBase::InitScreenSize(" + FString::SanitizeFloat(FOV_Horizontal) + ") " 
		+ FString::SanitizeFloat(this->RelativeScale3D.X) + " x " + FString::SanitizeFloat(this->RelativeScale3D.Y);
	UE_LOG(LogAUR, Log, TEXT("%s"), *msg)
}
