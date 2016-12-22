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

#include "AURVideoSource.h"

#include <mutex>
#include <condition_variable>
#if PLATFORM_ANDROID
	#include "../../../Launch/Public/Android/AndroidJNI.h"
#endif

#include "AURVideoSourceAndroidCamera.generated.h"

/**
 * VideoSource using a cv::VideoCapture.
 */
UCLASS(Blueprintable, BlueprintType)
class UAURVideoSourceAndroidCamera : public UAURVideoSource
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = AugmentedReality)
	FIntPoint DesiredResolution;

	virtual bool Connect() override;
	virtual bool IsConnected() const override;
	virtual void Disconnect() override;
	virtual bool GetNextFrame(cv::Mat_<cv::Vec3b>& frame_out) override;
	virtual FIntPoint GetResolution() const override;
	virtual float GetFrequency() const override;

	UAURVideoSourceAndroidCamera();

#if PLATFORM_ANDROID
	void OnFrameCaptured(JNIEnv* LocalJNIEnv, jobject LocalThiz, jbyteArray data);
#endif

private:
	bool bConnected;
	FIntPoint Resolution;
	cv::Mat_<uint8_t> FrameYUV;

	bool NewFrameReady;
	std::mutex MutexNewFrame;
	std::condition_variable ConditionNewFrame;

#if PLATFORM_ANDROID
	static void AndroidMessage(FString msg);
	static void InitJNI();
#endif
};
	
