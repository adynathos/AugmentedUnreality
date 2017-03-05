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
#include "AURVideoSourceCamera.generated.h"

/**
 * Video stream from a camera.
 * Change the settings only before calling Connect
 */
UCLASS(Blueprintable, BlueprintType)
class UAURVideoSourceCamera : public UAURVideoSourceCvCapture
{
	GENERATED_BODY()

public:
	// Index (0-based) of the camera to use.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	int32 CameraIndex;

	// Resolutions near this will be given higher priority
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	int32 PreferredResolutionX;

	/*
		Resolutions to be selectable in the program.
		OpenCV does not give us a way to find camera resolutions, so we need to guess.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VideoSource)
	TArray<FIntPoint> OfferedResolutions;

	UAURVideoSourceCamera();

	virtual FText GetSourceName() const override;
	virtual FString GetIdentifier() const override;
	virtual void DiscoverConfigurations() override;
	virtual bool Connect(FAURVideoConfiguration const& configuration) override;
};
