/*
Copyright 2016 Krzysztof Lis

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once

/*
	In Unreal applications, normal stdout/stderr does not print in console,
	but instead UE_LOG is used to transmit log messages.

	Since we cannot use UE_LOG in this module, we will ask for a logging callback.
*/
#include <opencv2/core.hpp> // for CV_EXPORTS
#include <functional>

namespace cv {
namespace aur {

enum class LogLevel {
	Log = 0,
	Warning = 1,
	Error = 2
};

enum class DiagnosticLevel {
	Silent = 0,
	Basic = 1,
	Full = 2
};

using LogCallback = std::function<void(LogLevel, std::string)>;

void log(LogLevel const level, std::string const& message);
CV_EXPORTS void setLogCallback(LogCallback callback);

} // namepsace
}

