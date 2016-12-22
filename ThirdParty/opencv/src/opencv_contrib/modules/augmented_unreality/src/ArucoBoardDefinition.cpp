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

#include "opencv2/augmented_unreality/ArucoBoardDefinition.hpp"

namespace cv {
namespace aur {

ArucoBoardDefinition::ArucoBoardDefinition()
{
	SetArucoDictionaryId(cv::aruco::DICT_4X4_100);
}

ArucoBoardDefinition::~ArucoBoardDefinition()
{
}

void ArucoBoardDefinition::Clear()
{
	ArucoBoard.ids.clear();
	ArucoBoard.objPoints.clear();
}

void ArucoBoardDefinition::SetArucoDictionaryId(const int32_t predefined_dictionary_id)
{
	ArucoPredefinedDictionaryId = predefined_dictionary_id;
	ArucoBoard.dictionary = cv::aruco::getPredefinedDictionary(
		cv::aruco::PREDEFINED_DICTIONARY_NAME(predefined_dictionary_id)
	);
}

void ArucoBoardDefinition::AddMarker(const int32_t marker_id, const std::vector<cv::Point3f>& corners)
{
	ArucoBoard.ids.push_back(marker_id);
	ArucoBoard.objPoints.push_back(corners);
}

} //namespace
}
