#ifndef VENUS_COLORSPACE_H_
#define VENUS_COLORSPACE_H_

namespace venus {

/*
 * @breif: Utility functions which convert colors between different color models.
 *
 * When programming pixel data manipulation functions you will often use algorithms 
 * operating on a color model different from the one you uses.  This file provides 
 * utility functions to convert colors between different color spaces.
 */

void rgb2hsv(const float* rgb, float* hsv);
void hsv2rgb(const float* hsv, float* rgb);

void rgb2hsl(const float* rgb, float* hsl);
void hsl2rgb(const float* hsl, float* rgb);

/**
 * @param[in] rgb      RGB value in range [0, 1].
 * @param[in] pullout  A scaling value (0-1) indicating how much black should be pulled out.
 *		A @p pullout value of 0 makes this a conversion to CMY. A value of 1 causes the maximum
 *		amount of black to be pulled out.
 * @param[out] cmyk    CMYK value
 */
void rgb2cmyk(const float* rgb, const float& pullout, float* cmyk);
void cmyk2rgb(const float* cmyk, float* rgb);

/**
 * Convert a specified color to transparency, works best with white.
 *
 * @param[in]  color  Reference color, 4 channels, and alpha is last.
 * @param[in]  src    Input color
 * @param[out] dst    Output color
 */
void color2alpha(const float* color, const float* src, float* dst);


} /* namespace venus */
#endif  /* VENUS_COLORSPACE_H_ */
