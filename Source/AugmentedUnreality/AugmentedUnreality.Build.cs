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

using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using UnrealBuildTool;

public class AugmentedUnreality : ModuleRules
{
	protected string PluginRootDirectory
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../")); }
	}

	protected string ThirdPartyPath
	{
		get { return Path.Combine(PluginRootDirectory, "ThirdParty/"); }
	}

	protected string BinariesDir
	{
		get { return Path.Combine(PluginRootDirectory, "Binaries"); }
	}

	protected List<string> OpenCVModules = new List<string>()
	{
		"opencv_core",
		"opencv_augmented_unreality", // parts of our plugin that are easier to build as a custom OpenCV module
		"opencv_aruco",		// Aruco markers
		"opencv_calib3d",	// camera calibration
		"opencv_features2d", // cv::SimpleBlobDetector for calibration
		"opencv_flann",     // Aruco needs this
		"opencv_imgcodecs",	// imwrite
		"opencv_imgproc",   // Aruco needs this
		"opencv_video",		// Kalman filter, suprisingly it is in modules/video/...
		"opencv_videoio",	// VideoCapture
	};

	protected List<string> LinuxAdditionalLibs = new List<string>()
	{
		"libippicv.a"
	};

	protected List<string> LinuxStdLibs = new List<string>()
	{
		"libc++.so",
		"libc++abi.so"
	};

	protected string OpenCVVersion = "320";

	public AugmentedUnreality(ReadOnlyTargetRules Target)
		: base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"RHI",
			"RenderCore",
			"SlateCore"
		});

		LoadOpenCV(Target);

		Console.WriteLine("Include headers from directories:");
		PublicIncludePaths.ForEach(m => Console.WriteLine("	" + m));

		Console.WriteLine("Libraries - static:");
		PublicAdditionalLibraries.ForEach(m => Console.WriteLine("	" + m));

		Console.WriteLine("Libraries - dynamic:");
		PublicDelayLoadDLLs.ForEach(m => Console.WriteLine("	" + m));

		RegisterAndroidCameraBridge();
	}

	protected string PlatformString(ReadOnlyTargetRules Target)
	{
		if (Target.Platform == UnrealTargetPlatform.Win64) {return "Win64";}
		if (Target.Platform == UnrealTargetPlatform.Win32) {return "Win32";}
		if (Target.Platform == UnrealTargetPlatform.Linux) {return "Linux";}
		if (Target.Platform == UnrealTargetPlatform.Android) {return "Android";}
		return "Unknown";
	}

	protected string BinariesDirForTarget(ReadOnlyTargetRules Target)
	{
		return Path.Combine(BinariesDir, PlatformString(Target));
	}

	public bool IsDebug(ReadOnlyTargetRules Target)
	{
		return Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT;
	}

	public void LoadOpenCV(ReadOnlyTargetRules Target)
	{
		string opencv_dir = Path.Combine(ThirdPartyPath, "opencv");

		// Include OpenCV headers
		PublicIncludePaths.Add(Path.Combine(opencv_dir, "include"));

		// Libraries are platform-dependent
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			Console.WriteLine("AUR: OpenCV for Win64");

			var suffix = OpenCVVersion;
			if(IsDebug(Target))
			{
				Console.WriteLine("AUR: Debug");
				suffix += "d";
			}
			else
			{
				Console.WriteLine("AUR: Not debug");
			}

			// Add the aur_allocator fix for crashes on windows
			List<string> modules = new List<string>();
			modules.AddRange(OpenCVModules);
			//modules.Add("opencv_aur_allocator");

			// Static linking
			var lib_dir = Path.Combine(opencv_dir, "install", "Win64", "x64", "vc15", "lib");
			PublicAdditionalLibraries.AddRange(
				OpenCVModules.ConvertAll(m => Path.Combine(lib_dir, m + suffix + ".lib"))
			);

			// Dynamic libraries
			// The DLLs need to be in Binaries/Win64 anyway, so let us keep them there instead of ThirdParty/opencv
			PublicDelayLoadDLLs.AddRange(
				OpenCVModules.ConvertAll(m => Path.Combine(BinariesDirForTarget(Target), m + suffix + ".dll"))
			);

			bEnableExceptions = true;
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux )
		{
			Console.WriteLine("AUR: OpenCV for Linux");

			//var lib_dir = Path.Combine(opencv_dir, "install", "Linux", "lib");
			//var opencv_libs = OpenCVModules.ConvertAll(m => Path.Combine(lib_dir, "lib" + m + ".a"));

			var opencv_libs = OpenCVModules.ConvertAll(m => Path.Combine(BinariesDirForTarget(Target), "lib" + m + ".so"));
			PublicAdditionalLibraries.AddRange(opencv_libs);

			PublicAdditionalLibraries.AddRange(
				LinuxStdLibs.ConvertAll(m => Path.Combine(BinariesDirForTarget(Target), m))
			);

			//var lib_dir_other = Path.Combine(opencv_dir, "install", "Linux", "share", "OpenCV", "3rdparty", "lib");
			//var opencv_libs_other = LinuxAdditionalLibs.ConvertAll(m => Path.Combine(lib_dir_other, m));
			//PublicAdditionalLibraries.AddRange(opencv_libs_other);
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			Console.WriteLine("AUR: Android with arch=", Target.Architecture);

			var arch = "armeabi-v7a"; //Target.Architecture

			var src_dir = Path.Combine(opencv_dir, "install", "Android", "sdk", "native");

			var modules_lib_dir = Path.Combine(src_dir, "libs", arch);
			var opencv_libs = OpenCVModules.ConvertAll(
				m => Path.Combine(modules_lib_dir, "lib" + m + ".a")
			);
			PublicLibraryPaths.Add(modules_lib_dir);
			PublicAdditionalLibraries.AddRange(opencv_libs);

			var thirdparty_lib_dir = Path.Combine(src_dir, "3rdparty", "libs", arch);
			var thirdparty_libs = new List<string>(Directory.GetFiles(thirdparty_lib_dir)).ConvertAll(
				fn => Path.Combine(thirdparty_lib_dir, fn)
			);
			PublicLibraryPaths.Add(thirdparty_lib_dir);
			PublicAdditionalLibraries.AddRange(thirdparty_libs);

			bEnableExceptions = true;
		}
		else
		{
			Console.WriteLine("AUR: No prebuilt binaries for OpenCV on platform "+Target.Platform);
		}
	}

	public void RegisterAndroidCameraBridge()
	{
		var android_mod_file = Path.Combine(ModuleDirectory, "AugmentedUnrealityAndroid_UPL.xml");
		Console.WriteLine("Android modification: " + android_mod_file);
		AdditionalPropertiesForReceipt.Add(
			new ReceiptProperty("AndroidPlugin", android_mod_file)
		);
	}
}
