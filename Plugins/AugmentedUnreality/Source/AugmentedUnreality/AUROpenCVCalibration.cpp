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

	/*
	uint16 config_video_width = 0;
	uint16 config_video_height = 0;

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

	this->Tracker.SetCameraParameters(this->CameraMatrix, this->CameraDistortionCoefficients);

	param_ss << '\n'
		<< "width: " << config_video_width << '\n'
		<< "height: " << config_video_height << '\n'
		<< "fovx: " << fovx << '\n'
		<< "fovy: " << fovy << '\n'
		<< "f: " << focalLength << '\n'
		<< "aspect ratio: " << aspectRatio << '\n';
	UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Camera parameters %s"), UTF8_TO_TCHAR(param_ss.str().c_str()))




		UE_LOG(LogAUR, Error, TEXT("AURDriverOpenCV: Failed to load config file %s"), *this->CalibrationFilePath)
	*/

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

void FOpenCVCameraProperties::PrintToLog() const
{
	std::stringstream param_ss;

	param_ss << '\n'
		//	<< "width: " << config_video_width << '\n'
		//	<< "height: " << config_video_height << '\n'
		<< "FOV horizontal: " << FOV.X << '\n'
		<< "FOV vertical: " << FOV.Y << '\n'
		;
	//	<< "f: " << focalLength << '\n'
	//	<< "aspect ratio: " << aspectRatio << '\n';
	UE_LOG(LogAUR, Log, TEXT("OpenCVCameraProperties: Camera properties %s"), UTF8_TO_TCHAR(param_ss.str().c_str()))
}

/**
if (false)
{
	calibration_detected_points.clear();
	bool found = cv::findCirclesGrid(CapturedFrame, calibration_board_size, calibration_detected_points, cv::CALIB_CB_ASYMMETRIC_GRID);
	//bool found = findChessboardCorners(CapturedFrame, calibration_board_size, calibration_detected_points, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

	if (found)
	{
		UE_LOG(LogAUR, Log, TEXT("AURDriverOpenCV: Found calibration markers"))

			// If we create a temporary Mat from the 
			cv::drawChessboardCorners(CapturedFrame, calibration_board_size, calibration_detected_points, found);
	}
	else
	{
		UE_LOG(LogAUR, Log, TEXT("No"))
	}
}
*/
