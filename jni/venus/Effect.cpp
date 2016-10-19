#ifdef _WIN32
#	define _USE_MATH_DEFINES  // for M_PI
#endif
#include <cmath>
#include <cassert>

#include <opencv2/imgproc.hpp>
#if TRACE_IMAGES 
#include <opencv2/highgui.hpp>
#endif

#include "venus/Effect.h"


using namespace cv;

namespace venus {

// bilinear interpolation
template <typename T, int N>
cv::Vec<T, N> interpolate(const Mat& image,
	const int& x0, const int& y0, const int& x1, const int& y1,
	const float& wx, const float& wy)
{
	using Vector = cv::Vec<T, N>;

	// note at<T>(y, x) or at<T>(Point(x, y)) or at<T>(r, c)
	const Vector& c00 = image.at<Vector>(y0, x0);
	const Vector& c01 = image.at<Vector>(y0, x1);
	const Vector& c10 = image.at<Vector>(y1, x0);
	const Vector& c11 = image.at<Vector>(y1, x1);

	Vector color;
	const float l_wx = 1.0f - wx;
	const float l_wy = 1.0f - wy;
	for(int i = 0; i < N; ++i)
	{
		float c0 = c00[i] * wx + c01[i] * l_wx;
		float c1 = c10[i] * wx + c11[i] * l_wx;
		float c = c0 * wy + c1 * l_wy;

		// round off to the nearest for integer types
		if(std::is_integral<T>::value)
			c += T(0.5);

		color[i] = static_cast<T>(c);
	}

	return color;
}

cv::Mat Effect::gaussianBlur(const cv::Mat& image, float radius)
{
	assert(radius >= 0);
	int r = cvRound(radius);
	
	int width = (r << 1) + 1;
	double std_dev = radius * 3;  // 3-sigma rule https://en.wikipedia.org/wiki/68–95–99.7_rule

	cv::Mat result;
	cv::GaussianBlur(image, result, Size(width, width), std_dev);

	return result;
}



} /* namespace venus */