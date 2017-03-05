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

#include "Object.h"
#include "AUROpenCV.h"
#include "AUROpenCVCalibration.h"
#include "AURVideoSource.generated.h"
class UAURVideoSource;

USTRUCT(BlueprintType)
struct FAURVideoConfiguration
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = VideoSource)
	FString Identifier;

	UPROPERTY(Transient, BlueprintReadOnly, Category = VideoSource)
	UAURVideoSource* VideoSourceObject;

	UPROPERTY(BlueprintReadOnly, Category = VideoSource)
	FText DisplayName;

	// How appropriate this configuration is for default at first launch
	UPROPERTY(BlueprintReadOnly, Category = VideoSource)
	float Priority;

// Information stored by VideoSource
	FIntPoint Resolution;
	FString FilePath;

	FAURVideoConfiguration();
	FAURVideoConfiguration(UAURVideoSource* parent, FString const& variant);

	void SetPriorityFromDesiredResolution(int32 desired_resolution_x, int32 stdev = 1024.0);
};

/**
 * Base class for a video stream source.
 * Extends UObject for BP editing.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class UAURVideoSource : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VideoSource)
	FText Description;

	// Name of the file to save/load calibration from, relative to Saved/AugmentedUnreality/Calibration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	FString CalibrationFileName;

	UPROPERTY(Transient, BlueprintReadOnly, Category = VideoSource)
	TArray<FAURVideoConfiguration> Configurations;

	UPROPERTY(Transient, BlueprintReadOnly, Category = VideoSource)
	FAURVideoConfiguration CurrentConfiguration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	float PriorityMultiplier;

	// Name to be displayed in the list of available sources
	UFUNCTION(BlueprintCallable, Category = VideoSource)
	virtual FText GetSourceName() const;

	virtual FString GetIdentifier() const;

	// Populates the Configurations list with available video versions.
	UFUNCTION(BlueprintCallable, Category = VideoSource)
	virtual void DiscoverConfigurations();

	UAURVideoSource();

	// Attempt to start streaming the video, returns true on success.
	virtual bool Connect(FAURVideoConfiguration const& configuration);

	// Is the stream open
	UFUNCTION(BlueprintCallable, Category = VideoSource)
	virtual bool IsConnected() const;

	// Close the connection/file/stream.
	virtual void Disconnect();

	// Read the next frame from the source - BLOCKING.
	virtual bool GetNextFrame(cv::Mat_<cv::Vec3b>& frame);

	UFUNCTION(BlueprintCallable, Category = VideoSource)
	virtual FIntPoint GetResolution() const;

	UFUNCTION(BlueprintCallable, Category = VideoSource)
	virtual float GetFrequency() const;

	UFUNCTION(BlueprintCallable, Category = VideoSource)
	bool IsCalibrated() const
	{
		return bCalibrated;
	}

	// Loads the calibration from file, returns true if file was found and correct.
	virtual bool LoadCalibration();

	FOpenCVCameraProperties const& GetCameraProperties() const
	{
		return CameraProperties;
	}

	// Use CameraProperties obtained during calibration and save them to file.
	void SaveCalibration(FOpenCVCameraProperties const& NewCalibration);

	static FString ResolutionToString(FIntPoint const& resolution);

protected:
	bool bCalibrated;

	FOpenCVCameraProperties CameraProperties;

	FString GetCalibrationFileFullPath() const;

	static const FString CalibrationDir;
};
