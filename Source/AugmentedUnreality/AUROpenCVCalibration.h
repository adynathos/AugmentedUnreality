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

#include "AUROpenCV.h"
#include "AUROpenCVCalibration.generated.h"

USTRUCT(BlueprintType)
struct FOpenCVCameraProperties
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CameraCalibration)
	FIntPoint Resolution;

	/** Camera intrinsic matrix in form:
		f_x,	0,		center_x;
		0,		f_y,	center_y;
		0,		0		1;

		During the calibration we will assume
		that f_x == f_y,
		center_x = res_x/2
		center_y = res_y/2
	*/
	cv::Mat_<double> CameraMatrix;

	cv::Mat_<double> DistortionCoefficients;

	// Field of view, X is horizontal, Y is vertical,
	// calculated from CameraMatrix and Resolution.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CameraCalibration)
	FVector2D FOV;

	FOpenCVCameraProperties();

	// Builds the camera matrix using resolution and focal length (in pixels)
	void SetFromResolutionAndFocal(FIntPoint const& resolution, double focal_pixels);

	// Builds the camera matrix using resolution and horizontal FOV angle (degrees)
	void SetFromResolutionAndFOV(FIntPoint const& resolution, double horizontal_fov_deg);

	/**
	 * Attempts to load calibration data from file in OpenCV format.
	 * Returns true if successful, otherwise file was probably not found.
	 */
	bool LoadFromFile(FString const& file_path);

	/**
	 * Saves calibration data to file, in OpenCV format.
	 * http://docs.opencv.org/3.1.0/dd/d74/tutorial_file_input_output_with_xml_yml.html
	 */
	bool SaveToFile(FString const& file_path) const;

	/** Change the resolution, trying to preserve the FOV */
	void SetResolution(FIntPoint const& new_resolution);

	/** Change the horizontal FOV (in degrees), preserving the resolution */
	void SetHorizontalFOV(double horizontal_fov_deg);

	/** Calucalte FOV from CameraMatrix */
	void DeriveFOV();

	void PrintToLog() const;

protected:
	static const char* KEY_RESOLUTION;
	static const char* KEY_CAMERA_MATRIX;
	static const char* KEY_DISTORTION;
};

/*
	OpenCV camera calibration using the asymmetric circles 4x11 pattern.
*/
class FOpenCVCameraCalibrationProcess
{
public:
	FOpenCVCameraCalibrationProcess();

	// Prepare for a new calibration, clear any the process if it is in progress.
	void Reset();

	// Try using a new frame. Time is given so that there is appropriate interval
	// between consecutive captured frames.
	bool ProcessFrame(cv::Mat& frame, float time_now);

	bool IsFinished() const
	{
		return FramesCollected == FramesNeeded;
	}

	float GetProgress() const
	{
		return float(FramesCollected) / float(FramesNeeded);
	}

	FOpenCVCameraProperties const& GetCameraProperties() const
	{
		return CameraProperties;
	}

protected:
	// Number of frames to be captured before calibration is calculated.
	int32 FramesNeeded;

	// Time between capturing consecutive frames
	float MinInterval;

	// Number of rows / columns in the pattern.
	cv::Size PatternSize;

	// Distance between rows/columns
	float SquareSize;

	int32 CalibrationFlags;

	std::vector<cv::Mat> DetectedPointSets;
	int32 FramesCollected;
	float LastFrameTime;

	FOpenCVCameraProperties CameraProperties;

	void CalculateCalibration();
};
