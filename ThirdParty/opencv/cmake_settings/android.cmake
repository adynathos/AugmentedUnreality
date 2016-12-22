
include(${CMAKE_CURRENT_LIST_DIR}/base.cmake)

set(CMAKE_TOOLCHAIN_FILE "../../src/opencv/platforms/android/android.toolchain.cmake" CACHE PATH "")
set(ANDROID_NATIVE_API_LEVEL "19" CACHE STRING "")
set(BUILD_SHARED_LIBS "FALSE" CACHE BOOL "")

# On Android, PNG conflicts with UE's version of PNG
# (probably due to static linking)
# UE checks if it runs with the exact version of PNG it wants, so crashes if another is supplied
set(WITH_PNG "FALSE" CACHE BOOL "")
set(BUILD_PNG "FALSE" CACHE BOOL "")

set(BUILD_ANDROID_EXAMPLES "FALSE" CACHE BOOL "")
set(BUILD_JASPER "FALSE" CACHE BOOL "")
