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
#include "AURVideoSourceAndroidCamera.h"

#if PLATFORM_ANDROID
#include "../../../Core/Public/Android/AndroidApplication.h"

static UAURVideoSourceAndroidCamera* Instance = nullptr;

static JNIEnv* JavaEnv = nullptr;
static jmethodID ActivityMethod_Message;
static jmethodID ActivityMethod_CameraStartCapture;
static jmethodID ActivityMethod_CameraStopCapture;
static jmethodID ActivityMethod_GetFrameWidth;
static jmethodID ActivityMethod_GetFrameHeight;

extern "C" void Java_com_epicgames_ue4_GameActivity_AURNotify(JNIEnv* LocalJNIEnv, jobject LocalThiz)
{
	UE_LOG(LogAUR, Warning, TEXT("AURNotify"));
}

extern "C" void Java_com_epicgames_ue4_GameActivity_AURReceiveFrame(JNIEnv* LocalJNIEnv, jobject LocalThiz, jbyteArray data)
{
	if (Instance)
	{
		Instance->OnFrameCaptured(LocalJNIEnv, LocalThiz, data);
	}
	else
	{
		UE_LOG(LogAUR, Warning, TEXT("UAURVideoSourceAndroidCamera camera capture is running but no instance to process it"));
	}
}

void UAURVideoSourceAndroidCamera::InitJNI()
{
	if (!JavaEnv)
	{
		JavaEnv = FAndroidApplication::GetJavaEnv();
		ActivityMethod_Message = FJavaWrapper::FindMethod(JavaEnv, FJavaWrapper::GameActivityClassID, "AUR_Message", "(Ljava/lang/String;)V", true);
		ActivityMethod_CameraStartCapture = FJavaWrapper::FindMethod(JavaEnv, FJavaWrapper::GameActivityClassID, "AUR_CameraStartCapture", "(II)Z", true);
		ActivityMethod_CameraStopCapture = FJavaWrapper::FindMethod(JavaEnv, FJavaWrapper::GameActivityClassID, "AUR_CameraStopCapture", "()V", true);
		ActivityMethod_GetFrameWidth = FJavaWrapper::FindMethod(JavaEnv, FJavaWrapper::GameActivityClassID, "AUR_GetFrameWidth", "()I", true);
		ActivityMethod_GetFrameHeight = FJavaWrapper::FindMethod(JavaEnv, FJavaWrapper::GameActivityClassID, "AUR_GetFrameHeight", "()I", true);

		UE_LOG(LogAUR, Warning, TEXT("UAURVideoSourceAndroidCamera::InitJNI"));
	}
}

void UAURVideoSourceAndroidCamera::AndroidMessage(FString msg)
{
	UE_LOG(LogAUR, Warning, TEXT("UAURVideoSourceAndroidCamera::AndroidMessage %s"), *msg);

	InitJNI();
	jstring jstr = JavaEnv->NewStringUTF(TCHAR_TO_UTF8(*msg));
	FJavaWrapper::CallVoidMethod(JavaEnv, FJavaWrapper::GameActivityThis, ActivityMethod_Message, jstr);
}

void UAURVideoSourceAndroidCamera::OnFrameCaptured(JNIEnv* LocalJNIEnv, jobject LocalThiz, jbyteArray data_array)
{
	const int32 data_size = LocalJNIEnv->GetArrayLength(data_array);
	/*
	We receive the frames in YUV420sp format which means their size is 
		width x (1.5 * height) x 1
	as opposed to RGB's size:
		width x height x 3
	*/
	const int32 data_height = Resolution.Y + Resolution.Y / 2;
	const int32 data_width = Resolution.X;

	if (data_size != data_width*data_height)
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoSourceAndroidCamera frame dimensions don't match %dx%d but size %d"), Resolution.X, Resolution.Y, data_size);
		return;
	}


	{
		// acquire lock on FrameYUV and NewFrameReady
		std::unique_lock<std::mutex> lock(MutexNewFrame);

		FrameYUV.create(data_height, data_width);

		// We should check FrameYUV.isContinuous(), but it says .create always create continuous

		// Copy to cv array
		LocalJNIEnv->GetByteArrayRegion(data_array, 0, data_size, reinterpret_cast<jbyte*>(FrameYUV.ptr()));

		NewFrameReady = true;

		// release lock
	}

	// wake up the processing thread if its waiting
	ConditionNewFrame.notify_one();
}
#endif

