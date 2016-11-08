<h1>Augmented Unreality</h1>
<p>
Augmented Unreality is a plugin for <a href="https://www.unrealengine.com">Unreal Engine 4</a>
which enables creation of augmented reality applications by displaying a video stream from a camera 
inside the game and tracking camera position using fiducial markers.
</p>
<p>
It was created by <b>Krzysztof Lis (Adynathos)</b> as part of a project for <b>ETH Zürich</b>.
</p>

<h2>Features</h2>
<p>
<ul>
	<li>Video stream displayed in-game</li>
	<li>Multiple video sources: cameras, video files, network streams. Source can be switched using in-game UI</li>
	<li>Camera position tracked using fiducial markers, multiple independent sets of markers can be tracked at once</li>
	<li>Editable spatial configurations of markers </li>
	<li>Camera calibration</li>
</ul>
</p>

<figure>
<a href="https://www.youtube.com/watch?v=OYQLKUhKeTk">
	<!--<img src="images/youtube.jpg" width="600"/>-->
	<img src="https://polybox.ethz.ch/index.php/s/UqF2pp4m4l6iahV/download" width="600"/>
</a>
</figure>

<h2 name="downloads">Downloads</h2>
<p>
<ul>
	<li><a name="downloads_project" href="https://polybox.ethz.ch/index.php/s/ucl1amKJ7CnLtYn/download">Augmented Unreality Plugin 1.1.0</a> (UE 4.13.1)
		- the plugin files only, 
	</li>
	<li><a name="downloads_plugin" href="https://polybox.ethz.ch/index.php/s/nzkm3GrZas5Zp3X/download">Augmented Unreality Example Project 1.1.0</a> (UE 4.13.1) 
		- an example project using the plugin
	</li> 
</ul>
</p>

<h2 name="install">Installation</h2>

<h3>Getting started - try the example project</h2>
<p>
<ul>
<li><a href="#downloads">Download the example project</a></li>
<li>Decompress the archive - and move <tt>AugmentedUnrealityPr</tt> to the location where you store your Unreal projects.</li>
<li>Launch Unreal Engine and open <tt>AugmentedUnrealityPr/AugmentedUnrealityPr.uproject</tt>.</li>
<li>Navigate to <tt>AugmentedUnrealityPr/Saved/AugmentedUnreality/Markers</tt> and print the board images:
	<tt>AURBoard_SquareA_C_0/AURBoard_SquareA_C_0.png</tt>, <tt>AURBoard_SquareB_C_0/AURBoard_SquareB_C_0.png</tt>.
</li>
<li>Connect a camera and launch the game.</li>
<li>If the virtual object are not well aligned with the markers, perform <a href="#section_calibration">camera calibration</a>.</li>
</ul>
</p>

<figure>
<figcaption> Overview of elements visible in the example project: </figcaption>
	<!--<img src="images/aur_overview.jpg" width="800" />-->
	<img src="https://polybox.ethz.ch/index.php/s/Kc4hJHLdvjARvnJ/download" width="800" />
</figure>

<h3>Add plugin to an existing project:</h3>
<p>
<ul>
<li><a href="#downloads">Download the plugin</a></li>
<li>Decompress the archive - and move directory <tt>AugmentedUnreality</tt> to <tt>YourProject/Plugins</tt></li>
<li>Reopen your project</li>
</ul>
</p>

<h2 name="video">Camera / Video</h2>
<p>
Video acquisition is achieved using OpenCV's <a href="http://docs.opencv.org/3.1.0/d8/dfe/classcv_1_1VideoCapture.html">VideoCapture</a>. 
</p>
<p>
Video capture and processing is performed by the <tt>AURDriver</tt> object, like <tt>ExampleDriver</tt> class in the example project.
You can adjust the video settings by creating a child blueprint of AURDriver_Default and editing its properties.
Once you have your AURDriver blueprint, use it as the value for your PlayerController's <tt>CameraDriverClass</tt>
(if you inherit the PlayerController from example project) or pass it as <tt>DriverClass</tt> when spawning an <tt>AURCameraActor</tt>.
</p>
<p>
The key property of <tt>AURDriver</tt> is <tt>DefaultVideoSources</tt> - a list of video source classes that will be 
automatically created and available to switch through the UI.
</p>

