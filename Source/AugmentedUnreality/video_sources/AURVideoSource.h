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

#include "Object.h"
#include "AUROpenCV.h"
#include "AUROpenCVCalibration.h"
#include "AURVideoSource.generated.h"

/**
 * Base class for a video stream source.
 * Extends UObject for BP editing.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class UAURVideoSource : public UObject
{
	GENERATED_BODY()
	
public:
	// Name to be displayed in the list of available sources
	// Leave empty to generate use a name created from the source's settings
	UPROPERTY(EditAnywhere, Category = VideoSource)
	FText SourceName;

	// Name of the file to save/load calibration from, relative to Saved/AugmentedUnreality/Calibration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	FString CalibrationFileName;

	// Name to be displayed in the list of available sources
	UFUNCTION(BlueprintCallable, Category = VideoSource)
	virtual FText GetSourceName() const;

	UAURVideoSource();

	// Attempt to start streaming the video, returns true on success.
	virtual bool Connect();

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
	bool LoadCalibration();

	FOpenCVCameraProperties const& GetCameraProperties() const 
	{
		return CameraProperties;
	}

	// Use CameraProperties obtained during calibration and save them to file.
	void SaveCalibration(FOpenCVCameraProperties const& NewCalibration);

protected:
	bool bCalibrated;

	FOpenCVCameraProperties CameraProperties;

	FString GetCalibrationFileFullPath() const;

	static const FString CalibrationDir;
};
