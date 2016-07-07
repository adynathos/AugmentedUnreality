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
#include "opencv2/aur_allocator.hpp"

namespace cv 
{
namespace aur_allocator 
{

template<typename WrappedType>
OpenCvWrapper<WrappedType>::OpenCvWrapper()
	: Instance(new WrappedType)
{
}

template<typename WrappedType>
OpenCvWrapper<WrappedType>::OpenCvWrapper(WrappedType const& other)
	: Instance(new WrappedType(other))
{
}

template<typename WrappedType>
WrappedType* OpenCvWrapper<WrappedType>::operator->()
{
	return Instance;
}

template<typename WrappedType>
WrappedType const* OpenCvWrapper<WrappedType>::operator->() const
{
	return Instance;
}

template<typename WrappedType>
WrappedType& OpenCvWrapper<WrappedType>::operator*()
{
	return *Instance;
}

template<typename WrappedType>
WrappedType const& OpenCvWrapper<WrappedType>::operator*() const
{
	return *Instance;
}

template<typename WrappedType>
OpenCvWrapper<WrappedType>::~OpenCvWrapper()
{
	if (Instance)
	{
		delete Instance;
	}
}

// Instantiate for the types we will be using in AUR
// These instantiations will be in the shared library
template class CV_EXPORTS OpenCvWrapper< std::vector<int> >;

template class CV_EXPORTS OpenCvWrapper< cv::Mat >;
template class CV_EXPORTS OpenCvWrapper< std::vector<cv::Mat> >;

template class CV_EXPORTS OpenCvWrapper< VectorOfPoint2f >;
template class CV_EXPORTS OpenCvWrapper< std::vector<VectorOfPoint2f> >;

template class CV_EXPORTS OpenCvWrapper< VectorOfPoint3f >;
template class CV_EXPORTS OpenCvWrapper< std::vector<VectorOfPoint3f> >;

template class CV_EXPORTS OpenCvWrapper< cv::aruco::Board >;
template class CV_EXPORTS OpenCvWrapper< cv::aruco::Dictionary >;
}
}
