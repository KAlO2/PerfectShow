#ifndef VENUS_BLUR_H_
#define VENUS_BLUR_H_

#include <opencv2/core/core.hpp>

namespace venus {

/**
	* Note that @p dst can be same as @p src image, so you can write:
	* <code>
	* gaussianBlur(image, image, radius);
	* </code>
	*
	* @param[out] dst    The output image.
	* @param[in]  src    The input image.
	* @param[in]  radius Blur radius.
	*/
void gaussianBlur(cv::Mat& dst, const cv::Mat& src, float radius);


/**
 * "Selective Gaussian blur" blurs neighboring pixels, but only in low-contrast areas. It can't take in-place.
 * Large filters (radius > 5) are very slow, so it is recommended to use radius = 3 for real-time applications.
 * And a mask can be used to shorten processing time.
 *
 * @param[out] dst        The output image.
 * @param[in]  src        The input image.
 * @param[in]  mask       CV_8UC1 format
 * @param[in]  radius     Gaussian kernel radius, or bluring radius.
 * @param[in]  tolerance  Range [0, 255], pixels within tolerance will be handled.
 */
void gaussianBlurSelective(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, float radius, float tolerance);

void radialBlur(cv::Mat& dst, const cv::Mat& src, cv::Point2f& center, float inner_radius, float outer_radius);
void bilinearBlur(cv::Mat& dst, const cv::Mat& src, cv::Point2f& point0, cv::Point2f& point1, float band_width);



} /* namespace venus */
#endif /* VENUS_BLUR_H_ */