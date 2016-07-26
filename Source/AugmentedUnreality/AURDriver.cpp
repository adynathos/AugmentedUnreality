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
//#include "AURSmoothingFilter.h"

UAURDriver* UAURDriver::CurrentDriver = nullptr;

UAURDriver::UAURDriver()
	: bPerformOrientationTracking(true)
	, FrameResolution(1280, 720)
	, bActive(false)
	, bCalibrationInProgress(false)
{
}

void UAURDriver::Initialize()
{
	if (CurrentDriver)
	{
		UE_LOG(LogAUR, Error, TEXT("UAURDriver::Initialize: CurrentDriver is not null"))
	}

	CurrentDriver = this;

	bActive = true;
}

void UAURDriver::Tick()
{
}

void UAURDriver::Shutdown()
{
	bActive = false;

	CurrentDriver = nullptr;
}

bool UAURDriver::OpenDefaultVideoSource()
{
	UE_LOG(LogAUR, Error, TEXT("UAURDriver::OpenDefaultVideoSource: Not implemented"))
	return false;
}

bool UAURDriver::RegisterBoard(AAURMarkerBoardDefinitionBase * board_actor, bool use_as_viewpoint_origin)
{
	UE_LOG(LogAUR, Error, TEXT("UAURDriver::RegisterBoard: Not implemented"))
	return false;
}

void UAURDriver::UnregisterBoard(AAURMarkerBoardDefinitionBase * board_actor)
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

FString UAURDriver::GetDiagnosticText() const
{
	return "Not implemented";
}

UAURDriver * UAURDriver::GetCurrentDriver()
{
	return CurrentDriver;
}