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

#pragma once

#include "video_sources/AURVideoSource.h"
#include "AURDriver.generated.h"

class AAURFiducialPattern;

UENUM(BlueprintType)
enum class EAURDiagnosticInfoLevel : uint8
{
	AURD_Silent = 0 	UMETA(DisplayName = "Diagnostic: Silent"),
	AURD_Basic = 1		UMETA(DisplayName = "Diagnostic: Basic"),
	AURD_Advanced = 2	UMETA(DisplayName = "Diagnostic: Advanced")
};

USTRUCT(BlueprintType)
struct FAURVideoFrame
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AugmentedReality)
	FIntPoint FrameResolution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AugmentedReality)
	TArray<FColor> Image;

	FAURVideoFrame()
	{
		this->SetResolution(FIntPoint(1280, 720));
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
 * Represents a way of connecting to a camera.
 */
UCLASS(Blueprintable, BlueprintType, Abstract, Config = Game)
class UAURDriver : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAURDriverVideoPropertiesChange, UAURDriver*, Driver);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAURDriverCalibrationStatusChange, UAURDriver*, Driver);
	DECLARE_DYNAMIC_DELEGATE_OneParam(FAURDriverInstanceChangeSingle, UAURDriver*, Driver);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAURDriverInstanceChange, UAURDriver*, Driver);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAURDriverViewpointTransformUpdate, UAURDriver*, Driver, FTransform, ViewportTransform);

	/** Called when the resolution / FOV changes / connection status changes */
	UPROPERTY(BlueprintAssignable)
	FAURDriverVideoPropertiesChange OnVideoPropertiesChange;

	/** Called when calibration starts or ends */
	UPROPERTY(BlueprintAssignable)
	FAURDriverCalibrationStatusChange OnCalibrationStatusChange;

	/** Called when a new viewpoint (camera) position is measured by the tracker */
	UPROPERTY(BlueprintAssignable)
	FAURDriverViewpointTransformUpdate OnViewpointTransformUpdate;

	/** True if it should track markers and calculate camera position+rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	uint32 bPerformOrientationTracking : 1;

	// Automatically creates those video sources on Initialize
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AugmentedReality)
	TArray< TSubclassOf<UAURVideoSource> > AvailableVideoSources;

	// Automatically creates those video sources on Initialize
	UPROPERTY(Transient, BlueprintReadOnly, Category = AugmentedReality)
	TArray<FAURVideoConfiguration> VideoConfigurations;

	// Switch to a new video source configuration
	// This does not intantly change the VideoSource variable - wait for the other
	// thread to close the previous one and open new one.
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void OpenVideoSource(FAURVideoConfiguration const& VideoConfiguration);

	// Switch to a new video source configuration found by identifier string
	// Returns true if a video configuration with that name was found.
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	bool OpenVideoSourceByName(FString const& VideoConfigurationName);

	// Switch to the last used video source configuration
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	bool OpenVideoSourceDefault();

	/** Called when a new viewpoint (camera) position is measured by the tracker */
	//UPROPERTY(BlueprintAssignable)
	//FAURDriverViewpointTransformUpdate OnViewpointTransformUpdate;

	UAURDriver();

	/**
	 * Start capturing video, setup marker tracking.
	 * @param parent_actor: Actor from which we take GetWorld for time measurement
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void Initialize(AActor* parent_actor);

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

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	UTexture2D* GetOutputTexture() const
	{
		return OutputTexture;
	}

	// Is the camera connected and working.
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual bool IsConnected() const;

	/**
	 * True if the specific calibration file for this camera has been found,
	 * False if it uses default fallback file.
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual bool IsCalibrated() const;

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual bool IsCalibrationInProgress() const;

	/**
	 * 0.0 <= progress <= 1.0 - calibration in progress
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual float GetCalibrationProgress() const;

	/**
	 * Attempt to calibrate the camera by observing a known pattern from different
	 * viewpoints.
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void StartCalibration();

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void CancelCalibration();

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual FIntPoint GetResolution() const;

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual FVector2D GetFieldOfView() const;

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	void GetCameraParameters(FIntPoint & camera_resolution, FVector2D field_of_view) const
	{
		camera_resolution = this->GetResolution();
		field_of_view = this->GetFieldOfView();
	}

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual FTransform GetCurrentViewportTransform() const;

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
	virtual bool IsNewFrameAvailable() const;

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	EAURDiagnosticInfoLevel GetDiagnosticInfoLevel() const
	{
		return DiagnosticLevel;
	}

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual void SetDiagnosticInfoLevel(EAURDiagnosticInfoLevel NewLevel);

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	void ToggleDiagnosticInfoLevel();

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	virtual FString GetDiagnosticText() const;

	// Start tracking a board - called by the static board list mechanism (RegisterBoardForTracking)
	virtual bool RegisterBoard(AAURFiducialPattern* board_actor, bool use_as_viewpoint_origin = false);

	virtual void UnregisterBoard(AAURFiducialPattern* board_actor);

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	static UAURDriver* GetCurrentDriver();

	/**
		Registers the board to be tracked by the currently running AURDriver
		(if one is running) and any AURDriver created in the future.

		Boards for AR tracking may be placed on various unconnected actors,
		who do not have the reference to the currently used AURDriver,
		or due to initialization order there may be no currently running driver at all.

		Therefore boards will be put on a global list and when a driver is present,
		it will read the list and track the board.
	**/
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	static void RegisterBoardForTracking(AAURFiducialPattern* board_actor, bool use_as_viewpoint_origin = false);

	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	static void UnregisterBoardForTracking(AAURFiducialPattern* board_actor);

	/*
	 * Add a callback to be notified about a new AURDriver instance being used
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	static void BindToOnDriverInstanceChange(FAURDriverInstanceChangeSingle const& Slot);

	static FAURDriverInstanceChange& GetDriverInstanceChangeDelegate();

	/*
	 * Remove the object from the delegate - it is important to call this on object's EndPlay
	 * to prevent old references from being in the static global delegate.
	 */
	UFUNCTION(BlueprintCallable, Category = AugmentedReality)
	static void UnbindOnDriverInstanceChange(UObject* SlotOwner);

