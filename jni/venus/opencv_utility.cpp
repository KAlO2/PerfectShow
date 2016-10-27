#include "venus/opencv_utility.h"

#include <opencv2/imgproc.hpp>

using namespace cv;

namespace venus {

std::vector<cv::Point2i> cast(const std::vector<cv::Point2f>& points)
{
	std::vector<Point2i> result;
	result.reserve(points.size());
	for(const Point2f& point: points)
		result.push_back(Point(cvRound(point.x), cvRound(point.y)));

	return result;
}

cv::Vec4f cast(uint32_t color)
{
	uint8_t r = color, g = color >> 8, b = color >> 16, a = color >> 24;
#if USE_OPENCV_BGRA_LAYOUT
	return Vec4f(b, g, r ,a) / 255;
#else
	return Vec4f(r, g, b ,a) / 255;
#endif
}

cv::Rect2i box2Rect(const cv::Vec4f& box)
{
	cv::Rect2i rect;
	rect.x = static_cast<int>(box[0]);
	rect.y = static_cast<int>(box[1]);

	// half open half close inteval, need to add 1 extra pixel.
	rect.width = static_cast<int>(std::ceil(box[2])) + 1 - rect.x;
	rect.height =static_cast<int>(std::ceil(box[3])) + 1 - rect.y;
	return rect;
}

cv::Size2f calculateSize(const cv::Vec4f& box, const cv::Vec4f& line)
{
	assert(box[0] < box[2] && box[1] < box[3]);  // left < right, top < bottom
	cv::Size2f size;
	size.width = box[2] - box[0];
	size.height = box[3] - box[1];
	
	float scale = std::sqrt(line[0]*line[0] + line[1]*line[1]) / std::abs(line[1]);
	size *= scale;
	return size;
}

float distance(const cv::Point2f& pt0, const cv::Point2f& pt1)
{
//	return std::hypot(pt0.x - pt1.x, pt0.y - pt1.y);
	float dx = pt0.x - pt1.x;
	float dy = pt0.y - pt1.y;
	return std::sqrt(dx*dx + dy*dy);
}

// @see https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
float distance(const cv::Point2f& point, const cv::Point2f& A, const cv::Point2f& B)
{
	assert(A.x != B.x || A.y != B.y);  // A and B coincide.

	float dy = B.y - A.y;
	float dx = B.x - A.x;
	return std::abs(dy*point.x - dx*point.y + B.x*A.y - B.y*A.x) / (dx*dx + dy*dy);
}

float distance(const cv::Point2f& point, const cv::Vec4f& line)
{
	assert(std::abs(line[0]*line[0] + line[1]*line[1] - 1.0F) <= 1E-5);
	Point2f vector(point.x - line[2], point.y - line[3]);
	Point2f along(line[0], line[1]);
	Point2f project = (vector.x * along.x + vector.y * along.y) * along;
	Point2f normal = vector - project;
	return std::sqrt(normal.x*normal.x + normal.y*normal.y);
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
	Point2f p0(0, 0), p1(image.cols - 1, image.rows - 1);
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
	cv::line(image, p0, p1, color, thickness, lineType, shift);
}

void curve(cv::Mat& image, const cv::Point2f& p0, const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3,
		const cv::Scalar& color, int thickness/* = 1 */, int lineType/* = cv::LINE_8 */, int shift/* = 0 */)
{
	// TODO use std:map for binary split interval

	cv::Point2f pm = catmullRomSpline(0.5f, p0, p1, p2, p3);
	float d = distance(pm, p1, p2);
	if(d < 1.0f)
	{
		cv::line(image, p1, pm, color, thickness, lineType, shift);
		cv::line(image, pm, p2, color, thickness, lineType, shift);
	}
	else
	{
		cv::Point2f pl = catmullRomSpline(0.25f, p0, p1, p2, p3);
		cv::Point2f pr = catmullRomSpline(0.25f, p0, p1, p2, p3);
		
		// notice p1 pl !!!
		cv::line(image, p1, pl, color, thickness, lineType, shift);
		cv::line(image, pl, pm, color, thickness, lineType, shift);
		cv::line(image, pm, pr, color, thickness, lineType, shift);
		cv::line(image, pr, p2, color, thickness, lineType, shift);
	}
}

void drawCross(cv::Mat& image, const cv::Point2f& position, int radius, const Scalar& color,
		int thickness/* = 1 */, int lineType/* = cv::LINE_8 */, int shift/* = 0 */)
{
	cv::Point2f p00(position.x - radius, position.y - radius), p01(position.x - radius, position.y + radius);
	cv::Point2f p10(position.x + radius, position.y - radius), p11(position.x + radius, position.y + radius);
	cv::line(image, p00, p11, color, thickness, lineType, shift);
	cv::line(image, p01, p10, color, thickness, lineType, shift);
}

/*
	Cubic interpolation http://www.paulinternet.nl/?page=bicubic
	https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Interpolation_on_the_unit_interval_without_exact_derivatives

	                                    [0   2  0  0]   [p0]
	f(n + t) = 0.5 * [1, t, t^2, t^3] * [-1  0  1  0] * [p1]
	                                    [2  -5  4 -1]   [p2]
										[-1  3 -3  1]   [p3]
*/
cv::Point2f catmullRomSpline(float t, const cv::Point2f& p0, const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3)
{
//	assert(0 <= t && t <= 1);
	float tt = t*t, ttt = tt*t;
	float b0 = -ttt + 2*tt - t;
	float b1 = 3*ttt - 5*tt + 2;
	float b2 = -3*ttt + 4*tt + t;
	float b3 = ttt - tt;
	
	return 0.5f * (b0*p0 + b1*p1 + b2*p2 + b3*p3);
}

cv::Point2f bezier1(float t, const cv::Point2f& p0, const cv::Point2f& p1)
{
	return (1-t)*p0 + t*p1;
}

cv::Point2f bezier2(float t, const cv::Point2f& p0, const cv::Point2f& p1, const cv::Point2f& p2)
{
	float lmt = 1 -t;
//	return lmt * bezier1(t, p0, p1) + t * bezier1(t, p1, p2);
	return lmt*lmt*p0 + 2*lmt*p1 + t*t*p2;
}

cv::Point2f bezier3(float t, const cv::Point2f& p0, const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3)
{
	float lmt = 1 -t, lmt_lmt = lmt * lmt, t_t = t * t;
	return lmt_lmt * lmt * p0 + 3 * (lmt_lmt * t * p1 + lmt * t_t * p2) + t_t * t * p3;
}

cv::Mat susan(const cv::Mat& image, int radius, int tolerance)
{
	assert(image.type() == CV_8UC1);  // only handles gray image
	const int cols = image.cols, rows = image.rows;
	Mat result(rows, cols, CV_8UC1);

	const int rr = radius * radius;
	for(int r = 0; r < rows; ++r)
	for(int c = 0; c < cols; ++c)
	{
		int sum = 0, hit = 0;
		uint8_t center = image.at<uint8_t>(r, c);
		for(int dy = -radius; dy < radius; ++dy)
		{
			int y = r + dy;
			for(int dx = -radius; dx < radius; ++dx)
			{
				int x = c + dx;
				if(0 <= x && x < cols && 0 <= y && y < rows)
				if(dx*dx + dy*dy <= rr)  // comment this line if you want to detect squared region.
				{
					++sum;
					if(std::abs(image.at<uint8_t>(y, x) - center) <= tolerance)
						++hit;
				}
			}
		}

		float g = static_cast<float>(sum - hit)/sum;
//		if(g < 0.4f || g > 0.5f)
//			g = 0.0f;
		result.at<uint8_t>(r, c) = cvRound(g*255);
	}
	return result;
}



} /* namespace venus */