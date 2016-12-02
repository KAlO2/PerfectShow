#ifndef VENUS_BEAUTY_H_
#define VENUS_BEAUTY_H_

#include <stdint.h>
#include <functional>

#include <opencv2/core/mat.hpp>

namespace venus {

class Beauty
{
private:


public:

	/**
	 * Skin detection is performed in the RGB colour space, its predicative rules are very much subject to the influence of illumination
	 *
	 * @param[in] image an RGB(A) image
	 * @return single channel mask of uint8_t type
	 */
	static cv::Mat calculateSkinRegion_RGB(const cv::Mat& image);

	/**
	 * Skin detection is performed in the YCbCr colour space. Usually, skin color segmentation algorithm model by using Gaussian mixture model,
	 * because facial color region can be described by Gaussian distribution.
	 *
	 * @param[in] image an RGB(A) image
	 * @return single channel mask of float type
	 */
	static cv::Mat calculateSkinRegion_YCbCr(const cv::Mat& image);

	/**
	 * Skin detection is performed in the HSV colour space.
	 *
	 * @param[in] image an RGB(A) image
	 * @return single channel mask of uint8_t type
	 */
	static cv::Mat calculateSkinRegion_HSV(const cv::Mat& image);

};

} /* namespace venus */
#endif /* VENUS_BEAUTY_H_ */