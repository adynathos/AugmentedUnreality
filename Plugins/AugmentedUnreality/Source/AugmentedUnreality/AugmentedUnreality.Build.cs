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
using UnrealBuildTool;

public class AugmentedUnreality : ModuleRules
{
	private string ModulePath
	{
		get { return Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name)); }
	}

	private string ThirdPartyPath
	{
		get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
	}

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

		Console.WriteLine("nm: " + this.GetType().Name);
		Console.WriteLine("mp: " + ModulePath);
		Console.WriteLine("tp: " + ThirdPartyPath);

		LoadOpenCV(Target);
	}

	public void LoadOpenCV(TargetInfo Target)
	{
		string opencv_build_path = Path.Combine(ThirdPartyPath, "opencv", "build");

		// Include path
		string include_path = Path.Combine(opencv_build_path, "include");
		PublicIncludePaths.Add(Path.Combine(include_path));

		// Link
		string lib_path = Path.Combine(opencv_build_path, "lib"); //Path.Combine(opencv_build_path, "x64", "vc14");
		PublicAdditionalLibraries.Add(Path.Combine(lib_path, "opencv_world310.lib"));
		PublicAdditionalLibraries.Add(Path.Combine(lib_path, "opencv_aruco310.lib"));

		string bin_path = Path.Combine(opencv_build_path, "bin");
		PublicDelayLoadDLLs.Add(Path.Combine(bin_path, "bin", "opencv_world310.dll"));
		PublicDelayLoadDLLs.Add(Path.Combine(bin_path, "bin", "opencv_aruco310.dll"));

		//Definitions.Add("WITH_OPENCV_BINDING=1");
	}
}
