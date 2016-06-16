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

#include "AURSmoothingFilter.h"

#include "opencv2/video/tracking.hpp"

#include "AURSmoothingFilterKalman.generated.h"

/**
 * 
 */
UCLASS()
class UAURSmoothingFilterKalman : public UAURSmoothingFilter
{
	GENERATED_BODY()

public:
	UAURSmoothingFilterKalman();

	virtual void Init();
	virtual void Measurement(FTransform const & MeasuredTransform) override;
	virtual FTransform GetCurrentTransform() const override;

protected:
	FTransform CurrentTransform;

	cv::KalmanFilter Filter;
	cv::Mat TransformAsMat;
};
