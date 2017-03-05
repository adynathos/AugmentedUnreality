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
#include "AURVideoSource.h"

const FString UAURVideoSource::CalibrationDir = "AugmentedUnreality/Calibration";

FAURVideoConfiguration::FAURVideoConfiguration()
	: Identifier("INVALID")
	, VideoSourceObject(nullptr)
	, DisplayName(NSLOCTEXT("AUR", "InvalidVideoSourceConfiguration", "INVALID"))
	, Priority(-1.0)
{}

FAURVideoConfiguration::FAURVideoConfiguration(UAURVideoSource* parent, FString const& variant)
	: Identifier(parent->GetIdentifier() + "_" + variant)
	, VideoSourceObject(parent)
	, Priority(parent->PriorityMultiplier)
{
	if (variant.IsEmpty())
	{
		DisplayName = parent->GetSourceName();
	}
	else
	{
		DisplayName = FText::Format(NSLOCTEXT("AUR", "VideoConfigurationNameFmt", "{0}: {1}"), parent->GetSourceName(), FText::FromString(variant));
	}
}

void FAURVideoConfiguration::SetPriorityFromDesiredResolution(int32 desired_resolution_x, int32 stdev)
{
	Priority = 1.0 + FMath::Exp(-(float)FMath::Abs(Resolution.X - desired_resolution_x) / stdev);
	Priority *= VideoSourceObject->PriorityMultiplier;
}

UAURVideoSource::UAURVideoSource()
	: PriorityMultiplier(1.0)
	, bCalibrated(false)
{
}

FText UAURVideoSource::GetSourceName() const
{
	return NSLOCTEXT("AUR", "VideoSourceBase", "INVALID");
}

FString UAURVideoSource::GetIdentifier() const
{
	return "INVALID";
}

void UAURVideoSource::DiscoverConfigurations()
{
	Configurations.Empty();
}

bool UAURVideoSource::Connect(FAURVideoConfiguration const& configuration)
{
	CurrentConfiguration = configuration;
	return false;
}

bool UAURVideoSource::IsConnected() const
{
	return false;
}

void UAURVideoSource::Disconnect()
{
}

bool UAURVideoSource::GetNextFrame(cv::Mat_<cv::Vec3b>& frame)
{
	UE_LOG(LogAUR, Error, TEXT("UAURVideoSource::GetNextFrame: Not implemented"))
	return false;
}

FIntPoint UAURVideoSource::GetResolution() const
{
	UE_LOG(LogAUR, Error, TEXT("UAURVideoSource::GetResolution: Not implemented"))
	return FIntPoint(0, 0);
}

float UAURVideoSource::GetFrequency() const
{
	UE_LOG(LogAUR, Error, TEXT("UAURVideoSource::GetFrequency: Not implemented"))
	return 1.0f;
}

bool UAURVideoSource::LoadCalibration()
{
	FString calib_file_path = GetCalibrationFileFullPath();
	if (CameraProperties.LoadFromFile(calib_file_path))
	{
		UE_LOG(LogAUR, Log, TEXT("AURVideoSource: Calibration loaded from %s"), *calib_file_path)
		this->bCalibrated = true;
	}
	else
	{
		UE_LOG(LogAUR, Warning, TEXT("AURVideoSource: Failed to load calibration from %s"), *calib_file_path)
		this->bCalibrated = false;
	}

	if (IsConnected())
	{
		FIntPoint camera_res = GetResolution();

		if (camera_res.X <= 0 || camera_res.Y <= 0)
		{
			UE_LOG(LogAUR, Warning, TEXT("AURVideoSource: Can not determine resolution now - returned resolution %d x %d"),
				camera_res.X, camera_res.Y)
		}
		else
		{
			CameraProperties.SetResolution(camera_res);
		}
	}
	else
	{
		UE_LOG(LogAUR, Warning, TEXT("AURVideoSource::LoadCalibration: Is not connected, can not determine resolution"))
	}

	return this->bCalibrated;
}

void UAURVideoSource::SaveCalibration(FOpenCVCameraProperties const & NewCalibration)
{
	CameraProperties = NewCalibration;
	bCalibrated = true;
	CameraProperties.SaveToFile(GetCalibrationFileFullPath());
}

FString UAURVideoSource::ResolutionToString(FIntPoint const & resolution)
{
	return FString::Printf(TEXT("%dx%d"), resolution.X, resolution.Y);
}

FString UAURVideoSource::GetCalibrationFileFullPath() const
{
	FString file_name = CalibrationFileName;

	if (file_name.IsEmpty())
	{
		file_name = this->GetName() + TEXT(".xml");
	}

	return FPaths::GameSavedDir() / CalibrationDir / file_name;
}

/*
if (this->CameraProperties.Resolution != this->DesiredResolution)
{
	UE_LOG(LogAUR, Warning, TEXT("AURVideoSource: The resolution in the calibration file is different than the desired resolution of the driver. Trying to convert."))
		this->CameraProperties.SetResolution(this->Resolution);
}
*/