protected:
	// The video is drawn on this dynamic texture
	UPROPERTY(BlueprintReadOnly, Transient, Category = AugmentedReality)
	UTexture2D* OutputTexture;

	// How much diagnostic information should be displayed. High value may reduce performance.
	UPROPERTY(EditAnywhere, Category = AugmentedReality)
	EAURDiagnosticInfoLevel DiagnosticLevel;

	// Automatically creates those video sources on Initialize
	UPROPERTY(Transient)
	TArray< UAURVideoSource* > VideoSourceInstances;

	/*
	Name of the last used video source, saved in config so that it is opened on next start
	*/
	UPROPERTY(Config)
	FString DefaultVideoSourceName;

	// Is the driver turned on
	uint32 bActive : 1;

	// Resolution of the frames used for image transfer
	FIntPoint FrameResolution;

	uint32 bCalibrationInProgress : 1;

	/** Reference to UWorld for time measurement */
	UWorld* WorldReference;

	// Resizes output texture and frames to fit the resolution provided by video source
	virtual void SetFrameResolution(FIntPoint const& new_res);

	static void EnsureDirExists(FString FilePath)
	{
		IPlatformFile & filesystem = FPlatformFileManager::Get().GetPlatformFile();
		filesystem.CreateDirectoryTree(*FPaths::GetPath(FilePath));
	}

private:
	// The rendering scheduled by ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	// takes place in the rendering thread:
	// https://docs.unrealengine.com/latest/INT/Programming/Rendering/ThreadedRendering/index.html
	// so data has to be passed through a struct.
	struct FTextureUpdateParameters
	{
		FTexture2DResource*	Texture2DResource;
		FUpdateTextureRegion2D RegionDefinition;
		UAURDriver* Driver;
	};
	// The constant values of these parameters:
	FTextureUpdateParameters TextureUpdateParameters;

	void WriteFrameToTexture();

	// Global registry of boards to track
	struct BoardRegistration
	{
		AAURFiducialPattern* Board;
		bool ViewpointOrigin;

		BoardRegistration(AAURFiducialPattern* board_actor, bool use_as_viewpoint_origin = false)
			: Board(board_actor)
			, ViewpointOrigin(use_as_viewpoint_origin)
		{}

		// For TArray.AddUnique
		bool operator==(BoardRegistration const & other) const
		{
			return Board == other.Board && ViewpointOrigin == other.ViewpointOrigin;
		}
	};
	static TArray<BoardRegistration> RegisteredBoards;
	static UAURDriver* CurrentDriver;
	static FAURDriverInstanceChange OnDriverInstanceChange;

	static void RegisterDriver(UAURDriver* driver);
	static void UnregisterDriver(UAURDriver* driver);
};
