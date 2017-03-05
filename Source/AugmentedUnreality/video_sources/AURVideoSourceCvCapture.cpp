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
#include "AURVideoSourceCvCapture.h"

bool UAURVideoSourceCvCapture::IsConnected() const
{
	return Capture.isOpened();
}

void UAURVideoSourceCvCapture::Disconnect()
{
	if (Capture.isOpened())
	{
		Capture.release();
	}
}

bool UAURVideoSourceCvCapture::GetNextFrame(cv::Mat_<cv::Vec3b>& frame)
{
	return Capture.read(frame);
}

FIntPoint UAURVideoSourceCvCapture::GetResolution() const
{
	FIntPoint camera_res;
	camera_res.X = FPlatformMath::RoundToInt(Capture.get(cv::CAP_PROP_FRAME_WIDTH));
	camera_res.Y = FPlatformMath::RoundToInt(Capture.get(cv::CAP_PROP_FRAME_HEIGHT));
	return camera_res;
}

float UAURVideoSourceCvCapture::GetFrequency() const
{
	return Capture.get(cv::CAP_PROP_FPS);
}

bool UAURVideoSourceCvCapture::OpenVideoCapture(const FString argument)
{
#if !PLATFORM_ANDROID
	try
	{
#endif
		return Capture.open(TCHAR_TO_UTF8(*argument));
#if !PLATFORM_ANDROID
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("cv::VideoCapture::open exception\n    %s"), UTF8_TO_TCHAR(exc.what()))
	}
	return false;
#endif
}
