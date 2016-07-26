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
#include "AURDriverOpenCV.h"
#include "AURSmoothingFilter.h"

UAURDriverOpenCV::UAURDriverOpenCV()
{
}

void UAURDriverOpenCV::Initialize()
{
	//this->LoadCalibrationFile();
	this->Tracker.SetSettings(this->TrackerSettings);

	AvailableVideoSources.Add(nullptr);

	for (auto const& vid_src_class : DefaultVideoSources)
	{
		UAURVideoSource* vid_src = NewObject<UAURVideoSource>(this, vid_src_class);
		AvailableVideoSources.Add(vid_src);
	}

	Super::Initialize();
}

void UAURDriverOpenCV::Tick()
{
	// 
	if (bActive)
	{
		FScopeLock lock(&this->TrackerLock);
		Tracker.PublishTransformUpdatesOnTick();
	}
}

UAURVideoSource * UAURDriverOpenCV::GetVideoSource()
{
	FScopeLock lock(&VideoSourceLock);
	return VideoSource;
}

void UAURDriverOpenCV::SetVideoSource(UAURVideoSource * NewVideoSource)
{
	FScopeLock lock(&VideoSourceLock);
	NextVideoSource = NewVideoSource;
}

bool UAURDriverOpenCV::OpenDefaultVideoSource()
{
	for (auto vid_src : AvailableVideoSources)
	{
		if (vid_src)
		{
			SetVideoSource(vid_src);
			return true;
		}
	}
	return false;
}

bool UAURDriverOpenCV::RegisterBoard(AAURMarkerBoardDefinitionBase * board_actor, bool use_as_viewpoint_origin)
{
	return Tracker.RegisterBoard(board_actor, use_as_viewpoint_origin);
}

void UAURDriverOpenCV::UnregisterBoard(AAURMarkerBoardDefinitionBase * board_actor)
{
	Tracker.UnregisterBoard(board_actor);
}

void UAURDriverOpenCV::OnVideoSourceSwitch()
{
	if (IsCalibrationInProgress())
	{
		CancelCalibration();
	}

	OnCameraPropertiesChange();
	NotifyVideoSourceStatusChange();
}

void UAURDriverOpenCV::OnCalibrationFinished()
{
	if (VideoSource)
	{
		VideoSource->SaveCalibration(CalibrationProcess.GetCameraProperties());
	}

	this->bCalibrationInProgress = false;

	// Notify about the change
	this->OnCameraPropertiesChange();

	// Notify about calibration end
	this->NotifyCalibrationStatusChange();
}

void UAURDriverOpenCV::OnCameraPropertiesChange()
{
	if (VideoSource)
	{
		VideoSource->GetCameraProperties().PrintToLog();

		// Give the camera matrix to the tracker
		this->Tracker.SetCameraProperties(VideoSource->GetCameraProperties());

		// Allocate proper frame sizes
		this->SetFrameResolution(VideoSource->GetResolution());

		this->NotifyCameraParametersChange();
	}
}

FRunnable * UAURDriverOpenCV::CreateWorker()
{
	return new FWorkerRunnable(this);
}

FVector2D UAURDriverOpenCV::GetFieldOfView() const
{
	if (VideoSource)
	{
		return VideoSource->GetCameraProperties().FOV;
	}
	else
	{
		return FIntPoint(1, 1);
	}
}

bool UAURDriverOpenCV::IsConnected() const
{
	if (VideoSource)
	{
		return VideoSource->IsConnected();
	}
	else
	{
		return false;
	}
}

bool UAURDriverOpenCV::IsCalibrated() const
{
	if (VideoSource)
	{
		return VideoSource->IsCalibrated();
	}
	else
	{
		return false;
	}
}

float UAURDriverOpenCV::GetCalibrationProgress() const
{
	return CalibrationProcess.GetProgress();
}

void UAURDriverOpenCV::StartCalibration()
{
	FScopeLock lock(&CalibrationLock);

	CalibrationProcess.Reset();
	bCalibrationInProgress = true;

	NotifyCalibrationStatusChange();
}

void UAURDriverOpenCV::CancelCalibration()
{
	FScopeLock lock(&CalibrationLock);

	CalibrationProcess.Reset();
	bCalibrationInProgress = false;

	NotifyCalibrationStatusChange();
}

FString UAURDriverOpenCV::GetDiagnosticText() const
{
	return this->DiagnosticText;
}

UAURDriverOpenCV::FWorkerRunnable::FWorkerRunnable(UAURDriverOpenCV * driver)
	: Driver(driver)
{
	CapturedFrame = cv::Mat(1920, 1080, CV_8UC3, cv::Scalar(0, 0, 255));
}