<h3 name="video_sources">Video sources</h3>
<p>
The plugin can obtain video from various sources.
To use video from a given source, create a blueprint for it and add it to your <tt>AURDriver</tt>'s <tt>DefaultVideoSources</tt>.
Your video source blueprint should extend one of these superclasses:
</p>
<p>
<ul>
	<li><tt>AURVideoSourceCamera</tt> - video from a camera directly connected to the computer. Properties:
		<ul>
			<li><tt>CameraIndex</tt> - 0-based number of the camera. If you have only one camera, the index should be 0.</li>
			<li><tt>DesiredResolution</tt> - the driver will attempt to set the camera's resolution to the desired resolution specified in this attribute,
				however it is not guaranteed that the camera accepts this resolution.
				Generally, lower resolution means lower quality and accuracy but higher refresh rate.
			</li>
		</ul>
	</li>
	<li><tt>AURVideoVideoFile</tt> - video from a file. The <tt>VideoFile</tt> should be the path to the file
		relative to <tt>FPaths::GameDir()</tt>.
	</li>
	<li><tt>AURVideoSourceStream</tt> - video streamed through network. Set only one of the following:
		<ul>
			<li><tt>ConnectionString</tt> - a <a href="http://www.z25.org/static/_rd_/videostreaming_intro_plab/">GStreamer pipeline</a> ending with appsink.</li>
			<li><tt>StreamFile</tt> - path to a </tt>.sdp</tt> file relative to <tt>FPaths::GameDir()</tt>.</li>
		</ul>
	</li>
</ul>
</p>
<p>
Shared properties of all <tt>VideoStream</tt>s:
<ul>
	<li><tt>SourceName</tt> - name to be displayed in the graphical list of video sources</li>
	<li><tt>CalibrationFileName</tt> - location of the file storing calibration for this video source, 
		relative to <tt>FPaths::GameSavedDir()/AugmentedUnreality/Calibration</tt>.
		If two sources use the same camera, they should have the same calibration file.
	</li>
</ul>
</p>

<h3 name="calibration">Camera calibration</h3>
<p>
Best quality is obtained if the camera is calibrated. 
It is important to find the camera's <b>field of view</b>.
If the camera's field of view differs from the rendering engine's field of view,
the virtual objects will not be properly aligned to the real world.
If you notice that the virtual objects move in real world when you move the camera, it means the camera is not correctly calibrated
</p>
<p>
Each <tt>VideoSource</tt> can have different camera parameters, therefore each has its own calibration file
located at located in <tt>FPaths::GameSavedDir()/AugmentedUnreality/VideoSource.CalibrationFilePath</tt>.
The driver will attempt to load this file and display the information whether the camera is calibrated in the UI.
</p>

<p>
To perform calibration of your camera:
<ul>
<li>Print or display on an additional screen the calibration pattern found in <tt>AugmentedUnreality/Content/Calibration/calibration_pattern_asymmetric_circles.png</tt></li>
<li>Open the example project and start the game</li>
<li>In the menu in the top-right corner of the screen, choose the right video source and click <tt>Calibrate</tt></li>
<li>Point the camera at the calibration pattern from different directions - pattern is detected if a <a href="#fig_calibration">colorful overlay is drawn</a></li>
<li>Wait until the progress bar is full</li>
<li>The camera properties are now saved to the calibraiton file and will be loaded whenever you use this video source again</li>
</ul>
</p>

<figure name="fig_calibration">
	<figcaption>Camera calibration in progress - the colorful overlay indicates the calibration pattern was detected:<figcaption>
	<!--<img src="images/aur_calibration.jpg" width="800" />-->
	<img src="https://polybox.ethz.ch/index.php/s/YZ2wQpd7tnGBrgh/download" width="800" />
</figure>

<h2 name="tracking">Tracking</h2>
<p>
This plugin uses <a href="http://www.uco.es/investiga/grupos/ava/node/26">ArUco</a> boards for camera pose estimation,
specifically the <a href="http://docs.opencv.org/3.1.0/d5/dae/tutorial_aruco_detection.html">implementation of ArUco in OpenCV contrib</a>.
</p>
<p>
Boards are used for two purposes:
<ul>
	<li>Positioning the camera in game world - this aligns the real and virtual world.
		The board's position in real world is equivalent to the point (0, 0, 0) in game world.
		Boards used for camera positioning are set in the <tt>PlayerController</tt>'s <tt>MarkerBoardDefinitions</tt>
		property (if you are extending the example player controller)
		or in <tt>AURCameraActor</tt>'s <tt>BoardDefinitions</tt> if you are spawning the camera actor directly.
	</li>
	<li>
		Positioning independent actors - to bind an actor's pose to an AR board,
		add an <tt>AURTrackingComponent</tt> to the actor and select the <tt>ChildActorClass</tt> to one of the board blueprints
	</li>
</ul>
</p>

