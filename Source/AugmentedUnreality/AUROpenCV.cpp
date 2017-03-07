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
#include "AUROpenCV.h"

#include <cstdlib>
#if PLATFORM_WINDOWS
// windows has renamed the setenv func to _putenv_s
int setenv(const char *name, const char *value, int overwrite)
{
	if (!overwrite) 
	{
		// see if env var already exists
		size_t envsize = 0;
		int ret = getenv_s(&envsize, NULL, 0, name);
		// if exists, don't overwrite
		if (ret || envsize)
		{
			return ret;
		}
	}

	// set env var
	return _putenv_s(name, value);
}
#endif

FString FindGstreamerPluginDir()
{
	FString platform =
#if PLATFORM_WINDOWS
		"Win64"
#elif PLATFORM_LINUX
		"Linux"
#else
		"unknown"
#endif
		;

	//return "C:\\Program Files (x86)\\gstreamer\\1.0\\x86_64\\lib\\gstreamer-1.0";

	FString gst_subdir = "Binaries" / platform / "gstreamer_plugins";

	FString main_gst_dir = FPaths::ConvertRelativePathToFull(FPaths::GameDir() / gst_subdir);
	FString plugin_gst_dir = FPaths::ConvertRelativePathToFull(FPaths::GamePluginsDir() / "AugmentedUnreality" / gst_subdir);

	if (FPaths::DirectoryExists(main_gst_dir))
	{
		return main_gst_dir;
	}
	else
	{
		UE_LOG(LogAUR, Log, TEXT("GStreamer plugin dir not here: %s"), *main_gst_dir)
	}
	
	if (FPaths::DirectoryExists(plugin_gst_dir))
	{
		return plugin_gst_dir;
	}
	else
	{
		UE_LOG(LogAUR, Log, TEXT("GStreamer plugin dir not here: %s"), *plugin_gst_dir)
	}

	return "";
}

void FAUROpenCV::SetGstreamerPluginEnv()
{
	const FString gst_plugin_dir = FindGstreamerPluginDir();
	
	if (!gst_plugin_dir.IsEmpty())
	{
		UE_LOG(LogAUR, Log, TEXT("GStreamer plugin dir: %s"), *gst_plugin_dir)
		setenv("GST_PLUGIN_PATH", TCHAR_TO_UTF8(*gst_plugin_dir), 0);
	}
}