// TODO add a function through which Android can tell us about the camera being disconnected

bool UAURVideoSourceAndroidCamera::Connect()
{
	UE_LOG(LogAUR, Log, TEXT("UAURVideoSourceAndroidCamera::Connect"));

	NewFrameReady = false;

#if PLATFORM_ANDROID
	if (Instance == nullptr)
	{
		Instance = this;
	}
	else
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoSourceAndroidCamera another instance is already running"));
		return false;
	}

	InitJNI();
	bConnected = FJavaWrapper::CallBooleanMethod(JavaEnv, FJavaWrapper::GameActivityThis, ActivityMethod_CameraStartCapture, DesiredResolution.X, DesiredResolution.Y);

	if (bConnected)
	{
		Resolution.X = FJavaWrapper::CallIntMethod(JavaEnv, FJavaWrapper::GameActivityThis, ActivityMethod_GetFrameWidth);
		Resolution.Y = FJavaWrapper::CallIntMethod(JavaEnv, FJavaWrapper::GameActivityThis, ActivityMethod_GetFrameHeight);
		LoadCalibration();
	}
	else
	{
		UE_LOG(LogAUR, Warning, TEXT("UAURVideoSourceAndroidCamera failed to connect"));
	}

	return bConnected;
#else
	UE_LOG(LogAUR, Warning, TEXT("UAURVideoSourceAndroidCamera not available on this platform"));
	return false;
#endif
}

bool UAURVideoSourceAndroidCamera::IsConnected() const
{
	return bConnected;
}

void UAURVideoSourceAndroidCamera::Disconnect()
{
	UE_LOG(LogAUR, Log, TEXT("UAURVideoSourceAndroidCamera::Disconnect"));

#if PLATFORM_ANDROID
	if (Instance == this)
	{
		Instance = nullptr;
		InitJNI();
		FJavaWrapper::CallVoidMethod(JavaEnv, FJavaWrapper::GameActivityThis, ActivityMethod_CameraStopCapture);
	}
#else
	UE_LOG(LogAUR, Warning, TEXT("UAURVideoSourceAndroidCamera not available on this platform"));
#endif
}

cv::RNG random_gen2;

bool UAURVideoSourceAndroidCamera::GetNextFrame(cv::Mat_<cv::Vec3b>& frame_out)
{
	UE_LOG(LogAUR, Log, TEXT("UAURVideoSourceAndroidCamera::GetNewFrame"));
	
	{
		// acquire lock on FrameYUV and NewFrameReady
		std::unique_lock<std::mutex> lock(MutexNewFrame);

		// if the frame is not ready, wait for next one
		if (!NewFrameReady)
		{
			ConditionNewFrame.wait(lock, [this]{ return this->NewFrameReady; });
		}

		// Convert stored frame to RGB and write to output
		frame_out.create(Resolution.Y, Resolution.X);
		cv::cvtColor(FrameYUV, frame_out, cv::COLOR_YUV420sp2BGR);

		// frame is consumed
		NewFrameReady = false;

		// release lock
	}

	return true;
}

FIntPoint UAURVideoSourceAndroidCamera::GetResolution() const
{
	return Resolution;
}

float UAURVideoSourceAndroidCamera::GetFrequency() const
{
	return 1.0;
}

UAURVideoSourceAndroidCamera::UAURVideoSourceAndroidCamera()
	: DesiredResolution(640, 360)
	, bConnected(false)
	, NewFrameReady(false)
{
}

