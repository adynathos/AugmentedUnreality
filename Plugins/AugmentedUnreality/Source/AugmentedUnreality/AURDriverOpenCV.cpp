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

#include <opencv2/calib3d.hpp>
#include <sstream>
#include <utility> // swap

#define _USE_MATH_DEFINES
#include <math.h>

UAURDriverOpenCV::UAURDriverOpenCV()
	: CameraIndex(0)
	, bUseMaximalCameraResolution(true)
	, bHighlightMarkers(true)
	, bNewFrameReady(false)
	, bNewOrientationReady(false)
{
	this->TranslationScale = 2.0;
	this->CameraParametersFile = FPaths::GameContentDir() / "AugmentedUnreality" / "Cameras" / "opencv_calibration_default.xml";
}

void UAURDriverOpenCV::Initialize()
{
	Super::Initialize();

	this->bNewFrameReady = false;
	this->bNewOrientationReady = false;

	this->InitializeCamera();

	if (this->VideoCapture.isOpened())
	{
		this->WorkerFrame = new FAURVideoFrame(this->Resolution);
		this->AvailableFrame = new FAURVideoFrame(this->Resolution);
		this->PublishedFrame = new FAURVideoFrame(this->Resolution);

		this->InitializeWorker();
	}
}

void UAURDriverOpenCV::StoreNewOrientation(FTransform const & measurement)
{
	// Mutex of orientation vars
	FScopeLock(&this->OrientationLock);

	Super::StoreNewOrientation(measurement);
	this->bNewOrientationReady = true;
}

void UAURDriverOpenCV::CreateBoard(FArucoGridBoardDefinition const & definition)
{
	this->ArucoBoard = new cv::aruco::GridBoard;


	this->ArucoMarkerDictionary = cv::aruco::getPredefinedDictionary(
		cv::aruco::PREDEFINED_DICTIONARY_NAME(definition.DictionaryId));

	*this->ArucoBoard = cv::aruco::GridBoard::create(
		definition.GridWidth, definition.GridHeight,
		float(definition.MarkerSize), float(definition.SeparationSize),
		this->ArucoMarkerDictionary);

	cv::Size img_size;
	int32 marker_with_separation = definition.MarkerSize + definition.SeparationSize;
	// add one more separation as missing margin
	img_size.width = definition.GridWidth * marker_with_separation + definition.SeparationSize;
	img_size.height = definition.GridHeight * marker_with_separation + definition.SeparationSize;

	cv::Mat img_data;
	this->ArucoBoard->draw(img_size, img_data, definition.SeparationSize, 1);

	FString board_image_filename = FPaths::GameSavedDir() / definition.SavedFileName;
	cv::imwrite(TCHAR_TO_UTF8(*board_image_filename), img_data);
	UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Saved board to: %s"), *board_image_filename)
}

void UAURDriverOpenCV::InitializeCamera()
{
	this->CreateBoard(this->MarkerDefinition);

	// Load camera parameters
	cv::FileStorage cam_param_file(TCHAR_TO_UTF8(*this->CameraParametersFile), cv::FileStorage::READ);

	uint16 config_video_width = 0;
	uint16 config_video_height = 0;

	if (cam_param_file.isOpened())
	{
		std::stringstream param_ss;

		// these keys are case-sensitive
		cam_param_file["image_Width"] >> config_video_width;
		cam_param_file["image_Height"] >> config_video_height;
		cam_param_file["Distortion_Coefficients"] >> this->CameraDistortionCoefficients;
		cam_param_file["Camera_Matrix"] >> this->CameraMatrix;

		// Extract useful camera parameters
		double fovx, fovy, focalLength, aspectRatio;
		cv::Point2d principalPoint;

		cv::calibrationMatrixValues(this->CameraMatrix, cv::Size(config_video_width, config_video_height), 1, 1,
			fovx, fovy, focalLength, principalPoint, aspectRatio);

		this->CameraPixelRatio = aspectRatio;
		this->CameraFov = fovx;

		param_ss << '\n'
			<< "width: " << config_video_width << '\n'
			<< "height: " << config_video_height << '\n'
			<< "fovx: " << fovx << '\n'
			<< "fovy: " << fovy << '\n'
			<< "f: " << focalLength << '\n'
			<< "aspect ratio: " << aspectRatio << '\n';

		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Camera parameters %s"), BytesToWString(param_ss.str()).c_str())
	}
	else
	{
		UE_LOG(LogAUR, Error, TEXT("AURDriverOpenCV: Failed to load config file %s"), *this->CameraParametersFile)
	}

	// Start the video capture

	UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Trying to open camera with index %d"), this->CameraIndex);

	this->VideoCapture = cv::VideoCapture(this->CameraIndex);
	if (VideoCapture.isOpened())
	{
		if (this->bUseMaximalCameraResolution || (config_video_height == 0 || config_video_width == 0))
		{
			UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Trying maximal camera resolution"))

			// Request an impossibly high resolution to tell the camera to use max resolution
			VideoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 8192);
			VideoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 8192);
		}
		else
		{
			// Use the resolution specified in config
			VideoCapture.set(CV_CAP_PROP_FRAME_WIDTH, config_video_width);
			VideoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, config_video_height);
		}

		// Find the resolution used by the camera
		this->Resolution.X = VideoCapture.get(CV_CAP_PROP_FRAME_WIDTH);
		this->Resolution.Y = VideoCapture.get(CV_CAP_PROP_FRAME_HEIGHT);
		this->CameraAspectRatio = (float)this->Resolution.X / (float)this->Resolution.Y / this->CameraPixelRatio;

		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Using camera resolution %d x %d"), this->Resolution.X, this->Resolution.Y)
	}
	else
	{
		UE_LOG(LogAUR, Error, TEXT("AURDriverOpenCV: Failed to open VideoCapture"))
	}
}

