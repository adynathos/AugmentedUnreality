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

#include "AURVideoSourceCvCapture.h"
#include "AURVideoSourceVideoFile.generated.h"

/**
 * Video stream from a video file, looped.
 */
UCLASS(Blueprintable, BlueprintType)
class UAURVideoSourceVideoFile : public UAURVideoSourceCvCapture
{
	GENERATED_BODY()

public:
	// Path to the video file, relative to FPaths::GameDir()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	FString VideoFile;

	virtual FString GetIdentifier() const override;
	virtual FText GetSourceName() const override;
	virtual void DiscoverConfigurations() override;

	virtual bool Connect(FAURVideoConfiguration const& configuration) override;
	virtual bool GetNextFrame(cv::Mat_<cv::Vec3b>& frame) override;

protected:
	// Time between consecutive frames
	float Period;

	// Number of frames in the file
	int FrameCount;

	static const float MAX_FPS;
	static const float MIN_FPS;
};

