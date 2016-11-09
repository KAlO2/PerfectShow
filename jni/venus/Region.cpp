#include "venus/Region.h"

#include <stdint.h>

#include <opencv2/imgproc.hpp>

using namespace cv;

namespace venus {

Region::Region(cv::Point2f& pivot, cv::Size2f& size, cv::Mat& mask):
	pivot(pivot),
	size(size),
	mask(mask)
{
}

cv::Rect Region::getRect(const cv::Point2f& pivot, const cv::Size2f& size)
{
	int left = static_cast<int>(pivot.x - size.width/2);
	int top = static_cast<int>(pivot.y - size.height/2);
	int width = cvCeil(size.width);
	int height = cvCeil(size.height);
	
	return Rect(left, top, width, height);
}

cv::Rect Region::getRect() const
{
	return getRect(pivot, size);
}

Region Region::merge(const Region& region1, const Region& region2)
{
	const Mat& mask1 = region1.mask, &mask2 = region2.mask;
	Rect2f rect1(region1.pivot - Point2f(mask1.cols, mask1.rows)/2, mask1.size());
	Rect2f rect2(region2.pivot - Point2f(mask2.cols, mask2.rows)/2, mask2.size());
	Rect2f rect = rect1 | rect2;

	rect1.x -= rect.x; rect1.y -= rect.y;
	rect2.x -= rect.x; rect2.y -= rect.y;
	Rect2i rect1_ = rect1, rect2_ = rect2;

	Point2f pivot = (region1.pivot + region2.pivot)/2;
	Size2f  size = rect.size();
	Mat     mask(rect.size(), CV_8UC1, Scalar(0));
	mask1.copyTo(mask(rect1_));
	mask2.copyTo(mask(rect2_));  // overlapping area will be overridden
	return Region(pivot, size, mask);
}

cv::Mat Region::resize(const cv::Mat& image, const Point2f& pivot,
	float left_scale, float top_scale, float right_scale, float bottom_scale,
	int interpolation/* = INTER_LINEAR */)
{
	int pivot_x = cvRound(pivot.x), pivot_y = cvRound(pivot.y);
//	printf("pivot: %d %d, image(%dx%d)", pivot_x, pivot_y, image.cols, image.rows);
	assert(0 <= pivot_x && pivot_x < image.cols);
	assert(0 <= pivot_y && pivot_y < image.rows);

	cv::Mat top, bottom;  // seperate the whole image vertically
	image(Rect2i(0, 0, image.cols, pivot_y)).copyTo(top);
	image(Rect2i(0, pivot_y, image.cols, image.rows - pivot_y)).copyTo(bottom);

	const cv::Size EMPTY(0, 0);
	cv::resize(top, top, EMPTY, 1.0f, top_scale, interpolation);
	cv::resize(bottom, bottom, EMPTY, 1.0f, bottom_scale, interpolation);

	cv::Mat result;
	cv::vconcat(top, bottom, result);

	cv::Mat left, right;  // seperate the whole image horizontally
	result(Rect2i(0, 0, pivot_x, result.rows)).copyTo(left);
	result(Rect2i(pivot_x, 0, result.cols - pivot_x, result.rows)).copyTo(right);

	cv::resize(left, left, EMPTY, left_scale, 1.0f, interpolation);
	cv::resize(right, right, EMPTY, right_scale, 1.0f, interpolation);

	cv::hconcat(left, right, result);
	return result;
}

cv::Mat Region::inset(const cv::Mat& mat, int offset)
{
	assert(!mat.empty());

	Rect rect(0, 0, mat.cols, mat.rows);
	inset(rect, offset);

	if(rect.x >= 0)  // rect.x == rect.y
		return mat(rect).clone();

	Rect rect2(-offset, -offset, mat.cols, mat.rows);
	Mat mat2(rect.size(), mat.type(), Scalar::all(0));
	mat.copyTo(mat2(rect2));
	return mat2;
}

void Region::inset(float offset)
{
	pivot.x -= offset;
	pivot.y -= offset;

	if(offset > std::numeric_limits<float>::epsilon())
		mask = inset(mask, offset);
}

cv::Rect2i Region::boundingRect(const cv::Mat& mask, int tolerance/* = 0 */)
{
	assert(!mask.empty() && mask.depth() == CV_8U);

	std::vector<uint8_t> row(mask.cols, 0), col(mask.rows, 0);
	const uint8_t* p = mask.ptr<uint8_t>();
	const int channel = mask.channels();
	for(int r = 0; r < mask.rows; ++r)
	for(int c = 0; c < mask.cols; ++c)
	{
		// single channel is alpha, multiple channels select the last channel as alpha.
		uint8_t value = p[(r * mask.cols + c + 1) * channel - 1];
//		uint8_t value = mask.at<uint8_t>(r, c);
		if(value <= tolerance)
			continue;

		row[c] |= value;
		col[r] |= value;
	}

#if 0
/*
	// http://stackoverflow.com/questions/24425127/explaining-a-string-trimming-function
	template< class Iterator >
	constexpr std::reverse_iterator<Iterator> make_reverse_iterator( Iterator i )
	{
		return std::reverse_iterator<Iterator>(i);
	}

	std::make_reverse_iterator comes from C++14, so you can write
		std::make_reverse_iterator(left)
	instead of
		std::reverse_iterator<decltype(left)>(left)
*/
	auto predicate = [](uint8_t ch) { return ch == 0; };
	auto left   = std::find_if_not(row.begin(),  row.end(),  predicate);
	auto right  = std::find_if_not(row.rbegin(), std::reverse_iterator<decltype(left)>(left), predicate);
	auto top    = std::find_if_not(col.begin(),  col.end(),  predicate);
	auto bottom = std::find_if_not(col.rbegin(), std::reverse_iterator<decltype(top)>(top), predicate);
	return cv::Rect2i(std::distance(row.begin(), left), std::distance(col.begin(), top),
		std::distance(left, right.base()), std::distance(top, bottom.base()))
#else
	int left = mask.rows, right = 0;
	int top = mask.cols, bottom = 0;
	for(int i = 0; i < mask.cols; ++i)         if(row[i] != 0) { left   = i; break; }
	for(int i = mask.cols - 1; i >= left; --i) if(row[i] != 0) { right  = i; break; }
	for(int i = 0; i < mask.rows; ++i)         if(col[i] != 0) { top    = i; break; }
	for(int i = mask.rows - 1; i >= top; --i)  if(col[i] != 0) { bottom = i; break; }

	if(left > right || top > bottom)  // In fact, arbitrary one condition should be enough.
		return Rect2i(0, 0, 0, 0);    // Empty rectangle means the input mask is all zeros!

	int width = right - left + 1, height = bottom - top + 1;
	return cv::Rect2i(left, top, width, height);
#endif
}

void Region::shrink(cv::Mat& dst, const cv::Mat& mask, int offset)
{
	mask.copyTo(dst);  // self copy will be skipped
	if(offset == 0)  // shortcut
		return;

	int radius  = std::abs(offset);
	int size    = radius * 2 + 1;
	Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, Size(size, size), Point(radius, radius));

