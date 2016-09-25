#ifndef VENUS_OPENCV_UTILITY_H_
#define VENUS_OPENCV_UTILITY_H_

#include <stdint.h>

#include <opencv2/core.hpp>

#include "venus/common.h"

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

/**
 * OpenCV helper functions
 */
namespace venus {

/**
 * like cv::boundingRect but return float version
 */
cv::Rect2f boundingRect(const std::vector<cv::Point2f>& points);

/**
 * Euclid distance between pt0 and pt1
 */
float distance(const cv::Point2f& pt0, const cv::Point2f& pt1);

/**
 * inset border for rectangle
 *
 * @param width positve get smaller rectangle, negative get larger rectangle.
 */
void inset(cv::Rect& rect, int width);

cv::Mat merge(const cv::Mat& rgb, const cv::Mat& alpha);

/**
 * like cv::line() but draw line that run through whole image, not line segment between pt0 and pt1
 */
void line(cv::Mat& image, const cv::Point2f& pt0, const cv::Point2f& pt1, const cv::Scalar& color,
		int thickness = 1, int lineType = cv::LINE_8, int shift = 0);

void drawCross(cv::Mat& image, const cv::Point2f& position, int radius, const cv::Scalar& color,
		int thickness = 1, int lineType = cv::LINE_8, int shift = 0);

} /* namespace venus */
#endif /* VENUS_OPENCV_UTILITY_H_ */