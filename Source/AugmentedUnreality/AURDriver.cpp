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
#include "AURDriver.h"
#include "tracking/AURFiducialPattern.h"

UAURDriver* UAURDriver::CurrentDriver = nullptr;
UAURDriver::FAURDriverInstanceChange UAURDriver::OnDriverInstanceChange;
TArray<UAURDriver::BoardRegistration> UAURDriver::RegisteredBoards;

UAURDriver::UAURDriver()
	: bPerformOrientationTracking(true)
	, DiagnosticLevel(EAURDiagnosticInfoLevel::AURD_Silent)
	, bActive(false)
	, FrameResolution(1, 1)
	, bCalibrationInProgress(false)
{
}

void UAURDriver::Initialize(AActor* parent_actor)
{
	SetWorld(parent_actor->GetWorld());
	RegisterDriver(this);
	bActive = true;

	// Create video sources
	for (auto const& vid_src_class : AvailableVideoSources)
	{
		if (vid_src_class)
		{
			UAURVideoSource* vid_src = NewObject<UAURVideoSource>(this, vid_src_class);
			VideoSourceInstances.Add(vid_src);

			// Get the list of configurations offered by this source
			vid_src->DiscoverConfigurations();
			VideoConfigurations.Append(vid_src->Configurations);
		}
		else
		{
			UE_LOG(LogAUR, Error, TEXT("UAURDriver::Initialize Null entry in AvailableVideoSources"));
		}
	}
}

void UAURDriver::Tick()
{
	if (this->bActive)
	{
		this->WriteFrameToTexture();
	}
	else
	{
		UE_LOG(LogAUR, Warning, TEXT("UAURVideoScreenBase::TickComponent: ticking but neither active nor has video driver"));
	}
}

void UAURDriver::WriteFrameToTexture()
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
			if (UpdateParameters->Texture2DResource && UpdateParameters->Texture2DResource->GetCurrentFirstMip() <= 0)
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
						UE_LOG(LogAUR, Error, TEXT("UAURDriver::WriteFrameToTexture null frame returned. It has probably been shut down."));
					}
				}
				else
				{
					//UE_LOG(LogAUR, Log, TEXT("UAURDriver::WriteFrameToTexture No frame ready"));
				}
			}
			else
			{
				//UE_LOG(LogAUR, Log, TEXT("UAURDriver::WriteFrameToTexture No texture"));
			}
		// We do not delete anything, because the region structures we use are members of the class,
		// and the frames belong to the driver.
		}
	);
}

void UAURDriver::Shutdown()
{
	bActive = false;

	UnregisterDriver(this);
}

void UAURDriver::OpenVideoSource(FAURVideoConfiguration const& VideoConfiguration)
{
	// Save the index to open same source on next run
	DefaultVideoSourceName = VideoConfiguration.Identifier;
	SaveConfig();
}

bool UAURDriver::OpenVideoSourceByName(FString const& VideoConfigurationName)
{
	for (auto const& cfg : VideoConfigurations)
	{
		if (cfg.Identifier == VideoConfigurationName)
		{
			OpenVideoSource(cfg);
			return true;
		}
	}

	UE_LOG(LogAUR, Warning, TEXT("UAURDriver::OpenVideoSourceByName: Missing configuration '%s'"), *VideoConfigurationName);
	return false;
}

bool UAURDriver::OpenVideoSourceDefault()
{
	// try opening the last used video source
	if (OpenVideoSourceByName(DefaultVideoSourceName))
	{
		return true;
	}
	// if its the first time, open the one with highest priority
	else if(VideoConfigurations.Num() > 0)
	{
		FAURVideoConfiguration* best_cfg = nullptr;

		for (auto& cfg : VideoConfigurations)
		{
			if ((!best_cfg) || cfg.Priority > best_cfg->Priority)
			{
				best_cfg = &cfg;
			}
		}

		if (best_cfg)
		{
			OpenVideoSource(*best_cfg);
			return true;
		}
	}

	return false;
}

bool UAURDriver::RegisterBoard(AAURFiducialPattern * board_actor, bool use_as_viewpoint_origin)
{
	UE_LOG(LogAUR, Error, TEXT("UAURDriver::RegisterBoard: Not implemented"))
	return false;
}

void UAURDriver::UnregisterBoard(AAURFiducialPattern * board_actor)
{
	UE_LOG(LogAUR, Error, TEXT("UAURDriver::UnregisterBoard: Not implemented"))
}

bool UAURDriver::IsConnected() const
{
	return false;
}

bool UAURDriver::IsCalibrated() const
{
	return false;
}

bool UAURDriver::IsCalibrationInProgress() const
{
	return this->bCalibrationInProgress;
}

float UAURDriver::GetCalibrationProgress() const
{
	return 0.0f;
}

void UAURDriver::StartCalibration()
{
	UE_LOG(LogAUR, Error, TEXT("UAURDriver::StartCalibration: This driver does not have calibration implemented"))
}

