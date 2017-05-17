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

	/**
	 * Remove the red eye effect caused by camera flashes.
	 * This procedure removes the red eye effect caused by camera flashes by using a percentage based red color threshold.
	 * Make a selection containing the eyes, and apply the filter while adjusting the threshold to accurately remove the red eyes.
	 * @see http://graphicon.ru/html/2007/proceedings/Papers/Paper_11.pdf
	 *
	 * @param[out] dst        The destination image, can be the same as @p src.
	 * @param[in]  src        The source image.
	 * @param[in]  polygon    the region on source image to be processed.
	 * @param[in]  threshold  Range [0, 1], default to 0.5
	 */
	static void removeRedEye(cv::Mat& dst, const cv::Mat& src, const std::vector<cv::Point2f>& polygon, float threshold = 0.5F);

	/**
	 * Whiten skin by logarithmic Curve: v(x, y) = log(w(x, y)*(beta - 1) + 1) / log(beta),
	 * It refers to paper "A Two-Stage Contrast Enhancement Algorithm for Digital Images".
	 *
	 * @param[out] dst
	 * @param[in]  src
	 * @param[in] mask  Skin mask, it should be the same size as the source image. If mask is nullptr, operates on whole image.
	 * @param[in] level The bigger, the more bright it looks. range[2, 10]
	 */
	static void whitenSkinByLogCurve(cv::Mat& dst, const cv::Mat& src, float level);
	static void whitenSkinByLogCurve(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, float level);
	
	static void beautifySkin(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, float radius, float level);
};

} /* namespace venus */
#endif /* VENUS_BEAUTY_H_ */