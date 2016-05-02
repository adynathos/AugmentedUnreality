## OpenCVWrapper

Using the OpenCV functions which write their output to ``vector<vector<something>>``
causes crashes in UE4 because of some custom delete mechanism of the engine.

It has been reported however that performing those operations in a separate shared library
prevents the crashes, as the UE4's delete is not used.
https://answers.unrealengine.com/questions/36777/crash-with-opencvfindcontour.html

This shared library is a wrapper around Aruco marker detection mechanism.
