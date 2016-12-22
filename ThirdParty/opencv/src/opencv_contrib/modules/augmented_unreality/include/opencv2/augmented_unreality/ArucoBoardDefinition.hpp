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

#include <opencv2/aruco.hpp>

namespace cv {
namespace aur {

class CV_EXPORTS ArucoBoardDefinition
{
public:
	ArucoBoardDefinition();
	~ArucoBoardDefinition();


	cv::aruco::Board const& GetArucoBoard() const
	{
		return ArucoBoard;
	}

	cv::aruco::Dictionary const& GetArucoDictionary() const
	{
		return ArucoBoard.dictionary;
	}

	std::vector<int> const& GetMarkerIds() const
	{
		return ArucoBoard.ids;
	}

	int GetMinMarkerId() const
	{
		return *std::min_element(std::begin(GetMarkerIds()), std::end(GetMarkerIds()));
	}

	int GetArucoDictionaryId() const
	{
		return ArucoPredefinedDictionaryId;
	}

	void SetArucoDictionaryId(const int32_t predefined_dictionary_id);

	void AddMarker(const int32_t marker_id, std::vector<cv::Point3f> const& corners);
	void Clear();

protected:
	cv::aruco::Board ArucoBoard;
	int32_t ArucoPredefinedDictionaryId;
};

} //namespace
}
