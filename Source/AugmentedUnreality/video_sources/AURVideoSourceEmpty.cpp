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
#include "AURVideoSourceEmpty.h"

UAURVideoSourceEmpty::UAURVideoSourceEmpty()
{
	PriorityMultiplier = 0.0;
}

FString UAURVideoSourceEmpty::GetIdentifier() const
{
	return "empty";
}

FText UAURVideoSourceEmpty::GetSourceName() const
{
	return NSLOCTEXT("AUR", "VideoSourceEmpty", "Video Off");
}

void UAURVideoSourceEmpty::DiscoverConfigurations()
{
	Configurations.Empty();

	FAURVideoConfiguration cfg(this, "");
	Configurations.Add(cfg);
}

bool UAURVideoSourceEmpty::Connect(FAURVideoConfiguration const& configuration)
{
	Super::Connect(configuration);
	return false;
}

bool UAURVideoSourceEmpty::IsConnected() const
{
	return false;
}

void UAURVideoSourceEmpty::Disconnect()
{
}

bool UAURVideoSourceEmpty::GetNextFrame(cv::Mat_<cv::Vec3b>& frame)
{
	return false;
}

FIntPoint UAURVideoSourceEmpty::GetResolution() const
{
	return FIntPoint(1, 1);
}

float UAURVideoSourceEmpty::GetFrequency() const
{
	return 0.0f;
}

