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
#include "tracking/AURArucoTracker.h"
#include "video_sources/AURVideoSource.h"

#include "AURDriverOpenCV.generated.h"

/**
 *
 */
UCLASS(Blueprintable, BlueprintType, Config=Game)
class UAURDriverOpenCV : public UAURDriverThreaded
{
	GENERATED_BODY()

public:
	// ONLY SET THESE PROPERTIES BEFORE CALLING Initialize()	 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	FArucoTrackerSettings TrackerSettings;

	// Automatically creates those video sources on Initialize
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AugmentedReality)
	TArray< TSubclassOf<UAURVideoSource> > DefaultVideoSources;

	/* 
	Index of the last used video source, saved in config so that
	it is opened on next start 
	*/
	UPROPERTY(Config)
	int32 DefaultVideoSourceIndex;

	// Convenience list of existing instances of video sources
	// Sources from outside this list can be used as well for SetVideoSource
	UPROPERTY(Transient, BlueprintReadOnly, Category = AugmentedReality)
	TArray<UAURVideoSource*> AvailableVideoSources;

	// Get the currently active video source
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	UAURVideoSource* GetVideoSource();

	// Switch to a new video source object, nullptr to disable video
	// This does not intantly change the VideoSource variable - wait for the other
	// thread to close the previous one and open new one.
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	void SetVideoSource(UAURVideoSource* NewVideoSource);

	// Switch to a new video source from AvailableVideoSources list
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	void SetVideoSourceByIndex(const int32 index);

	//UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	//void SetTrackingBoardDefinition(AAURMarkerBoardDefinitionBase* board_definition);

	UAURDriverOpenCV();

	virtual void Initialize(AActor* parent_actor) override;
	virtual void Tick() override;

	virtual bool OpenDefaultVideoSource() override;
	virtual bool RegisterBoard(AAURMarkerBoardDefinitionBase* board_actor, bool use_as_viewpoint_origin = false) override;
	virtual void UnregisterBoard(AAURMarkerBoardDefinitionBase* board_actor) override;

	virtual FVector2D GetFieldOfView() const override;
	
	virtual bool IsConnected() const override;
	virtual bool IsCalibrated() const override;
	virtual float GetCalibrationProgress() const override;
	virtual void StartCalibration() override;
	virtual void CancelCalibration() override;

	virtual FString GetDiagnosticText() const override;

protected:
	FCriticalSection VideoSourceLock;

	// Video source instance providing the video stream for the AR
	UPROPERTY(Transient)
	UAURVideoSource* VideoSource;

	// Switch to this video source on next iteration
	UPROPERTY(Transient)
	UAURVideoSource* NextVideoSource;

	// Camera calibration
	FCriticalSection CalibrationLock;
	FOpenCVCameraCalibrationProcess CalibrationProcess;

	// Marker tracking
	FAURArucoTracker Tracker;

	FString DiagnosticText;

	// Called by the worker thread when the new video source is ready
	void OnVideoSourceSwitch();

	//void LoadCalibrationFile();
	void OnCalibrationFinished();
	// set resolution_override to use this resolution instead of Source->GetResolution
	void OnCameraPropertiesChange(FIntPoint resolution_override = FIntPoint(0, 0));

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

		cv::Mat CapturedFrame;
	};
};
