set(the_description "Visual odometry for augmented reality")
set(CMAKE_CXX_STANDARD 11)
ocv_define_module(augmented_unreality opencv_core opencv_calib3d opencv_video opencv_aruco WRAP python)
