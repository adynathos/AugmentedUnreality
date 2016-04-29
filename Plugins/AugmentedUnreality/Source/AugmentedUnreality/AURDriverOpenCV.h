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

#include "AURDriver.h"
#include <vector>
#include "OpenCV_includes.h"

#include "AURDriverOpenCV.generated.h"


USTRUCT(BlueprintType)
struct FArucoGridBoardDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoBoard)
	FString SavedFileName;

	// Size of the grid in X direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoBoard)
	int32 GridWidth;

	// Size of the grid in Y direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoBoard)
	int32 GridHeight;

	// Size of the marker in pixels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoBoard)
	int32 MarkerSize;

	// Space between markers in pixels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoBoard)
	int32 SeparationSize;

	/**
	Id of the predefined marker dictionary. Choices:
	DICT_4X4_50 = 0,
	DICT_4X4_100,
	DICT_4X4_250,
	DICT_4X4_1000,
	DICT_5X5_50,
	DICT_5X5_100,
	DICT_5X5_250,
	DICT_5X5_1000,
	DICT_6X6_50,
	DICT_6X6_100,
	DICT_6X6_250,
	DICT_6X6_1000,
	DICT_7X7_50,
	DICT_7X7_100,
	DICT_7X7_250,
	DICT_7X7_1000,
	DICT_ARUCO_ORIGINAL
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArucoBoard)
	int32 DictionaryId;

	FArucoGridBoardDefinition()
		: SavedFileName("AUR/marker_aruco_default.png")
		, GridWidth(1)
		, GridHeight(2)
		, MarkerSize(400)
		, SeparationSize(100)
		, DictionaryId(1)
	{
	}
};

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class UAURDriverOpenCV : public UAURDriver
{
	GENERATED_BODY()

public:
	/**
	 *	ONLY SET THESE PROPERTIES BEFORE CALLING Initialize()
	 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	FArucoGridBoardDefinition MarkerDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	int32 CameraIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	FString CameraParametersFile;

	/**
	* true -> will attempt to use the maximal resolution supported by the camera.
	* false -> will use the resolution specified in the configuration file.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	uint32 bUseMaximalCameraResolution : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AugmentedReality)
	uint32 bHighlightMarkers : 1;

	UAURDriverOpenCV();

	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual FAURVideoFrame* GetFrame() override;
	virtual bool IsNewFrameAvailable() override;
	virtual FTransform GetOrientation() override;
	virtual FString GetDiagnosticText() override;

	static FVector ConvertOpenCvVectorToUnreal(cv::Vec3f const & cv_vector);

protected:
	// Connection to the camera
	cv::VideoCapture VideoCapture;

	// Marker detection
	cv::aruco::Dictionary ArucoMarkerDictionary;
	cv::aruco::GridBoard* ArucoBoard;

	cv::Mat CameraMatrix;
	cv::Mat CameraDistortionCoefficients;
	float CameraPixelRatio;

	// Threaded capture model
	FAURVideoFrame* WorkerFrame; // the frame processed by worker thread
	FAURVideoFrame* AvailableFrame; // the frame ready to be published
	FAURVideoFrame* PublishedFrame; // the frame currently held by tje game

	FCriticalSection FrameLock; // mutex which needs to be obtained before manipulating the frame pointers
	FThreadSafeBool bNewFrameReady; // is there a new frame in AvailableFrame

	FCriticalSection OrientationLock; // mutex which needs to be obtained before using CameraOrientation variable.
	FThreadSafeBool bNewOrientationReady;

	/**
		Adds thread safety to the storing operation
	*/
	virtual void StoreNewOrientation(FTransform const & measurement);

	// Creates a default aruco board and saves a copy to a file.
	void CreateBoard(FArucoGridBoardDefinition const & BoardDefinition);

	void InitializeCamera();
	void InitializeWorker();

	/**
	 * cv::VideoCapture::read blocks untill a new frame is available.
	 * If it was executed in the main thread, the main tick would be
	 * bound to the camera FPS.
	 * To prevent that, we use a worker thread.
	 * This also prevents the marker detection from slowing down the main thread.
	 */
	class FWorkerRunnable : public FRunnable
	{
	public:
		FWorkerRunnable(UAURDriverOpenCV* driver);

		// Begin FRunnable interface.
		virtual bool Init();
		virtual uint32 Run();
		virtual void Stop();
		// End FRunnable interface

	protected:
		// The driver on which we work
		UAURDriverOpenCV* Driver;

		// Set to false to stop the thread
		FThreadSafeBool bContinue;

		cv::Mat CapturedFrame;
		//std::vector<std::vector< cv::Point2f > > MarkerCorners;

		uint32 bTracking : 1;
	};

	TUniquePtr<FWorkerRunnable> Worker;
	TUniquePtr<FRunnableThread> WorkerThread;

	///=============================================
	FString DiagnosticText;
};