void UAURDriver::CancelCalibration()
{
}

FIntPoint UAURDriver::GetResolution() const
{
	return FIntPoint(0, 0);
}

FVector2D UAURDriver::GetFieldOfView() const
{
	return FVector2D(50., 50.);
}

FAURVideoFrame * UAURDriver::GetFrame()
{
	return nullptr;
}

bool UAURDriver::IsNewFrameAvailable() const
{
	return false;
}

void UAURDriver::SetDiagnosticInfoLevel(EAURDiagnosticInfoLevel NewLevel)
{
	DiagnosticLevel = NewLevel;
}

void UAURDriver::ToggleDiagnosticInfoLevel()
{
	const uint8 next = ((uint8)GetDiagnosticInfoLevel() + 1) % ((uint8)EAURDiagnosticInfoLevel::AURD_Advanced + 1);
	SetDiagnosticInfoLevel((EAURDiagnosticInfoLevel)next);
}

FString UAURDriver::GetDiagnosticText() const
{
	return "Not implemented";
}

void UAURDriver::SetFrameResolution(FIntPoint const & new_res)
{
	if (new_res.GetMin() <= 0)
	{
		UE_LOG(LogAUR, Error, TEXT("UAURDriver::SetFrameResolution: invalid resolution %d x %d"), new_res.X, new_res.Y);
		this->FrameResolution = FIntPoint(1, 1);
	}
	else
	{
		this->FrameResolution = new_res;
	}

	// Create transient texture to be able to draw on it
	this->OutputTexture = UTexture2D::CreateTransient(this->FrameResolution.X, this->FrameResolution.Y);
	this->OutputTexture->UpdateResource();

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
	whole_texture_region.Width = FrameResolution.X;
	whole_texture_region.Height = FrameResolution.Y;

	// The AVideoDisplaySurface::FTextureUpdateParameters struct
	// holds information sent from this class to the render thread.
	this->TextureUpdateParameters.Texture2DResource = (FTexture2DResource*)this->OutputTexture->Resource;
	this->TextureUpdateParameters.RegionDefinition = whole_texture_region;
	this->TextureUpdateParameters.Driver = this;
}

UAURDriver * UAURDriver::GetCurrentDriver()
{
	return CurrentDriver;
}

void UAURDriver::RegisterBoardForTracking(AAURFiducialPattern * board_actor, bool use_as_viewpoint_origin)
{
	RegisteredBoards.AddUnique(BoardRegistration(board_actor, use_as_viewpoint_origin));

	if (CurrentDriver)
	{
		CurrentDriver->RegisterBoard(board_actor, use_as_viewpoint_origin);
	}
}

void UAURDriver::UnregisterBoardForTracking(AAURFiducialPattern * board_actor)
{
	RegisteredBoards.RemoveAll([&](BoardRegistration const & entry) {
		return entry.Board == board_actor;
	});

	if (CurrentDriver)
	{
		CurrentDriver->UnregisterBoard(board_actor);
	}
}

void UAURDriver::BindToOnDriverInstanceChange(FAURDriverInstanceChangeSingle const & Slot)
{
	if (Slot.IsBound())
	{
		FScriptDelegate del;
		del.BindUFunction(const_cast<UObject*>(Slot.GetUObject()), Slot.GetFunctionName());
		OnDriverInstanceChange.AddUnique(del);
	}
}

UAURDriver::FAURDriverInstanceChange& UAURDriver::GetDriverInstanceChangeDelegate()
{
	return OnDriverInstanceChange;
}

void UAURDriver::UnbindOnDriverInstanceChange(UObject * SlotOwner)
{
	OnDriverInstanceChange.RemoveAll(SlotOwner);
}

void UAURDriver::RegisterDriver(UAURDriver* driver)
{
	UE_LOG(LogAUR, Log, TEXT("UAURDriver::RegisterDriver: Register driver: %s"), *driver->GetFullName())

	if (CurrentDriver)
	{
		UE_LOG(LogAUR, Warning, TEXT("UAURDriver::RegisterDriver: CurrentDriver is not null, replacing"))
	}

	CurrentDriver = driver;

	for (auto const & entry : RegisteredBoards)
	{
		CurrentDriver->RegisterBoard(entry.Board, entry.ViewpointOrigin);
	}

	OnDriverInstanceChange.Broadcast(CurrentDriver);
}

void UAURDriver::UnregisterDriver(UAURDriver* driver)
{
	UE_LOG(LogAUR, Log, TEXT("UAURDriver::UnregisterDrive: Remove driver: %s"), *driver->GetFullName());

	if (CurrentDriver == driver)
	{
		CurrentDriver = nullptr;
		OnDriverInstanceChange.Broadcast(CurrentDriver);
	}
}

FTransform UAURDriver::GetCurrentViewportTransform() const
{
	UE_LOG(LogAUR, Warning, TEXT("UAURDriver::GetCurrentViewportTransform(): Not implemented, returning identity"))
	return FTransform::Identity;
}
