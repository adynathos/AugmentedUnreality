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
	: CameraMatrix(new cv::Mat(cv::Mat::eye(3, 3, CV_64FC1)))
	, DistortionCoefficients(new cv::Mat(cv::Mat::zeros(5, 1, CV_64FC1)))
	, BoardImage(new cv::Mat)
	, bDisplayMarkers(false)
{
}

void ArucoWrapper::SetMarkerDefinition(int32_t DictionaryId, int32_t GridWidth, int32_t GridHeight, int32_t MarkerSize, int32_t SeparationSize)
{
	this->MarkerDictionary.reset(new cv::aruco::Dictionary(cv::aruco::getPredefinedDictionary(
		cv::aruco::PREDEFINED_DICTIONARY_NAME(DictionaryId)))
	);

	this->Board.reset(new cv::aruco::GridBoard(cv::aruco::GridBoard::create(
		GridWidth, GridHeight,
		float(MarkerSize), float(SeparationSize),
		*this->MarkerDictionary)));

	cv::Size img_size;
	int32_t marker_with_separation = MarkerSize + SeparationSize;
	// add one more separation as missing margin
	img_size.width = GridWidth * marker_with_separation + SeparationSize;
	img_size.height = GridHeight * marker_with_separation + SeparationSize;

	// create marker image
	this->Board->draw(img_size, *this->BoardImage, SeparationSize, 1);
}

void ArucoWrapper::SetCameraProperties(cv::Mat const & CameraMatrix, cv::Mat const & DistortionCoefficients)
{
	*this->CameraMatrix = CameraMatrix;
	*this->DistortionCoefficients = DistortionCoefficients;
}

bool ArucoWrapper::DetectMarkers(cv::Mat & Image, cv::Vec3d & OutTranslation, cv::Vec3d & OutRotation) const
{
	// http://docs.opencv.org/3.1.0/db/da9/tutorial_aruco_board_detection.html

	std::vector<std::vector<cv::Point2f> > MarkerCorners;
	std::vector<int> MarkerIds;

	try
	{
		cv::aruco::detectMarkers(Image, *MarkerDictionary, MarkerCorners, MarkerIds);

		// we cannot see any markers at all
		if (MarkerIds.size() <= 0)
		{
			return false;
		}

		if (bDisplayMarkers)
		{
			cv::aruco::drawDetectedMarkers(Image, MarkerCorners, MarkerIds);
		}

		cv::Vec3d trans, rot;

		// determine translation and rotation + write to output
		int number_of_markers = cv::aruco::estimatePoseBoard(MarkerCorners, MarkerIds,
			*Board, *CameraMatrix, *DistortionCoefficients,
			rot, trans);

		// the markers do not fit
		if (number_of_markers <= 0)
		{
			return false;
		}

		if (bDisplayMarkers)
		{
			cv::aruco::drawAxis(Image, *CameraMatrix, *DistortionCoefficients,
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
