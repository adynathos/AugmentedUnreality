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

#include "AURDriver.h"
#include <vector>
#include "AURDriverThreaded.generated.h"

/**
 * A type of driver that spawns a thread which does camera capture.
 * Useful if the operation of capture is blocking - like in OpenCV.
 */
UCLASS(Blueprintable, BlueprintType)
class UAURDriverThreaded : public UAURDriver
{
	GENERATED_BODY()

public:
	UAURDriverThreaded();

	virtual void Initialize(AActor* parent_actor) override;
	virtual void Shutdown() override;

	virtual FIntPoint GetResolution() const override;
	virtual FAURVideoFrame* GetFrame() override;
	virtual bool IsNewFrameAvailable() const override;

protected:
	// Threaded capture model
	FCriticalSection FrameLock; // mutex which needs to be obtained before manipulating the frame pointers
	FThreadSafeBool bNewFrameReady; // is there a new frame in AvailableFrame

	FAURVideoFrame* WorkerFrame; // the frame processed by worker thread
	FAURVideoFrame* AvailableFrame; // the frame ready to be published
	FAURVideoFrame* PublishedFrame; // the frame currently held by the game

	FAURVideoFrame FrameInstances[3];

	//FCriticalSection TrackerLock; // mutex which needs to be obtained before using marker tracker
	
	TUniquePtr<FRunnable> Worker;
	TUniquePtr<FRunnableThread> WorkerThread;

	// Override this method and create the specific class of FRunnable.
	virtual FRunnable* CreateWorker();

	// Publish a new frame - switched the publicly available frame with the one held by background thread.
	virtual void StoreWorkerFrame();

	virtual void SetFrameResolution(FIntPoint const& new_res) override;

	// Call the delegates from game thread
	void NotifyVideoPropertiesChange();
	void NotifyCalibrationStatusChange();
};
