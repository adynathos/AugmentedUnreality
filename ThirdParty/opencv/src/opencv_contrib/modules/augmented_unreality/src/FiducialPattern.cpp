/*
Copyright 2016 Krzysztof Lis

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "opencv2/augmented_unreality.hpp"
#include <sstream>

namespace cv {
namespace aur {

class TrackedPose;

FiducialPattern::~FiducialPattern()
{
}

void FiducialPattern::setArucoDictionaryId(const int32_t predefined_dictionary_id)
{
	arucoPredefinedDictionaryId = predefined_dictionary_id;
	board->dictionary = cv::aruco::getPredefinedDictionary(
		cv::aruco::PREDEFINED_DICTIONARY_NAME(predefined_dictionary_id)
	);
}

FiducialPattern::FiducialPattern(int32_t dictionary_id)
	: arucoPredefinedDictionaryId(dictionary_id)
{
}

FiducialPatternArUco::Builder::Builder()
	: predefinedDictionaryId(cv::aruco::DICT_4X4_100)
{
}

FiducialPatternArUco::Builder & FiducialPatternArUco::Builder::marker(const int32_t marker_id, cv::Mat_<cv::Vec3f> const & corners)
{
	markerIds.push_back(marker_id);
	markerCorners.push_back(corners);
	return *this;
}

FiducialPatternArUco::Builder& FiducialPatternArUco::Builder::dictionary(int32_t predefined_dictionary_id)
{
	predefinedDictionaryId = predefined_dictionary_id;
	return *this;
}

cv::Ptr<FiducialPatternArUco> FiducialPatternArUco::Builder::build()
{
	return cv::Ptr<FiducialPatternArUco>(new FiducialPatternArUco(
		cv::aruco::Board::create(markerCorners, cv::aruco::getPredefinedDictionary(predefinedDictionaryId), markerIds),
		predefinedDictionaryId
	));
}

cv::Ptr<FiducialPatternArUco::Builder> FiducialPatternArUco::builder()
{
	return cv::Ptr<Builder>(new FiducialPatternArUco::Builder);
}

bool FiducialPatternArUco::determinePose(TrackedPose* pose_info)
{
	FiducialTracker const* tr = pose_info->tracker;

	// Translation and rotation: transform from camera to world
	// Now estimatePoseBoard will write NaN is given cv::Vec3d, so change to Mat
	cv::Mat_<double> rotation_axis_angle, translation;

	int success = cv::aruco::estimatePoseBoard(
		pose_info->foundMarkerCorners, pose_info->foundMarkerIds, board,
		tr->cameraIntrinsicMat, tr->cameraDistortion,
		rotation_axis_angle, translation
	);
	pose_info->clearFound();

	if (success > 0)
	{
		pose_info->setTransform(rotation_axis_angle, translation);
		return true;
	}

	return false;
}

bool FiducialPatternChArUcoBoard::determinePose(TrackedPose* pose_info)
{
	// http://docs.opencv.org/3.2.0/df/d4a/tutorial_charuco_detection.html

	FiducialTracker const* tr = pose_info->tracker;

	// ChArUco will find the chessboard corners and use those for pose estimation
	std::vector<cv::Point2f> charuco_found_corners;
	std::vector<int> charuco_found_ids;

	int num_corners = cv::aruco::interpolateCornersCharuco(
		pose_info->foundMarkerCorners, pose_info->foundMarkerIds,
		tr->imageGrey, boardChArUco,
		charuco_found_corners, charuco_found_ids,
		tr->cameraIntrinsicMat, tr->cameraDistortion
	);
	pose_info->clearFound();

	if (num_corners > 0)
	{
		// Translation and rotation: transform from camera to world
		cv::Mat_<double> rotation_axis_angle, translation;

		bool success = cv::aruco::estimatePoseCharucoBoard(
			charuco_found_corners, charuco_found_ids,
			boardChArUco, tr->cameraIntrinsicMat, tr->cameraDistortion,
			rotation_axis_angle, translation
		);

		if (success)
		{
			pose_info->setTransform(rotation_axis_angle, translation);
			return true;
		}
	}

	return false;
}

FiducialPatternArUco::FiducialPatternArUco(cv::Ptr<cv::aruco::Board> from_board, int32_t dictionary_id)
	: FiducialPattern(dictionary_id)
{
	board = from_board;
}

cv::Ptr<FiducialPatternChArUcoBoard> FiducialPatternChArUcoBoard::build(int32_t width, int32_t height, float square_side, float marker_margin, int32_t initial_marker_id, int32_t dictionary_id)
{
	if (marker_margin * 2 >= square_side)
	{
		std::stringstream msg;
		msg << "FiducialPatternChArUcoBoard::build: Margin too high "
			<< "(margin = " << marker_margin << ", square side = " << square_side << ")";
		log(LogLevel::Error, msg.str());
		return cv::Ptr<FiducialPatternChArUcoBoard>();
	}

	auto board = cv::aruco::CharucoBoard::create(
		width, height,
		square_side, square_side - 2 * marker_margin,
		cv::aruco::getPredefinedDictionary(dictionary_id)
	);

	// the board is created starting from id=0, so shift by desired minimal id
	for(auto& marker_id : board->ids)
	{
		marker_id += initial_marker_id;
	}

	// shift the board in space so that origin is in the boards center
	cv::Point3f const center(width*square_side*0.5f, height*square_side*0.5f, 0.0f);

	for (auto& point_vector : board->objPoints)
	{
		for (auto& point : point_vector)
		{
			point -= center;
		}
	}
	for (auto& point : board->chessboardCorners)
	{
		point -= center;
	}

	return cv::Ptr<FiducialPatternChArUcoBoard>(new FiducialPatternChArUcoBoard(board, dictionary_id));
}

cv::Mat_<uint8_t> FiducialPatternChArUcoBoard::drawPattern()
{
	const int scale = 256;
	cv::Mat_<uint8_t> board_img;

	cv::Size const board_size_squares = boardChArUco->getChessboardSize();
	cv::Size const board_size_pix = board_size_squares * scale;

	board_img.create(board_size_pix);
	boardChArUco->draw(board_size_pix, board_img);

	return board_img;
}

FiducialPatternChArUcoBoard::FiducialPatternChArUcoBoard(cv::Ptr<cv::aruco::CharucoBoard> from_board, int32_t dictionary_id)
	: FiducialPattern(dictionary_id)
	, boardChArUco(from_board)
{
	board = from_board;
}

} //namespace
}
