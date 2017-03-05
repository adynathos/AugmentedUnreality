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
#include <opencv2/imgproc.hpp>
#include <sstream>

namespace cv {
namespace aur {

FiducialTracker::FiducialTracker()
	: arucoParameters(cv::aruco::DetectorParameters::create())
{
}

FiducialTracker::~FiducialTracker()
{
}

void FiducialTracker::setDiagnosticLevel(DiagnosticLevel new_diag_level)
{
	diagnosticLvl = new_diag_level;
}

void FiducialTracker::setCameraInfo(cv::Mat_<double> const& intrinsic_mat, cv::Mat_<double> const& distortion)
{
	intrinsic_mat.convertTo(cameraIntrinsicMat, CV_64F);
	distortion.convertTo(cameraDistortion, CV_64F);
}

void FiducialTracker::setArucoParameters(cv::aruco::DetectorParameters const& new_params)
{
	*arucoParameters = new_params;
}

TrackedPose* FiducialTracker::registerPoseToTrack(cv::Ptr<FiducialPattern> pattern)
{
	cv::Ptr<TrackedPose> new_pose(new TrackedPose(this, pattern));

	// no registered boards yet, so take dictionary from this one
	if(posesById.size() == 0)
	{
		markerDictionary = pattern->getArucoDictionary();
	}
	else
	// all boards must use the same dictionary
	{
		const int32_t current_dict_id = posesById.cbegin()->second->pattern->getArucoDictionaryId();
		if(pattern->getArucoDictionaryId() != current_dict_id)
		{
			std::stringstream msg;
			msg << "Error when adding board: New board uses dictionary " << pattern->getArucoDictionaryId()
				<< " but previous boards use dictionary " << current_dict_id;
			log(LogLevel::Error, msg.str());
			return nullptr;
		}

		// one marker id must not belong to many boards
		for(const int32_t marker_id : pattern->getMarkerIds())
		{
			if(posesByMarker.find(marker_id) != posesByMarker.end())
			{
				std::stringstream msg;
				msg << "Error when adding board: New board contains marker " << marker_id
					<< "which is already in use by another board";
				log(LogLevel::Error, msg.str());
				return nullptr;
			}
		}
	}

	// insert the new board
	posesById.emplace(new_pose->getPoseId(), new_pose);

	for(const int32_t marker_id : pattern->getMarkerIds())
	{
		posesByMarker.emplace(marker_id, new_pose.get());
	}

	return new_pose.get();
}

void FiducialTracker::unregisterPose(TrackedPose* pose)
{
	if(pose)
	{
		detectedPoses.erase(pose);

		for(int32_t marker_id : pose->pattern->getMarkerIds())
		{
			posesByMarker.erase(marker_id);
		}

		posesById.erase(pose->getPoseId());
	}
}

void FiducialTracker::processFrame(cv::Mat_<cv::Vec3b>& input_image)
{
	// http://docs.opencv.org/3.2.0/db/da9/tutorial_aruco_board_detection.html

	detectedPoses.clear();

	cv::cvtColor(input_image, imageGrey, cv::COLOR_BGR2GRAY);

	// No boards to detect
	if(posesById.size() <= 0)
	{
		return;
	}

	// Unreal, and specifically its Android build, does not want to compile try-catch
	// So we will log the exceptions here through the log callback
	try
	{
		std::vector< std::vector< cv::Point2f > > out_corners;
		std::vector< int32_t > out_ids;

		// Find squares and corners in the image
		cv::aruco::detectMarkers(imageGrey, markerDictionary, out_corners, out_ids, arucoParameters);

		if(diagnosticLvl >= DiagnosticLevel::Full)
		{
			cv::aruco::drawDetectedMarkers(input_image, out_corners, out_ids);
		}

		// Find which boards were detected
		for(size_t mk_id = 0; mk_id < out_ids.size(); mk_id++)
		{
			auto pose_iter = posesByMarker.find(out_ids[mk_id]);
			if(pose_iter != posesByMarker.end())
			{
				detectedPoses.insert(pose_iter->second);
				pose_iter->second->addFoundMarker(out_ids[mk_id], out_corners[mk_id]);
			}
		}

		// Perform PNP for each board and save the transforms
		for (auto it = detectedPoses.begin(); it != detectedPoses.end();)
		{
			if ((*it)->determinePose())
			{
				// success, so move to next one
				it++;
			}
			else
			{
				// failed to detect, erase from list of detected poses
				it = detectedPoses.erase(it); //returns next iterator
			}
		}
	}
	catch (std::exception& exc)
	{
		std::stringstream msg;
		msg << "Exception in FiducialTracker::processFrame: \n" << exc.what();
		log(LogLevel::Error, msg.str());
	}
}

std::unordered_set<TrackedPose*> const& FiducialTracker::getDetectedPoses() const
{
	return detectedPoses;
}

} // namespace
}
