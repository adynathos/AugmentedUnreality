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
#include <opencv2/calib3d.hpp>

namespace cv {
namespace aur {

const cv::Mat_<double> TrackedPose::REBASE_CV_TO_UNREAL = (cv::Mat_<double>(3, 3) <<
	0, 1, 0,
	1, 0, 0,
	0, 0, 1
);

const cv::Mat_<double> TrackedPose::REBASE_UNREAL_TO_CV = TrackedPose::REBASE_CV_TO_UNREAL.t();

TrackedPose::TrackedPose(FiducialTracker* tracker_ptr, cv::Ptr<FiducialPattern> pattern_def)
	: tracker(tracker_ptr)
	, pattern(pattern_def)
	, poseId(pattern_def->getMinMarkerId())
	, Translation(3, 1)
{
}

TrackedPose::~TrackedPose()
{
}

void TrackedPose::clearFound()
{
	foundMarkerIds.clear();
	foundMarkerCorners.clear();
}

void TrackedPose::addFoundMarker(int32_t id, std::vector< cv::Point2f>& detected_corners)
{
	foundMarkerIds.push_back(id);
	foundMarkerCorners.push_back(detected_corners);
}

bool TrackedPose::determinePose()
{
	bool success = pattern->determinePose(this);
	clearFound();
	return success;
}

void TrackedPose::setTransform(cv::Mat_<double> const & rotation_axis_angle, cv::Mat_<double> const & translation)
{
	//for(int idx : {0, 1, 2}) Translation(idx) = translation(idx);
	Translation = translation;

	cv::Rodrigues(rotation_axis_angle, RotationMat);
	cv::Mat_<double> inv_rot(RotationMat.t());

	TranslationWorldToCam_U = REBASE_CV_TO_UNREAL * -1.0 * inv_rot * Translation;
	RotationMatWorldToCam_U = REBASE_CV_TO_UNREAL * inv_rot * REBASE_UNREAL_TO_CV;

	/*
	std::stringstream msg;
	msg << "Pose: " << " raa = " << rotation_axis_angle
		<< ", t = " << translation << '\n'
		<< "Ru = " << TranslationWorldToCam_U
		<< ", Tu = " << TranslationWorldToCam_U;
	log(LogLevel::Log, msg.str());*/
}

void TrackedPose::unregister()
{
	if(tracker) tracker->unregisterPose(this);
}

} // namespace
}
