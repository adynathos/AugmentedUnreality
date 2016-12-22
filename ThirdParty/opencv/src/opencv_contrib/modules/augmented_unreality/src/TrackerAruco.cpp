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

#include "opencv2/augmented_unreality/TrackerAruco.hpp"
#include <opencv2/calib3d.hpp>
#include <sstream>

namespace cv {
namespace aur {

const cv::Mat_<double> TrackerAruco::REBASE_CV_TO_UNREAL = (cv::Mat_<double>(3, 3) <<
	0, 1, 0,
	1, 0, 0,
	0, 0, 1
);

const cv::Mat_<double> TrackerAruco::REBASE_UNREAL_TO_CV = TrackerAruco::REBASE_CV_TO_UNREAL.t();

TrackerAruco::TrackedPose::TrackedPose(TrackerAruco* tracker_ptr, ArucoBoardDefinition const& board_def)
	: tracker(tracker_ptr)
	, board(board_def)
	, poseId(board_def.GetMinMarkerId())
	, Translation(3, 1)
{
}

TrackerAruco::TrackedPose::~TrackedPose()
{
}

void TrackerAruco::TrackedPose::clearFound()
{
	foundMarkerIds.clear();
	foundMarkerCorners.clear();
}

void TrackerAruco::TrackedPose::addFoundMarker(int32_t id, std::vector< cv::Point2f>& detected_corners)
{
	foundMarkerIds.push_back(id);
	foundMarkerCorners.push_back(detected_corners);
}

void TrackerAruco::TrackedPose::unregister()
{
	if(tracker) tracker->unregisterPose(this);
}

TrackerAruco::TrackerAruco()
{
}

TrackerAruco::~TrackerAruco()
{
}

void TrackerAruco::setDiagnosticLevel(DiagnosticLevel new_diag_level)
{
	diagnosticLvl = new_diag_level;
}

void TrackerAruco::setCameraInfo(const cv::Mat_<double>& intrinsic_mat, const cv::Mat_<double>& distortion)
{
	intrinsic_mat.convertTo(cameraIntrinsicMat, CV_64F);
	distortion.convertTo(cameraDistortion, CV_64F);

/*	
	std::stringstream ss;
	ss << "Intrinsic\n" << cameraIntrinsicMat;
	
	log(LogLevel::Log, ss.str());
*/
}

void TrackerAruco::setArucoParameters(const cv::aruco::DetectorParameters& new_params)
{
	arucoParameters = new_params;
}

TrackerAruco::TrackedPose* TrackerAruco::registerPoseToTrack(const ArucoBoardDefinition& board)
{
	std::shared_ptr<TrackedPose> new_pose(new TrackedPose(this, board));

	// no registered boards yet, so take dictionary from this one
	if(posesById.size() == 0)
	{
		markerDictionary = board.GetArucoDictionary();
	}
	else
	// all boards must use the same dictionary
	{
		const int32_t current_dict_id = posesById.cbegin()->second->board.GetArucoDictionaryId();
		if(board.GetArucoDictionaryId() != current_dict_id)
		{
			std::stringstream msg;
			msg << "Error when adding board: New board uses dictionary " << board.GetArucoDictionaryId()
				<< "but previous boards use dictionary " << current_dict_id;
			log(LogLevel::Error, msg.str());
			return nullptr;
		}

		// one marker id must not belong to many boards
		for(const int32_t marker_id : board.GetArucoBoard().ids)
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

	for(const int32_t marker_id : board.GetArucoBoard().ids)
	{
		posesByMarker.emplace(marker_id, new_pose.get());
	}

	return new_pose.get();
}

void TrackerAruco::unregisterPose(TrackerAruco::TrackedPose* pose)
{
	if(pose)
	{
		detectedPoses.erase(pose);

		for(int32_t marker_id : pose->board.GetArucoBoard().ids)
		{
			posesByMarker.erase(marker_id);
		}

		posesById.erase(pose->getPoseId());
	}
}

void TrackerAruco::processFrame(cv::Mat_<cv::Vec3b>& input_image)
{
	// http://docs.opencv.org/3.1.0/db/da9/tutorial_aruco_board_detection.html
	
	detectedPoses.clear();
	
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
		cv::aruco::detectMarkers(input_image, markerDictionary, out_corners, out_ids, arucoParameters);

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
		for(TrackedPose* pose : detectedPoses)
		{
			determineBoardPose(pose);
		}
	}
	catch (std::exception& exc)
	{
		std::stringstream msg;
		msg << "Exception in TrackerAruco::processFrame:              \n" << exc.what();
		log(LogLevel::Error, msg.str());		
	}
}

std::unordered_set<TrackerAruco::TrackedPose*> const& TrackerAruco::getDetectedPoses() const
{
	return detectedPoses;
}

bool TrackerAruco::determineBoardPose(TrackedPose* pose)
{
	// Translation and rotation: transform from camera to world
	cv::Vec3d rotation_axis_angle, translation;

	int number_of_markers = cv::aruco::estimatePoseBoard(
		pose->foundMarkerCorners, pose->foundMarkerIds, pose->board.GetArucoBoard(),
		cameraIntrinsicMat, cameraDistortion,
		rotation_axis_angle, translation
	);

	pose->clearFound();

	// if markers fit
	if (number_of_markers > 0)
	{
		for(int idx : {0, 1, 2}) pose->Translation(idx) = translation(idx);
		
		cv::Rodrigues(rotation_axis_angle, pose->RotationMat);
		cv::Mat_<double> inv_rot(pose->RotationMat.t());

		pose->TranslationWorldToCam_U = REBASE_CV_TO_UNREAL * -1.0 * inv_rot * pose->Translation;
		pose->RotationMatWorldToCam_U = REBASE_CV_TO_UNREAL * inv_rot * REBASE_UNREAL_TO_CV;
		
		return true;
	}
	else
	{
		return false;
	}
}

} // namespace
}
