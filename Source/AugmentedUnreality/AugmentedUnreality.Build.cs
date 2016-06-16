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

using System;
using System.IO;
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
		"opencv_aur_allocator",
		"opencv_calib3d",	// camera calibration
		"opencv_features2d", // cv::SimpleBlobDetector for calibration
		"opencv_videoio",	// VideoCapture
		"opencv_aruco",		// Aruco markers
		"opencv_imgproc",   // Aruco needs this
		"opencv_flann",     // Aruco needs this
		"opencv_imgcodecs",	// imwrite
		"opencv_video"		// Kalman filter, suprisingly it is in modules/video/...
	};

	protected string OpenCVVersion = "310";

	public AugmentedUnreality(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"RHI",
			"RenderCore"
		});

		LoadOpenCV(Target);

		Console.WriteLine("Include headers from directories:");
		PublicIncludePaths.ForEach(m => Console.WriteLine("	" + m));

		Console.WriteLine("Libraries - static:");
		PublicAdditionalLibraries.ForEach(m => Console.WriteLine("	" + m));

		Console.WriteLine("Libraries - dynamic:");
		PublicDelayLoadDLLs.ForEach(m => Console.WriteLine("	" + m));
	}

	protected string PlatformString(TargetInfo Target)
	{
		if (Target.Platform == UnrealTargetPlatform.Win64) {return "Win64";}
		if (Target.Platform == UnrealTargetPlatform.Win32) {return "Win32";}
		if (Target.Platform == UnrealTargetPlatform.Linux) {return "Linux";}
		return "Unknown";
	}

	protected string BinariesDirForTarget(TargetInfo Target)
	{
		return Path.Combine(BinariesDir, PlatformString(Target));
	}

	public bool IsDebug(TargetInfo Target)
	{
		return Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT;
	}

	public void LoadOpenCV(TargetInfo Target)
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

			// Static linking
			var lib_dir = Path.Combine(opencv_dir, "lib", "Win64");
			PublicAdditionalLibraries.AddRange(
				OpenCVModules.ConvertAll(m => Path.Combine(lib_dir, m + suffix + ".lib"))
			);

			// Dynamic libraries
			// The DLLs need to be in Binaries/Win64 anyway, so let us keep them there instead of ThirdParty/opencv
			PublicDelayLoadDLLs.AddRange(
				OpenCVModules.ConvertAll(m => Path.Combine(BinariesDirForTarget(Target), m + suffix + ".dll"))
			);
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux )
		{
			Console.WriteLine("AUR: OpenCV for Linux");

			var opencv_libs = OpenCVModules.ConvertAll(m => Path.Combine(BinariesDirForTarget(Target), "lib" + m + ".so"));

			PublicAdditionalLibraries.AddRange(opencv_libs);
			PublicDelayLoadDLLs.AddRange(opencv_libs);
		}
		else
		{
			Console.WriteLine("AUR: No prebuilt binaries for OpenCV on platform "+Target.Platform);
		}

		// Force execption handling across all modules.
		UEBuildConfiguration.bForceEnableExceptions = true;
	}
}
