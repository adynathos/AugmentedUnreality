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
#include "AURVideoSourceTest.h"

UAURVideoSourceTest::UAURVideoSourceTest()
	: DesiredResolution(1280, 720)
	, FramesPerSecond(2.0)
{
	PriorityMultiplier = 0.5;
}

FString UAURVideoSourceTest::GetIdentifier() const
{
	return "Test";
}

FText UAURVideoSourceTest::GetSourceName() const
{
	return NSLOCTEXT("AUR", "VideoSourceTest", "Test Video");
}

void UAURVideoSourceTest::DiscoverConfigurations()
{
	Configurations.Empty();

	FAURVideoConfiguration cfg(this, "");
	cfg.Resolution = DesiredResolution;
	Configurations.Add(cfg);
}

bool UAURVideoSourceTest::Connect(FAURVideoConfiguration const& configuration)
{
	Super::Connect(configuration);

	UE_LOG(LogAUR, Log, TEXT("UAURVideoSourceTest::Connect()"));

	if (DesiredResolution.X <= 0 || DesiredResolution.Y <= 0)
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoSourceTest: overriding wrong DesiredResolution %dx%d"), DesiredResolution.X, DesiredResolution.Y);

		DesiredResolution.X = 1280;
		DesiredResolution.Y = 720;
	}

	return true;
}

bool UAURVideoSourceTest::IsConnected() const
{
	return true;
}

void UAURVideoSourceTest::Disconnect()
{
	UE_LOG(LogAUR, Log, TEXT("UAURVideoSourceTest::Disconnect()"));
}

cv::RNG random_gen;

bool UAURVideoSourceTest::GetNextFrame(cv::Mat_<cv::Vec3b>& frame)
{
	if (FramesPerSecond < 0.5)
	{
		UE_LOG(LogAUR, Error, TEXT("UAURVideoSourceTest: overriding wrong fps %f"), FramesPerSecond);
		FramesPerSecond = 0.5;
	}
	FPlatformProcess::Sleep(1.0 / FramesPerSecond);

	frame.create(DesiredResolution.Y, DesiredResolution.X);
	frame.setTo(cv::Vec3b(random_gen.uniform(0, 255), random_gen.uniform(0, 255), random_gen.uniform(0, 255)));

	return true;
}

FIntPoint UAURVideoSourceTest::GetResolution() const
{
	return DesiredResolution;
}

float UAURVideoSourceTest::GetFrequency() const
{
	return FramesPerSecond;
}

