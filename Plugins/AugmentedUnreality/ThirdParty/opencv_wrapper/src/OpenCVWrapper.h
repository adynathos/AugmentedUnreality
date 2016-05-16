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

#ifdef _WIN32
	#include "OpenCVWrapper_Export.h"
#else
	#define OpenCVWrapper_EXPORT
#endif
#include <opencv2/aruco.hpp>
#include <memory>

/*
Using the OpenCV functions which write their output to vector<vector<something>>
causes crashes in UE4 because of some custom delete mechanism of the engine.

It has been reported however that performing those operations in a separate shared library
prevents the crashes, as the UE4's delete is not used.
https://answers.unrealengine.com/questions/36777/crash-with-opencvfindcontour.html

This shared library is a wrapper around Aruco marker detection mechanism.
*/
class OpenCVWrapper_EXPORT ArucoWrapper
{
public:
	ArucoWrapper();
	~ArucoWrapper();

	void SetMarkerDefinition(int32_t DictionaryId, int32_t GridWidth, int32_t GridHeight, int32_t MarkerSize, int32_t SeparationSize);
	cv::Mat const& GetMarkerImage() const
	{
		return Data->BoardImage;
	}
	void SetCameraProperties(cv::Mat const& CameraMatrix, cv::Mat const& DistortionCoefficients);
	void SetDisplayMarkers(bool b_display)
	{
		this->bDisplayMarkers = b_display;
	}
	bool DetectMarkers(cv::Mat & Image, cv::Vec3d & OutTranslation, cv::Vec3d & OutRotation) const;

private:
	// prevent copying, which would result in double-delete of this->Data
	ArucoWrapper(ArucoWrapper const& other) {}

	// We store only pointers here, because we want allocation and deallocation to happen inside this shared library.
	// If unique_ptr is used, there is a warning that it crosses the shared library boundary and
	// the program that links with it may have a different implementation of it, so we are forced to new/delete.
	struct WrapperData {
		cv::aruco::Dictionary MarkerDictionary;
		cv::aruco::GridBoard Board; // board also contains vector<vector<point>> and crashes UE4
		cv::Mat BoardImage;

		cv::Mat CameraMatrix;
		cv::Mat DistortionCoefficients;

		std::vector<std::vector<cv::Point2f> > MarkerCorners;
		std::vector<int> MarkerIds;

		WrapperData();
	};

	WrapperData* Data;

	// Display
	bool bDisplayMarkers;
};
