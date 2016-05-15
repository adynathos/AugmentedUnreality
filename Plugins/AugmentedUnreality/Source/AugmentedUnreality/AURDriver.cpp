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
#include "AURDriver.h"
#include "AURSmoothingFilter.h"

UAURDriver::UAURDriver()
	: bPerformOrientationTracking(true)
	, CalibrationFilePath("AugmentedUnreality/Calibration/camera.xml")
	, CalibrationFallbackFilePath("AugmentedUnreality/Calibration/default.xml")
	, SmoothingFilterInstance(nullptr)
	, Resolution(800, 600)
	, bConnected(false)
	, bCalibrated(false)
	, bCalibrationInProgress(false)
{
}

void UAURDriver::Initialize()
{
	if (this->SmoothingFilterClass)
	{
		this->SmoothingFilterInstance = NewObject<UAURSmoothingFilter>(this, this->SmoothingFilterClass);
		this->SmoothingFilterInstance->Init();
	}
}

void UAURDriver::Tick()
{
}

void UAURDriver::Shutdown()
{
}

bool UAURDriver::IsConnected() const
{
	return false;
}

bool UAURDriver::IsCalibrated() const
{
	return this->bCalibrated;
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
	return this->Resolution;
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

FTransform UAURDriver::GetOrientation()
{
	return this->CurrentOrientation;
}

float UAURDriver::GetLastOrientationUpdateTime() const
{
	return this->LastOrientationUpdateTime;
}

float UAURDriver::GetTimeSinceLastOrientationUpdate() const
{
	if (this->WorldReference)
	{
		return this->WorldReference->RealTimeSeconds - this->GetLastOrientationUpdateTime();
	}
	else
	{
		return 1000;
	}
}

void UAURDriver::GetOrientationAndUpdateTime(FTransform & OutOrientation, float & OutUpdateTime)
{
	OutOrientation = this->GetOrientation();
	OutUpdateTime = this->GetLastOrientationUpdateTime();
}

bool UAURDriver::IsNewOrientationAvailable() const
{
	return false;
}

FString UAURDriver::GetDiagnosticText() const
{
	return "Not implemented";
}

void UAURDriver::StoreNewOrientation(FTransform const & measurement)
{
	if (this->SmoothingFilterInstance)
	{
		this->SmoothingFilterInstance->Measurement(measurement);
		this->CurrentOrientation = this->SmoothingFilterInstance->GetCurrentTransform();
	}
	else
	{
		this->CurrentOrientation = measurement;
	}

	if (WorldReference)
	{
		this->LastOrientationUpdateTime = WorldReference->RealTimeSeconds;
	}
	else
	{
		//UE_LOG(LogAUR, Error, TEXT("UAURDriver::StoreNewOrientation: GetWorld() is null"))
	}
}
