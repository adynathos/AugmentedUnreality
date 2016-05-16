cmake_minimum_required(VERSION 2.8)

project (OpenCVWrapperProject)
#set(CMAKE_BUILD_TYPE Release)

if(WIN32)
	set(PLATFORM "Win64" CACHE STRING "Win64 or Linux")
endif(WIN32)

if(UNIX)
	set(PLATFORM "Linux" CACHE STRING "Win64 or Linux")
endif(UNIX)

set(OPENCV_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../opencv/include" CACHE PATH "OpenCV include dir")
set(OPENCV_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../opencv/lib/${PLATFORM}" CACHE PATH "OpenCV .lib dir")
set(PLUGIN_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Binaries" CACHE PATH "yourplugin/Binaries")

# Find OpenCV
include_directories(${OPENCV_INCLUDE_DIR})

set(OPENCV_MODULES
	"opencv_core"
	"opencv_highgui"
	"opencv_aruco"
	"opencv_imgcodecs"
)

if(WIN32)
	foreach(module ${OPENCV_MODULES})
		set(lib_path ${OPENCV_LIB_DIR}/${module}310.lib)
		set(OPENCV_LIBS ${OPENCV_LIBS} ${lib_path})
		MESSAGE(STATUS ${lib_path})
	endforeach()
endif(WIN32)
if(UNIX)
	set(OPENCV_LIBS ${OPENCV_MODULES})
	link_directories(${PLUGIN_BINARY_DIR}/${PLATFORM})
endif(UNIX)

# Build our library
add_definitions(-std=c++11)
add_library(OpenCVWrapper SHARED src/OpenCVWrapper.cpp src/OpenCVWrapper.h)
target_link_libraries(OpenCVWrapper ${OPENCV_LIBS})

# Export DLL if on windows
if(WIN32)
	include(GenerateExportHeader)
	GENERATE_EXPORT_HEADER(OpenCVWrapper
		BASE_NAME OpenCVWrapper
		EXPORT_MACRO_NAME OpenCVWrapper_EXPORT
		EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/OpenCVWrapper_Export.h
	)
	include_directories(${CMAKE_BINARY_DIR}/exports)
endif(WIN32)

# Copy our library files
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})
install(FILES src/OpenCVWrapper.h DESTINATION include)
if(WIN32)
	install(FILES ${PROJECT_BINARY_DIR}/exports/OpenCVWrapper_Export.h DESTINATION include)
endif(WIN32)

install(TARGETS OpenCVWrapper DESTINATION lib/${PLATFORM})
install(TARGETS OpenCVWrapper DESTINATION ${PLUGIN_BINARY_DIR}/${PLATFORM})
