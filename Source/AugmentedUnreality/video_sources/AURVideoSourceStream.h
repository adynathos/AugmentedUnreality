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
#include "AURVideoSourceStream.generated.h"

/**
 * GStreamer stream.
 * For example - receive h264 stram on port 5000:
 * "udpsrc port=5000 ! application/x-rtp, encoding-name=H264,payload=96 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! appsink"
 */
UCLASS(Blueprintable, BlueprintType)
class UAURVideoSourceStream : public UAURVideoSourceCvCapture
{
	GENERATED_BODY()
	
public:
	//Connection string for GStreamer, leave empty to use StreamFile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	FString ConnectionString;

	// Stream definition file, like .sdp, used when ConnectionString is empty
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	FString StreamFile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	FText StreamName;

	UAURVideoSourceStream();

	virtual FString GetIdentifier() const override;
	virtual FText GetSourceName() const override;
	virtual void DiscoverConfigurations() override;

	virtual bool Connect(FAURVideoConfiguration const& configuration) override;
};
	