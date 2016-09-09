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

UAURVideoScreenBase::UAURVideoScreenBase(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
	, Resolution(100, 100)
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

		//this->SetResolution(VideoDriver->Resolution);

		UE_LOG(LogAUR, Log, TEXT("UAURVideoScreenBase initialized"));

		this->Activate();
	}
	else
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoScreenBase::Initialize The driver passed is null"));
	}
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

void UAURVideoScreenBase::SetResolution(FIntPoint resolution)
{
	if (resolution.GetMin() <= 0)
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoScreenBase::SetResolution: invalid resolution %d x %d"), resolution.X, resolution.Y);
		resolution.X = 1;
		resolution.Y = 1;
	}

	this->Resolution = resolution;

	// Create transient texture to be able to draw on it
	this->DynamicTexture = UTexture2D::CreateTransient(resolution.X, resolution.Y);

	if (!this->DynamicTexture)
	{
		UE_LOG(LogAUR, Error, TEXT("AVideoDisplaySurface::InitDynamicMaterial(): failed to create dynamic texture"));
		this->Deactivate();
	}
	else
	{
		this->DynamicTexture->UpdateResource();

		// Use this transient texture as argument for the material
		this->ScreenMaterial->SetTextureParameterValue(FName("VideoTexture"), this->DynamicTexture);

		/**
		To make a dynamic texture update, we need to specify the texture region which is being updated.
		A region is described by the {@link FUpdateTextureRegion2D} class.

		The region description will be used in the {@link #UpdateTexture()} method.
		**/
		FUpdateTextureRegion2D whole_texture_region;

		// Offset in source image data - we map the video data 1:1 to the texture
		whole_texture_region.SrcX = 0;
		whole_texture_region.SrcY = 0;

		// Offset in texture - we map the video data 1:1 to the texture
		whole_texture_region.DestX = 0;
		whole_texture_region.DestY = 0;

		// Size of the updated region equals to the size of video image
		whole_texture_region.Width = resolution.X;
		whole_texture_region.Height = resolution.Y;

		// The AVideoDisplaySurface::FTextureUpdateParameters struct
		// holds information sent from this class to the render thread.
		this->TextureUpdateParameters.Texture2DResource = (FTexture2DResource*)this->DynamicTexture->Resource;
		this->TextureUpdateParameters.RegionDefinition = whole_texture_region;
		this->TextureUpdateParameters.Driver = this->VideoDriver;
	}

	// the X size is tied to FOV, so scale Y with respect to X
	// With the new texture, the XY placement of texture is swapped
	this->SetRelativeScale3D(FVector(RelativeScale3D.Y * (float)Resolution.Y / (float)Resolution.X, RelativeScale3D.Y, 1));
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

void UAURVideoScreenBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (this->bIsActive && this->VideoDriver)
	{
		auto res = this->VideoDriver->GetResolution();

		if (!this->DynamicTexture || res.X != this->DynamicTexture->GetSizeX() || res.Y != this->DynamicTexture->GetSizeY())
		{
			this->SetResolution(res);
		}

		this->DisplayNextFrame();
	}
	else
	{
		UE_LOG(LogAUR, Warning, TEXT("UAURVideoScreenBase::TickComponent: ticking but neither active nor has video driver"));
	}
}

void UAURVideoScreenBase::DisplayNextFrame()
{
	/**
		The general code for updating UE's dynamic texture:
		https://wiki.unrealengine.com/Dynamic_Textures
		However, in case of a camera, we always update the whole image instead of chosen regions.
		Therefore, we can drop the multi-region-related code from the example.
		We will have just one constant region definition at this->WholeTextureRegion.
	**/
	//******************************************************************************************************************************
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		UpdateTextureRenderCommand,
		FTextureUpdateParameters*, UpdateParameters, &this->TextureUpdateParameters,
		{
			if (UpdateParameters->Texture2DResource->GetCurrentFirstMip() <= 0)
			{
				// Re-draw only if a new frame has been captured
				if (UpdateParameters->Driver->IsNewFrameAvailable())
				{
					FAURVideoFrame* new_video_frame = UpdateParameters->Driver->GetFrame();

					// If the driver was shut down, it will return null
					// better print an error than make the whole program disappear mysteriously
					if (new_video_frame)
					{
						/**
						Function signature, https://docs.unrealengine.com/latest/INT/API/Runtime/OpenGLDrv/FOpenGLDynamicRHI/RHIUpdateTexture2D/index.html

						virtual void RHIUpdateTexture2D
						(
						FTexture2DRHIParamRef Texture,
						uint32 MipIndex,
						const struct FUpdateTextureRegion2D & UpdateRegion,
						uint32 SourcePitch,
						const uint8 * SourceData
						)
						**/
						RHIUpdateTexture2D(
							UpdateParameters->Texture2DResource->GetTexture2DRHI(),
							0,
							UpdateParameters->RegionDefinition,
							sizeof(FColor) * UpdateParameters->RegionDefinition.Width, // width of the video in bytes
							new_video_frame->GetDataPointerRaw()
						);
					}
					else
					{
						UE_LOG(LogAUR, Error, TEXT("UAURVideoScreen::UpdateTexture driver returned null frame. It has probably been shut down."));
					}
				}
			}

			// We do not delete anything, because the region structures we use are members of the class,
			// and the frames belong to the driver.
		}
	);
}