void UAURDriverOpenCV::InitializeWorker()
{
	this->Worker = new FWorkerRunnable(this);
	this->WorkerThread = FRunnableThread::Create(this->Worker, TEXT("a"), 0, TPri_Normal);
}

void UAURDriverOpenCV::Shutdown()
{
	if (this->Worker)
	{
		this->Worker->Stop();
		this->WorkerThread->WaitForCompletion();

		if (this->WorkerThread)
		{
			delete this->WorkerThread;
			this->WorkerThread = nullptr;
		}

		delete this->Worker;
		this->Worker = nullptr;
	}

	this->VideoCapture.release();

	if (WorkerFrame) delete WorkerFrame;
	WorkerFrame = nullptr;
	if (AvailableFrame) delete AvailableFrame;
	AvailableFrame = nullptr;
	if (PublishedFrame) delete PublishedFrame;
	PublishedFrame = nullptr;

	//if (this->ArucoBoard) delete this->ArucoBoard;
	this->ArucoBoard = nullptr;
}

FAURVideoFrame* UAURDriverOpenCV::GetFrame()
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

bool UAURDriverOpenCV::IsNewFrameAvailable()
{
	return this->bNewFrameReady;
}

FTransform UAURDriverOpenCV::GetOrientation()
{
	// Accessing orientation
	FScopeLock lock(&this->OrientationLock);

	this->bNewOrientationReady = false;

	return this->CurrentOrientation;
}

FString UAURDriverOpenCV::GetDiagnosticText()
{
	return this->DiagnosticText;
}

FVector UAURDriverOpenCV::ConvertOpenCvVectorToUnreal(cv::Vec3f const & cv_vector)
{
	// UE.x = CV.y
	// UE.y = CV.x
	// UE.z = CV.z
	return FVector(cv_vector[1], cv_vector[0], cv_vector[2]);
}

UAURDriverOpenCV::FWorkerRunnable::FWorkerRunnable(UAURDriverOpenCV * driver)
	: Driver(driver)
	, bTracking(driver->bPerformOrientationTracking)
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

// UE4 has some custom c++ deallocator that crashes when trying to delete
// the type: std::vector<std::vector< cv::Point2f > > 
std::vector<std::vector< cv::Point2f > > MarkerCorners;

