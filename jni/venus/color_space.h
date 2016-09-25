#ifndef VENUS_COLOR_SPACE_H_
#define VENUS_COLOR_SPACE_H_

#include <opencv2/core.hpp>

namespace venus {

/**
 * @breif: Utility functions which convert colors between different color models.
 *
 * When programming pixel data manipulation functions you will often use algorithms 
 * operating on a color model different from the one you uses.  This file provides 
 * utility functions to convert colors between different color spaces.
 */

cv::Vec3f rgb2hsv(const cv::Vec3f& rgb);
cv::Vec3f hsv2rgb(const cv::Vec3f& hsv);

cv::Vec3f rgb2hsl(const cv::Vec3f& rgb);
cv::Vec3f hsl2rgb(const cv::Vec3f& hsl);

// pullout: A scaling value (0-1) indicating how much black should be pulled out
// A @pullout value of 0 makes this a conversion to CMY. A value of 1 causes the maximum amount of black to be pulled out.
cv::Vec4f rgb2cmyk(const cv::Vec3f& rgb, float pullout);
cv::Vec3f cmyk2rgb(const cv::Vec4f& cmyk);


// alpha value are directly copied
cv::Vec4f rgb2hsv(const cv::Vec4f& rgba);
cv::Vec4f hsv2rgb(const cv::Vec4f& hsva);

cv::Vec4f color2alpha(const cv::Vec4f& color, const cv::Vec3f& from);


} /* namespace venus */
#endif  /* VENUS_COLOR_SPACE_H_ */
