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

#include "AugmentedUnreality.h"
#include "AURVideoSourceAndroidCamera.h"

#if PLATFORM_ANDROID
#include "../../../Core/Public/Android/AndroidApplication.h"
#include "Algo/Reverse.h"

static UAURVideoSourceAndroidCamera* Instance = nullptr;

static jmethodID ActivityMethod_Message = nullptr;
static jmethodID ActivityMethod_CameraStartCapture = nullptr;
static jmethodID ActivityMethod_CameraStopCapture = nullptr;
static jmethodID ActivityMethod_GetFrameWidth = nullptr;
static jmethodID ActivityMethod_GetFrameHeight = nullptr;
static jmethodID ActivityMethod_GetAvailableResolutions = nullptr;
static jmethodID ActivityMethod_GetHorizontalFOV = nullptr;

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

JNIEnv* UAURVideoSourceAndroidCamera::InitJNI()
{
	// fresh JNI
	JNIEnv* java_env = FAndroidApplication::GetJavaEnv();

	if (!java_env)
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoSourceAndroidCamera: Failed to get JNI"));
		return nullptr;
	}

	// load methods if not loaded before
	if (! ActivityMethod_CameraStartCapture)
	{
		ActivityMethod_Message = FJavaWrapper::FindMethod(java_env, FJavaWrapper::GameActivityClassID, "AUR_Message", "(Ljava/lang/String;)V", true);
		ActivityMethod_CameraStartCapture = FJavaWrapper::FindMethod(java_env, FJavaWrapper::GameActivityClassID, "AUR_CameraStartCapture", "(II)Z", true);
		ActivityMethod_CameraStopCapture = FJavaWrapper::FindMethod(java_env, FJavaWrapper::GameActivityClassID, "AUR_CameraStopCapture", "()V", true);
		ActivityMethod_GetFrameWidth = FJavaWrapper::FindMethod(java_env, FJavaWrapper::GameActivityClassID, "AUR_GetFrameWidth", "()I", true);
		ActivityMethod_GetFrameHeight = FJavaWrapper::FindMethod(java_env, FJavaWrapper::GameActivityClassID, "AUR_GetFrameHeight", "()I", true);
		ActivityMethod_GetAvailableResolutions = FJavaWrapper::FindMethod(java_env, FJavaWrapper::GameActivityClassID, "AUR_GetAvailableResolutions", "()[I", true);
		ActivityMethod_GetHorizontalFOV = FJavaWrapper::FindMethod(java_env, FJavaWrapper::GameActivityClassID, "AUR_GetHorizontalFOV", "()F", true);

		UE_LOG(LogAUR, Log, TEXT("UAURVideoSourceAndroidCamera: Methods loaded"));
	}

	return java_env;
}

void UAURVideoSourceAndroidCamera::AndroidMessage(FString msg)
{
	UE_LOG(LogAUR, Warning, TEXT("UAURVideoSourceAndroidCamera::AndroidMessage %s"), *msg);

	JNIEnv* java_env = InitJNI();

	jstring jstr = java_env->NewStringUTF(TCHAR_TO_UTF8(*msg));
	FJavaWrapper::CallVoidMethod(java_env, FJavaWrapper::GameActivityThis, ActivityMethod_Message, jstr);
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

bool UAURVideoSourceAndroidCamera::Connect(FAURVideoConfiguration const& configuration)
{
	Super::Connect(configuration);

	UE_LOG(LogAUR, Log, TEXT("UAURVideoSourceAndroidCamera::Connect"));

	NewFrameReady = false;
	Resolution = configuration.Resolution;

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

	JNIEnv* java_env = InitJNI();
	bConnected = FJavaWrapper::CallBooleanMethod(java_env, FJavaWrapper::GameActivityThis, ActivityMethod_CameraStartCapture, Resolution.X, Resolution.Y);

	if (bConnected)
	{
		Resolution.X = FJavaWrapper::CallIntMethod(java_env, FJavaWrapper::GameActivityThis, ActivityMethod_GetFrameWidth);
		Resolution.Y = FJavaWrapper::CallIntMethod(java_env, FJavaWrapper::GameActivityThis, ActivityMethod_GetFrameHeight);
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

UAURVideoSourceAndroidCamera::UAURVideoSourceAndroidCamera()
	: PreferredResolutionX(640)
	, bConnected(false)
	, NewFrameReady(false)
{
}

FString UAURVideoSourceAndroidCamera::GetIdentifier() const
{
	return "Android";
}

FText UAURVideoSourceAndroidCamera::GetSourceName() const
{
	return NSLOCTEXT("AUR", "VideoSourceAndroidCamera", "Android");
}

void UAURVideoSourceAndroidCamera::DiscoverConfigurations()
{
	Configurations.Empty();

	// Only register available sources if we are on the right platform
#if PLATFORM_ANDROID

	TArray<FIntPoint> resolutions;

	{
		JNIEnv* java_env = InitJNI();

		// AUR_GetAvailableResolutions returns an int array where indices x*2 and x*2+1 represent x-th resolution
		jintArray java_array_of_res = (jintArray)FJavaWrapper::CallObjectMethod(java_env, FJavaWrapper::GameActivityThis, ActivityMethod_GetAvailableResolutions);
		if (!java_array_of_res)
		{
			UE_LOG(LogAUR, Error, TEXT("VideoSourceAndroidCamera::DiscoverConfigurations: received NULL array of resolutions"));
			return;
		}

		const int32 java_array_len = java_env->GetArrayLength(java_array_of_res);
		jint* java_array_elems = java_env->GetIntArrayElements(java_array_of_res, nullptr);

		if (!java_array_elems)
		{
			UE_LOG(LogAUR, Error, TEXT("VideoSourceAndroidCamera::DiscoverConfigurations: received NULL array content"));
			return;
		}

		const int32 res_count = java_array_len / 2;
		for (int32 res_id = 0; res_id < res_count; res_id++)
		{
			resolutions.Emplace(java_array_elems[2 * res_id], java_array_elems[2 * res_id + 1]);
		}

		java_env->ReleaseIntArrayElements(java_array_of_res, java_array_elems, 0);
		Algo::Reverse(resolutions);
	}

	for (auto const& resolution : resolutions)
	{
		FAURVideoConfiguration cfg(this, ResolutionToString(resolution));
		cfg.Resolution = resolution;
		cfg.SetPriorityFromDesiredResolution(PreferredResolutionX);

		Configurations.Add(cfg);
	}
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

		JNIEnv* java_env = InitJNI();

		FJavaWrapper::CallVoidMethod(java_env, FJavaWrapper::GameActivityThis, ActivityMethod_CameraStopCapture);
	}
#else
	UE_LOG(LogAUR, Warning, TEXT("UAURVideoSourceAndroidCamera not available on this platform"));
#endif
}

bool UAURVideoSourceAndroidCamera::GetNextFrame(cv::Mat_<cv::Vec3b>& frame_out)
{
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

bool UAURVideoSourceAndroidCamera::LoadCalibration()
{
	const bool loaded_from_file = Super::LoadCalibration();

#if PLATFORM_ANDROID
	if (!loaded_from_file)
	{
		JNIEnv* java_env = InitJNI();

		const float reported_horizontal_fov = java_env->CallFloatMethod(FJavaWrapper::GameActivityThis, ActivityMethod_GetHorizontalFOV);

		UE_LOG(LogAUR, Log, TEXT("UAURVideoSourceAndroidCamera: reported horizontal FOV = %f deg"), reported_horizontal_fov);

		CameraProperties.SetHorizontalFOV(reported_horizontal_fov);
	}
#endif

	return loaded_from_file;
}
