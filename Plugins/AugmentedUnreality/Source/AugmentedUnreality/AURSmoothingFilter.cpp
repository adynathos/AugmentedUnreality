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
#include "AURSmoothingFilter.h"

void UAURSmoothingFilter::Init()
{
	// Maybe the filter does not need initialization.
}

void UAURSmoothingFilter::Measurement(FTransform const & MeasuredTransform)
{
	UE_LOG(LogAUR, Error, TEXT("AURSmoothingFilter::Measurement is not implemented"))
}

FTransform UAURSmoothingFilter::GetCurrentTransform() const
{
	UE_LOG(LogAUR, Error, TEXT("AURSmoothingFilter::GetCurrentTransform is not implemented"))
	return FTransform();
}