uint32 UAURDriverOpenCV::FWorkerRunnable::Run()
{
	std::vector<int> MarkerIds;

	UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Worker thread starts"))

	while(this->bContinue)
	{
		// get a new frame from camera - this blocks untill the next frame is available
		Driver->VideoCapture >> CapturedFrame; 

		bool non_empty_image = CapturedFrame.total() > 0;

		if (!non_empty_image)
		{
			UE_LOG(LogAUR, Error, TEXT("AURDriverOpenCV: Camera returned empty image"))
		}

		// Detect Aruco board
		// http://docs.opencv.org/3.1.0/db/da9/tutorial_aruco_board_detection.html#gsc.tab=0

		if (this->bTracking && non_empty_image)
		{
			cv::aruco::detectMarkers(CapturedFrame, Driver->ArucoMarkerDictionary, MarkerCorners, MarkerIds);

			//cv::aruco::detectMarkers(frame_copy, dictionary, marker_corners, marker_ids, parameters, rejectedCandidates);

			if (MarkerIds.size() > 0)
			{
				if (Driver->bHighlightMarkers)
				{
					cv::aruco::drawDetectedMarkers(CapturedFrame, MarkerCorners, MarkerIds);
				}

				// Translation and rotation reported by detector
				cv::Vec3d rotation_raw, translation_raw;

				int valid = cv::aruco::estimatePoseBoard(MarkerCorners, MarkerIds,
					*Driver->ArucoBoard, Driver->CameraMatrix, Driver->CameraDistortionCoefficients,
					rotation_raw, translation_raw);

				if (valid > 0)
				{
					if (Driver->bHighlightMarkers)
					{
						cv::aruco::drawAxis(CapturedFrame, Driver->CameraMatrix, Driver->CameraDistortionCoefficients,
							rotation_raw, translation_raw, 100);
					}
				
					// Retrieve cv_rotation from the angle-axis rotation vector the detector provided
					FVector rotation_axis = ConvertOpenCvVectorToUnreal(rotation_raw);
					float angle = rotation_axis.Size();
					rotation_axis.Normalize();
					FQuat cv_rotation(rotation_axis, angle);
					
					// The reported translation vector "translation_vec" is the marker location
					// relative to camera in camera's coordinate system.
					// To obtain camera position in 3D, rotate by the provided rotation.
					FVector translation_cv = ConvertOpenCvVectorToUnreal(translation_raw);
					FVector translation = cv_rotation.RotateVector(translation_cv);
					translation *= -1;
					translation -= this->Driver->SceneCenterInTrackerCoordinates;
					translation /= this->Driver->TranslationScale;

					Driver->DiagnosticText =
						"T_raw: " + translation_cv.ToString() +
						", T: " + translation.ToString();
						//						", R: " + cv_rotation.Rotator().ToString() +
				
					/*
					OpenCV's rotation is
					from a coord system with XY on the marker plane and Z upwards from the table
					to a coord system such that camera is looking forward along the Z axis.

					But UE's cameras look towards the X axis.
					So after the main rotation we apply a rotation that moves
					the Z axis onto the X axis.
					(-90 deg around Y axis)
					*/
					FQuat ue_rotation = cv_rotation * FQuat(FVector(0, 1, 0), -M_PI / 2);

					// Report the rotation and location to the driver.
					// mutex locking performed by driver
					Driver->StoreNewOrientation(FTransform(ue_rotation, translation, FVector(1, 1, 1)));
				}
			}
		}

		// ---------------------------
		// Create the frame to publish

		// Camera image is in BGR format
		BGR_Color* src_pixels = (BGR_Color*)CapturedFrame.data;

		// Frame to fill is in RGBA format
		FColor* dest_pixels = Driver->WorkerFrame->Image.GetData();

		int32 pixel_count = Driver->Resolution.X * Driver->Resolution.Y;
		for (int32 pixel_idx = 0; pixel_idx < pixel_count; pixel_idx++)
		{
			BGR_Color* src_pix = &src_pixels[pixel_idx];
			FColor* dest_pix = &dest_pixels[pixel_idx];

			dest_pix->R = src_pix->R;
			dest_pix->G = src_pix->G;
			dest_pix->B = src_pix->B;
		}

		{
			FScopeLock(&Driver->FrameLock);

			// Put the generated frame as available frame
			std::swap(Driver->WorkerFrame, Driver->AvailableFrame);
			Driver->bNewFrameReady = true;
		}
	}

	//A reallocation is not guaranteed to happen, and the vector capacity is not guaranteed to change due to calling this function.A typical alternative that forces a reallocation is to use swap :
//	ArrayOfPts2D().swap(MarkerCorners);   // clear x reallocating 

//	MarkerIds.clear();
//	MarkerCorners.clear();

	UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Worker thread ends"))

	return 0;
}

void UAURDriverOpenCV::FWorkerRunnable::Stop()
{
	this->bContinue = false;
}
