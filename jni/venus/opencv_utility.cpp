#include "venus/opencv_utility.h"

#include <opencv2/imgproc.hpp>

using namespace cv;

namespace venus {

// for int version use cv::boundingRect(points);
Rect2f boundingRect(const std::vector<Point2f>& points)
{
	assert(points.size() >= 3);
#if 1
	const Point2f& p0 = points[0];
	float left = p0.x, right = p0.x, top = p0.y, bottom = p0.y;
	for(const Point2f& point: points)
	{
		if(point.x < left)
			left = point.x;
		else if(point.x > right)
			right = point.x;

		if(point.y < top)
			top = point.y;
		else if(point.y > bottom)
			bottom = point.y;
	}
 
#else
	assert(points.size() == 81);  // from 0 to 80
	float left = std::min(points[0].x, points[1].x);
	float right = std::max(points[11].x, points[12].x);
	float top = points[16].y + 1;
	float bottom = points[6].y + 1;

#  ifndef NDEBUG
	for(const Point2f& point: points)
	{
		assert(left <= point.x && point.x < right);
		assert(top <= point.y && point.y < bottom);
	}
#  endif  // NDEBUG
#endif

	return Rect2f(left, top, right - left, bottom - top);
}

float distance(const cv::Point2f& pt0, const cv::Point2f& pt1)
{
//	return std::hypot(pt0.x - pt1.x, pt0.y - pt1.y);
	float dx = pt0.x - pt1.x;
	float dy = pt0.y - pt1.y;
	return std::sqrt(dx*dx + dy*dy);
}

void inset(cv::Rect& rect, int width)
{
	assert(width >= 0 || rect.width > -width);

	int width2 = width<<1;
	rect.x -= width;
	rect.y -= width;
	rect.width += width2;
	rect.height += width2;
}

cv::Mat merge(const cv::Mat& rgb, const cv::Mat& alpha)
{
	assert(rgb.depth() == alpha.depth());
	assert(rgb.channels() == 3 && alpha.channels() == 1);

#if 1
	cv::Mat r_g_b_a[3];
	cv::split(rgb, r_g_b_a);
	r_g_b_a[3] = alpha;

	cv::Mat result;
	cv::merge(r_g_b_a, 4, result);
#else
	// TODO maybe we can direct merge RGB and A???
	cv::Mat result(rgb.size(), CV_MAKETYPE(rgb.depth(), 4));
	for()
	rgb.elemSize();
#endif

	return result;
}

void line(Mat& image, const Point2f& pt0, const Point2f& pt1, const Scalar& color,
		int thickness/* = 1*/, int lineType/* = cv::LINE_8*/, int shift/* = 0*/)
{
	assert(pt0.x != pt1.x || pt0.y != pt1.y);  // two points coincide!
	Point2f delta = pt1 - pt0;
	Point2f p0(0, 0), p1(static_cast<float>(image.cols - 1), static_cast<float>(image.rows - 1));
	if(std::abs(delta.x) > std::abs(delta.y))
	{
		float k = delta.y / delta.x;
		Point2f &left = p0, &right = p1;
		left.y = k * (left.x - pt0.x) + pt0.y;
		right.y = k * (right.x - pt0.x) + pt0.y;
	}
	else
	{
		float reciprocal_k = delta.x / delta.y;
		Point2f &top = p0, &bottom = p1;
		top.x = reciprocal_k * (top.y - pt0.y) + pt0.x;
		bottom.x = reciprocal_k * (bottom.y - pt0.y) + pt0.x;
	}
	cv::line(image, p0, p1, Scalar(0, 255, 0), 1, LINE_AA);
}

} /* namespace venus */