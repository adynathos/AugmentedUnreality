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

#include "AugmentedUnreality.h"
#include "AURSmoothingFilterKalman.h"

UAURSmoothingFilterKalman::UAURSmoothingFilterKalman()
{
}

void UAURSmoothingFilterKalman::Init()
{
	Filter.init(3, 3);

	cv::Mat transition(3, 3, CV_32F);
	cv::setIdentity(transition);
	//transition.at<float>()


	Filter.transitionMatrix = transition;
	Filter.statePre.setTo(0);

	cv::setIdentity(Filter.measurementMatrix);
	//cv::setIdentity(Filter.processNoiseCov, cv::Scalar::all(1e-4));
	//cv::setIdentity(Filter.measurementNoiseCov, cv::Scalar::all(10));
	//cv::setIdentity(Filter.errorCovPost, cv::Scalar::all(.1));

	TransformAsMat = cv::Mat(3, 1, CV_32F);
}

void UAURSmoothingFilterKalman::Measurement(FTransform const & MeasuredTransform)
{
	try
	{
		FVector translation = MeasuredTransform.GetTranslation();

		FString msg = "In: " + translation.ToString();
		//FQuat rotation = MeasuredTransform.GetRotation();

		TransformAsMat.at<float>(0) = translation.X;
		TransformAsMat.at<float>(1) = translation.Y;
		TransformAsMat.at<float>(2) = translation.Z;
		//TransformAsMat.at<float>(3) = rotation.X * rotation.W;
		//TransformAsMat.at<float>(4) = rotation.Y * rotation.W;
		//TransformAsMat.at<float>(5) = rotation.Z * rotation.W;

		cv::Mat FilterResult = Filter.predict();
		Filter.correct(TransformAsMat);

		translation.Set(FilterResult.at<float>(0), FilterResult.at<float>(1), FilterResult.at<float>(2));

		msg += "\nOut: " + translation.ToString();

		UE_LOG(LogAUR, Log, TEXT("%s"), *msg);

		//FVector rot_axis(FilterResult.at<float>(3), FilterResult.at<float>(4), FilterResult.at<float>(5));
		//float rot_magnitude = rot_axis.Size();
		//rot_axis.Normalize();

		//rotation.X = rot_axis.X;
		//rotation.Y = rot_axis.Y;
		//rotation.Z = rot_axis.Z;
		//rotation.W = rot_magnitude;
	
		CurrentTransform.SetComponents(MeasuredTransform.GetRotation(), translation, FVector(1, 1, 1));
	}
	catch (cv::Exception& e)
	{
		FString exception_msg(e.what());

		UE_LOG(LogAUR, Error, TEXT("Kalman: %s"), *exception_msg)

		CurrentTransform = MeasuredTransform;
	}
}

FTransform UAURSmoothingFilterKalman::GetCurrentTransform() const
{
	return this->CurrentTransform;
}





