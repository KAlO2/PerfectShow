#include "venus/Makeup.h"

#include "venus/blend.h"
#include "venus/Feature.h"
#include "venus/opencv_utility.h"
#include "venus/Scalar.h"

#include <opencv2/imgproc.hpp>

#include <omp.h>

using namespace cv;

namespace venus {

cv::Mat Makeup::pack(uint32_t color, const cv::Mat& gray)
{
	assert(gray.type() == CV_8UC1);
	cv::Mat image(gray.rows, gray.cols, CV_8UC4);

	const int length = gray.rows * gray.cols;
	const uint8_t* gray_data = gray.data;
	uint32_t* image_data = reinterpret_cast<uint32_t*>(image.data);

	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
	{
		uint8_t alpha = ((color >> 24) * gray_data[i] + 127) / 255;
#if USE_OPENCV_BGRA_LAYOUT
		// Swap R and B channel, then assembly it to BGRA format.
		image_data[i] = ((color >> 16) & 0xff) | (color &0x00ff00) | ((color & 0xff) << 16) | (alpha << 24);
#else
		image_data[i] = (color & 0x00FFFFFF) | (alpha << 24);
#endif
	}

	return image;
}

std::vector<cv::Point2f> Makeup::createShape(const std::vector<cv::Point2f>& points, BlushShape shape, bool right)
{
	assert(points.size() == Feature::COUNT);
	
	const Point2f& _02 = points[right ?  2:10];
	const Point2f& _62 = points[right ? 62:58];
	const Point2f& _00 = points[right ?  0:12];
	const Point2f& _01 = points[right ?  1:11];
	const Point2f& _33 = points[right ? 33:32];
	const Point2f& _41 = points[right ? 41:51];
	const Point2f& _61 = points[right ? 61:59];
	const Point2f& _03 = points[right ?  3: 9];
	const Point2f& _63 = points[right ? 63:69];

	switch(shape)
	{
	case BlushShape::DEFAULT:
		return Feature::calculateBlushPolygon(points, right);
		break;

	case BlushShape::DISK:
	{
		Point2f center((_62.x + _02.x)/2, _62.y);
		float radius = std::abs(_62.x - _02.x)/2;
		const int N = 12;  // can be tuned
		std::vector<Point2f> circle(N);
		for(int i = 0; i < N; ++i)
		{
			float t = i/static_cast<float>(2 * M_PI) * N;
			circle[i] = center + radius * Point2f(std::cos(t), std::sin(t));
		}
		
		return circle;
	}
		break;

	case BlushShape::OVAL:
		return std::vector<Point2f>
		{
			(_00 + _01*2)/3,
			_01,
			(_01*2 + _02)/3,
			(_01 + _02*2)/3,
			Point2f(_33.x, _61.y),
			_62,
			Point2f(_41.x, points[53].y)
		};
		break;

	case BlushShape::TRIANGLE:
	{
		Point2f down = points[56] - points[53];
		down /= (down.x*down.x + down.y*down.y);
		return std::vector<Point2f>
		{
			Point2f(_33.x, _62.y),
			(_02 + _03)/2,
			_02,
			catmullRomSpline(2.0f/3, _00, _01, _02, _03),
			catmullRomSpline(1.0f/3, _00, _01, _02, _03),
			_01,
			(_00 + _01*2)/3,
		};
	}
		break;

	case BlushShape::HEART:
	{
		Point2f center((_62.x + _02.x)/2, (points[53].y + points[56].y)/2);
		float radius = std::abs(_62.x - _02.x)/2;

		const int N = 32;
		std::vector<Point2f> heart(N);

		// http://mathworld.wolfram.com/HeartCurve.html
		// x = 16 * sin(t)^3
		// y = 13 * cos(t) - 5 * cos(2*t) - 2 * cos(3*t) - cos(4*t)
		// where parameter t in range [0 : 2*pi]
		//
		// cos(2*t) = cos(t)^2 - sin(t)^2
		// cos(3*t) = 4*cos(t)^3 - 3*cos(t)
		// cos(4*t) = 2*sin(2*t)*cos(2*t)
		for(int i = 0; i < N; ++i)
		{
			float t = i * static_cast<float>(2*M_PI / N);
			float sint  = std::sin(t), cost = std::cos(t);
			float sin2t = 2*sint*cost, cos2t = cost*cost - sint*sint;
			float cos3t = cost*cos2t - sint*sin2t;
			float cos4t = cos2t*cos2t - sin2t*sin2t;
			float x = sint * sint * sint;
			float y = (13*cost - 5*cos2t - 2*cos3t - cos4t) / -16;
			// A negative sign makes Y up coordinates to Y down coordinates.

			heart[i] = center + radius * Point2f(x, y);
		}
		return heart;
	}
		break;

	case BlushShape::SEAGULL:
	{
		const uint8_t knot_r[5] = { 42, 22, 23, 24, 25 };
		const uint8_t knot_l[5] = { 43, 29, 30, 31, 26 };
		const uint8_t* knot = right? knot_r:knot_l;
		
		// Feature::getSymmetryAxis() is expensive
		Point2f down = points[56] - points[53];
		down /= std::sqrt(down.x*down.x + down.y*down.y);

		const int N = 10;
		std::vector<Point2f> seagull(N);
		seagull[0] = _01;
		seagull[5] = points[right?54:52];

		const Point2f& carriage = points[knot[0]];
		for(int i = 1; i < 5; ++i)
		{
			const Point2f& point = points[knot[i]];
			Point2f d = carriage - point;
			float dot = d.x * down.x + d.y * down.y;  // project on vector down
			seagull[i]    = point + 3*(dot * down);
			seagull[10-i] = point + 2*(dot * down);
		}
		
		return seagull;
	}
		break;

	default:
		assert(false);
		return std::vector<cv::Point2f>();
	}
}

void Makeup::blend(cv::Mat& dst, const cv::Mat& src, const cv::Point2i& origin, float amount)
{
	assert(src.channels() == 4 && dst.channels() == 4 && src.depth() == dst.depth());
	
	Rect rect_src(origin.x, origin.y, src.cols, src.rows);
	Rect rect_dst(0, 0, dst.cols, dst.rows);
	Rect rect = rect_dst & rect_src;

	switch(dst.type())
	{
	case CV_8UC4:
		for(int r = rect.y, r_end = rect.y + rect.height; r < r_end; ++r)
		for(int c = rect.x, c_end = rect.x + rect.width;  c < c_end; ++c)
		{
			const cv::Vec4b& src_color = src.at<cv::Vec4b>(r - origin.y, c - origin.x);
			cv::Vec4b& dst_color = dst.at<cv::Vec4b>(r, c);

			dst_color = venus::mix(dst_color, src_color, amount);
		}
		break;
#if 0  // currently unused case
	case CV_32FC3:
		for(int r = rect.y, r_end = rect.y + rect.height; r < r_end; ++r)
		for(int c = rect.x, c_end = rect.x + rect.width;  c < c_end; ++c)
		{
			const cv::Vec3f& src_color = src.at<cv::Vec3f>(r - origin.y, c - origin.x);
			cv::Vec3f& dst_color = dst.at<cv::Vec3f>(r, c);

			dst_color = venus::mix(dst_color, src_color, amount);
		}
		break;
#endif
	case CV_32FC4:
		for(int r = rect.y, r_end = rect.y + rect.height; r < r_end; ++r)
		for(int c = rect.x, c_end = rect.x + rect.width;  c < c_end; ++c)
		{
			const cv::Vec4f& src_color = src.at<cv::Vec4f>(r - origin.y, c - origin.x);
			cv::Vec4f& dst_color = dst.at<cv::Vec4f>(r, c);

			dst_color = venus::mix(dst_color, src_color, amount);
		}
		break;
	default:
		assert(false);
		break;
	}
}

void Makeup::blend(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, const cv::Point2i& origin, float amount)
{
	assert(src.channels() == dst.channels() && src.depth() == dst.depth());
	assert(mask.type() == CV_8UC1);

	Rect rect_src(origin.x, origin.y, src.cols, src.rows);
	Rect rect_dst(0, 0, dst.cols, dst.rows);
	Rect rect = rect_dst & rect_src;

	Rect2i rect_mask(0, 0, mask.cols, mask.rows);
	int offset_x = (src.cols - mask.cols)/2;
	int offset_y = (src.rows - mask.rows)/2;

	for(int r = rect.y, r_end = rect.y + rect.height; r < r_end; ++r)
	for(int c = rect.x, c_end = rect.x + rect.width;  c < c_end; ++c)
	{
		const int src_r = r - origin.y, src_c = c - origin.x;
		Point2i mask_position(src_c - offset_x, src_r - offset_y);
		if(!rect_mask.contains(mask_position) || mask.at<uchar>(mask_position) != 0)
			continue;

		const cv::Vec4b& src_color = src.at<cv::Vec4b>(src_r, src_c);

		switch(dst.type())
		{
		case CV_8UC4:
		{
			const cv::Vec4b& src_color = src.at<cv::Vec4b>(src_r, src_c);
			cv::Vec4b& dst_color = dst.at<cv::Vec4b>(r, c);

			dst_color = venus::mix(dst_color, src_color, amount);
		}
			break;
		case CV_32FC3:
		{
			const cv::Vec3f& src_color = src.at<cv::Vec3f>(src_r, src_c);
			cv::Vec3f& dst_color = dst.at<cv::Vec3f>(r, c);

			dst_color = venus::mix(dst_color, src_color, amount);
		}
			break;
		case CV_32FC4:
		{
			const cv::Vec4f& src_color = src.at<cv::Vec4f>(src_r, src_c);
			cv::Vec4f& dst_color = dst.at<cv::Vec4f>(r, c);

			dst_color = venus::mix(dst_color, src_color, amount);
		}
			break;
		default:
			assert(false);
			break;
		}
	}
}

void Makeup::applyBlush(cv::Mat& dst, const cv::Mat& src, const std::vector<cv::Point2f>& points, BlushShape shape, uint32_t color, float amount)
{
	assert(!src.empty() && points.size() == Feature::COUNT);
	assert(0 <= amount && amount <= 1);

	// Note that src.copyTo(dst); will invoke dst.create(src.size(), src.type());
	// which has this clause if( dims <= 2 && rows == _rows && cols == _cols && type() == _type && data ) return;
	// which means that dst's memory will only be allocated the first time in if dst is empty.
	src.copyTo(dst);

	for(int i = 0; i <= 1; ++i)  // i == 0 for left cheek, i == 1 for right cheek
	{
		// static_cast<bool>(i) emits warning "C4800: 'int' : forcing value to bool 'true' or 'false' (performance warning)".
		// But why it says performance warning?
		// http://stackoverflow.com/questions/206564/what-is-the-performance-implication-of-converting-to-bool-in-c
		std::vector<Point2f> polygon = createShape(points, shape, i != 0);

		Rect rect = cv::boundingRect(polygon);
		Mat  mask = Feature::maskPolygonSmooth(rect, polygon, 8);  // level (here 8) can be tuned.
		Mat blush = pack(color, mask);
		
		blend(dst, blush, rect.tl(), amount);
	}
}

/*
void Makeup::applyLipColor(const uint32_t* dst, const uint32_t* const src, int width, int height, const Region& region, uint32_t color, float amount)
{
	Rect rect(0, 0, width, height);
	const Rect rect_mask = region.getRect();
	rect &= rect_mask;
	
	if(rect.area() <= 0)  // not overlap
		return;

	for(int r = rect.y, r_end = rect.y + rect.height; r < r_end; ++r)
	for(int c = rect.x, c_end = rect.x + rect.width;  c < c_end; ++c)
	{
		uint8_t mask = region.mask.at<uint8_t>(r - rect_mask.x, c - rect_mask.y);

		// skip transparent area
		if(mask == 0)
			continue;


	}
}
*/

void Makeup::applyLipColor(cv::Mat& dst, const cv::Mat& src, const Region& region, uint32_t color, float amount)
{
	assert(!src.empty() && src.channels() == 4);  // only handles RGBA image
	src.copyTo(dst);
	
	Rect rect(0, 0, src.cols, src.rows);
	const Rect rect_mask = region.getRect();
	rect &= rect_mask;
	
	if(rect.area() <= 0)  // not overlap
		return;

	// SRC_OVER mode, [Sa + (1 - Sa)*Da, Rc = Sc + (1 - Sa)*Dc]
	const float l_amount = 1 - amount;
	if(src.type() == CV_8UC4)
	{
		float color_0_x_amount = (color      &0xff) * amount;
		float color_1_x_amount = ((color>> 8)&0xff) * amount;
		float color_2_x_amount = ((color>>16)&0xff) * amount;
		float color_3_x_amount = ((color>>24)&0xff) * amount;

//		uint8_t color_0 = color, color_1 = color>>8, color_2 = color>>16, color_3 = color>>24;
		for(int r = rect.y, r_end = rect.y + rect.height; r < r_end; ++r)
		for(int c = rect.x, c_end = rect.x + rect.width;  c < c_end; ++c)
		{
			uint8_t mask = region.mask.at<uint8_t>(r - rect_mask.y, c - rect_mask.x);
			if(mask == 0)  // skip transparent area
				continue;

			const Vec4b& src_color = src.at<Vec4b>(r, c);
//			uint8_t src_0 = src_color, src_1 = src_color>>8, src_2 = src_color>>16, src_3 = src_color>>24;
//			uint32_t mixed = mix(src_color, color, amount);
			uint8_t _0 = saturate_cast<uint8_t>((src_color[0] & 0xff) * l_amount + color_0_x_amount);
			uint8_t _1 = saturate_cast<uint8_t>((src_color[1] & 0xff) * l_amount + color_1_x_amount);
			uint8_t _2 = saturate_cast<uint8_t>((src_color[2] & 0xff) * l_amount + color_2_x_amount);
			uint8_t _3 = saturate_cast<uint8_t>((src_color[3] & 0xff) * l_amount + color_3_x_amount);

			uint8_t dst_0 = (src_color[0] * (255 - mask) + _0 * mask + 127) / 255;
			uint8_t dst_1 = (src_color[1] * (255 - mask) + _1 * mask + 127) / 255;
			uint8_t dst_2 = (src_color[2] * (255 - mask) + _2 * mask + 127) / 255;
			uint8_t dst_3 = _3; // (src_color[3] * (255 - mask) + _3 * mask + 127) / 255;

//			uint32_t dst_color = dst_0 || (dst_1 << 8) || (dst_2 << 16) || (dst_3 << 24);
			dst.at<Vec4b>(r, c) = Vec4b(dst_0, dst_1, dst_2, dst_3);
		}
	}
	else if(src.type() == CV_32FC4)
	{
		float color_0_x_amount = (color      &0xff) * (amount / 255.0f);
		float color_1_x_amount = ((color>> 8)&0xff) * (amount / 255.0f);
		float color_2_x_amount = ((color>>16)&0xff) * (amount / 255.0f);
		float color_3_x_amount = ((color>>24)&0xff) * (amount / 255.0f);
		for(int r = rect.y, r_end = rect.y + rect.height; r < r_end; ++r)
		for(int c = rect.x, c_end = rect.x + rect.width;  c < c_end; ++c)
		{
			float mask = region.mask.at<float>(r - rect_mask.y, c - rect_mask.x);
			float l_mask = 1 - mask;
			const Vec4f& src_color = src.at<Vec4f>(r, c);
			float _0 = src_color[0] * l_amount + color_0_x_amount;
			float _1 = src_color[1] * l_amount + color_1_x_amount;
			float _2 = src_color[2] * l_amount + color_2_x_amount;
			float _3 = src_color[3] * l_amount + color_3_x_amount;

			Vec4f& dst_color = dst.at<Vec4f>(r, c);
			dst_color[0] = src_color[0] * l_mask + _0 * mask;
			dst_color[1] = src_color[1] * l_mask + _1 * mask;
			dst_color[2] = src_color[2] * l_mask + _2 * mask;
			dst_color[3] = src_color[3] * l_mask + _3 * mask;
		}
	}
	else
		assert(false);  // unimplemented yet
}

} /* namespace venus */