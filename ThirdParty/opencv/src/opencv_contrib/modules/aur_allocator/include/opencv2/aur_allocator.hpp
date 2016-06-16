#ifndef __OPENCV_AUR_ALLOCATOR_HPP__
#define __OPENCV_AUR_ALLOCATOR_HPP__

#include <opencv2/core.hpp>
#include <opencv2/aruco.hpp>
#include <vector>

namespace cv
{
namespace aur_allocator
{

typedef std::vector<cv::Point2f> VectorOfPoint2f;
typedef std::vector<cv::Point3f> VectorOfPoint3f;

CV_EXPORTS std::vector<VectorOfPoint2f>* allocate_vector2_Point2f();
CV_EXPORTS std::vector<VectorOfPoint3f>* allocate_vector2_Point3f();
CV_EXPORTS std::vector<int>* allocate_vector_int();
CV_EXPORTS cv::Mat* allocate_mat();
CV_EXPORTS std::vector<cv::Mat>* allocate_vector_mat();
CV_EXPORTS cv::aruco::Board* allocate_aruco_board();

CV_EXPORTS void external_delete(void* object);

}
}

#endif
