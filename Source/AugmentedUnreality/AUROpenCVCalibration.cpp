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
#include "AUROpenCVCalibration.h"
#include <sstream>

const char* FOpenCVCameraProperties::KEY_RESOLUTION = "Resolution";
const char* FOpenCVCameraProperties::KEY_CAMERA_MATRIX = "CameraMatrix";
const char* FOpenCVCameraProperties::KEY_DISTORTION = "DistortionCoefficients";

bool FOpenCVCameraProperties::LoadFromFile(FString const & file_path)
{
	try
	{
		cv::FileStorage cam_param_file(TCHAR_TO_UTF8(*file_path), cv::FileStorage::READ);

		if (!cam_param_file.isOpened())
		{
			return false;
		}

		std::vector<int32> res;
		cam_param_file[KEY_RESOLUTION] >> res;
		if (res.size() == 2)
		{
			Resolution.X = FPlatformMath::RoundToInt(res[0]);
			Resolution.Y = FPlatformMath::RoundToInt(res[1]);
		}

		cam_param_file[KEY_CAMERA_MATRIX] >> CameraMatrix;
		cam_param_file[KEY_DISTORTION] >> DistortionCoefficients;

		this->DeriveFOV();
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("Exception while reading file %s:\n%s"), *file_path, UTF8_TO_TCHAR(exc.what()))
		return false;
	}

	return true;
}

bool FOpenCVCameraProperties::SaveToFile(FString const & file_path) const
{
	// ensure directory exists
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FPaths::GetPath(file_path));
	cv::FileStorage cam_param_file(TCHAR_TO_UTF8(*file_path), cv::FileStorage::WRITE);

	if (!cam_param_file.isOpened())
	{
		return false;
	}

	cam_param_file << KEY_RESOLUTION << std::vector<int32>{ Resolution.X, Resolution.Y };
	cam_param_file << KEY_CAMERA_MATRIX << CameraMatrix;
	cam_param_file << KEY_DISTORTION << DistortionCoefficients;
	
	return true;
}

void FOpenCVCameraProperties::SetResolution(FIntPoint const & new_resolution)
{
	double resolution_new_to_old = 1.0;
	if (Resolution.X > 0)
	{
		resolution_new_to_old = double(new_resolution.X) / double(Resolution.X);
	}
	Resolution = new_resolution;

	// Scale focal distance by the same factor the resolution changed
	double current_f = CameraMatrix.at<double>(0, 0);
	double new_f = current_f * resolution_new_to_old;

	// Set focal distance
	CameraMatrix.at<double>(0, 0) = new_f;
	CameraMatrix.at<double>(1, 1) = new_f;

	// Set center of image
	CameraMatrix.at<double>(0, 2) = 0.5 * double(new_resolution.X);
	CameraMatrix.at<double>(1, 2) = 0.5 * double(new_resolution.Y);
}

void FOpenCVCameraProperties::DeriveFOV()
{
	/*
		CameraMatrix(0, 0) is f_x in "pixels" because "real" units cancel out: 
			x_img_pix = x_real / z_real * f_pix
		We fix the aspect ratio to 1, so f_x == f_y == f

		So an image plane of size (resX, resY) is located f away from camera center.
		So to calculate FOV from camera matrix
			tan(FovX/2) = (resX / 2) / f
	*/

	double fov_x, fov_y, focal_length, aspect_ratio;
	cv::Point2d principal_point;

	// given the resX, resY and f, calculate fov
	cv::calibrationMatrixValues(this->CameraMatrix, cv::Size(Resolution.X, Resolution.Y), 1, 1,
		fov_x, fov_y, focal_length, principal_point, aspect_ratio);

	FOV.X = fov_x;
	FOV.Y = fov_y;
}

