#include <assert.h>

#include <opencv2/imgproc.hpp>
#include "venus/colorspace.h"
#include "venus/Beauty.h"
#include "venus/Effect.h"
#include "venus/Feature.h"
#include "venus/opencv_utility.h"
#include "venus/scalar.h"

using namespace cv;

namespace venus {
/*
	Refer to paper "Human Skin Colour Clustering for Face Detection" by Jure Kovac, Peter Peer, and Franc Solina.

	This is a simple yet powerful method to construct a skin classifier directly from the RGB composites which sets
	a number of rules (N) for skin colour likelihood. Kovacˇ et al. state that RGB components must not be close
	together, e.g., luminance elimination. They utilize the following rules: an R, G, B pixel is classified as skin
	if and only if
		at uniform daylight illumination:
		1. R > 95 and G > 40 and B > 20 and max(R, G, B) - min(R, G, B) > 15;  // RGB components must not be close
			together - greyness elimination
		2. abs(R - G) > 15;  // also R and G components must not be close together, otherwise we are not dealing 
			with the fair complexion
		3. R > G and R > B;  // R component must be the greatest component

		under flashlight or (light) daylight:
		1. R > 220 and G > 210 and B > 170
		2. abs(R - G) <= 15;  // R and G components must be close together
		3. R > B and G > B;   // B component must be the smallest component
*/
static bool isSkinColor_RGB(const uint8_t* color)
{
#if USE_BGRA_LAYOUT
	const uint8_t& b = color[0], &g = color[1], &r = color[2];
#else
	const uint8_t& r = color[0], &g = color[1], &b = color[2];
#endif

	if(r > 95 && g > 40 && b > 20 &&
		r - g > 15 && r - b > 15)
		return true;

	if(r > 220 && g > 210 && b > 170 &&
		r > b && g > b &&
		std::abs(r - g) <= 15)
		return true;

	return false;
}

static cv::Mat calculateSkinRegion(const cv::Mat& image, bool(*predicate)(const uint8_t*))
{
	// an RGB/RGBA image is required
	assert(image.depth() == CV_8U && image.isContinuous());

	const int channel = image.channels();
	assert(channel >= 3);
	const int length = image.rows * image.cols;
	const uint8_t* image_data = image.data;

	Mat mask(image.rows, image.cols, CV_8UC1);
	uint8_t* mask_data = mask.data;

	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
		mask_data[i] = predicate(image_data + i * channel) ? 255 : 0;
	
	return mask;
}

cv::Mat Beauty::calculateSkinRegion_RGB(const cv::Mat& image)
{
	Mat mask = calculateSkinRegion(image, isSkinColor_RGB);

#if 1
	// post-processing, use closing morphology operation to kick out small holes
	// the larger the radius, the more time it consumes, so no more than 2 here.
	int radius  = std::min(2, cvRound(std::max(image.rows, image.cols) * 0.01F));

	const int size = radius * 2 + 1;
	const Point2i anchor(radius, radius);
	int iterations = 1;  // can be tuned
	Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, Size(size, size), anchor);

	cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel, anchor, iterations);
#endif

	return mask;
}

