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

void Effect::tone(cv::Mat& dst, const cv::Mat& src, uint32_t color, float amount)
{
	assert(src.type() == CV_8UC4 || src.type() == CV_32FC4);
	dst.create(src.size(), src.type());

	const float l_amount = 1.0F - amount;
	if(src.type() == CV_32FC4)
		amount /= 255;

	uint8_t r = color, g = color >> 8, b = color >> 16;
#if USE_OPENCV_BGRA_LAYOUT
	const Vec3f target(b * amount, g * amount, r * amount);
#else
	const Vec3f target(r * amount, g * amount, b * amount);
#endif
	
	switch(src.type())
	{
	case CV_8UC4:
		for(int r = 0; r < src.rows; ++r)
		for(int c = 0; c < src.cols; ++c)
		{
			const Vec4b& src_color = src.at<Vec4b>(r, c);
			Vec4b& dst_color = dst.at<Vec4b>(r, c);
			for(int i = 0; i < 3; ++i)  // loop unrolling
				dst_color[i] = src_color[i] * l_amount + target[i];
		}
		break;
	case CV_32FC4:
		for(int r = 0; r < src.rows; ++r)
		for(int c = 0; c < src.cols; ++c)
		{
			const Vec4f& src_color = src.at<Vec4f>(r, c);
			Vec4f& dst_color = dst.at<Vec4f>(r, c);
			for(int i = 0; i < 3; ++i)  // loop unrolling
				dst_color[i] = src_color[i] * l_amount + target[i];
		}
		break;
	default:
		assert(false);
		break;
	}
}

void Effect::gaussianBlur(cv::Mat& dst, const cv::Mat& src, float radius)
{
	assert(radius >= 0);
	int r = cvRound(radius);
	
	int width = (r << 1) + 1;
	double std_dev = radius * 3;  // 3-sigma rule https://en.wikipedia.org/wiki/68–95–99.7_rule

	cv::Mat result;
	cv::GaussianBlur(src, dst, Size(width, width), std_dev);
}



} /* namespace venus */