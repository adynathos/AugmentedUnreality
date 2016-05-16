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

#include "AURDriverThreaded.h"
#include <vector>
#include "AUROpenCV.h"
#include "AUROpenCVCalibration.h"
#include "AURArucoTracker.h"

#include "AURDriverOpenCV.generated.h"

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class UAURDriverOpenCV : public UAURDriverThreaded
{
	GENERATED_BODY()

public:
	/**
	 *	ONLY SET THESE PROPERTIES BEFORE CALLING Initialize()
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	FArucoTrackerSettings TrackerSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	int32 CameraIndex;

	UAURDriverOpenCV();

	virtual void Initialize() override;

	virtual FIntPoint GetResolution() const override;
	virtual FVector2D GetFieldOfView() const override;
	
	virtual float GetCalibrationProgress() const override;
	virtual void StartCalibration() override;
	virtual void CancelCalibration() override;

	virtual FString GetDiagnosticText() const override;

protected:
	// Camera calibration
	FOpenCVCameraProperties CameraProperties;
	FCriticalSection CalibrationLock;
	FOpenCVCameraCalibrationProcess CalibrationProcess;

	// Marker tracking
	FAURArucoTracker Tracker;

	FString DiagnosticText;

	void LoadCalibrationFile();
	void OnCalibrationFinished();
	void OnCameraPropertiesChange();

	virtual FRunnable* CreateWorker() override;

	/**
	 * cv::VideoCapture::read blocks untill a new frame is available.
	 * If it was executed in the main thread, the main tick would be
	 * bound to the camera FPS.
	 * To prevent that, we use a worker thread.
	 * This also prevents the marker detection from slowing down the main thread.
	 */
	class FWorkerRunnable : public FRunnable
	{
	public:
		FWorkerRunnable(UAURDriverOpenCV* driver);

		// Begin FRunnable interface.
		virtual bool Init();
		virtual uint32 Run();
		virtual void Stop();
		// End FRunnable interface

	protected:
		// The driver on which we work
		UAURDriverOpenCV* Driver;

		// Set to false to stop the thread
		FThreadSafeBool bContinue;

		// Connection to the camera
		cv::VideoCapture VideoCapture;

		cv::Mat CapturedFrame;
	};
};
