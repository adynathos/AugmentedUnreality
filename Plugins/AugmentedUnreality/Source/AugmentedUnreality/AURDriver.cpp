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

#include <codecvt> // FString -> bytes conversion

UAURDriver::UAURDriver()
	: bPerformOrientationTracking(true)
	, TranslationScale(1.0)
	, SceneCenterInTrackerCoordinates(0, 0, 0)
	, SmoothingFilterInstance(nullptr)
	, Resolution(1920, 1080)
	, CameraFov(50)
	, CameraAspectRatio(1920.0 / 1080.0)
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

void UAURDriver::GetCameraParameters(FIntPoint & resolution, float & field_of_view_angle, float & aspect_ratio_x_to_y)
{
	resolution = this->Resolution;
	field_of_view_angle = this->CameraFov;
	aspect_ratio_x_to_y = this->CameraAspectRatio;
}

FAURVideoFrame * UAURDriver::GetFrame()
{
	return nullptr;
}

bool UAURDriver::IsNewFrameAvailable()
{
	return false;
}

FTransform UAURDriver::GetOrientation()
{
	FTransform out_orientation;
	float out_update_time;
	this->GetOrientationAndUpdateTime(out_orientation, out_update_time);
	return out_orientation;
}

float UAURDriver::GetLastOrientationUpdateTime()
{
	FTransform out_orientation;
	float out_update_time;
	this->GetOrientationAndUpdateTime(out_orientation, out_update_time);
	return out_update_time;
}

float UAURDriver::GetTimeSinceLastOrientationUpdate()
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
	OutOrientation = this->CurrentOrientation;
	OutUpdateTime = this->LastOrientationUpdateTime;
}

bool UAURDriver::IsNewOrientationAvailable()
{
	return false;
}

FString UAURDriver::GetDiagnosticText()
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

const std::string UAURDriver::FStringToBytes(FString const & ue_str)
{
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;

	return converter.to_bytes(std::wstring(*ue_str));
}

std::wstring UAURDriver::BytesToWString(std::string const & bytes)
{
	std::wstring result;
	result.assign(bytes.begin(), bytes.end());
	return result;
}