/*
	"Pixel-Based Skin Color Classifier: A Review" by Amit Kumar and Shivani Malhotra

	高斯肤色概率模型：
	由于统计表明不同人种的肤色区别主要受亮度信息影响，而受色度信息的影响较小，所以直接考虑YCbCr空间的CbCr分量，
	映射为CbCr空间，在CbCr空间下，受亮度变化的影响少，且是两维独立分布。通过实践，选取大量肤色样本进行统计，发
	现肤色在CbCr空间的分布呈现良好的聚类特性。
		统计分布满足：77 <= Cb <= 127, 133 <= Cr <= 173

	根据肤色在色度空间的高斯分布，对根据肤色在色度空间的高斯分布，对于彩色图像中每个像素，将其从RGB色彩空间转换
	到YCbCr空间后，就可以计算该点属于皮肤区域的概率，即根据该点离高斯分布中心的远近得到和肤色的相似度，将彩色图
	像转化为灰度图，其中每个像素的灰度对应该点与肤色的相似度，相似度的计算公式如下
	P(CbCr) = exp(-0.5*(x-m)^T * C^-1 * (x-m))
	其中 m 为均值， m = E(x), C 为协方差矩阵， x = (CbCr)^T, C = E{(x-m)*((x-m)^T)}
	通过计算得到m和C的值如下：
		m = [117.4316 148.5599]
		C = [97.0946 24.4700; 24.4700 141.9966]
		inv(C) = [0.0107668  -0.0018554; -0.0018554   0.0073622]
*/
static float skinColorProbability(const uint8_t* color)
{
#if USE_BGRA_LAYOUT
	const uint8_t& B = color[0], &G = color[1], &R = color[2];
#else
	const uint8_t& R = color[0], &G = color[1], &B = color[2];
#endif

/*
	https://en.wikipedia.org/wiki/YUV#Conversion_to.2Ffrom_RGB
	the term YUV is commonly used in the computer industry to describe file-formats
	that are encoded using YCbCr.

	/ Y  \   /  65.481 128.553  24.966 \   / R \   /  16 \
	| Cb | = | -37.797 -74.203 112     | * | G | + | 128 |
	\ Cr /   \ 112     -93.786 -18.214 /   \ B /   \ 128 /

	to transform back...
	/ R \   /  0.00456621  0           0.00625893 \   / Y  \   /  16 \
	| G | = |  0.00456621 -0.00153632 -0.00318811 | * | Cb | - | 128 |
	\ B /   \  0.00456621  0.00791071  0          /   \ Cr /   \ 128 /

	tranform RGB to YUV color space. Here, we reduce 6 multiplications to 5.
*/
//	float U = (-0.148 * R - 0.291 * G + 0.439 * B + 128);
//	float V = ( 0.439 * R - 0.368 * G - 0.071 * B + 128);
	float Y = 0.299F * R + 0.587F * G + 0.114F * B + 16.0F;
	float U = 0.492F * (B - Y) + 128.0F;
	float V = 0.877F * (R - Y) + 128.0F;

	// [Cb-m0 Cr-m1] * [  0.0107668 -0.0018554 ] * [ Cb-m0 ]
	//                 [ -0.0018554  0.0073622 ]   [ Cr-m1 ]
	float tmp0 = U - 117.4316F;
	float tmp1 = V - 148.5599F;
	float tmp2 = +0.0107668F * tmp0 - 0.0018554F * tmp1;
	float tmp3 = -0.0018554F * tmp0 + 0.0073622F * tmp1;
	float tmp4 = tmp0 * tmp2 + tmp1 * tmp3;

	return std::exp(-0.5F * tmp4);
}

