
#include "opencv2/aur_allocator.hpp"

namespace cv 
{
namespace aur_allocator 
{

std::vector<VectorOfPoint2f>* allocate_vector2_Point2f()
{
	return new std::vector<VectorOfPoint2f>;
}

std::vector<VectorOfPoint3f>* allocate_vector2_Point3f()
{
	return new std::vector<VectorOfPoint3f>;
}

std::vector<int>* allocate_vector_int()
{
	return new std::vector<int>;
}

cv::Mat* allocate_mat()
{
	return new cv::Mat;
}

std::vector<cv::Mat>* allocate_vector_mat()
{
	return new std::vector<cv::Mat>;
}

CV_EXPORTS cv::aruco::Board * allocate_aruco_board()
{
	return new cv::aruco::Board;
}

CV_EXPORTS void external_delete(void * object)
{
	if (object)
	{
		delete object;
	}
}

}
}