<h3 name="boards">Boards</h3>
<figure>
	<!--<img src="images/board_arena.jpg" width="600" />-->
	<img src="https://polybox.ethz.ch/index.php/s/H0GGOXE5XZNL9Ci/download" width="600" />
</figure>

<p>
An ArUco board is a set of square markers, together with their positions and orientations in space.
When a board is visible in the video, its pose relative to the camera can be calculated.
In Augmented Unreality, we use boards for finding the pose of the camera in game world and for positioning independent actors with their own markers.
</p>
<p>
Augmented Unreality allows the user to create their own custom spatial configurations of markers in Unreal Editor.
Please see the example boards in <tt>AugmentedUnreality/Content/Markers</tt> and <tt>AugmentedUnrealityPr/Content/AugmentedUnrealityExample/Markers</tt>.
<p>
<p>
To design a new board, create a child blueprint of <tt>AURBoardDefinition</tt> and edit it by adding <tt>AURMarkerComponents</tt> inside it.
Each <tt>AURMarkerComponent</tt> represents one square on the board.
<ul>
	<li><tt>Location</tt>, <tt>Rotation</tt> - pose of square in space. You can use <tt>SceneCompoenent</tt>s to organize the board hierarchically.</li>
	<li><tt>Id</tt> - identifier of the pattern shown in this square. Each square should have a different Id.</li>
	<li><tt>BoardSizeCm</tt> - length of the square's size. This will automatically set the scale.
		When printing the boards, please ensure the squares match this size.</li>
	<li><tt>MarginCm</tt> - margin inside the square, does not affect the total size.</li>
</ul>
If you want to use the board to position the (0, 0, 0) point, add to to <tt>MarkerBoardDefinitions</tt> in <tt>PlayerController</tt>.
If you want an actor to follow the position of the board, add an <tt>AURTrackingComponent</tt> to the actor and select the <tt>ChildActorClass</tt> to the board blueprint.
</p>
<p>
After you create or edit the board blueprint, launch the game to generate the marker images.
Then open the directory <tt>YourProject/Saved/AugmentedUnreality/Markers/YourBoardName</tt>, print the images,
and arrange them in space to match your designed configuration.
The IDs of the markers in the editor need to match the numbers present in the images:
</p>

<figure>
	<!--<img src="images/marker.png" width="400" />-->
	<img src="https://polybox.ethz.ch/index.php/s/nBO36jy0kzjSsRP/download" width="400" />
</figure>

<figure>
	<!--<img src="images/board_tower.jpg" width="600" />-->
	<img src="https://polybox.ethz.ch/index.php/s/31CTBjbqRJYoiqT/download" width="600" />
</figure>

<h3 name="platforms">Platforms</h3>
<ul>
	<li><i>Windows</i> - fully functional, pre-built packages available.</li>
	<li><i>Linux</i> - the plugin compiles and editor can open the project, but when the game is launched a crash occurs,
		which is potentially related to differences in memory management between Unreal Engine and OpenCV.
		I would be grateful for help from someone experience with memory management across shared libraries.
	</li>
	<li><i>Android</i> - there is an Android version of OpenCV so it should be possible to port the plugin to that platform.
		This is outside the scope of my project but I am willing to help if someone wants to try this.
	</li>
</ul>

<h3 name="solutions">Education</h3>
<p>
The following problems have been solved in this plugin,
if you want to learn about these topics, please see:
<ul>
<li>
<a href="https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AugmentedUnreality.Build.cs">Including external libraries in UE4</a>
</li>
<li>Multi-threading in UE4 
<a href="https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURDriverThreaded.h">(1)</a>
<a href="https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURDriverThreaded.cpp">(2)</a>
<a href="https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURDriverOpenCV.h">(3)</a>
<a href="https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURDriverOpenCV.h">(4)</a>
</li>
<li>Performing OpenCV camera calibration
<a href="http://docs.opencv.org/3.1.0/d4/d94/tutorial_camera_calibration.html">OpenCV tutorial</a>, 
<a href="https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AUROpenCVCalibration.cpp">adaptation for the plugin</a>
</li>
<li> Drawing on dynamic textures
	<a href="https://wiki.unrealengine.com/Dynamic_Textures">UE tutorial</a> (a bit old)
	<a href="https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/AURVideoScreenBase.cpp">my adaptation</a>
</li>
<li>
<a href="https://github.com/adynathos/AugmentedUnreality/blob/master/Source/AugmentedUnreality/tracking/AURArucoTracker.cpp">Conversion between OpenCV's and Unreal's coordinate systems</a>
</li>
</ul>
</p>
