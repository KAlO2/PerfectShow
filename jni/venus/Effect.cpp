#ifdef _WIN32
#	define _USE_MATH_DEFINES  // for M_PI
#endif
#include <cmath>
#include <cassert>

#include <opencv2/imgproc.hpp>
#if TRACE_IMAGES 
#include <opencv2/highgui.hpp>
#endif

#include "venus/colorspace.h"
#include "venus/Effect.h"
#include "venus/scalar.h"


using namespace cv;

namespace venus {

// bilinear interpolation
template <typename T, int N>
cv::Vec<T, N> interpolate(const Mat& image,
	const int& x0, const int& y0, const int& x1, const int& y1,
	const float& wx, const float& wy)
{
	using Vector = cv::Vec<T, N>;

	// note at<T>(y, x) or at<T>(Point(x, y)) or at<T>(r, c)
	const Vector& c00 = image.at<Vector>(y0, x0);
	const Vector& c01 = image.at<Vector>(y0, x1);
	const Vector& c10 = image.at<Vector>(y1, x0);
	const Vector& c11 = image.at<Vector>(y1, x1);

	Vector color;
	const float l_wx = 1.0f - wx;
	const float l_wy = 1.0f - wy;
	for(int i = 0; i < N; ++i)
	{
		float c0 = c00[i] * wx + c01[i] * l_wx;
		float c1 = c10[i] * wx + c11[i] * l_wx;
		float c = c0 * wy + c1 * l_wy;

		// round off to the nearest for integer types
		if(std::is_integral<T>::value)
			c += T(0.5);

		color[i] = static_cast<T>(c);
	}

	return color;
}

void Effect::tone(cv::Mat& dst, const cv::Mat& src, uint32_t color, float amount)
{
	assert(src.type() == CV_8UC4 || src.type() == CV_32FC4);
	dst.create(src.size(), src.type());

	const float l_amount = 1.0F - amount;
	if(src.type() == CV_32FC4)
		amount /= 255;

	uint8_t r = color, g = color >> 8, b = color >> 16;
#if USE_OPENCV_BGRA_LAYOUT
	const Vec3f target(b * amount, g * amount, r * amount);
#else
	const Vec3f target(r * amount, g * amount, b * amount);
#endif
	
	switch(src.type())
	{
	case CV_8UC4:
		for(int r = 0; r < src.rows; ++r)
		for(int c = 0; c < src.cols; ++c)
		{
			const Vec4b& src_color = src.at<Vec4b>(r, c);
			Vec4b& dst_color = dst.at<Vec4b>(r, c);
			for(int i = 0; i < 3; ++i)  // loop unrolling
				dst_color[i] = src_color[i] * l_amount + target[i];
		}
		break;
	case CV_32FC4:
		for(int r = 0; r < src.rows; ++r)
		for(int c = 0; c < src.cols; ++c)
		{
			const Vec4f& src_color = src.at<Vec4f>(r, c);
			Vec4f& dst_color = dst.at<Vec4f>(r, c);
			for(int i = 0; i < 3; ++i)  // loop unrolling
				dst_color[i] = src_color[i] * l_amount + target[i];
		}
		break;
	default:
		assert(false);
		break;
	}
}

void Effect::gaussianBlur(cv::Mat& dst, const cv::Mat& src, float radius)
{
	assert(radius >= 0);
	int r = cvRound(radius);
	
	int width = (r << 1) + 1;
	double std_dev = radius * 3;  // 3-sigma rule https://en.wikipedia.org/wiki/68–95–99.7_rule

	cv::Mat result;
	cv::GaussianBlur(src, dst, Size(width, width), std_dev);
}

float Effect::mapColorBalance(float value, float lightness, float shadows, float midtones, float highlights)
{
	/* Apply masks to the corrections for shadows, midtones and highlights so that each correction affects only one range.
	 * Those masks look like this:
	 *     ‾\___
	 *     _/‾\_
	 *     ___/‾
	 * with ramps of width a at x = b and x = 1 - b.
	 *
	 * The sum of these masks equals 1 for x in 0..1, so applying the same correction in the shadows and in the midtones 
	 * is equivalent to applying this correction on a virtual shadows_and_midtones range.
	 */
	const float a = 0.25f, b = 1.00f/3, scale = 0.70f;
	
	constexpr auto clamp_01 = [](float x) -> float { return clamp<float>(x, 0, 1); };

	shadows = shadows * clamp_01((lightness - b) / -a + 0.5f) * scale;
	midtones = midtones * clamp_01((lightness - b) /  a + 0.5f) *
			clamp_01((lightness + b - 1) / -a + 0.5f) * scale;
	highlights = highlights * clamp_01((lightness + b - 1) / a + 0.5f) * scale;
	
	value += shadows;
	value += midtones;
	value += highlights;
	value = clamp_01(value);
	
	return value;
}

void Effect::adjustColorBalance(float* const dst, const float* const src, int width, int height, const cv::Vec3f config[3], bool preserve_luminosity)
{
	assert(src != nullptr && dst != nullptr);
	constexpr int SHADOWS    = static_cast<int>(RangeMode::SHADOWS);
	constexpr int MIDTONES   = static_cast<int>(RangeMode::MIDTONES);
	constexpr int HIGHLIGHTS = static_cast<int>(RangeMode::HIGHLIGHTS);
	
	for(int i = 0, length = height * width * 4; i < length; i += 4)
	{
		const cv::Vec3f& rgb = src[i];
		cv::Vec3f hsl = rgb2hsl(rgb);

		float& lightness = hsl[2];
		cv::Vec3f rgb2 = dst[i];
		rgb2[0] = mapColorBalance(rgb[0], lightness, config[SHADOWS][0], config[MIDTONES][0], config[HIGHLIGHTS][0]);
		rgb2[1] = mapColorBalance(rgb[1], lightness, config[SHADOWS][1], config[MIDTONES][1], config[HIGHLIGHTS][1]);
		rgb2[2] = mapColorBalance(rgb[2], lightness, config[SHADOWS][2], config[MIDTONES][2], config[HIGHLIGHTS][2]);

		if(preserve_luminosity)
		{
			cv::Vec3f hsl2 = rgb2hsl(rgb2);
			hsl2[2] = hsl[2];  // preserve brightness info
			rgb2 = hsl2rgb(hsl2);
		}

		dst[i+3] = src[i+3];  // keep alpha

		for(int b = 0; b < 3; ++b)
			assert(0 <= dst[b] && dst[b] <= 1);
	}
}

void Effect::adjustGamma(cv::Mat& dst, const cv::Mat& src, float gamma)
{
	assert(gamma > 0);
	gamma = 1/gamma;

	dst.create(src.rows, src.cols, src.type());
	int depth = src.depth(), channel = src.channels();
	if(depth == CV_8U)
	{
		// build a lookup table mapping the pixel values [0, 255] to their adjusted gamma values
		uint8_t table[256];
		for(int i = 0; i < 256; ++i)
			table[i] = cvRound(std::pow(i/255.0, gamma) * 255.0);

		if(channel == 4)
		{
			const cv::Vec4b* src_data = reinterpret_cast<const cv::Vec4b*>(src.data);
			cv::Vec4b* const dst_data = reinterpret_cast<cv::Vec4b* const>(dst.data);
			const int length = src.rows * src.cols;

			#pragma omp parallel for
			for(int i = 0; i < length; ++i)
			{
				const cv::Vec4b& s = src_data[i];
				cv::Vec4b& d = dst_data[i];
				for(int i = 0; i < 3; ++i)  // loop unrolling
					d[i] = table[s[i]];
			}
		}
		else
		{
			const uint8_t* src_data = src.data;
			uint8_t* const dst_data = dst.data;
			const int length = src.rows * src.cols * channel;

			#pragma omp parallel for
			for(int i = 0; i < length; ++i)
				dst_data[i] = table[src_data[i]];
		}
	}
	else if(depth == CV_32F && channel == 4)
	{
		const float* src_data = reinterpret_cast<const float*>(src.data);
		float* const dst_data = reinterpret_cast<float* const>(dst.data);
		const int length = src.rows * src.cols * 4;

		#pragma omp parallel for
		for(int i = 0; i < length; i += 4)
		{
			dst_data[i+0] = std::pow(src_data[i+0], gamma);
			dst_data[i+1] = std::pow(src_data[i+1], gamma);
			dst_data[i+2] = std::pow(src_data[i+2], gamma);
			dst_data[i+3] = src_data[i+3];  // keep alpha untouched
		}
	}
	else
		assert(false);  // unimplemented branch goes here.
}

} /* namespace venus */
