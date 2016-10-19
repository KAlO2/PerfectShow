#ifndef VENUS_MAKEUP_H_
#define VENUS_MAKEUP_H_

#include <stdint.h>

#include <opencv2/core.hpp>

#include "venus/Region.h"

namespace venus {

class Makeup
{
public:
	enum class BlushShape
	{
		DEFAULT,
		DISK,
		OVAL,
		TRIANGLE,
		HEART,
		SEAGULL,
		
//	private:  // for internal use only
		SHAPE_COUNT
	};

private:
	static cv::Mat pack(uint32_t color, const cv::Mat& gray);

	static std::vector<cv::Point2f> createShape(const std::vector<cv::Point2f>& points, BlushShape shape, bool right);
public:


	/**
	 * @param[in,out] dst The destination image
	 * @param[in] src     The source image
	 * @param[in] mask, value 0 means transparent, namely blending area, 255 means opaque.
	 * @param[in] origin  Relative origin of the <code>src</code> image on <code>dst</code> image.
	 * @param[in] amount  Blending amount in range [0, 1], 0 being no effect, 1 being fully applied.
	 */
	static void blend(cv::Mat& dst, const cv::Mat& src, const cv::Point2i& origin, float amount);
	static void blend(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, const cv::Point2i& origion, float amount);
	

	/**
	 * http://www.makeupforever.com/us/en-us/learn/how-to/blush-applications
	 * <a href="https://simple.wikipedia.org/wiki/Blush_(color)">Tones of blush color</a> that used for theatre makeup.
	 *
	 * @param[in,out] dst
	 * @param[in] src
	 * @param[in] points Feature points detected from <code>src</code> image.
	 * @param[in] shape  Shape of blush.
	 * @param[in] color  0xAABBGGRR, RGB channel will be blush's primary color, and alpha will be premultiplied to <code>blush</code>.
	 * @param[in] amount Blending amount in range [0, 1], 0 being no effect, 1 being fully applied.
	 */
	static void applyBlush(cv::Mat& dst, const cv::Mat& src, const std::vector<cv::Point2f>& points, BlushShape shape, uint32_t color, float amount);

	/**
	 * Lip gloss and lipstick can make your lips look fuller, glossier and better! See how to 
	 * <a href="http://www.wikihow.com/Apply-Lip-Color">apply lip color</a>.
	 * 
	 * @param color RGBA memory layout
	 */
	static void applyLipColor(const uint32_t* dst, const uint32_t* const src, int width, int height, const Region& region, uint32_t color, float amount);
	static void applyLipColor(const float* dst, const float* const src, int width, int height, const Region& region, float color, float amount);
	static void applyLipColor(cv::Mat& dst, const cv::Mat& src, const Region& region, uint32_t color, float amount);

};

} /* namespace venus */
#endif /* VENUS_MAKEUP_H_ */