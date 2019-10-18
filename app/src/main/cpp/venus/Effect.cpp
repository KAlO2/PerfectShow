#ifdef _WIN32
#	define _USE_MATH_DEFINES  // for M_PI
#endif
#include <cmath>
#include <cassert>

#include <opencv2/imgproc.hpp>
#if TRACE_IMAGES 
#include <opencv2/highgui.hpp>
#endif

#include "venus/blur.h"
#include "venus/colorspace.h"
#include "venus/Effect.h"
#include "venus/opencv_utility.h"
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
	const float l_wx = 1.0F - wx;
	const float l_wy = 1.0F - wy;
	for(int i = 0; i < N; ++i)
	{
		float c0 = c00[i] * wx + c01[i] * l_wx;
		float c1 = c10[i] * wx + c11[i] * l_wx;
		float c = c0 * wy + c1 * l_wy;

		// round off to the nearest for integer types
		if(std::is_integral<T>::value)
			c += T(0.5F);

		color[i] = static_cast<T>(c);
	}

	return color;
}

void Effect::mapColor(cv::Mat& dst, const cv::Mat&src, const uint8_t table[256])
{
	assert(src.depth() == CV_8U);
	dst.create(src.rows, src.cols, src.type());

	const int channels = src.channels();
	const int length = src.rows * src.cols * channels;

	if(channels == 4)
	{
		uint8_t* dst_data = dst.data;
		const uint8_t* src_data = src.data;

		#pragma omp parallel for
		for(int i = 0; i < length; i += 4)
		{
			dst_data[i+0] = table[src_data[i+0]];
			dst_data[i+1] = table[src_data[i+1]];
			dst_data[i+2] = table[src_data[i+2]];
			dst_data[i+3] = src_data[i+3];  // keep alpha untouched
		}
	}
	else
	{
		uint8_t* dst_data = dst.data;
		const uint8_t* src_data = src.data;

		#pragma omp parallel for
		for(int i = 0; i < length; ++i)
			dst_data[i] = table[src_data[i]];
	}
}

void Effect::mapColor(cv::Mat& dst, const cv::Mat&src, const uint8_t* mask, const uint8_t table[256])
{
	assert(src.depth() == CV_8U && mask != nullptr);
	dst.create(src.rows, src.cols, src.type());

	const int channels = src.channels();
	const int length = src.rows * src.cols * channels;

	if(channels == 4)
	{
		uint8_t* dst_data = dst.data;
		const uint8_t* src_data = src.data;

		#pragma omp parallel for
		for(int i = 0; i < length; i += 4)
		{
			int _0 = i, _1 = i + 1, _2 = i + 2, _3 = i + 3;
			dst_data[_0] = lerp(src_data[_0], table[src_data[_0]], mask[i]);
			dst_data[_1] = lerp(src_data[_1], table[src_data[_1]], mask[i]);
			dst_data[_2] = lerp(src_data[_2], table[src_data[_2]], mask[i]);
			dst_data[_3] =      src_data[_3];  // keep alpha untouched
		}
	}
	else
	{
		uint8_t* dst_data = dst.data;
		const uint8_t* src_data = src.data;

		#pragma omp parallel for
		for(int i = 0; i < length; ++i)
		{
			if(mask[i] == 0)
				dst_data[i] = src_data[i];  // copy transparent region
			else
//				dst_data[i] = lerp(src_data[i], table[src_data[i]], mask[i]/255.0F);
				dst_data[i] = lerp(src_data[i], table[src_data[i]], mask[i]);
		}
	}
}