	// Apply the specified morphology operation
	const Point anchor(-1, -1);  // default value (-1, -1) means that the anchor is at the element center.
	int iterations = 1;
	if(offset > 0)
		cv::erode(mask, dst, element, anchor, iterations);
	else
		cv::dilate(mask, dst, element, anchor, iterations);
}

void Region::grow(cv::Mat& dst, const cv::Mat& mask, int offset)
{
	shrink(dst, mask, -offset);
}

void Region::overlay(cv::Mat& dst, const cv::Mat& patch, const cv::Point2i& position, const cv::Mat& mask)
{
	assert(patch.size() == mask.size());
	Rect rect(position, mask.size());
	patch.copyTo(dst(rect), mask);
}

cv::Mat Region::transform(cv::Size& size, cv::Point2f& pivot, float angle, const cv::Vec2f& scale/* = cv::Vec2f(1.0f, 1.0f) */)
{
/*
	@see http://stackoverflow.com/questions/22041699/rotate-an-image-without-cropping-in-opencv-in-c
		cv::Mat matrix = cv::getRotationMatrix2D(pivot, angle, scale);
		cv::Rect rect = cv::RotatedRect(pivot, size, angle).boundingRect();

	getRotationMatrix2D is fine, but I need to tweak this function for anisotropical scaling.
	Y is top down, so a clockwise rotating `angle` means a counter-clockwise rotating `-angle` in Y bottom up coordinate.

	/ x' \  = / cos(angle)  -sin(angle) \ . / scale[0]     0     \ . / 1 0  -pivot.x \ . / x \
	\ y' /    \ sin(angle)   cos(angle) /   \    0      scale[1] /   \ 0 1  -pivot.y /   \ y /

	let
		ca_s0 = cos(angle) * scale[0];  sa_s1 = sin(angle) * scale[1];
		sa_s0 = sin(angle) * scale[0];  ca_s1 = cos(angle) * scale[1];

	and the affine matrix is:

		/ ca_s0  -sa_s1  -ca_s0*pivot.x+sa_s1*pivot.y \
		\ sa_s0   ca_s1  -sa_s0*pivot.x-ca_s1*pivot.y /
*/
	float c = std::cos(angle), s = std::sin(angle);

    Mat affine(2, 3, CV_32F);
    float* m = affine.ptr<float>();

    m[0] = c * scale[0];  m[1] =-s * scale[1];  m[2] = m[0] * -pivot.x + m[1] * -pivot.y;
    m[3] = s * scale[0];  m[4] = c * scale[1];  m[5] = m[3] * -pivot.x + m[4] * -pivot.y;
    
	assert(size.width > 1 && size.height > 1);  // in case underflow
	if(size.width <= 1 && size.height <= 1)
	{
		m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
		m[0] = 0.0f; m[1] = 1.0f; m[2] = 0.0f;
		return affine;
	}

	float x = (size.width > 1)? size.width - 1 : 0;
	float y = (size.height > 1)? size.height - 1 : 0;
	std::vector<Point2f> points{ pivot, Point2f(0, 0), Point2f(x, 0), Point2f(0, y), Point2f(x, y) };
	cv::transform(points, points, affine);
	
	Rect rect = cv::boundingRect(points);
	pivot = points[0];

	// make all the stuff relative to origin
	m[2] -= rect.x;  pivot.x -= rect.x;
	m[5] -= rect.y;  pivot.y -= rect.y;
	size.width = rect.width;
	size.height= rect.height;

	return affine;
}

cv::Point2f Region::transform(const cv::Mat& affine, const cv::Point2f& point)
{
	assert(affine.rows == 2 && affine.cols == 3);
	const float* m = affine.ptr<float>();
	return Point2f(
		m[0] * point.x + m[1] * point.y + m[2],
		m[3] * point.x + m[4] * point.y + m[5]);
}

cv::Mat Region::invert(const cv::Mat& mat)
{
	assert(mat.rows == 2 && mat.cols == 3);

	cv::Mat result;
	cv::invertAffineTransform(mat, result);
	return result;
}

} /* namespace venus */