cv::Mat Beauty::calculateSkinRegion_YCbCr(const cv::Mat& image)
{
	// an RGB/RGBA image is required
	assert(image.depth() == CV_8U && image.isContinuous());

	const int channel = image.channels();
	assert(channel >= 3);
	const int length = image.rows * image.cols;
	const uint8_t* image_data = image.data;

	Mat mask(image.rows, image.cols, CV_32FC1);
	float* mask_data = mask.ptr<float>();

/*
	https://github.com/Itseez/opencv/blob/master/modules/imgproc/src/color.cpp#L7502
	Currently, there is a bug in conversion CV_BGR2YUV and CV_RGB2YUV,
	The channels red and blue are misplaced in the implemented formula.
	see http://code.opencv.org/issues/4227 for details.

	Otherwise, we could use the line below to convert RGB to YUV color mode.
	cvtColor(image_rgb, image_yuv, CV_BGR2YUV);
*/
	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
		mask_data[i] = skinColorProbability(image_data + i * channel);

#if 1  // equalization
	double min; // = *std::min_element(region.begin<float>(), region.end<float>());
	double max; // = *std::max_element(region.begin<float>(), region.end<float>());
//	auto min_max = std::minmax_element(region.begin<float>(), region.end<float>());
	cv::minMaxLoc(mask, &min, &max);
//	std::cout << "min = " << min << ", max = " << max << '\n';

	// binaryzation
//	float mean = (min + max)/2;
//	cv::threshold(region, region, mean/* threshold */, 1.0/* max_value */, THRESH_BINARY);

	if(min == max)  // single color in original image, bypass divided by zero.
		mask.convertTo(mask, CV_8UC1, 255.0F);
	else
	{
		// [min, max] maps to [0, 1], y = (x - min)/(max - min);
		// y = k*x + b; k = 1/(max -min); b = -min/(max -min);
		double k = 255/(max - min), b = -min * k;
		mask.convertTo(mask, CV_8UC1, k, b);
	}
//	cv::imshow("mask", mask);
#endif
	return mask;
}

// Decision boundary of HSV color space is defined as
// H = [0, 50], S = [0.20, 0.68], and V = [0.35, 1.0] 
static bool isSkinColor_HSV(const uint8_t* color)
{
#if USE_BGRA_LAYOUT
	const uint8_t& b = color[0], &g = color[1], &r = color[2];
#else
	const uint8_t& r = color[0], &g = color[1], &b = color[2];
#endif

	float rgb[3] = { r/255.0F, g/255.0F, b/255.0F };
	float hsv[3];
	rgb2hsv(rgb, hsv);

	return 0 <= hsv[0] && hsv[0] <= 50/360.0F &&
		0.20F < hsv[1] && hsv[1] <= 0.68F &&
		0.35F < hsv[2] && hsv[2] <= 1.00F;
}

cv::Mat Beauty::calculateSkinRegion_HSV(const cv::Mat& image)
{
	Mat mask = calculateSkinRegion(image, isSkinColor_HSV);
	return mask;
}

// <gegl>/operations/common/red-eye-removal.c
void redEyeReduction(float* color, const float& threshold)
{
#if USE_BGRA_LAYOUT
	float &b = color[0], &g = color[1], &r = color[2];
#else
	float &r = color[0], &g = color[1], &b = color[2];
#endif

	constexpr float RED_FACTOR   = 0.5133333F;
	constexpr float GREEN_FACTOR = 1.0000000F;
	constexpr float BLUE_FACTOR  = 0.1933333F;

	float adjusted_r = r * RED_FACTOR;
	float adjusted_g = g * GREEN_FACTOR;
	float adjusted_b = b * BLUE_FACTOR;
	float adjusted_t = threshold * 1.6F - 0.8F;

	if(adjusted_r >= adjusted_g - adjusted_t &&
	   adjusted_r >= adjusted_b - adjusted_t)
	{
		float tmp = (adjusted_g + adjusted_b) / (2 * RED_FACTOR);
		r = venus::clamp(tmp, 0.0F, 1.0F);
	}
	// Otherwise, leave the red channel alone
}

void Beauty::removeRedEye(cv::Mat& dst, const cv::Mat& src, const std::vector<cv::Point2f>& polygon, float threshold/* = 0.5F */)
{
	assert(src.channels() >= 3 && src.depth() != CV_64F);
	assert(0 <= threshold && threshold <= 1.0F);

	if(dst.data != src.data)
		src.copyTo(dst);

	Point2i position;
	const Mat mask = Feature::createMask(polygon, 0.0F, &position);
	
	Rect rect(position.x, position.y, mask.cols, mask.rows);
	Mat roi = dst(rect).clone();
	bool is_float_type = src.depth() == CV_32F;
	if(!is_float_type)
		roi.convertTo(roi, CV_32F, 1/255.0F);

	float* roi_data = roi.ptr<float>();
	const uint8_t* mask_data = roi.ptr<uint8_t>();
	const int channel = roi.channels();
	const int length = mask.rows * mask.cols;

	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
		if(mask_data[i] == 255)
			redEyeReduction(roi_data + i * channel, threshold);

	if(!is_float_type)
		roi.convertTo(roi, src.depth(), 255.0F);

	roi.copyTo(dst(rect));
}

