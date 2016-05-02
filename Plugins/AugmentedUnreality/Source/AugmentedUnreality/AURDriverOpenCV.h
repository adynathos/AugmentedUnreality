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

#include "AURDriver.h"
#include <vector>
#include "OpenCV_includes.h"
#include "AUROpenCVCalibration.h"
#include "AURArucoTracker.h"

#include "AURDriverOpenCV.generated.h"

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class UAURDriverOpenCV : public UAURDriver
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	uint32 bHighlightMarkers : 1;

	UAURDriverOpenCV();

	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual FAURVideoFrame* GetFrame() override;
	virtual bool IsNewFrameAvailable() const override;
	virtual bool IsNewOrientationAvailable() const override;
	virtual FTransform GetOrientation() override;
	virtual FString GetDiagnosticText() const override;

protected:
	// Camera calibration
	FOpenCVCameraProperties CameraProperties;

	// Marker tracking
	FAURArucoTracker Tracker;

	// Threaded capture model
	FAURVideoFrame* WorkerFrame; // the frame processed by worker thread
	FAURVideoFrame* AvailableFrame; // the frame ready to be published
	FAURVideoFrame* PublishedFrame; // the frame currently held by tje game

	FAURVideoFrame FrameInstances[3];

	FCriticalSection FrameLock; // mutex which needs to be obtained before manipulating the frame pointers
	FThreadSafeBool bNewFrameReady; // is there a new frame in AvailableFrame

	FCriticalSection OrientationLock; // mutex which needs to be obtained before using CameraOrientation variable.
	FThreadSafeBool bNewOrientationReady;

	/**
		Adds thread safety to the storing operation
	*/
	virtual void StoreNewOrientation(FTransform const & measurement);

	void LoadCalibration();
	void InitializeWorker();

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

	TUniquePtr<FWorkerRunnable> Worker;
	TUniquePtr<FRunnableThread> WorkerThread;

	///=============================================
	FString DiagnosticText;
};
