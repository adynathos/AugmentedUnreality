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
#include <opencv2/core/ocl.hpp>
#include <opencv2/imgproc.hpp>

#include <opencv2/aur_allocator.hpp>

const float INCH = 2.54f;

ArucoWrapper::ArucoWrapper()
	: bDisplayMarkers(false)
{
	cv::ocl::setUseOpenCL(false);
	Data = new WrapperData;
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
	//BoardImage = cv::Mat(cv::Size(100, 100), CV_8UC1, cv::Scalar(125));
}

void ArucoWrapper::SetMarkerDefinition(int32_t DictionaryId, int32_t GridWidth, int32_t GridHeight, int32_t MarkerSize, int32_t SeparationSize)
{
	//Data->MarkerDictionary = cv::aruco::getPredefinedDictionary(
	//	cv::aruco::PREDEFINED_DICTIONARY_NAME(DictionaryId));

	//Data->BoardExperiment.SetDictionaryId(DictionaryId);
	//Data->BoardExperiment.AddMarker(FreeFormBoard::MarkerDefinition{ 10, cv::Point3f(10, 0, 0), cv::Point3f(10, 10, 0), cv::Point3f(0, 10, 0), cv::Point3f(0, 0, 0) });

	//Data->Board = cv::aruco::GridBoard::create(
	//	GridWidth, GridHeight, float(MarkerSize), float(SeparationSize),
	//	Data->MarkerDictionary);

	cv::Size img_size;
	int32_t marker_with_separation = MarkerSize + SeparationSize;
	// add one more separation as missing margin
	img_size.width = GridWidth * marker_with_separation + SeparationSize;
	img_size.height = GridHeight * marker_with_separation + SeparationSize;

	//Data->BoardExperiment.DrawMarkers();
	//Data->BoardImage = Data->BoardExperiment.DrawMarkers(); //Data->BoardExperiment.Pages[0];

	//Data->BoardImage = cv::Mat(cv::Size(100, 100), CV_8UC1, cv::Scalar(125));

	// create marker image
	//Data->Board.draw(img_size, Data->BoardImage, SeparationSize, 1);
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
		auto MarkerCorners = cv::aur_allocator::allocate_vector2_Point2f();//new std::vector<std::vector<cv::Point2f> >;
		auto MarkerIds = cv::aur_allocator::allocate_vector_int();

		cv::aruco::detectMarkers(Image, cv::aruco::getPredefinedDictionary(cv::aruco::PREDEFINED_DICTIONARY_NAME(Data->BoardExperiment.DictionaryId)), 
			*MarkerCorners, *MarkerIds);

		// we cannot see any markers at all
		if (MarkerIds->size() <= 0)
		{
			return false;
		}

		if (bDisplayMarkers)
		{
			cv::aruco::drawDetectedMarkers(Image, *MarkerCorners, *MarkerIds);
		}

		cv::Vec3d trans, rot;

		// determine translation and rotation + write to output
		int number_of_markers = cv::aruco::estimatePoseBoard(*MarkerCorners, *MarkerIds,
			Data->BoardExperiment.GetArucoBoard(), Data->CameraMatrix, Data->DistortionCoefficients,
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

		delete MarkerCorners;
		delete MarkerIds;
	}
	catch (std::exception& exc)
	{
		std::cout << "Exception in ArucoWrapper::DetectMarkers:\n" << exc.what() << '\n';
		return false;
	}

	return true;
}

FreeFormBoard& FreeFormBoard::AddMarker(MarkerDefinition const & marker_def)
{
	ArucoBoard->ids.push_back(marker_def.MarkerId);
	ArucoBoard->objPoints.push_back(marker_def.Corners);

	return *this;
}

FreeFormBoard::FreeFormBoard()
{
	ArucoBoard = cv::aur_allocator::allocate_aruco_board();

}

FreeFormBoard::~FreeFormBoard()
{
	cv::aur_allocator::external_delete(ArucoBoard);
	//cv::aur_allocator::external_delete(Dictionary);
}

FreeFormBoard & FreeFormBoard::Clear()
{
//	ArucoBoard->ids.clear();
//	ArucoBoard->objPoints.clear();

	return *this;
}

FreeFormBoard& FreeFormBoard::SetDictionaryId(uint32_t dict_id)
{
	DictionaryId = dict_id;
	//Dictionary.reset(new cv::aruco::Dictionary(cv::aruco::getPredefinedDictionary(cv::aruco::PREDEFINED_DICTIONARY_NAME(DictionaryId))));

	//cv::aruco::Dictionary dict = cv::aruco::getPredefinedDictionary(cv::aruco::PREDEFINED_DICTIONARY_NAME(DictionaryId));
	//ArucoBoard->dictionary = dict;

	return *this;
}

void FreeFormBoard::DrawMarkers(float marker_size_cm, uint32_t dpi, float margin_cm, cv::Size2f page_size_cm)
{
	float pixels_per_cm = float(dpi) / INCH;
	cv::Size page_size_pix(
		(int)std::round(pixels_per_cm*(page_size_cm.width - margin_cm)),
		(int)std::round(pixels_per_cm*(page_size_cm.height - margin_cm))
	);

	uint32_t marker_pixels = (uint32_t)std::round(pixels_per_cm*marker_size_cm);
	uint32_t margin_pixels = (uint32_t)std::round(pixels_per_cm*margin_cm);
	
	//cv::Mat img_out(page_size_pix, CV_8UC1);
	//img_out.setTo(255);

	//int marker_side = std::min(page_size_pix.width, page_size_pix.height) - 2*margin_pixels;

	//cv::Mat img_marker;
	//dictionary.drawMarker(ids[0], marker_side, img_marker, 1);
	//cv::putText(img_marker, "1", cv::Point(10, 10), cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 10.0, cv::Scalar::all(125));

	//img_marker.copyTo(img_out.rowRange(margin_pixels, margin_pixels+marker_side).colRange(margin_pixels, margin_pixels+marker_side));
	
	//cv::putText(img_out, "1", cv::Point(margin_pixels, margin_pixels + marker_side + 10), 
	//	cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 4.0, cv::Scalar(125), 3);

	//Pages.resize(1);
	//Pages[0] = img_marker;
	//putText(_image, s.str(), cent, FONT_HERSHEY_SIMPLEX, 0.5, textColor, 2);

	Pages.clear();
	for (int id = 0; id < 10; id++)
	{
		Pages.push_back(RenderMarker(id, marker_pixels, margin_pixels));
	}
}

cv::Mat FreeFormBoard::RenderMarker(int id, uint32_t marker_side, uint32_t margin)
{
	uint32_t canvas_side = marker_side + 2 * margin;
	cv::Mat canvas(cv::Size(canvas_side, canvas_side), CV_8UC1, cv::Scalar(255));

	cv::Mat img_marker;
	ArucoBoard->dictionary.drawMarker(id, marker_side, img_marker, 1);
	img_marker.copyTo(canvas.rowRange(margin, marker_side + margin).colRange(margin, marker_side + margin));

	std::string name = std::to_string(DictionaryId) + '.' + std::to_string(id);

	cv::rectangle(canvas, cv::Point(0, 0), cv::Point(canvas_side-1, canvas_side-1), cv::Scalar(125), 4);

	// putText takes bottom-left corner of text
	cv::putText(canvas, name, cv::Point(6, canvas_side - 8),
		cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 4.0, cv::Scalar(125), 3);

	return canvas;
}


