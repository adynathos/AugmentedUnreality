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
#include "AURVideoScreenBackground.h"

UAURVideoScreenBackground::UAURVideoScreenBackground()
	: Resolution(640, 480)
{
}

void UAURVideoScreenBackground::OnCameraPropertiesChange(UAURDriver* Driver)
{
	Super::OnCameraPropertiesChange(Driver);

	if (VideoDriver && VideoDriver->IsConnected())
	{
		SetResolution(VideoDriver->GetResolution());
		SetSizeForFOV(VideoDriver->GetFieldOfView().X);
	}
}

void UAURVideoScreenBackground::SetResolution(FIntPoint resolution)
{
	Resolution = resolution;
	// the X size is tied to FOV, so scale Y with respect to X
	// With the new texture, the XY placement of texture is swapped
	//this->SetRelativeScale3D(FVector(RelativeScale3D.Y * (float)Resolution.Y / (float)Resolution.X, RelativeScale3D.Y, 1));
}

void UAURVideoScreenBackground::SetSizeForFOV(float FOV_Horizontal)
{
	// Get camera parameters
	//FIntPoint cam_resolution;
	//float cam_fov, cam_aspect_ratio;
	//this->VideoDriver->GetCameraParameters(cam_resolution, cam_fov, cam_aspect_ratio);

	// Get local position
	float distance_to_origin = this->GetRelativeTransform().GetLocation().Size();

	// Try to adjust size so that it fills the whole screen
	float width = distance_to_origin * 2.0 * FMath::Tan(FMath::DegreesToRadians(0.5 * FOV_Horizontal));
	float height = width * (float)Resolution.Y / (float)Resolution.X;

	// The texture size is 100x100
	//this->SetRelativeScale3D(FVector(width / 100.0, height / 100.0, 1));
	// With the new texture, the XY placement of texture is swapped
	this->SetRelativeScale3D(FVector(height / 100.0, width / 100.0, 1));

	const FString msg = "UAURVideoScreenBackground::SetSizeForFOV(fov_horizontal=" + FString::SanitizeFloat(FOV_Horizontal) + ") -> scale = " 
		+ FString::SanitizeFloat(this->RelativeScale3D.X) + " x " + FString::SanitizeFloat(this->RelativeScale3D.Y);
	UE_LOG(LogAUR, Log, TEXT("%s"), *msg)
}
