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
#include "AURVideoSourceCamera.h"

UAURVideoSourceCamera::UAURVideoSourceCamera()
	: CameraIndex(0)
	, DesiredResolution(0, 0)
	, Autofocus(true)
{
}

FText UAURVideoSourceCamera::GetSourceName() const
{
	if(!SourceName.IsEmpty())
	{
		return SourceName;
	}
	else
	{
		return FText::FromString(FString::Printf(TEXT("Camera %d"), CameraIndex));
	}
}

bool UAURVideoSourceCamera::Connect()
{
#ifndef __ANDROID__
	try
	{
#endif
		Capture.open(CameraIndex);

		if (Capture.isOpened())
		{
			UE_LOG(LogAUR, Log, TEXT("UAURVideoSourceCamera::Connect: Connected to Camera %d"), CameraIndex)

#ifndef __linux__
			// Suggest resolution
			if(DesiredResolution.GetMin() > 0)
			{
				Capture.set(cv::CAP_PROP_FRAME_WIDTH, DesiredResolution.X);
				Capture.set(cv::CAP_PROP_FRAME_HEIGHT, DesiredResolution.Y);
			}

			// Suggest autofocus
			Capture.set(cv::CAP_PROP_AUTOFOCUS, Autofocus ? 1 : 0);
#endif

			LoadCalibration();
		}
		else
		{
			UE_LOG(LogAUR, Error, TEXT("UAURVideoSourceCamera::Connect: Failed to open Camera %d"), CameraIndex)
		}
#ifndef __ANDROID__
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("Exception in UAURVideoSourceCamera::Connect:\n	%s\n"), UTF8_TO_TCHAR(exc.what()))
		return false;
	}
#endif
	
	return Capture.isOpened();
}
