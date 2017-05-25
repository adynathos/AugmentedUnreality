
<h2>OpenCV - building instructions</h2>

<h4>General procedure:</h4>

- Download <a href="https://github.com/opencv/opencv/releases/tag/3.1.0">OpenCV 3.1.0</a> source and decompress it to <tt>./src/opencv</tt>
- Download <a href="https://github.com/opencv/opencv_contrib/releases/tag/3.1.0">OpenCV Contrib 3.1.0</a> source and decompress it to <tt>./src/opencv_contrib</tt>,
but do not delete the <tt>./src/opencv_contrib/modules/augmented_unreality</tt> directory that is already there
- Create a directory for OpenCV build for this platform (anywhere, preferably not inside your UE project).
- Run: ``python build.py build PLATFORM --build_dir MY_BUILD_DIR``
- Follow the instruction printed by the program to build the library, it will be to run <tt>make</tt> on Linux and <tt>Visual Studio</tt> on Windows
- Run: ``python build.py copy PLATFORM``

<h4>Android</h4>

Install <b>Android NDK</b> bBefore building:
- Install AndroidNDK from https://developer.android.com/ndk/downloads/index.html
- Set environment variable `ANDROID_NDK="path/to/android-ndk"`
- Follow the procedure shown above with <tt>python build.py build android ...</tt>

Recommended values for UE Project Settings:
- Min SDK version = 19
- Target SDK version = 19
because that is the API level that our build of OpenCV is targetting.

<a href="https://docs.unrealengine.com/latest/INT/Platforms/Android/GettingStarted">Running UE project on Android</a>

<h4>Linux</h4>

If you encounter the error:
`/usr/include/c++/v1/string:1938:44: error: 'basic_string<_CharT, _Traits, _Allocator>' is missing exception specification`
apply this <a href="http://stackoverflow.com/questions/37096062/get-a-basic-c-program-to-compile-using-clang-on-ubuntu-16">solution</a>:
add the following code after the line <tt>1938</tt> of <tt>/usr/include/c++/v1/string<tt>:

<pre>#if _LIBCPP_STD_VER <= 14
	_NOEXCEPT_(is_nothrow_copy_constructible<allocator_type>::value)
#else
	_NOEXCEPT
#endif</pre>