void Beauty::whitenSkinByLogCurve(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, float level)
{
	assert(src.channels() == 4);
	assert(2 <= level && level <= 10);

	const int depth = src.depth();
	const int channels = src.channels();
	src.copyTo(dst);  // copy alpha channel if the source image has.

	if(depth == CV_8U)
	{
		// formular:  y = log(x*(amount - 1) + 1) / log(amount)
		const float denorm = std::log(level) / 255.0F;

		uint8_t table[256];
		for(int i = 0; i < 256; ++i)
			table[i] = static_cast<uint8_t>(log(i/255.0F *(level - 1) + 1) / denorm);

		assert(mask.type() == CV_8UC1);
		Effect::mapColor(dst, dst, mask.data, table);
	}
	else if(depth == CV_32F)
	{
		assert(mask.type() == CV_32FC1);
		float* dst_data = dst.ptr<float>();
		const float* mask_data = mask.ptr<float>();
		const int length = src.rows * src.cols;

		// formular:  y = log(x*(amount - 1) + 1) / log(amount)
		const float log_amount = std::log(level);

		#pragma omp parallel for
		for(int i = 0; i < length; ++i)
		{
			assert(0 <= mask_data[i] && mask_data[i] <= 1.0F);
			const int index = i * channels;
			for(int k = 0; k < 3; ++k)  // force compiler loop unrolling
				dst_data[index + k] = lerp(dst_data[index + k], std::log(dst_data[index + k] * (level - 1) + 1) / log_amount, mask_data[i]);
		}
	}
}

// Refer to paper "Digital Image Enhancement and Noise Filtering by Use of Local Statistics" by Jong-sen Lee, 1979
void Beauty::beautifySkin(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, float radius, float level)
{
	assert(src.rows == mask.rows && src.cols == mask.cols && mask.channels() == 1);
	double max;
	Mat _src  = venus::normalize(src, &max);
	Mat _mask = venus::normalize(mask);

//	level = 10 + level * level * 5;
	int size = cvRound(radius) * 2 + 1;

	// expectation: E[x] = (x1 + x2 + ... + xn)/n;  //x1*p1 + x2*p2 + ... + xn*pn;
	// variance: Var(X) = E[(X - miu)^2] = E[X^2] - E[X]^2
	Mat expectation, variance;
	cv::blur(_src, expectation, Size(size, size), Point(-1, -1), BorderTypes::BORDER_CONSTANT);

	dst = expectation - _src;
//	cv::multiply(dst, dst, dst);  // dst = dst .* dst;  // element-wise multiplication
	float* const dst_data = dst.ptr<float>();
	const int channel = src.channels();
	const int length = src.rows * src.cols * channel;

	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
		dst_data[i] *= dst_data[i];

	cv::blur(dst, variance, Size(size, size), Point(-1, -1), BorderTypes::BORDER_CONSTANT);
	
	const float* _src_data = _src.ptr<float>();
	const float* _mask_data = _mask.ptr<float>();
	const float* expectation_data = expectation.ptr<float>();
	const float* variance_data = variance.ptr<float>();

	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
	{
		float k = variance_data[i] / (variance_data[i] + level);
		float interp = lerp(expectation_data[i], _src_data[i], k);
//		dst_data[i] = interp;
		dst_data[i] = lerp(_src_data[i], interp, _mask_data[i/channel]);
	}

	dst.convertTo(dst, src.depth(), max);  // keep dst and src the same type
}

} /* namespace venus */