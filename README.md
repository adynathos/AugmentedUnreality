## Augmented Unreality
### Augmented reality for Unreal Engine 4

by Krzysztof Lis (Adynathos)

### Features

- in-game displaying of camera video stream
- augmented reality, camera position tracking with ArUco markers
- camera calibration

Augmented reality in UE:

<img src="https://googledrive.com/host/0B4QRvG9KRioDNG9RaGlpaTR4a2s/images/aur_example_01.jpg" width="75%"
	alt="AR example" />

<img src="https://googledrive.com/host/0B4QRvG9KRioDNG9RaGlpaTR4a2s/images/aur_example_02.jpg" width="75%"
	alt="AR example" />

<img src="https://googledrive.com/host/0B4QRvG9KRioDNG9RaGlpaTR4a2s/images/aur_example_on_lcd.jpg" width="75%"
	alt="marker on computer screen" />

Camera calibration:<a name="fig_calibration"></a>

<img src="https://googledrive.com/host/0B4QRvG9KRioDNG9RaGlpaTR4a2s/images/aur_example_calibration.jpg" width="75%"
	alt="calibration" />

In-game video screen:

<img src="https://googledrive.com/host/0B4QRvG9KRioDNG9RaGlpaTR4a2s/images/aur_example_screen.jpg" width="75%"
	alt="in-game video screen" />

### Download

- [Augmented Unreality Project 1.00](https://googledrive.com/host/0B4QRvG9KRioDNG9RaGlpaTR4a2s/AugmentedUnrealityProject_1_00.zip)

### Installation

#### Running the example project:

- download the latest package [here](https://googledrive.com/host/0B4QRvG9KRioDNG9RaGlpaTR4a2s/) 
- decompress the  archive - and move `AugmentedUnrealityProject` to the location where you store your Unreal projects
- launch Unreal Engine and open `AugmentedUnrealityProject/AugmentedUnrealityPr.uproject`
- optional: perform [camera calibration](#section_calibration)

Augmented reality example:

- print or display the tracker marker `AugmentedUnrealityProject/Saved/AugmentedUnreality/Markers/marker.png` (the game needs to be run at least once to generate it)
- launch the game and point camera at the marker

In-game screen example:

- open `AugmentedUnrealityProject/Content/Maps/AURExample_ARScreen.umap`
- launch the game


#### Camera calibration<a name="section_calibration"></a>
Best quality is obtained if the camera is calibrated. It is important to find the camera's field of view to match the camera image and the AR objects.

The driver first tries to load the calibration data from `AugmentedUnrealityProject/Saved + Driver.CalibrationFilePath`, then if it fails, `AugmentedUnrealityProject/Content + Driver.CalibrationFallbackFilePath`. This mechanism should allow distributing a default calibration file in the project's `Content` directory. A default calibration file is supplied with the example project.

To perform calibration of your camera:

- print or display on an additional screen the calibration pattern found in `AugmentedUnrealityProject/Content/AugmentedUnreality/Calibration/calibration_pattern_asymmetric_circles.png`
- open the example project and start the game
- in the menu in the top-right corner of the screen, click `Calibrate` 
- point the camera at the calibration pattern from different directions - pattern is detected if a [colorful overlay is drawn](#fig_picture)
- wait until the progress bar is full
- the calibration data is now saved to file `AugmentedUnrealityProject/Saved/AugmentedUnreality/Calibration/camera.xml` by default, or in general to `AugmentedUnrealityProject/Saved + Driver.CalibrationFilePath` if you changed that property of your driver
- if you use the same driver blueprint next time, this file will be automatically loaded.

#### Camera capture
Camera stream capture is performed using [OpenCV's VideoCapture](http://docs.opencv.org/3.0-beta/modules/videoio/doc/reading_and_writing_video.html).

To choose which camera is used, change the driver blueprint's `CameraIndex` attribute.
The driver attempts to set the camera's resolution to the desired resolution specified in the `Resolution` attribute,
however it is not guaranteed that the camera may choose not to accept this setting.
Generally, lower resolution means faster refresh rate.

#### Tracker markers

This plugin uses [ArUco](http://www.uco.es/investiga/grupos/ava/node/26) boards for camera pose estimation,
specifically the [implementation of ArUco in OpenCV contrib](http://docs.opencv.org/3.1.0/d5/dae/tutorial_aruco_detection.html).

The parameters of the marker used can be changed in the driver blueprint's `TrackerSettings.BoardDefinition` attribute.
Tracker markers are created and saved to file when the application starts.
By default they are saved to `AugmentedUnrealityProject/Saved/AugmentedUnreality/Markers/marker.png` but this location can be changed in the settings.

#### Engine version

The pre-built package was built with UE4.11.2.

#### Presented solutions

The following problems have been solved in this plugin,
if you want to learn about these topics, please see:

- [Including external libraries in UE4](https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AugmentedUnreality.Build.cs)
- Multi-threading in UE4 
[1](https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURDriverThreaded.h)
[2](https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURDriverThreaded.cpp)
[3](https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURDriverOpenCV.h)
[4](https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURDriverOpenCV.h)
- Performing OpenCV camera calibration
[OpenCV tutorial](http://docs.opencv.org/3.1.0/d4/d94/tutorial_camera_calibration.html)
[integration 1](https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AUROpenCVCalibration.h)
[integration 2](https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AUROpenCVCalibration.cpp)
- Drawing on dynamic textures
[UE tutorial](https://wiki.unrealengine.com/Dynamic_Textures) (a bit old)
[our adaptation](https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURVideoScreenBase.cpp)
- [Conversion between OpenCV's and Unreal's coordinate systems](https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURArucoTracker.cpp)
