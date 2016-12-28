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

void Effect::gaussianBlur(cv::Mat& dst, const cv::Mat& src, float radius)
{
	assert(radius >= 0);
	int r = cvRound(radius);
	
	int width = (r << 1) + 1;
	double std_dev = radius * 3;  // 3-sigma rule https://en.wikipedia.org/wiki/68–95–99.7_rule

	cv::GaussianBlur(src, dst, Size(width, width), std_dev, std_dev, cv::BorderTypes::BORDER_CONSTANT);
}

void Effect::gaussianBlurSelective(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, float radius, float tolerance)
{
	assert(dst.data != src.data);
	int R = cvRound(radius);
	if(R <= 1)
	{
		src.copyTo(dst);
		return;
	}
	
	int width = (R << 1) + 1;
#if 0
	std::vector<cv::Mat> channels;
	cv::split(src, channels);

	const int N = src.channels() < 4 ? src.channels() : 3;
	#pragma omp parallel for
//	for(cv::Mat& channel: src_channels)  // It seems that OpenMP doesn't support range based for in C++11.
	// error C3016: 'i' : index variable in OpenMP 'for' statement must have signed integral type
	// The Microsoft C/C++ Compiler 12.0 integrated with Visual Studio 2013 still only support OpenMP 2.5 and doesn't allow unsigned int for the loop counter.
	// GCC support OpenMP 3.0 since its version 4.4 and allows unsigned int for the loop counter.
	for(int i = 0; i < N; ++i)
	{
		cv::Mat& channel = channels[i];
		cv::Mat tmp;  // Bilateral filter does not work inplace.
		cv::bilateralFilter(channel, tmp, width, width*2.0, width/2.0);
		channel = tmp;
	}

	cv::merge(channels.data(), channels.size(), dst);
#else
	Mat _src;
	src.convertTo(_src, CV_32F);
//	dst.create(src.rows, src.cols, CV_32F);
	_src.copyTo(dst);
	
	assert(mask.type() == CV_8UC1);
	const uint8_t* const mask_data = mask.ptr<uint8_t>();
//	Mat _mask = normalize(mask);
//	const float* const mask_data = mask.ptr<float>();

	// initialize Gaussian kernel
	Mat kernel(width, width, CV_32FC1);
//	kernel.at<float>(R, R) = 1.0F;
	for(int r = 0; r <= R; ++r)
	for(int c = 0; c <= R; ++c)
	{
		float v = std::exp(-0.5F * (r*r + c*c) / radius);
		kernel.at<float>(R - r, R - c) = v;
		kernel.at<float>(R - r, R + c) = v;
		kernel.at<float>(R + r, R - c) = v;
		kernel.at<float>(R + r, R + c) = v;
	}

	const int channel = src.channels();
	assert(channel >= 3);  // currently only support color image
	const int length = src.rows * src.cols;

	assert(_src.depth() == CV_32F && dst.depth() == CV_32F);
	const float* const _src_data = _src.ptr<float>();
	float* const dst_data = dst.ptr<float>();

	#pragma omp parallel for
	for(int k = 0; k < length; ++k)
	{
		if(mask_data[k] == 0)
			continue;

		int r = k / src.cols, c = k % src.cols;
		int offset = k * channel;
		const float* center = _src_data + offset;
		
		Vec3f accumulated(0, 0, 0), count(0, 0, 0);
		for(int y = -R; y <= R; ++y)
		{
			int j = r + y;
			if(j < 0 || j >= src.rows)
				continue;

			for(int x = -R; x <= R; ++x)
			{
				int i = c + x;
				if(i < 0 || i >= src.cols)
					continue;
				
				float weight = kernel.at<float>(R + y, R + x);
				const float* around = _src_data + (j * src.cols + i) * channel;
#if 1
				// individual channel
				for(int m = 0; m < 3; ++m)
				{
					float diff = std::abs(center[m] - around[m]);
					if(diff > tolerance)
						continue;
					
					accumulated[m] += weight * around[m];
					count[m] += weight;
				}
#else
				// RGB weighted measure method
				float diff = (std::abs(center[0] - around[0]) +
					std::abs(center[1] - around[1]) + std::abs(center[2] - around[2]))/3;
				if(diff > tolerance)
					continue;

				for(int m = 0; m < 3; ++m)
				{
					accumulated[m] += weight * around[m];
					count[m] += weight;
				}
#endif
			}
		}

		// weight_sum can not be ZERO, since center point is counted.
		dst_data[offset + 0] = accumulated[0] / count[0];
		dst_data[offset + 1] = accumulated[1] / count[1];
		dst_data[offset + 2] = accumulated[2] / count[2];
	}

	if(src.depth() == CV_8U)
		dst.convertTo(dst, CV_8U);
#endif
}

void Effect::unsharpMask(cv::Mat& dst, const cv::Mat& src,  float radius/* = 5.0F*/, int threshold/* = 0*/, float amount/* = 0.5F*/)
{
	assert(radius >= 0.0F);
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
		gaussianBlur(blurred, src, radius);

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
	
	constexpr auto clamp_01 = [](float x) -> float { return clamp<float>(x, 0, 1); };

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
				for(int k = 0; i < 3; ++k)
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
