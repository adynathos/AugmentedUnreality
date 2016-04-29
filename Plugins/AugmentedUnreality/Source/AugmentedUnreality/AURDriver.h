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

#include "Object.h"

#include <string>

class UAURSmoothingFilter;
#include "AURDriver.generated.h"

USTRUCT(BlueprintType)
struct FAURVideoFrame
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AugmentedReality)
	FIntPoint FrameResolution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AugmentedReality)
	TArray<FColor> Image;

	FAURVideoFrame()
		: FrameResolution(0, 0)
	{
	}

	FAURVideoFrame(FIntPoint resolution)
		: FrameResolution(resolution)
	{
		this->SetResolution(resolution);
	}

	void SetResolution(FIntPoint resolution)
	{
		this->FrameResolution = resolution;
		this->Image.Init(FColor::MakeRandomColor(), this->GetImageSize());
	}

	/**
	 * Number of pixels.
	 */
	int32 GetImageSize() const
	{
		return this->FrameResolution.X * this->FrameResolution.Y;
	}

	uint8* GetDataPointerRaw() const
	{
		return (uint8*)this->Image.GetData();
	}
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, Abstract)
class UAURDriver : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	uint32 bPerformOrientationTracking:1;

	/**
	 *	Scale of the tracking coordinates:
	 *	unreal coords / tracking coords
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	float TranslationScale;

	/**
	 *	Desired center of scene in tracker coorindates.
	 *	Will be subtracted from the measured position.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	FVector SceneCenterInTrackerCoordinates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	TSubclassOf<UAURSmoothingFilter> SmoothingFilterClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = AugmentedReality)
	UAURSmoothingFilter* SmoothingFilterInstance;

	UAURDriver();

	/**
	 * Start capturing video, setup marker tracking.
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void Initialize();

	/**
	* Provide an UWorld reference for time measurement.
	*/
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	void SetWorld(UWorld* world_reference) 
	{
		this->WorldReference = world_reference;
	}

	/**
	 * Some drivers may need constant updating
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void Tick();

	/**
	 * Stop capturing video, release resources.
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void Shutdown();

	/**
	 * FOV of the camera, available only after Initialize().
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void GetCameraParameters(FIntPoint & resolution, float & field_of_view_angle, float & aspect_ratio_x_to_y);

	/**
	 * Returns a pointer to FAURVideoFrame containing the current camera frame.
	 * Do not delete the pointer.
	 * The data in this frame will not change.
	 * Call this again to get a pointer to the new frame.
	 * THE EXISTING FRAME POINTER BECOMES INVALID WHEN GetFrame IS CALLED (it may be processed by other thread).
	 * If you need to store the data, copy it before GetFrame().
	 */
	virtual FAURVideoFrame* GetFrame();

	/**
	 * @returns true if a new frame has been captured since the last GetFrame() call.
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual bool IsNewFrameAvailable();

	/** Provide the camera orientation and last update time
	The other accessor functions will by default call this
	*/
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void GetOrientationAndUpdateTime(FTransform & OutOrientation, float & OutUpdateTime);

	/**
	 * Get the current position and rotation of the camera.
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual FTransform GetOrientation();

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual float GetLastOrientationUpdateTime();

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual float GetTimeSinceLastOrientationUpdate();

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual bool IsNewOrientationAvailable();

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual FString GetDiagnosticText();

protected:
	/** Camera parameters */
	FIntPoint Resolution;
	float CameraFov;
	float CameraAspectRatio;

	/** Tracking state */
	FTransform CurrentOrientation;
	float LastOrientationUpdateTime;

	/** Reference to UWorld for time measurement */
	UWorld* WorldReference;

	/** 
		Called by the driver when a new orientation is detected
		set CurrentOrientation and LastOrientationUpdateTime

		If a filter is present, filtering is done here.
	**/
	virtual void StoreNewOrientation(FTransform const & measurement);

public:
	struct BGR_Color {
		uint8 B;
		uint8 G;
		uint8 R;
	};
};
