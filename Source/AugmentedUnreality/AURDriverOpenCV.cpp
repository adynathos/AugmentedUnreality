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

#include <sstream>
#include <utility> // swap

#define _USE_MATH_DEFINES
#include <math.h>

UAURDriverOpenCV::UAURDriverOpenCV()
	: CameraIndex(0)
	, TrackingBoardDefinition(nullptr)
{
	this->CameraProperties.SetResolution(this->Resolution);
}

/*
void UAURDriverOpenCV::SetTrackingBoardDefinition(AAURMarkerBoardDefinitionBase * board_definition)
{
	TrackingBoardDefinition = board_definition;
	Tracker.UpdateBoardDefinition(TrackingBoardDefinition);
}*/

void UAURDriverOpenCV::Initialize()
{
	this->LoadCalibrationFile();
	this->Tracker.SetSettings(this->TrackerSettings);
	this->CameraProperties.SetResolution(this->Resolution);

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

bool UAURDriverOpenCV::RegisterBoard(AAURMarkerBoardDefinitionBase * board_actor, bool use_as_viewpoint_origin)
{
	return Tracker.RegisterBoard(board_actor, use_as_viewpoint_origin);
}

void UAURDriverOpenCV::UnregisterBoard(AAURMarkerBoardDefinitionBase * board_actor)
{
	Tracker.UnregisterBoard(board_actor);
}

bool UAURDriverOpenCV::CreateCameraCapture()
{
	if (!VideoFile.IsEmpty())
	{
		FString fpath = FPaths::GameDir() / VideoFile;

		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Trying to open file: [%s]"), *fpath);

		VideoSourceFile* vid_src = new VideoSourceFile;
		vid_src->OpenFile(fpath);
		VideoSrc.Reset(vid_src);
	}
	else if (!CameraConnectionString.IsEmpty())
	{
		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Trying to open stream: [%s]"), *CameraConnectionString);
		
		VideoSourceStream* vid_src = new VideoSourceStream;
		vid_src->OpenStream(this->CameraConnectionString);
		VideoSrc.Reset(vid_src);
	}
	else
	{
		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Trying to open camera with index %d"), CameraIndex);

		VideoSourceCamera* vid_src = new VideoSourceCamera;
		vid_src->OpenCamera(this->CameraIndex, this->Resolution);
		VideoSrc.Reset(vid_src);
	}

	//return CameraCapture.isOpened();

	return VideoSrc->IsConnected();

	/*
	if (CameraConnectionString.IsEmpty())
	{
		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Trying to open camera with index %d"), CameraIndex);
		CameraCapture.open(this->CameraIndex);

		if (CameraCapture.isOpened())
		{
			// Use the resolution specified
			CameraCapture.set(cv::CAP_PROP_FRAME_WIDTH, this->Resolution.X);
			CameraCapture.set(cv::CAP_PROP_FRAME_HEIGHT, this->Resolution.Y);
		}
	}
	else
	{
		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Trying to open camera address: [%s]"), *CameraConnectionString);
		CameraCapture.open(TCHAR_TO_UTF8(*CameraConnectionString), cv::CAP_GSTREAMER);
	}

	return CameraCapture.isOpened();
	*/
}



bool UAURDriverOpenCV::ConnectToCamera()
{
	bool result = CreateCameraCapture();

	if (!result) {
		UE_LOG(LogAUR, Error, TEXT("AURDriverOpenCV: Failed to open VideoCapture"))
			return false;
	}

	// Find the resolution used by the camera
	/*FIntPoint camera_res;
	camera_res.X = FPlatformMath::RoundToInt(CameraCapture.get(cv::CAP_PROP_FRAME_WIDTH));
	camera_res.Y = FPlatformMath::RoundToInt(CameraCapture.get(cv::CAP_PROP_FRAME_HEIGHT));
	*/

	FIntPoint camera_res = VideoSrc->GetResolution();

	// Show the reported FPS


	if (camera_res.X <= 0 || camera_res.Y <= 0)
	{
		UE_LOG(LogAUR, Warning, TEXT("AURDriverOpenCV: Can not determine resolution now - camera returned resolution %d x %d"),
			camera_res.X, camera_res.Y)
	}
	else if (camera_res != Resolution)
	{
		UE_LOG(LogAUR, Warning, TEXT("AURDriverOpenCV: Camera returned resolution %d x %d even though %d x %d was requested"),
			camera_res.X, camera_res.Y, Resolution.X, Resolution.Y)

		Resolution = camera_res;
		CameraProperties.SetResolution(camera_res);
	}

	// this will allocate the frame with proper size
	OnCameraPropertiesChange();

	// Announce the fact that connection is established
	bConnected = true;
	NotifyConnectionStatusChange();

	return true;
}

void UAURDriverOpenCV::DisconnectCamera()
{
	/*if (CameraCapture.isOpened())
	{
		this->CameraCapture.release();
	}*/
	VideoSrc->Disconnect();

	bConnected = false;
	NotifyConnectionStatusChange();
}

void UAURDriverOpenCV::LoadCalibrationFile()
{
	FString calib_file_path = this->GetCalibrationFileFullPath();
	if (this->CameraProperties.LoadFromFile(calib_file_path))
	{
		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Calibration loaded from %s"), *calib_file_path)
		this->bCalibrated = true;
	}
	else
	{
		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Failed to load calibration from %s"), *calib_file_path)
		this->bCalibrated = false;

		calib_file_path = this->GetCalibrationFallbackFileFullPath();
		if (this->CameraProperties.LoadFromFile(calib_file_path))
		{
			UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Fallback calibration loaded from %s"), *calib_file_path)
		}
		else
		{
			UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Failed to load fallback calibration from %s"), *calib_file_path)
		}
	}

	if (this->CameraProperties.Resolution != this->Resolution)
	{
		UE_LOG(LogAUR, Warning, TEXT("AURDriverOpenCV: The resolution in the calibration file is different than the desired resolution of the driver. Trying to convert."))
		this->CameraProperties.SetResolution(this->Resolution);
	}

	this->OnCameraPropertiesChange();
}

void UAURDriverOpenCV::OnCalibrationFinished()
{
	this->CameraProperties = this->CalibrationProcess.GetCameraProperties();
	this->bCalibrated = true;
	this->bCalibrationInProgress = false;
	this->CameraProperties.SaveToFile(this->GetCalibrationFileFullPath());

	// Notify about the change
	this->OnCameraPropertiesChange();

	// Notify about calibration end
	this->NotifyCalibrationStatusChange();
}

void UAURDriverOpenCV::OnCameraPropertiesChange()
{
	this->CameraProperties.PrintToLog();

	// Give the camera matrix to the tracker
	this->Tracker.SetCameraProperties(this->CameraProperties);

	// Allocate proper frame sizes
	this->SetFrameResolution(this->GetResolution());

	this->NotifyCameraParametersChange();
}

FRunnable * UAURDriverOpenCV::CreateWorker()
{
	return new FWorkerRunnable(this);
}

FIntPoint UAURDriverOpenCV::GetResolution() const
{
	return this->CameraProperties.Resolution;
}

FVector2D UAURDriverOpenCV::GetFieldOfView() const
{
	return this->CameraProperties.FOV;
}

float UAURDriverOpenCV::GetCalibrationProgress() const
{
	return this->CalibrationProcess.GetProgress();
}

void UAURDriverOpenCV::StartCalibration()
{
	FScopeLock lock(&this->CalibrationLock);

	this->CalibrationProcess.Reset();
	this->bCalibrationInProgress = true;

	this->NotifyCalibrationStatusChange();
}

void UAURDriverOpenCV::CancelCalibration()
{
	FScopeLock lock(&this->CalibrationLock);

	this->CalibrationProcess.Reset();
	this->bCalibrationInProgress = false;

	this->NotifyCalibrationStatusChange();
}

FString UAURDriverOpenCV::GetDiagnosticText() const
{
	return this->DiagnosticText;
}

UAURDriverOpenCV::FWorkerRunnable::FWorkerRunnable(UAURDriverOpenCV * driver)
	: Driver(driver)
{
	FIntPoint res = driver->Resolution;
	CapturedFrame = cv::Mat(res.X, res.Y, CV_8UC3, cv::Scalar(0, 0, 255));
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

	// Start the video capture
	if (!this->Driver->ConnectToCamera())
	{
		// if connection failed, do not run the main loop
		this->bContinue = false;
	}

	while (this->bContinue)
	{
		// get a new frame from camera - this blocks untill the next frame is available
		//Driver->CameraCapture >> CapturedFrame;
		Driver->VideoSrc->GetNextFrame(CapturedFrame);

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
			if (frame_size.width != Driver->Resolution.X || frame_size.height != Driver->Resolution.Y)
			{
				FIntPoint new_camera_res(frame_size.width, frame_size.height);

				UE_LOG(LogAUR, Warning, TEXT("AURDriverOpenCV: Adjusting resolution to match the frame returned by camera: %dx%d (previously %dx%d)"),
					new_camera_res.X, new_camera_res.Y, Driver->Resolution.X, Driver->Resolution.Y);

				Driver->Resolution = new_camera_res;
				Driver->CameraProperties.SetResolution(new_camera_res);

				// this will allocate the frame with proper size
				Driver->OnCameraPropertiesChange();
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

	// Exiting the loop means the program ends, so release camera
	Driver->DisconnectCamera();
	UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Worker thread ends"))

	return 0;
}

void UAURDriverOpenCV::FWorkerRunnable::Stop()
{
	this->bContinue = false;
}

void VideoSourceCvCapture::Disconnect()
{
	if (Capture.isOpened())
	{
		this->Capture.release();
	}
}

bool VideoSourceCvCapture::IsConnected() const
{
	return Capture.isOpened();
}

FIntPoint VideoSourceCvCapture::GetResolution() const
{
	FIntPoint camera_res;
	camera_res.X = FPlatformMath::RoundToInt(Capture.get(cv::CAP_PROP_FRAME_WIDTH));
	camera_res.Y = FPlatformMath::RoundToInt(Capture.get(cv::CAP_PROP_FRAME_HEIGHT));
	return camera_res;
}

void VideoSourceCvCapture::OnOpen()
{
}

bool VideoSourceCvCapture::GetNextFrame(cv::Mat & frame)
{
	return Capture.read(frame);
}

bool VideoSourceCamera::OpenCamera(int camera_index, FIntPoint desired_resolution)
{
	Capture.open(camera_index);

	if (Capture.isOpened() && desired_resolution.GetMin() > 0)
	{
		Capture.set(cv::CAP_PROP_FRAME_WIDTH, desired_resolution.X);
		Capture.set(cv::CAP_PROP_FRAME_HEIGHT, desired_resolution.Y);
	}

	return Capture.isOpened();
}

bool VideoSourceStream::OpenStream(FString connection_string)
{
	return Capture.open(TCHAR_TO_UTF8(*connection_string));
}

const double MAX_FPS = 60.0;
const double MIN_FPS = 0.1;

bool VideoSourceFile::OpenFile(FString file_path)
{
	bool success = Capture.open(TCHAR_TO_UTF8(*file_path));

	Period = 1.0;
	if (success)
	{
		double fps = Capture.get(cv::CAP_PROP_FPS);

		UE_LOG(LogAUR, Log, TEXT("VideoSourceFile: Reported FPS: %lf"), fps)
		fps = FMath::Clamp(fps, MIN_FPS, MAX_FPS);

		Period = 1.0 / fps;

		FrameCount = FPlatformMath::RoundToInt(Capture.get(cv::CAP_PROP_FRAME_COUNT));
	}

	return Capture.isOpened();
}

bool VideoSourceFile::GetNextFrame(cv::Mat & frame)
{
	FPlatformProcess::Sleep(Period);
	bool success = Capture.read(frame);

	if (Capture.get(cv::CAP_PROP_POS_FRAMES) >= FrameCount - 1)
	{
		Capture.set(cv::CAP_PROP_POS_FRAMES, 0);
	}

	return success;
}
