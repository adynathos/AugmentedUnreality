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
#pragma once

#include <opencv2/aruco/charuco.hpp>

namespace cv {
namespace aur {

class TrackedPose;

/*
	Wrapper for board definitions,
	contains also the method of determining pose from detected markers
*/
class CV_EXPORTS FiducialPattern
{
public:
	virtual ~FiducialPattern();

	virtual cv::Ptr<cv::aruco::Board> getBoard() const
	{
		return board;
	}

	cv::Ptr<cv::aruco::Dictionary> getArucoDictionary() const
	{
		return board->dictionary;
	}

	std::vector<int> const& getMarkerIds() const
	{
		return board->ids;
	}

	int getMinMarkerId() const
	{
		return *std::min_element(std::begin(getMarkerIds()), std::end(getMarkerIds()));
	}

	int getArucoDictionaryId() const
	{
		return arucoPredefinedDictionaryId;
	}

	void setArucoDictionaryId(const int32_t predefined_dictionary_id);

	// Determines the camera pose from information already collected in TrackedPose
	// and writes the result to TrackedPose
	// @return True if this pattern should be considered detected
	virtual bool determinePose(TrackedPose* pose_info) = 0;

protected:
	cv::Ptr<cv::aruco::Board> board;
	int32_t arucoPredefinedDictionaryId;

	// Need to remember the dictionary id because we can't get it from Dictionary object later
	FiducialPattern(int32_t dictionary_id);
};

/*
	Free form spatial board
*/
class CV_EXPORTS FiducialPatternArUco : public FiducialPattern
{
public:
	/*
		Used to construct the board by accumulating markers.
		Methods can be chained.
	*/
	class CV_EXPORTS Builder
	{
	public:
		// Add a marker
		Builder& marker(const int32_t marker_id, cv::Mat_<cv::Vec3f> const& corners);

		// Set dictionary id
		Builder& dictionary(int32_t predefined_dictionary_id);

		// Finalize the build and create a board object
		cv::Ptr<FiducialPatternArUco> build();

	protected:
		int32_t predefinedDictionaryId;
		std::vector< int32_t > markerIds;
		std::vector< cv::Mat_<cv::Vec3f> > markerCorners;

		Builder();

		friend class FiducialPatternArUco;
	};

	// Expose in shared pointer to prevent using UE's crashing memory deallocator
	static cv::Ptr<Builder> builder();

	virtual bool determinePose(TrackedPose* pose_info) override;

protected:
	// use Builder instead
	FiducialPatternArUco(cv::Ptr<cv::aruco::Board> from_board, int32_t dictionary_id);
};

/*
	Chessboard variant
*/
class CV_EXPORTS FiducialPatternChArUcoBoard : public FiducialPattern
{
public:
	virtual bool determinePose(TrackedPose* pose_info) override;

	cv::Mat_<uint8_t> drawPattern();

	static cv::Ptr<FiducialPatternChArUcoBoard> build(int32_t width, int32_t height, float square_side, float marker_margin = 1.0, int32_t initial_marker_id = 0, int32_t dictionary_id=cv::aruco::DICT_4X4_100);

protected:
	FiducialPatternChArUcoBoard(cv::Ptr<cv::aruco::CharucoBoard> from_board, int32_t dictionary_id);

	cv::Ptr<cv::aruco::CharucoBoard> boardChArUco;
};

} //namespace
}
