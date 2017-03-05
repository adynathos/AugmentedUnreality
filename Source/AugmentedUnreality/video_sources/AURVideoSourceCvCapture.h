/*
Copyright 2016-2017 Krzysztof Lis

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

#include "AURVideoSource.h"
#include "AURVideoSourceCvCapture.generated.h"

/**
 * VideoSource using a cv::VideoCapture.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class UAURVideoSourceCvCapture : public UAURVideoSource
{
	GENERATED_BODY()
	
public:
	virtual bool IsConnected() const override;
	virtual void Disconnect() override;
	virtual bool GetNextFrame(cv::Mat_<cv::Vec3b>& frame) override;
	virtual FIntPoint GetResolution() const override;
	virtual float GetFrequency() const override;

protected:
	bool OpenVideoCapture(const FString argument);

	cv::VideoCapture Capture;
};
	
