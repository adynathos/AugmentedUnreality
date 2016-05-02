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

#include "OpenCVWrapper.h"
#include <iostream>

ArucoWrapper::ArucoWrapper()
	: Data(new WrapperData)
	, bDisplayMarkers(false)
{
}

ArucoWrapper::~ArucoWrapper()
{
	if (Data)
	{
		delete Data;
		Data = nullptr;
	}
}

ArucoWrapper::WrapperData::WrapperData()
	: CameraMatrix(cv::Mat::eye(3, 3, CV_64FC1))
	, DistortionCoefficients(cv::Mat::zeros(5, 1, CV_64FC1))
{
}

void ArucoWrapper::SetMarkerDefinition(int32_t DictionaryId, int32_t GridWidth, int32_t GridHeight, int32_t MarkerSize, int32_t SeparationSize)
{
	Data->MarkerDictionary = cv::aruco::getPredefinedDictionary(
		cv::aruco::PREDEFINED_DICTIONARY_NAME(DictionaryId));

	Data->Board = cv::aruco::GridBoard::create(
		GridWidth, GridHeight, float(MarkerSize), float(SeparationSize),
		Data->MarkerDictionary);

	cv::Size img_size;
	int32_t marker_with_separation = MarkerSize + SeparationSize;
	// add one more separation as missing margin
	img_size.width = GridWidth * marker_with_separation + SeparationSize;
	img_size.height = GridHeight * marker_with_separation + SeparationSize;

	// create marker image
	Data->Board.draw(img_size, Data->BoardImage, SeparationSize, 1);
}

void ArucoWrapper::SetCameraProperties(cv::Mat const & CameraMatrix, cv::Mat const & DistortionCoefficients)
{
	Data->CameraMatrix = CameraMatrix;
	Data->DistortionCoefficients = DistortionCoefficients;
}

bool ArucoWrapper::DetectMarkers(cv::Mat & Image, cv::Vec3d & OutTranslation, cv::Vec3d & OutRotation) const
{
	// http://docs.opencv.org/3.1.0/db/da9/tutorial_aruco_board_detection.html

	try
	{
		cv::aruco::detectMarkers(Image, Data->MarkerDictionary, Data->MarkerCorners, Data->MarkerIds);

		// we cannot see any markers at all
		if (Data->MarkerIds.size() <= 0)
		{
			return false;
		}

		if (bDisplayMarkers)
		{
			cv::aruco::drawDetectedMarkers(Image, Data->MarkerCorners, Data->MarkerIds);
		}

		cv::Vec3d trans, rot;

		// determine translation and rotation + write to output
		int number_of_markers = cv::aruco::estimatePoseBoard(Data->MarkerCorners, Data->MarkerIds,
			Data->Board, Data->CameraMatrix, Data->DistortionCoefficients,
			rot, trans);

		// the markers do not fit
		if (number_of_markers <= 0)
		{
			return false;
		}

		if (bDisplayMarkers)
		{
			cv::aruco::drawAxis(Image, Data->CameraMatrix, Data->DistortionCoefficients,
				rot, trans, 100);
		}

		OutRotation = rot;
		OutTranslation = trans;
	}
	catch (std::exception& exc)
	{
		std::cout << "Exception in ArucoWrapper::DetectMarkers:\n" << exc.what() << '\n';
		return false;
	}

	return true;
}

