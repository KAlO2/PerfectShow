#ifndef VENUS_OPENCV_UTILITY_H_
#define VENUS_OPENCV_UTILITY_H_

#include <stdint.h>

#include <opencv2/core.hpp>

#include "venus/compiler.h"

// implements std::hash for cv::Point2f
namespace std
{

template <>
struct hash<cv::Point2f>
{
	size_t operator()(const cv::Point2f& point) const noexcept
	{
		const size_t fnv_prime = 16777619u;
		const size_t fnv_offset_basis = 2166136261u;

		size_t state = fnv_offset_basis;
		for(const uint8_t* p = reinterpret_cast<const uint8_t*>(&point), *end = p + sizeof(cv::Point2f); p < end; ++p)
			state = (state ^ *p) * fnv_prime;
			
		return state;
	}
};

} /* namespace std */

namespace cv
{
// make it backward compatible

//typedef Rect_<float> Rect2f;
//typedef Rect_<int>   Rect2i;


} /* namespace cv */

/**
 * OpenCV helper functions
 */
namespace venus {

template <typename T>
cv::Vec<T, 4> boundingBox(const std::vector<cv::Point_<T>>& points, int start, int length)
{
	assert(points.size() >= 3);

	const cv::Point_<T>& p0 = points[start];
	float left = p0.x, right = p0.x, top = p0.y, bottom = p0.y;
	for(int i = 1; i < length; ++i)
	{
		const cv::Point_<T>& point = points[start + i];
		if(point.x < left)
			left = point.x;
		else if(point.x > right)
			right = point.x;

		if(point.y < top)
			top = point.y;
		else if(point.y > bottom)
			bottom = point.y;
	}

	return cv::Vec<T, 4>(left, top, right, bottom);
}

/**
 * like cv::boundingRect but return float type
 * @return Vec4f(left, top, right, bottom)
 */
// for int version use cv::boundingRect(points);
template <typename T>
inline cv::Vec<T, 4> boundingBox(const std::vector<cv::Point_<T>>& points)
{
	return boundingBox(points, 0, static_cast<int>(points.size()));
}


std::vector<cv::Point2i> cast(const std::vector<cv::Point2f>& points);

/**
 * For android.graphics.Color, it use BGRA memory layout as OpenCV does,
 * But native side use RGBA memory layout as shader does.
 * getNativeColor() in jni_bridge.h return RGBA memory layout,
 * this cast() casts RGBA color to OpenCV's color if @p USE_BGRA_LAYOUT
 * is defined as 1.
 */
cv::Vec4f cast(uint32_t color);
uint32_t cast(const cv::Vec4f& color);

cv::Rect2i box2Rect(const cv::Vec4f& box);

/**
 * calculate box size, box could be skew, described by line.
 * @param box Vec4f(left, top, right, bottom)
 * @param line vertical line, usually calculated by Feature::getSymmetryAxis()
 */
cv::Size2f calculateSize(const cv::Vec4f& box, const cv::Vec4f& line);

/**
 * Euclid distance between pt0 and pt1, can be write as: cv::norm(pt1 - pt0)
 */
float distance(const cv::Point2f& pt0, const cv::Point2f& pt1);

/**
 * Euclid distance between point and line AB
 */
float distance(const cv::Point2f& point, const cv::Point2f& A, const cv::Point2f& B);

/**
 * Calculate the distance from a point to a line.
 * @param[in] point
 * @param[in] line Vec4f(vx, vy, x0, y0), where (vx, vy) forms a unit vector.
 */
float distance(const cv::Point2f& point, const cv::Vec4f& line);

/**
 * Determine the intersection point of two lines, if this point exists.
 * Two lines intersect if they are not parallel (Parallel lines intersect at infinity).
 *
 * The lines are specified by a pair of points each. If they intersect, then
 * the function returns true, else it returns false.
 *
 * Lines can be specified in the following form:
 *      A1x + B1x = C1
 *      A2x + B2x = C2
 *
 * If det (= A1*B2 - A2*B1) == 0, then lines are parallel else they intersect
 *
 * If they intersect, then let us denote the intersection point with P(x, y) where:
 *      x = (C1*B2 - C2*B1) / (det)
 *      y = (C2*A1 - C1*A2) / (det)
 *
 * @param a1           First point for determining the first line
 * @param b1           Second point for determining the first line
 * @param a2           First point for determining the second line
 * @param b2           Second point for determining the second line
 * @param intersection The intersection point, if this point exists. pass nullptr if you don't care about it.
 * @return whether the two lines -- a1b1 and a2b2 intersects.
 */
bool lineIntersection(const cv::Point2f& a1, const cv::Point2f& b1, const cv::Point2f& a2, const cv::Point2f& b2, cv::Point2f* intersection);

/**
 * Triangle ABC Circumscribed_circle https://en.wikipedia.org/wiki/Circumscribed_circle
 */
cv::Point2f centerOfCircumscribedCircle(const cv::Point2f& A, const cv::Point2f& B, const cv::Point2f& C);

cv::Mat merge(const cv::Mat& rgb, const cv::Mat& alpha);

/**
 * It's like cv::split() but only cares alpha channel.
 */
cv::Mat splitAlpha(const cv::Mat& image);

cv::Mat normalize(const cv::Mat& mat, double* max = nullptr);

/**
 * like cv::line() but draw line that run through whole image, not line segment between pt0 and pt1
 */
void drawLine(cv::Mat& image, const cv::Point2f& pt0, const cv::Point2f& pt1, const cv::Scalar& color,
		int thickness = 1, int lineType = cv::LINE_8, int shift = 0);

void drawRotatedRect(cv::Mat& image, const cv::RotatedRect& rotated_rect, const cv::Scalar& color,
		int thickness = 1, int lineType = cv::LINE_8, int shift = 0);

void drawPolygon(cv::Mat& image, cv::Point2i* points, int count, const cv::Scalar& color,
		int thickness = 1, int lineType = cv::LINE_8, int shift = 0);

void drawPolygon(cv::Mat& image, std::vector<cv::Point2f>& points, const cv::Scalar& color,
		int thickness = 1, int lineType = cv::LINE_8, int shift = 0);

// draw curve p1~p2 with respect to p0, p3
void drawCurve(cv::Mat& image, const cv::Point2f& p0, const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3, const cv::Scalar& color,
		int thickness = 1, int lineType = cv::LINE_8, int shift = 0);

void drawCrossHair(cv::Mat& image, const cv::Point2f& position, int radius = 20, const cv::Scalar& color = cv::Scalar(0, 255, 0, 255), int gap = 4,
		int thickness = 1, int lineType = cv::LINE_8, int shift = 0);

void drawCross(cv::Mat& image, const cv::Point2f& position, int radius, const cv::Scalar& color,
		int thickness = 1, int lineType = cv::LINE_8, int shift = 0);

/**
 * Catmull Rom spline interpolation
 *
 * @param p0 control point 
 * @param p1 ditto 
 * @param p2 ditto 
 * @param p3 ditto 
 * @param t  0 returns p1, 1 returns p2, (0, 1) interval returns a smooth interpolating point.
 * @return point between p1 and p2
 */
cv::Point2f catmullRomSpline(float t, const cv::Point2f& p0, const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3);

// https://en.wikipedia.org/wiki/B%C3%A9zier_curve
// linear/quadratic/cubic Bezier curves
cv::Point2f bezier1(float t, const cv::Point2f& p0, const cv::Point2f& p1);
cv::Point2f bezier2(float t, const cv::Point2f& p0, const cv::Point2f& p1, const cv::Point2f& p2);
cv::Point2f bezier3(float t, const cv::Point2f& p0, const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3);

/**
 * the SUSAN (Smallest Univalue Segment Assimilating Nucleus) edge operator, used to detect eye corners in our case.
 * @see https://en.wikipedia.org/wiki/Corner_detection#The_SUSAN_corner_detector
 */
cv::Mat susan(const cv::Mat& image, int radius, int tolerance);

/**
 * Numerical <a href="https://cn.mathworks.com/help/matlab/ref/gradient.html">gradient</a> function from MATLAB,
 * OpenCV doesn't come with a straight function likewise, and it use operators like cv::Sobel(), so I implement one.
 *
 * @param[in] image  The source image, 2 dimensional array.
 * @param[out] dx    Like FX = gradient(F) in MATLAB
 * @param[out] dy    Like [FX, FY] = gradient(F) in MATLAB
 */
void gradient(const cv::Mat& image, cv::Mat* dx, cv::Mat* dy = nullptr);

/**
 * <a href="https://cn.mathworks.com/help/matlab/ref/interp2.html">interp2</a> function from MATLAB,
 * Interpolation for 2-D gridded data in meshgrid format.
 * @param[in] image  The source image, 2 dimensional array.
 * @param[in] point  The point in the image.
 * @return interpolated value.
 */
float interp2(const cv::Mat1f& image, const cv::Point2f& point);

} /* namespace venus */
#endif /* VENUS_OPENCV_UTILITY_H_ */