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

#include "ArucoBoardDefinition.hpp"
#include "log.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace cv {
namespace aur {

class CV_EXPORTS TrackerAruco
{
public:

	class CV_EXPORTS TrackedPose
	{
		friend class TrackerAruco;

	public:
		void* userObject;
		
		~TrackedPose();

		int32_t getPoseId() const
		{
			return poseId;
		}
		
		cv::Mat_<double> const& getTranslation() const
		{
			return Translation;
		}

		cv::Mat_<double> const& getRotationMat() const
		{
			return RotationMat;
		}

		cv::Mat_<double> const& getTranslationCameraUnreal() const
		{
			return TranslationWorldToCam_U;
		}

		cv::Mat_<double> const& getRotationCameraUnreal() const
		{
			return RotationMatWorldToCam_U;
		}

		void unregister();
		
	protected:

		TrackerAruco* tracker;
		ArucoBoardDefinition board;
		int32_t poseId;

		// Transform cam to world:
		cv::Mat_<double> Translation;
		cv::Mat_<double> RotationMat;

		// Transform world to cam, in unreal basis:
		cv::Mat_<double> TranslationWorldToCam_U;
		cv::Mat_<double> RotationMatWorldToCam_U;

		std::vector<int> foundMarkerIds;
		std::vector< std::vector< cv::Point2f >  > foundMarkerCorners;

		TrackedPose(TrackerAruco* tracker_ptr, ArucoBoardDefinition const& board_def);
		void clearFound();
		void addFoundMarker(int32_t id, std::vector< cv::Point2f >& detected_corners);
	};

	TrackerAruco();
	~TrackerAruco();

	void setDiagnosticLevel(DiagnosticLevel new_diag_level);
	void setCameraInfo(cv::Mat_<double> const& intrinsic_mat, cv::Mat_<double> const& distortion);
	void setArucoParameters(cv::aruco::DetectorParameters const& new_params);

	TrackedPose* registerPoseToTrack(ArucoBoardDefinition const& board);
	void processFrame(cv::Mat_<cv::Vec3b>& input_image);

	std::unordered_set< TrackedPose* > const& getDetectedPoses() const;

	// Basis matrix that swaps X and Y
	static const cv::Mat_<double> REBASE_CV_TO_UNREAL;
	// REBASE_CV_TO_UNREAL inverted
	static const cv::Mat_<double> REBASE_UNREAL_TO_CV;

protected:
	std::unordered_map< int32_t, std::shared_ptr<TrackedPose> > posesById;
	std::unordered_map< int32_t, TrackedPose* > posesByMarker;

	std::unordered_set< TrackedPose* > detectedPoses;

	cv::aruco::Dictionary markerDictionary;
	cv::aruco::DetectorParameters arucoParameters;

	cv::Mat_<double> cameraIntrinsicMat;
	cv::Mat_<double> cameraDistortion;

	DiagnosticLevel diagnosticLvl;

	bool determineBoardPose(TrackedPose* pose);
	void unregisterPose(TrackedPose* pose);
};

} // namespace
}

