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

#ifndef __OPENCV_AUR_ALLOCATOR_HPP__
#define __OPENCV_AUR_ALLOCATOR_HPP__

/**
Some OpenCV functions take a std::vector reference and write its output to it.
If the other binary that included OpenCv then tries to delete those std::vectors, a crash may happen.
This module provides wrappers for data structures to which OpenCV writes so that they
can be allocated/deallocated in the OpenCV binaries.
**/

#include <opencv2/core.hpp>
#include <opencv2/aruco.hpp>
#include <vector>

namespace cv
{
namespace aur_allocator
{

template<typename WrappedType>
class CV_EXPORTS OpenCvWrapper {
public:
	OpenCvWrapper();
	OpenCvWrapper(WrappedType const& other);
	WrappedType* operator->();
	WrappedType const* operator->() const;
	WrappedType& operator*();
	WrappedType const& operator*() const;

	OpenCvWrapper(OpenCvWrapper const& other) = delete;
	void operator=(OpenCvWrapper const& other) = delete;

	~OpenCvWrapper();

private:
	WrappedType* Instance;
};

typedef std::vector<cv::Point2f> VectorOfPoint2f;
typedef std::vector<cv::Point3f> VectorOfPoint3f;
}
}

#endif
