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
#include "AURVideoSourceStream.h"

UAURVideoSourceStream::UAURVideoSourceStream()
	: ConnectionString("udpsrc port=5000 ! application/x-rtp, encoding-name=H264,payload=96 ! rtph264depay ! queue ! h264parse ! avdec_h264 ! videoconvert ! appsink")
	, StreamName(NSLOCTEXT("AUR", "VideoSourceVideoStream", "Stream"))
{
}

FString UAURVideoSourceStream::GetIdentifier() const
{
	return StreamName.ToString();
}

FText UAURVideoSourceStream::GetSourceName() const
{
	if (!StreamName.IsEmpty())
	{
		return StreamName;
	}
	else
	{
		if (!ConnectionString.IsEmpty())
		{
			return FText::FromString("GStreamer");
		}
		else
		{
			return FText::FromString("Stream " + FPaths::GetCleanFilename(StreamFile));
		}
	}
}

void UAURVideoSourceStream::DiscoverConfigurations()
{
	Configurations.Empty();

	FAURVideoConfiguration cfg(this, "");

	if (!ConnectionString.IsEmpty())
	{
		cfg.FilePath = ConnectionString;
		Configurations.Add(cfg);
	}
	else
	{
		const FString full_path = FPaths::GameDir() / StreamFile;

		if (FPaths::FileExists(full_path))
		{
			cfg.FilePath = full_path;
			Configurations.Add(cfg);
		}
		else
		{
			UE_LOG(LogAUR, Warning, TEXT("UAURVideoSourceStream:: File %s does not exist"), *full_path)
		}
	}
}

bool UAURVideoSourceStream::Connect(FAURVideoConfiguration const& configuration)
{
	Super::Connect(configuration);

	bool success = false;
#if !PLATFORM_ANDROID
	try
	{
#endif
		success = OpenVideoCapture(configuration.FilePath);

		if (success)
		{
			Capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
			LoadCalibration();
		}

		return Capture.isOpened();
#if !PLATFORM_ANDROID
	}
	catch (std::exception& exc)
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoSourceStream::Connect: exception \n    %s"), UTF8_TO_TCHAR(exc.what()))
	}

	return false;
#endif
}
