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

const char* FOpenCVCameraProperties::KEY_CAMERA_MATRIX = "CameraMatrix";
const char* FOpenCVCameraProperties::KEY_DISTORTION = "DistortionCoefficients";
const char* FOpenCVCameraProperties::KEY_FOV = "FieldOfView";

bool FOpenCVCameraProperties::LoadFromFile(FString const & file_path)
{
	try
	{
		cv::FileStorage cam_param_file(TCHAR_TO_UTF8(*file_path), cv::FileStorage::READ);

		if (!cam_param_file.isOpened())
		{
			return false;
		}

		cam_param_file[KEY_CAMERA_MATRIX] >> CameraMatrix;
		cam_param_file[KEY_DISTORTION] >> DistortionCoefficients;

		std::vector<double> fovs;
		cam_param_file[KEY_FOV] >> fovs;
		if (fovs.size() == 2)
		{
			FOV.Set(fovs[0], fovs[1]);
		}
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

	cam_param_file << KEY_CAMERA_MATRIX << CameraMatrix;
	cam_param_file << KEY_DISTORTION << DistortionCoefficients;
	cam_param_file << KEY_FOV << std::vector<double>{ FOV.X, FOV.Y };

	return true;
}

void FOpenCVCameraProperties::DeriveFOV(FIntPoint const resolution)
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

	cv::calibrationMatrixValues(this->CameraMatrix, cv::Size(resolution.X, resolution.Y), 1, 1,
		fov_x, fov_y, focal_length, principal_point, aspect_ratio);

	FOV.X = fov_x;
	FOV.Y = fov_y;

	std::stringstream param_ss;
	param_ss << "Extracting from CameraMatrix:\n"
		<< "Resolution: " << resolution.X << 'x' << resolution.Y << '\n'
		<< "FOV horizontal: " << FOV.X << '\n'
		<< "FOV vertical: " << FOV.Y << '\n'
		<< "Camera matrix: " << CameraMatrix << '\n'
		<< "Distortion coefficients: " << DistortionCoefficients << '\n';

	UE_LOG(LogAUR, Log, TEXT("OpenCVCameraProperties: %s"), UTF8_TO_TCHAR(param_ss.str().c_str()))
}

void FOpenCVCameraProperties::PrintToLog() const
{
	std::stringstream param_ss;

	
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
	cv::Mat new_calib_points;

	bool found = cv::findCirclesGrid(frame, PatternSize, new_calib_points, cv::CALIB_CB_ASYMMETRIC_GRID);

	if (found)
	{
		// Draw to show that we have found the points
		cv::drawChessboardCorners(frame, PatternSize, new_calib_points, found);
		
		// Store the captured frame if enough time has passed since the last was captured
		if (time_now >= LastFrameTime + MinInterval)
		{
			DetectedPointSets.push_back(new_calib_points);
			FramesCollected += 1;
			LastFrameTime = time_now;

			UE_LOG(LogAUR, Log, TEXT("FOpenCVCameraCalibrationProcess: Recorded %d/%d"), FramesCollected, FramesNeeded)

			// Have enough frames, finish the calibration
			if (FramesCollected >= FramesNeeded)
			{
				this->CalculateCalibration(FIntPoint(frame.size().width, frame.size().height));
			}
		}
	}

	return found;
}

void FOpenCVCameraCalibrationProcess::CalculateCalibration(FIntPoint const resolution)
{
	// Calculate points in the pattern
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

	FOpenCVVectorOfMat r_vecs, t_vecs;

	try 
	{
		cv::setIdentity(CameraProperties.CameraMatrix);
		CameraProperties.DistortionCoefficients.setTo(0.0);

		// the error is root-square-mean 
		double calibration_error = cv::calibrateCamera(object_points, DetectedPointSets, cv::Size(resolution.X, resolution.Y),
			CameraProperties.CameraMatrix, CameraProperties.DistortionCoefficients, *r_vecs, *t_vecs,
			CalibrationFlags);

		UE_LOG(LogAUR, Log, TEXT("FOpenCVCameraCalibrationProcess: Calibration finished, error: %lf"), calibration_error)

		CameraProperties.DeriveFOV(resolution);
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("CalculateCalibration: exception\n    %s"), UTF8_TO_TCHAR(exc.what()))
	}
}