void Effect::tone(cv::Mat& dst, const cv::Mat& src, uint32_t color, float amount)
{
	assert(src.type() == CV_8UC4 || src.type() == CV_32FC4);
	dst.create(src.size(), src.type());

	const float l_amount = 1.0F - amount;
	if(src.depth() == CV_32F)
		amount /= 255;

	Vec4f target = cast(color) * amount;
	
	switch(src.type())
	{
	case CV_8UC4:
		for(int r = 0; r < src.rows; ++r)
		for(int c = 0; c < src.cols; ++c)
		{
			const Vec4b& src_color = src.at<Vec4b>(r, c);
			Vec4b& dst_color = dst.at<Vec4b>(r, c);
			for(int i = 0; i < 3; ++i)  // loop unrolling
				dst_color[i] = static_cast<uint8_t>(src_color[i] * l_amount + target[i]);
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

void Effect::posterize(cv::Mat& dst, const cv::Mat& src, float level)
{
	assert(1.0F <= level && level <= 256.0F);
	const int depth = src.depth();
	const int channel = dst.channels();
	if(src.data != dst.data)
		src.copyTo(dst);

	if(depth == CV_8U)
	{
		level = 256 / level;
		uint8_t table[256];
		for(int i = 0; i < 256; ++i)
			table[i] = cvRound(std::floor(i / level) * level);
	
		mapColor(dst, src, table);
	}
	else if(depth == CV_32F)
	{
		float* data = dst.ptr<float>();
		const int length = dst.rows * dst.cols * channel;

		if(channel > 3)
		{
			#pragma omp parallel for
			for(int i = 0; i < length; i += channel)
			{
				data[i+0] = cvRound(data[i+0] * level) / level;
				data[i+1] = cvRound(data[i+1] * level) / level;
				data[i+2] = cvRound(data[i+2] * level) / level;
			}
		}
		else
		{
			#pragma omp parallel for
			for(int i = 0; i < length; ++i)
				data[i] = cvRound(data[i] * level) / level;
		}
	}
	else
		assert(false);
}

void Effect::pixelize(cv::Mat& dst, const cv::Mat& src, int width, int height)
{
	assert(width > 0 && height > 0);
	dst.create(src.rows, src.cols, src.type());
	
	const int br_count = (src.rows + height - 1) / height;
	const int bc_count = (src.cols + width  - 1) / width;
	for(int br = 0; br < br_count; ++br)
	{
		int sr0 = br * height;
		int height = std::min(height, src.rows - sr0);
		
		for(int bc = 0; bc < bc_count; ++bc)
		{
			int sc0 = bc * width;
			int width = std::min(width, src.cols - sc0);
			Rect rect(sc0, sr0, width, height);
			
			Scalar sum = cv::sum(src(rect));
			Scalar avg = sum / (width * height);
			
			dst(rect).setTo(avg);
		}
	}
}

cv::Mat Effect::grayscale(const cv::Mat& image)
{
	Mat gray;
	switch(image.channels())
	{
#if USE_BGRA_LAYOUT
	case 4: cvtColor(image, gray, CV_BGRA2GRAY); break;
	case 3: cvtColor(image, gray, CV_BGR2GRAY);  break;
#else
	case 4: cvtColor(image, gray, CV_RGBA2GRAY); break;
	case 3: cvtColor(image, gray, CV_RGB2GRAY);  break;
#endif
	case 1: image.copyTo(gray);                  break;
	default:
		assert(false);
		break;
	}

	return gray;
}

void Effect::colorize(cv::Mat& dst, const cv::Mat& src, float hue/* = 0.0F */, float saturation/* = 0.5F */, float lightness/* = 0.0F*/)
{
	assert(src.type() == CV_32FC4 && src.data != dst.data);
	assert(0.0F <= hue && hue <= 1.0F);
	assert(0.0F <= saturation && saturation <= 1.0F);
	assert(-1.0F <= lightness && lightness <= 1.0F);
	src.copyTo(dst);

	float hsl[3];
	hsl[0] = hue;
	hsl[1] = saturation;

	const int length = src.rows * src.cols * 4;
	const float* src_color = src.ptr<float>();
	float* dst_color = dst.ptr<float>();
	
	constexpr float LUMINANCE_R = 0.22248840F;
	constexpr float LUMINANCE_G = 0.71690369F;
	constexpr float LUMINANCE_B = 0.06060791F;

#if USE_BGRA_LAYOUT
	constexpr int _0 = 2, _1 = 1, _2 = 0;
#else
	constexpr int _0 = 0, _1 = 1, _2 = 2;
#endif
	for(int i = 0; i < length; i += 4)
	{
		float luminance = LUMINANCE_R * src_color[_0] + LUMINANCE_G * src_color[_1] + LUMINANCE_B * src_color[_2];

		if(lightness > 0)
			luminance = (1.0F - lightness) * luminance + lightness;
		else
			luminance*= (1.0F + lightness);

		hsl[2] = luminance;

		float rgb[3];
		hsl2rgb(hsl, rgb);

		dst_color[_0] = rgb[0];
		dst_color[_1] = rgb[1];
		dst_color[_2] = rgb[2];
//		dst_color[3]  = src_color[3];  // already copied

		src_color += 4;
		dst_color += 4;
	}
}

void Effect::unsharpMask(cv::Mat& dst, const cv::Mat& src,  float radius/* = 5.0F*/, int threshold/* = 0*/, float amount/* = 0.5F*/)
{
	assert(1.0F <= radius);
	assert(0 <= threshold && threshold <= 255);  // threshold = clamp(threshold, 0, 255);
	assert(0.0F <= amount && amount <= 1.0F);

	cv::Mat blurred;

	/* If the radius is less than 10, use a true gaussian kernel. This is slower, but more accurate and allows for finer adjustments.
	 * Otherwise use a three-pass box blur; this is much faster but it isn't a perfect approximation, and it only allows radius
	 * increments of about 0.42.
	 */
	const bool use_box_blur = radius >= 10;
	if(use_box_blur)
	{
		// Three box blurs of this width approximate a gaussian
		int box_width = cvRound(radius * 3 * sqrt (2 * M_PI) / 4);
		const Size size(box_width, box_width);
		const Point2i point(-1, -1);  // means center
		BorderTypes type = BorderTypes::BORDER_REPLICATE;
		cv::blur(src, blurred, size, point, type);
		cv::blur(blurred, dst, size, point, type);  // trampoline
		cv::blur(dst, blurred, size, point, type);
	}
	else
		venus::gaussianBlur(blurred, src, radius);

	// Sharpened = Original + ( Original - Blurred ) * Amount
	Mat lowContrastMask = cv::abs(src - blurred) <= threshold;
	dst = src + (src - blurred) * amount; // src*(1 + amount) + blurred*(-amount);
	src.copyTo(dst, lowContrastMask);
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
	const float a = 0.25F, b = 1.00F/3, scale = 0.70F;
	
//	constexpr // g++ pass, but clang fails with error: constexpr variable 'clamp_01' must be initialized by a constant expression.
	auto clamp_01 = [](float x) -> float { return clamp<float>(x, 0, 1); };

	shadows = shadows * clamp_01((lightness - b) / -a + 0.5F) * scale;
	midtones = midtones * clamp_01((lightness - b) /  a + 0.5F) *
			clamp_01((lightness + b - 1) / -a + 0.5F) * scale;
	highlights = highlights * clamp_01((lightness + b - 1) / a + 0.5F) * scale;
	
	value += shadows;
	value += midtones;
	value += highlights;
	value = clamp_01(value);
	
	return value;
}

void Effect::adjustColorBalance(cv::Mat& dst, const cv::Mat& src, const cv::Vec3f config[3], bool preserve_luminosity)
{
	assert((src.depth() == CV_8U || src.depth() == CV_32F) && src.channels() >= 3);
	if(src.data != dst.data)
		dst.create(src.rows, src.cols, src.type());

	const int channels = src.channels();
	const int length = src.rows * src.cols * channels;
	bool need_cast = src.depth() == CV_8U;
	Mat _src, _dst;
	const float* src_data;
	float* dst_data;
	if(need_cast)
	{
		src.convertTo(_src, CV_32F, 1/255.0F);
		_dst.create(_src.rows, _src.cols, _src.type());
		dst_data = _dst.ptr<float>();
		src_data = _src.ptr<float>();
	}
	else
	{
		dst_data = dst.ptr<float>();
		src_data = src.ptr<float>();
	}

	for(int i = 0; i < length; i += channels)
	{
		const float* src_rgb = src_data + i;
		float* dst_rgb = dst_data + i;

		float hsl[3];
		rgb2hsl(src_rgb, hsl);

		float& lightness = hsl[2];
		
		for(int k = 0; k < 3; ++k)
			dst_rgb[k] = mapColorBalance(src_rgb[k], lightness, config[RANGE_SHADOW][k], config[RANGE_MIDTONE][k], config[RANGE_HIGHLIGHT][k]);

		if(preserve_luminosity)
		{
			float hsl2[3];
			rgb2hsl(dst_rgb, hsl2);
			hsl2[2] = hsl[2];  // preserve brightness info
			hsl2rgb(hsl2, dst_rgb);
		}

		for(int k = 0; k < 3; ++k)
			assert(0.0F <= dst_rgb[k] && dst_rgb[k] <= 1.0F);
	}

	if(need_cast)
		_dst.convertTo(dst, 255.0F);
}

void Effect::adjustBrightnessAndContrast(Mat& dst, const Mat& src, float brightness/* = 0.0F */, float contrast/* = 1.0F */)
{
	assert(-0.5F <= brightness && brightness <= 0.5F);
	assert(0.0F <= contrast && contrast < std::numeric_limits<float>::infinity());

	// y = k*x + b, k is contrast, b is brightness.
	// brightness and contrast interval used to be [-1.0, 1.0].
	// when contrast = 1.0, tan(pi/2) = +INF, which yields wrong black result.
	// So contrast should be open interval (-1.0, 1.0).
//	brightness /= 2.0F;
//	contrast = std::tan((contrast + 1.0F) * M_PI_4);

	auto brightness_contrast = [&brightness, &contrast](const float& x)
	{
		float value;
		if(brightness < 0.0F)
			value = x * (1.0F + brightness);  // [0, x]
		else
			value = x + ((1.0F - x) * brightness);  // [x, 1]

		value = (value - 0.5F) * contrast + 0.5F;
		return value;
	};

	const int depth = src.depth();
	if(depth == CV_8U)
	{
		uint8_t table[256];
		for(int i = 0; i < 256; ++i)
			table[i] = saturate_cast<uint8_t>(brightness_contrast(i/255.0F) * 256.0F);

		mapColor(dst, src, table);
	}
	else if(depth == CV_32F)
	{
		if(src.data != dst.data)
			src.copyTo(dst);

		float* const data = dst.ptr<float>();
		const int channels = src.channels();
		const int length = src.rows * src.cols * channels;
		constexpr float threshold = 0.5F;
		if(channels >= 4)  // has alpha
		{
			#pragma omp parallel for
			for(int i = 0; i < length; i += 4)
			{
				data[i+0] = clamp((data[i+0] - threshold) * contrast + brightness + threshold);
				data[i+1] = clamp((data[i+1] - threshold) * contrast + brightness + threshold);
				data[i+2] = clamp((data[i+2] - threshold) * contrast + brightness + threshold);
			}
		}
		else
			#pragma omp parallel for
			for(int i = 0; i < length; ++i)
				data[i] = clamp((data[i] - threshold) * contrast + brightness + threshold);
	}
	else
		assert(false);  // unimplemented branch goes here.
}

void Effect::adjustGamma(cv::Mat& dst, const cv::Mat& src, float gamma)
{
	assert(gamma > 0);
	gamma = 1/gamma;

	dst.create(src.rows, src.cols, src.type());
	int depth = src.depth();
	if(depth == CV_8U)
	{
		// build a lookup table mapping the pixel values [0, 255] to their adjusted gamma values
		uint8_t table[256];
		for(int i = 0; i < 256; ++i)
			table[i] = cvRound(std::pow(i/255.0, gamma) * 255.0);

		mapColor(dst, src, table);
	}
	else if(depth == CV_32F)
	{
		const float* src_data = reinterpret_cast<const float*>(src.data);
		float* const dst_data = reinterpret_cast<float* const>(dst.data);
		const int channels = src.channels();
		const int length = src.rows * src.cols * channels;

		if(channels == 4)  // has alpha
		{
			#pragma omp parallel for
			for(int i = 0; i < length; i += 4)
			{
				for(int k = 0; k < 3; ++k)
					dst_data[i+k] = std::pow(src_data[i+k], gamma);
				dst_data[i+3] = src_data[i+3];  // keep alpha untouched
			}
		}
		else
			#pragma omp parallel for
			for(int i = 0; i < length; ++i)
				dst_data[i] = std::pow(src_data[i], gamma);
	}
	else
		assert(false);  // unimplemented branch goes here.
}

void Effect::adjustGamma(cv::Mat& dst, const cv::Mat& src, const cv::Vec3f& gamma)
{
	assert(src.channels() >= 3);

	std::vector<cv::Mat> channels;
	cv::split(src, channels);

	for(int i = 0; i < 3; ++i)  // skip alpha channel if it has.
		adjustGamma(channels[i], channels[i], gamma[i]);

	cv::merge(channels, dst);
}

void Effect::adjustHueSaturation(cv::Mat& dst, const cv::Mat& src, float hue/* = 0.0F */, float saturation/* = 1.0F */, float lightness/* = 0.0F */)
{
	assert(-0.5F <= hue && hue <= 0.5F);
	assert(0.0F <= saturation && saturation <= 2.0F);
	assert(-0.5F <= lightness && lightness <= 0.5F);

	const int channel = src.channels();
	const int depth   = src.depth();
	const int length  = src.rows * src.cols * channel;
	assert(channel >= 3 && (depth == CV_8U || depth == CV_32F));
	assert(dst.data != src.data);

	Mat _dst, _src;  // float type storage
	if(depth == CV_8U)
		src.convertTo(_src, CV_32F, 1/255.0F);
	else
		src.copyTo(_src);
#if USE_BGRA_LAYOUT
	const bool has_alpha = channel > 3;
	cvtColor(_src, _src, has_alpha?CV_BGRA2RGBA:CV_BGR2RGB);
#endif

	_src.copyTo(_dst);
	const float* _src_data = _src.ptr<float>();
	float* _dst_data = _dst.ptr<float>();

	#pragma omp parallel for
	for(int i = 0; i < length; i += channel)
	{
		float hsl[3];
		rgb2hsl(_src_data + i, hsl);

		// wrap around interval [0, 1], make sure that hue interval length is 1.
		hsl[0] += hue;
		if(hsl[0] < 0.0F)
			hsl[0] += 1.0F;
		else if(hsl[0] > 1.0F)
			hsl[0] -= 1.0F;

		hsl[1] *= saturation;
		hsl[1] = clamp(hsl[1]);

		float v = lightness;
		if(v < 0.0F)
			hsl[2] *= (v + 1.0F);
		else
			hsl[2] += v * (1.0F - hsl[2]);
		
		hsl2rgb(hsl, _dst_data + i);
	}
	
#if USE_BGRA_LAYOUT
	cvtColor(_dst, _dst, has_alpha?CV_RGBA2BGRA:CV_RGB2BGR);
#endif
	if(depth == CV_8U)
		_dst.convertTo(dst, CV_8U, 255.0F);
	else
		_dst.copyTo(dst);
}

} /* namespace venus */
