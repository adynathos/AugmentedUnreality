cmake_minimum_required(VERSION 2.8)

project (OpenCVWrapperProject)
#set(CMAKE_BUILD_TYPE Release)

set(PLATFORM "Win64" CACHE STRING "Win64 or Linux")
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})

set(OPENCV_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../opencv/include" CACHE PATH "OpenCV include dir")
set(OPENCV_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../opencv/lib/${PLATFORM}" CACHE PATH "OpenCV .lib dir")

# Our library
add_library(OpenCVWrapper SHARED src/OpenCVWrapper.cpp src/OpenCVWrapper.h)

include(GenerateExportHeader)
GENERATE_EXPORT_HEADER(OpenCVWrapper
	BASE_NAME OpenCVWrapper
	EXPORT_MACRO_NAME OpenCVWrapper_EXPORT
	EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/OpenCVWrapper_Export.h
)
include_directories(${CMAKE_BINARY_DIR}/exports)

install(FILES
	src/OpenCVWrapper.h
	${PROJECT_BINARY_DIR}/exports/OpenCVWrapper_Export.h 
DESTINATION include
)
install(TARGETS OpenCVWrapper DESTINATION lib/${PLATFORM})

# OpenCV

set(OPENCV_MODULES
	"opencv_core"
	"opencv_highgui"
	"opencv_aruco"
	"opencv_imgcodecs"
)

foreach(module ${OPENCV_MODULES}) 
	set(lib_path ${OPENCV_LIB_DIR}/${module}310d.lib)
	set(OPENCV_LIBS ${OPENCV_LIBS} ${lib_path})
	MESSAGE(STATUS ${lib_path})
endforeach()

include_directories(${OPENCV_INCLUDE_DIR})
target_link_libraries(OpenCVWrapper "${OPENCV_LIBS}")