bool UAURDriverOpenCV::FWorkerRunnable::Init()
{
	this->bContinue = true;
	UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Worker init"))
	return true;
}

uint32 UAURDriverOpenCV::FWorkerRunnable::Run()
{
	UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Worker thread start"))

	UAURVideoSource* current_video_source = nullptr;

	while (this->bContinue)
	{
		bool new_video_source = false;

		// Switch video source
		{
			FScopeLock lock(&Driver->VideoSourceLock);

			if (current_video_source != Driver->NextVideoSource)
			{
				UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Switching video source"))

				if (current_video_source)
				{
					current_video_source->Disconnect();
				}

				current_video_source = Driver->NextVideoSource;
				// need to be kept in UPROPERTY for GC
				Driver->VideoSource = current_video_source;

				new_video_source = true;
			}
		}
		
		// activate new video source after switch
		if (new_video_source)
		{
			if (current_video_source)
			{
				current_video_source->Connect();
			}

			Driver->OnVideoSourceSwitch();
		}

		// If no video source or it is not open, wait for a change
		if (!current_video_source || !current_video_source->IsConnected())
		{
			FPlatformProcess::Sleep(0.25);
		}
		else
		{
			// get a new frame from camera - this blocks untill the next frame is available
			//Driver->CameraCapture >> CapturedFrame;
			//Driver->VideoSrc->GetNextFrame(CapturedFrame);

			// get a new frame from camera - this blocks untill the next frame is available
			current_video_source->GetNextFrame(CapturedFrame);

			// compare the frame size to the size we expect from capture parameters
			auto frame_size = CapturedFrame.size();
			if (frame_size.width <= 0 || frame_size.height <= 0)
			{
				UE_LOG(LogAUR, Error, TEXT("AURDriverOpenCV: Camera returned frame of wrong size: %dx%d"),
					frame_size.width, frame_size.height);
			}
			else
			{
				// Adjust the frame size if the camera returned a frame of different size than anticipated
				if (frame_size.width != Driver->FrameResolution.X || frame_size.height != Driver->FrameResolution.Y)
				{
					FIntPoint new_camera_res(frame_size.width, frame_size.height);

					UE_LOG(LogAUR, Warning, TEXT("AURDriverOpenCV: Adjusting resolution to match the frame returned by camera: %dx%d (previously %dx%d)"),
						new_camera_res.X, new_camera_res.Y, Driver->FrameResolution.X, Driver->FrameResolution.Y);

					Driver->SetFrameResolution(new_camera_res);
				}

				if (Driver->IsCalibrationInProgress()) // calibration
				{
					FScopeLock(&Driver->CalibrationLock);
					Driver->CalibrationProcess.ProcessFrame(CapturedFrame, Driver->WorldReference->RealTimeSeconds);

					if (Driver->CalibrationProcess.IsFinished())
					{
						Driver->OnCalibrationFinished();
					}
				}
				if (this->Driver->bPerformOrientationTracking)
				{
					/**
					* Tracking markers and relative position with respect to them
					*/
					{
						FScopeLock lock(&Driver->TrackerLock);
						Driver->Tracker.DetectMarkers(CapturedFrame);
					}
				}

				// ---------------------------
				// Create the frame to publish

				// Frame to fill is in RGBA format
				FColor* dest_pixel_ptr = Driver->WorkerFrame->Image.GetData();

				for (int32 pixel_r = 0; pixel_r < CapturedFrame.rows; pixel_r++)
				{
					for (int32 pixel_c = 0; pixel_c < CapturedFrame.cols; pixel_c++)
					{
						cv::Vec3b& src_pixel = CapturedFrame.at<cv::Vec3b>(pixel_r, pixel_c);

						// Captured image is in BGR format
						dest_pixel_ptr->R = src_pixel.val[2];
						dest_pixel_ptr->G = src_pixel.val[1];
						dest_pixel_ptr->B = src_pixel.val[0];

						dest_pixel_ptr++;
					}
				}

				Driver->StoreWorkerFrame();
			}
		}
	}

	// Disconnect video sources and notify the driver about that
	{
		FScopeLock lock(&Driver->VideoSourceLock);
		if (current_video_source && current_video_source->IsConnected())
		{
			current_video_source->Disconnect();
		}
		
		Driver->VideoSource = nullptr;
		Driver->NextVideoSource = nullptr;
		Driver->OnVideoSourceSwitch();
	}

	// Exiting the loop means the program ends, so release camera
	UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Worker thread ends"))

	return 0;
}

void UAURDriverOpenCV::FWorkerRunnable::Stop()
{
	this->bContinue = false;
}
