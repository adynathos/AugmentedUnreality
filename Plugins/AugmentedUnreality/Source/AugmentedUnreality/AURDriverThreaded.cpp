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
#include "AURDriverThreaded.h"
#include <utility> // swap

UAURDriverThreaded::UAURDriverThreaded()
	: bNewFrameReady(false)
	, bNewOrientationReady(false)
{
}

void UAURDriverThreaded::Initialize()
{
	Super::Initialize();

	this->bNewFrameReady = false;
	this->bNewOrientationReady = false;

	this->WorkerFrame = &this->FrameInstances[0];
	this->AvailableFrame = &this->FrameInstances[1];
	this->PublishedFrame = &this->FrameInstances[2];

	FRunnable* to_run = CreateWorker();
	if (to_run)
	{
		this->Worker.Reset(to_run);
		this->WorkerThread.Reset(FRunnableThread::Create(to_run, TEXT("AUR_Capture_Thread"), 0, TPri_Normal));
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
}

FRunnable * UAURDriverThreaded::CreateWorker()
{
	UE_LOG(LogAUR, Error, TEXT("UAURDriverThreaded::CreateWorker() not implemented"))
	return nullptr;
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

		this->bNewFrameReady = false;
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
	bNewFrameReady = true;
}

FTransform UAURDriverThreaded::GetOrientation()
{
	// Accessing orientation
	FScopeLock lock(&this->OrientationLock);

	this->bNewOrientationReady = false;
	return this->CurrentOrientation;
}

bool UAURDriverThreaded::IsNewOrientationAvailable() const
{
	return this->bNewOrientationReady;
}

void UAURDriverThreaded::StoreNewOrientation(FTransform const & measurement)
{
	// Mutex of orientation vars
	FScopeLock(&this->OrientationLock);

	Super::StoreNewOrientation(measurement);
	this->bNewOrientationReady = true;
}

void UAURDriverThreaded::SetFrameResolution(FIntPoint const & new_res)
{
	for (int idx = 0; idx < 3; idx++)
	{
		this->FrameInstances[idx].SetResolution(new_res);
	}
}
