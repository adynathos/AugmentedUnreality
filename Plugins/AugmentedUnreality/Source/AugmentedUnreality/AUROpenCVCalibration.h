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

#pragma once

#include "OpenCV_includes.h"

#include "AUROpenCVCalibration.generated.h"

USTRUCT(BlueprintType)
struct FOpenCVCameraProperties
{
	GENERATED_BODY()

	cv::Mat CameraMatrix;
	cv::Mat DistortionCoefficients;

	// Parameters of the marker image to use.
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraCalibration)
	//FIntPoint Resolution;

	//float CameraPixelRatio;

	// Field of view, X is horizontal, Y is vertical
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraCalibration)
	FVector2D FOV;

	FOpenCVCameraProperties()
		: CameraMatrix(cv::Mat::eye(3, 3, CV_64FC1))
		, DistortionCoefficients(cv::Mat::zeros(5, 1, CV_64FC1))
		, FOV(50., 50.)
	{
	}

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

	void PrintToLog() const;

protected:
	static const char* KEY_CAMERA_MATRIX;
	static const char* KEY_DISTORTION;
	static const char* KEY_FOV;
};

class FOpenCVCameraCalibration
{
};
