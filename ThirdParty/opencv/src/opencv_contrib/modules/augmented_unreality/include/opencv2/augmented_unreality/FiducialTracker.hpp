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

#include "log.hpp"
#include <unordered_map>
#include <unordered_set>

namespace cv {
namespace aur {

class FiducialPattern;
class TrackedPose;

class CV_EXPORTS FiducialTracker
{
public:
	FiducialTracker();
	~FiducialTracker();

	void setDiagnosticLevel(DiagnosticLevel new_diag_level);
	void setCameraInfo(cv::Mat_<double> const& intrinsic_mat, cv::Mat_<double> const& distortion);
	void setArucoParameters(cv::aruco::DetectorParameters const& new_params);

	TrackedPose* registerPoseToTrack(cv::Ptr<FiducialPattern> pattern);
	void processFrame(cv::Mat_<cv::Vec3b>& input_image);

	std::unordered_set< TrackedPose* > const& getDetectedPoses() const;

protected:
	std::unordered_map< int32_t, cv::Ptr<TrackedPose> > posesById;
	std::unordered_map< int32_t, TrackedPose* > posesByMarker;

	cv::Ptr< cv::aruco::Dictionary > markerDictionary;
	cv::Ptr< cv::aruco::DetectorParameters > arucoParameters;

	cv::Mat_<double> cameraIntrinsicMat;
	cv::Mat_<double> cameraDistortion;

	DiagnosticLevel diagnosticLvl;

	cv::Mat_<uint8_t> imageGrey;
	std::unordered_set< TrackedPose* > detectedPoses;

	void unregisterPose(TrackedPose* pose);

	friend class TrackedPose;
	friend class FiducialPatternArUco;
	friend class FiducialPatternChArUcoBoard;
};

} // namespace
}
