#include <stdint.h>

#include <opencv2/imgproc.hpp>

#include "venus/blur.h"
#include "venus/opencv_utility.h"
#include "venus/scalar.h"

using namespace cv;

namespace venus {

void gaussianBlur(cv::Mat& dst, const cv::Mat& src, float radius)
{
	assert(radius >= 0);
	int r = cvRound(radius);
	
	int width = (r << 1) + 1;
	double std_dev = radius * 3;  // 3-sigma rule https://en.wikipedia.org/wiki/68¨C95¨C99.7_rule

	cv::GaussianBlur(src, dst, Size(width, width), std_dev, std_dev, cv::BorderTypes::BORDER_CONSTANT);
}

void gaussianBlurSelective(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, float radius, float tolerance)
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
	float sum = 0.0F;
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
		sum += v * 4;
	}
	printf("kernel sum: %f\n", sum);
	//TODO scale all the values???

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


// Radius can be tuned, but dont't make the radius too large, since it's time-consuming.
static constexpr float BLUR_RADIUS = 8;

static cv::Vec4b getBlurredValue(const cv::Mat& image, const cv::Point2i& point, const float& blur_radius)
{
	assert(image.type() == CV_8UC4 && blur_radius >= 1.0F);
	int count = 0;
	Vec4d sum(0, 0, 0, 0);

	const int R = static_cast<int>(std::ceil(blur_radius));
	const int RR= static_cast<int>(blur_radius * blur_radius);

	for(int dy = -R; dy < R; ++dy)
	for(int dx = -R; dx < R; ++dx)
	{
		if(dx*dx + dy*dy > RR)
			continue;

		int c = point.x + dx, r = point.y + dy;
		if(c < 0 || c >= image.cols || r < 0 || r >= image.rows)
			continue;
				
		const Vec4b& color = image.at<Vec4b>(r, c);
		sum += static_cast<Vec4d>(color);
		++count;
	}

	Vec4d avg = sum / count;
	return Vec4b(
		saturate_cast<uint8_t>(avg[0]),
		saturate_cast<uint8_t>(avg[1]), 
		saturate_cast<uint8_t>(avg[2]),
		saturate_cast<uint8_t>(avg[3]));
}

void radialBlur(cv::Mat& dst, const cv::Mat& src, cv::Point2f& center, float inner_radius, float outer_radius)
{
	assert(0 < inner_radius && inner_radius < outer_radius);
	assert(src.data != dst.data);
	src.copyTo(dst);

	#pragma omp parallel for
	for(int r = 0; r < src.rows; ++r)
	for(int c = 0; c < src.cols; ++c)
	{
		Point2f point = Point2i(c, r);
		float distance = venus::distance(point, center);

#if DEBUG_BLUR
		// color values are the weight of blurring radius, so white means full blurring, black means no blurring.
		if(distance >= outer_radius)
			dst.at<Vec4b>(r, c) = Vec4b(255, 255, 255, 255);
		else if(distance >= inner_radius)
		{
			float t = (distance - inner_radius) / (outer_radius - inner_radius);
			uint8_t x = t * 253 + 1;  // map to interval [1, 254]
			dst.at<Vec4b>(r, c) = Vec4b(x, x, x, 255);
		}
		else  // since blur radius == 1 means no blurring.
			dst.at<Vec4b>(r, c) = Vec4b(0, 0, 0, 255);
#else
		if(distance >= outer_radius)
			dst.at<Vec4b>(r, c) = getBlurredValue(src, point, BLUR_RADIUS);
		else if(distance >= inner_radius)
		{
			float t = (outer_radius - distance) / (outer_radius - inner_radius);
			float blur_radius = t * (BLUR_RADIUS - 1.0F) + 1.0F;
			dst.at<Vec4b>(r, c) = getBlurredValue(src, point, blur_radius);
		}
//		else  // distance < inner_radius
//			dst.at<Vec4b>(r, c) = src.at<Vec4b>(r, c);
#endif
	}
}

void bilinearBlur(cv::Mat& dst, const cv::Mat& src, cv::Point2f& point0, cv::Point2f& point1, float band_width)
{
	assert(band_width > 0);
	assert(src.data != dst.data);
	src.copyTo(dst);

	Point2f center = (point1 + point0) / 2.0F;
	Vec2f v01 = point1 - point0;
	Vec2f N = normalize(v01);

	// (x, y) perpendicular to (y, -x) or (-y, x)
	// line are the perpendicular bisector of line segment @p point0 and @p point1.
	Vec4f line(-N[1], N[0], center.x, center.y);

	float half_length  = std::sqrt(v01.dot(v01))/2;
	float inner_radius = half_length - band_width/2;
	float outer_radius = half_length + band_width/2;

	#pragma omp parallel for
	for(int r = 0; r < src.rows; ++r)
	for(int c = 0; c < src.cols; ++c)
	{
		Point2f point = Point2i(c, r);
		float distance = venus::distance(point, line);
#if DEBUG_BLUR
		// color values are the weight of blurring radius, so white means full blurring, black means no blurring.
		distance = venus::clamp(distance, inner_radius, outer_radius);
		float t = (distance - inner_radius) / (outer_radius - inner_radius);
		uint8_t x = t * 255;
		dst.at<Vec4b>(r, c) = Vec4b(x, x, x, 255);
#else
		if(distance >= outer_radius)
			dst.at<Vec4b>(r, c) = getBlurredValue(src, point, BLUR_RADIUS);
		else if(distance >= inner_radius)
			dst.at<Vec4b>(r, c) = getBlurredValue(src, point, distance / outer_radius * BLUR_RADIUS);
		else  // distance < inner_radius
			dst.at<Vec4b>(r, c) = src.at<Vec4b>(r, c);
#endif
	}
}

} /* namespace venus */