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
#include "AURDriverThreaded.h"
#include <utility> // swap
#include "Async.h"

UAURDriverThreaded::UAURDriverThreaded()
	: bNewFrameReady(false)
{
}

void UAURDriverThreaded::Initialize(AActor* parent_actor)
{
	Super::Initialize(parent_actor);

	this->bNewFrameReady = false;

	this->WorkerFrame = &this->FrameInstances[0];
	this->AvailableFrame = &this->FrameInstances[1];
	this->PublishedFrame = &this->FrameInstances[2];

	FRunnable* to_run = CreateWorker();
	if (to_run)
	{
		this->Worker.Reset(to_run);
		FString thread_name = this->GetName() + "_CameraCaptureThread";
		this->WorkerThread.Reset(FRunnableThread::Create(to_run, *thread_name, 0, TPri_Normal));
	}
}

void UAURDriverThreaded::Shutdown()
{
	if (this->Worker.IsValid())
	{
		this->Worker->Stop();

		if (this->WorkerThread.IsValid())
		{
			this->WorkerThread->WaitForCompletion();
		}

		// Destroy
		this->WorkerThread.Reset(nullptr);
		this->Worker.Reset(nullptr);
	}

	Super::Shutdown();
}

FRunnable * UAURDriverThreaded::CreateWorker()
{
	UE_LOG(LogAUR, Error, TEXT("UAURDriverThreaded::CreateWorker() not implemented"))
	return nullptr;
}

FIntPoint UAURDriverThreaded::GetResolution() const
{
	return FrameResolution;
}

FAURVideoFrame* UAURDriverThreaded::GetFrame()
{
	// If there is a new frame produced
	if(this->bNewFrameReady)
	{
		// Manipulating frame pointers
		FScopeLock lock(&this->FrameLock);

		// Put the new ready frame in PublishedFrame
		std::swap(this->AvailableFrame, this->PublishedFrame);

		this->bNewFrameReady.AtomicSet(false);
	}
	// if there is no new frame, return the old one again

	return this->PublishedFrame;
}

bool UAURDriverThreaded::IsNewFrameAvailable() const
{
	return this->bNewFrameReady;
}

void UAURDriverThreaded::StoreWorkerFrame()
{
	FScopeLock(&this->FrameLock);

	// Put the generated frame as available frame
	std::swap(WorkerFrame, AvailableFrame);
	bNewFrameReady.AtomicSet(true);
}

void UAURDriverThreaded::SetFrameResolution(FIntPoint const & new_res)
{
	Super::SetFrameResolution(new_res);

	FrameResolution = new_res;
	for (int idx = 0; idx < 3; idx++)
	{
		this->FrameInstances[idx].SetResolution(new_res);
	}
}

void UAURDriverThreaded::NotifyVideoPropertiesChange()
{
	AsyncTask(ENamedThreads::GameThread, [this]() {
		UE_LOG(LogAUR, Log, TEXT("NotifyVideoSourceStatusChange"))
		this->OnVideoPropertiesChange.Broadcast(this);
	});
}

void UAURDriverThreaded::NotifyCalibrationStatusChange()
{
	AsyncTask(ENamedThreads::GameThread, [this]() {
		UE_LOG(LogAUR, Log, TEXT("NotifyCalibrationStatusChange"))
		this->OnCalibrationStatusChange.Broadcast(this);
	});
}