void FOpenCVCameraProperties::PrintToLog() const
{
	std::stringstream param_ss;

	param_ss << "\n"
		<< "Resolution: " << Resolution.X << 'x' << Resolution.Y << '\n'
		<< "FOV horizontal: " << FOV.X << '\n'
		<< "FOV vertical: " << FOV.Y << '\n'
		<< "Camera matrix: " << CameraMatrix << '\n'
		<< "Distortion coefficients: " << DistortionCoefficients << '\n';
	
	UE_LOG(LogAUR, Log, TEXT("OpenCVCameraProperties: %s"), UTF8_TO_TCHAR(param_ss.str().c_str()))
}

FOpenCVCameraCalibrationProcess::FOpenCVCameraCalibrationProcess()
	: FramesNeeded(25)
	, MinInterval(0.75)
	, PatternSize(4, 11)
	, SquareSize(1.7) // cm if printed on A4 paper
	, CalibrationFlags(
		cv::CALIB_FIX_K4 | 
		cv::CALIB_FIX_K5 | 
		cv::CALIB_FIX_PRINCIPAL_POINT |
		cv::CALIB_ZERO_TANGENT_DIST |
		cv::CALIB_FIX_ASPECT_RATIO
	)
{
	Reset();
}

void FOpenCVCameraCalibrationProcess::Reset()
{
	FramesCollected = 0;
	LastFrameTime = 0;
	DetectedPointSets.clear();
}

bool FOpenCVCameraCalibrationProcess::ProcessFrame(cv::Mat& frame, float time_now)
{
	// Store the captured frame if enough time has passed since the last was captured
	if (time_now >= LastFrameTime + MinInterval)
	{
		cv::Mat new_calib_points;

		// Derive resolution from the given frame
		auto frame_size = frame.size();
		CameraProperties.SetResolution(FIntPoint(frame_size.width, frame_size.height));

		// Detect point positions in the given frame
		bool found = cv::findCirclesGrid(frame, PatternSize, new_calib_points, cv::CALIB_CB_ASYMMETRIC_GRID);

		if (found)
		{
			// Draw to show that we have found the points
			cv::drawChessboardCorners(frame, PatternSize, new_calib_points, found);

			DetectedPointSets.push_back(new_calib_points);
			FramesCollected += 1;
			LastFrameTime = time_now;

			UE_LOG(LogAUR, Log, TEXT("FOpenCVCameraCalibrationProcess: Recorded %d/%d"), FramesCollected, FramesNeeded)

			// Have enough frames, finish the calibration
			if (FramesCollected >= FramesNeeded)
			{
				this->CalculateCalibration();
			}
		}

		return found;
	}

	return false;
}

void FOpenCVCameraCalibrationProcess::CalculateCalibration()
{
	// Calculate points in the pattern
	// OpenCV does not write to the vector, so it can be created/deleted here
	std::vector< std::vector< cv::Point3f > > object_points(1);
	for (int8 col = 0; col < PatternSize.height; col++)
	{
		for (int8 row = 0; row < PatternSize.width; row++)
		{
			object_points[0].push_back(cv::Point3f(
				SquareSize * float(2 * row + col % 2),
				SquareSize * float(col),
				0));
		}
	}
	object_points.resize(DetectedPointSets.size(), object_points[0]);

	// OpenCV writes directoy to those vectors, so they need to be allocated/deleted outside AUR binary
	cv::aur_allocator::OpenCvWrapper< std::vector<cv::Mat> > r_vecs, t_vecs;

	try 
	{
		cv::setIdentity(CameraProperties.CameraMatrix);
		CameraProperties.DistortionCoefficients.setTo(0.0);

		// the error is root-square-mean 
		double calibration_error = cv::calibrateCamera(object_points, DetectedPointSets, cv::Size(CameraProperties.Resolution.X, CameraProperties.Resolution.Y),
			CameraProperties.CameraMatrix, CameraProperties.DistortionCoefficients, *r_vecs, *t_vecs,
			CalibrationFlags);

		UE_LOG(LogAUR, Log, TEXT("FOpenCVCameraCalibrationProcess: Calibration finished, error: %lf"), calibration_error)

		CameraProperties.DeriveFOV();
		CameraProperties.PrintToLog();
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("CalculateCalibration: exception\n    %s"), UTF8_TO_TCHAR(exc.what()))
	}
}
