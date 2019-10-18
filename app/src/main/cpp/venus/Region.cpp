#include "venus/Effect.h"
#include "venus/opencv_utility.h"
#include "venus/Region.h"

#include <stdint.h>

#include <opencv2/imgproc.hpp>

#include "venus/compiler.h"

using namespace cv;

namespace venus {

Region::Region(cv::Point2f& pivot, cv::Size2f& size, cv::Mat& mask):
	pivot(pivot),
	size(size),
	mask(mask)
{
}

cv::Rect Region::getRect(const cv::Point2f& pivot, const cv::Size2f& size)
{
	int left = static_cast<int>(pivot.x - size.width/2);
	int top  = static_cast<int>(pivot.y - size.height/2);
	int width = cvRound(size.width);
	int height = cvRound(size.height);
	
	return Rect(left, top, width, height);
}

cv::Rect Region::getRect() const
{
	return getRect(pivot, size);
}

Region Region::merge(const Region& region1, const Region& region2)
{
	const Mat& mask1 = region1.mask, &mask2 = region2.mask;
	Rect2f rect1(region1.pivot - Point2f(mask1.cols, mask1.rows)/2, mask1.size());
	Rect2f rect2(region2.pivot - Point2f(mask2.cols, mask2.rows)/2, mask2.size());
	Rect2f rect = rect1 | rect2;

	rect1.x -= rect.x; rect1.y -= rect.y;
	rect2.x -= rect.x; rect2.y -= rect.y;
	Rect2i rect1_ = rect1, rect2_ = rect2;

	Point2f pivot = (region1.pivot + region2.pivot)/2;
	Size2f  size = rect.size();
	Mat     mask(rect.size(), CV_8UC1, Scalar(0));
	mask1.copyTo(mask(rect1_));
	mask2.copyTo(mask(rect2_));  // overlapping area will be overridden
	return Region(pivot, size, mask);
}

cv::Mat Region::resize(const cv::Mat& image, const Point2f& pivot,
	float left_scale, float top_scale, float right_scale, float bottom_scale,
	int interpolation/* = INTER_LINEAR */)
{
	int pivot_x = cvRound(pivot.x), pivot_y = cvRound(pivot.y);
//	printf("pivot: %d %d, image(%dx%d)", pivot_x, pivot_y, image.cols, image.rows);
	assert(0 <= pivot_x && pivot_x < image.cols);
	assert(0 <= pivot_y && pivot_y < image.rows);

	cv::Mat top, bottom;  // seperate the whole image vertically
	image(Rect2i(0, 0, image.cols, pivot_y)).copyTo(top);
	image(Rect2i(0, pivot_y, image.cols, image.rows - pivot_y)).copyTo(bottom);

	const cv::Size EMPTY(0, 0);
	cv::resize(top, top, EMPTY, 1.0f, top_scale, interpolation);
	cv::resize(bottom, bottom, EMPTY, 1.0f, bottom_scale, interpolation);

	cv::Mat result;
	cv::vconcat(top, bottom, result);

	cv::Mat left, right;  // seperate the whole image horizontally
	result(Rect2i(0, 0, pivot_x, result.rows)).copyTo(left);
	result(Rect2i(pivot_x, 0, result.cols - pivot_x, result.rows)).copyTo(right);

	cv::resize(left, left, EMPTY, left_scale, 1.0f, interpolation);
	cv::resize(right, right, EMPTY, right_scale, 1.0f, interpolation);

	cv::hconcat(left, right, result);
	return result;
}

cv::Mat Region::inset(const cv::Mat& mat, int offset)
{
	assert(!mat.empty());

	Rect rect(0, 0, mat.cols, mat.rows);
	inset(rect, offset);

	if(rect.x >= 0)  // rect.x == rect.y
		return mat(rect).clone();

	Rect rect2(-offset, -offset, mat.cols, mat.rows);
	Mat mat2(rect.size(), mat.type(), Scalar::all(0));
	mat.copyTo(mat2(rect2));
	return mat2;
}

void Region::inset(float offset)
{
	pivot.x -= offset;
	pivot.y -= offset;

	if(offset > std::numeric_limits<float>::epsilon())
		mask = inset(mask, offset);
}

cv::Rect2i Region::boundingRect(const cv::Mat& mask, int tolerance/* = 0 */)
{
	assert(!mask.empty() && mask.depth() == CV_8U);

	std::vector<uint8_t> row(mask.cols, 0), col(mask.rows, 0);
	const uint8_t* p = mask.ptr<uint8_t>();
	const int channel = mask.channels();
	for(int r = 0; r < mask.rows; ++r)
	for(int c = 0; c < mask.cols; ++c)
	{
		// single channel is alpha, multiple channels select the last channel as alpha.
		uint8_t value = p[(r * mask.cols + c + 1) * channel - 1];
//		uint8_t value = mask.at<uint8_t>(r, c);
		if(value <= tolerance)
			continue;

		row[c] |= value;
		col[r] |= value;
	}

#if 0
/*
	// http://stackoverflow.com/questions/24425127/explaining-a-string-trimming-function
	template< class Iterator >
	constexpr std::reverse_iterator<Iterator> make_reverse_iterator( Iterator i )
	{
		return std::reverse_iterator<Iterator>(i);
	}

	std::make_reverse_iterator comes from C++14, so you can write
		std::make_reverse_iterator(left)
	instead of
		std::reverse_iterator<decltype(left)>(left)
*/
	auto predicate = [](uint8_t ch) { return ch == 0; };
	auto left   = std::find_if_not(row.begin(),  row.end(),  predicate);
	auto right  = std::find_if_not(row.rbegin(), std::reverse_iterator<decltype(left)>(left), predicate);
	auto top    = std::find_if_not(col.begin(),  col.end(),  predicate);
	auto bottom = std::find_if_not(col.rbegin(), std::reverse_iterator<decltype(top)>(top), predicate);
	return cv::Rect2i(std::distance(row.begin(), left), std::distance(col.begin(), top),
		std::distance(left, right.base()), std::distance(top, bottom.base()))
#else
	int left = mask.rows, right = 0;
	int top = mask.cols, bottom = 0;
	for(int i = 0; i < mask.cols; ++i)         if(row[i] != 0) { left   = i; break; }
	for(int i = mask.cols - 1; i >= left; --i) if(row[i] != 0) { right  = i; break; }
	for(int i = 0; i < mask.rows; ++i)         if(col[i] != 0) { top    = i; break; }
	for(int i = mask.rows - 1; i >= top; --i)  if(col[i] != 0) { bottom = i; break; }

	if(left > right || top > bottom)  // In fact, arbitrary one condition should be enough.
		return Rect2i(0, 0, 0, 0);    // Empty rectangle means the input mask is all zeros!

	int width = right - left + 1, height = bottom - top + 1;
	return cv::Rect2i(left, top, width, height);
#endif
}

template <typename T>
T colorDifference(const T* color, const T* reference_color, int channel, bool has_alpha,
		T threshold, SelectCriterion criterion, bool antialias,  bool select_transparent)
{
	// std::is_unsigned also returns true for bool type, so kick it out.
	static_assert(std::is_floating_point<T>::value ||
			(std::is_unsigned<T>::value && !std::is_same<T, bool>::value), "limit to integral/floating primitive type");
	assert(criterion == SelectCriterion::COMPOSITE || channel >= 3);  // RGB or HSV mode have 3 channels.
	constexpr T ZERO(0), FULL(std::is_floating_point<T>::value?T(1):std::numeric_limits<T>::max());
	assert(0 < channel && channel <= 4);
	assert(ZERO <= threshold && threshold <= FULL);

	// if there is an alpha channel, never select transparent regions
	if(!select_transparent && has_alpha && color[channel - 1] == ZERO)
		return ZERO;
	
	T max = ZERO;
	if(select_transparent && has_alpha)
	{
		int channel_a = channel - 1;
		max = std::abs(color[channel_a] - reference_color[channel_a]);
	}
	else
	{
		if(has_alpha)
			--channel;
		
		switch(criterion)
		{
		case SelectCriterion::COMPOSITE:
			for(int c = 0; c < channel; ++c)
			{
				T diff = std::abs(color[c] - reference_color[c]);
				if(diff > max)
					max = diff;
			}
			break;
		
		case SelectCriterion::RED:
			max = std::abs(color[0] - reference_color[0]);
			break;
		
		case SelectCriterion::HUE:
			max = std::abs(color[0] - reference_color[0]);

			// Note that for uint8_t RGBA type, need to use CV_RGB2HSV_FULL instead of CV_RGB2HSV.
			max = std::min<T>(max, FULL - max);
			break;
		
		case SelectCriterion::GREEN:
		case SelectCriterion::SATURATION:
			max = std::abs(color[1] - reference_color[1]);
			break;
		
		case SelectCriterion::BLUE:
		case SelectCriterion::VALUE:
			max = std::abs(color[2] - reference_color[2]);
			break;
		
		case SelectCriterion::ALPHA:
			max = std::abs(color[3] - reference_color[3]);
			break;
		
		default:
			assert(false);
		}
	}

	if(antialias && threshold > ZERO)
	{
		float x = 1.5F - max / static_cast<float>(threshold);
//		return clamp(x, ZERO, FULL/2) * 2;
		if(x <= ZERO)
			return ZERO;
		else if(x < 0.5F)
			return static_cast<T>(x * (2 * FULL));
		else
			return FULL;
	}
	else
		return (max > threshold) ? ZERO : FULL;
}

void Region::selectContiguousRegionByColor(cv::Mat& mask, const cv::Mat& image, const cv::Vec4b& color, uint8_t threshold, 
		SelectCriterion criterion, bool select_transparent, bool antialias)
{
	assert(image.depth() == CV_8U);
	mask.create(image.rows, image.cols, CV_8UC1);

	int nb_channel = image.channels();
	bool has_alpha = nb_channel >= 4;  // TODO: cope with G8A8 format

	if(has_alpha)
	{
		if(select_transparent)
		{
			// don't select transparancy if "color" isn't fully transparent
			if(color[nb_channel - 1] > 0)
				select_transparent = false;
		}
	}
	else
		select_transparent = false;

	uint8_t* _image_color = reinterpret_cast<uint8_t*>(image.data);
	uint8_t*   mask_color = reinterpret_cast<uint8_t*>(mask.data);
	const uint8_t* reference_color = &color[0];
	const int length = image.rows * image.cols;

	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
		mask_color[i] = colorDifference(_image_color + i * nb_channel, reference_color, nb_channel, has_alpha,
				threshold, criterion, antialias,  select_transparent);
}

void Region::selectContiguousRegionByColor(cv::Mat& mask, const cv::Mat& image, const cv::Vec4f& color, float threshold, 
		SelectCriterion criterion, bool select_transparent, bool antialias)
{
	assert(image.depth() == CV_32F);
	assert(0.0F <= threshold && threshold <= 1.0F);
	mask.create(image.rows, image.cols, CV_32FC1);

	int nb_channel = image.channels();
	bool has_alpha = nb_channel >= 4;  // TODO: cope with G8A8 format

	if(has_alpha)
	{
		if(select_transparent)
		{
			// don't select transparancy if "color" isn't fully transparent
			if(color[nb_channel - 1] > 0.0F)
				select_transparent = false;
		}
	}
	else
		select_transparent = false;
	
	float* _image_color = reinterpret_cast<float*>(image.data);
	float*   mask_color = reinterpret_cast<float*>(mask.data);
	const float* reference_color = &color[0];
	const int length = image.rows * image.cols;

	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
		mask_color[i] = colorDifference(_image_color + i * nb_channel, reference_color, nb_channel, has_alpha,
				threshold, criterion, antialias,  select_transparent);
}

void Region::shrink(cv::Mat& dst, const cv::Mat& src, int offset)
{
	if(src.data != dst.data)
		src.copyTo(dst);
	if(offset == 0)  // shortcut
		return;

	int radius = std::abs(offset);
	int size   = radius * 2 + 1;
	Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, Size(size, size), Point(radius, radius));

	// Apply the specified morphology operation
	const Point anchor(-1, -1);  // default value (-1, -1) means that the anchor is at the element center.
	int iterations = 1;
	if(offset > 0)
		cv::erode(src, dst, kernel, anchor, iterations);
	else
		cv::dilate(src, dst, kernel, anchor, iterations);
}

void Region::grow(cv::Mat& dst, const cv::Mat& src, int offset)
{
	shrink(dst, src, -offset);
}

void Region::overlay(cv::Mat& dst, const cv::Mat& patch, const cv::Point2i& position, const cv::Mat& mask)
{
	assert(patch.size() == mask.size());
	Rect rect(position, mask.size());
	patch.copyTo(dst(rect), mask);
}

cv::Mat Region::transform(cv::Size& size, cv::Point2f& pivot, const float& angle, const cv::Vec2f& scale/* = cv::Vec2f(1.0f, 1.0f) */)
{
/*
	@see http://stackoverflow.com/questions/22041699/rotate-an-image-without-cropping-in-opencv-in-c
		cv::Mat matrix = cv::getRotationMatrix2D(pivot, angle, scale);
		cv::Rect rect = cv::RotatedRect(pivot, size, angle).boundingRect();

	getRotationMatrix2D is fine, but I need to tweak this function for anisotropical scaling.
	Y is top down, so a clockwise rotating `angle` means a counter-clockwise rotating `-angle` in Y bottom up coordinate.

	/ x' \  = / cos(angle)  -sin(angle) \ . / scale[0]     0     \ . / 1 0  -pivot.x \ . / x \
	\ y' /    \ sin(angle)   cos(angle) /   \    0      scale[1] /   \ 0 1  -pivot.y /   \ y /

	let
		ca_s0 = cos(angle) * scale[0];  sa_s1 = sin(angle) * scale[1];
		sa_s0 = sin(angle) * scale[0];  ca_s1 = cos(angle) * scale[1];

	and the affine matrix is:

		/ ca_s0  -sa_s1  -ca_s0*pivot.x+sa_s1*pivot.y \
		\ sa_s0   ca_s1  -sa_s0*pivot.x-ca_s1*pivot.y /
*/
	float c = std::cos(angle), s = std::sin(angle);

    Mat affine(2, 3, CV_32F);
    float* m = affine.ptr<float>();

    m[0] = c * scale[0];  m[1] =-s * scale[1];  m[2] = m[0] * -pivot.x + m[1] * -pivot.y;
    m[3] = s * scale[0];  m[4] = c * scale[1];  m[5] = m[3] * -pivot.x + m[4] * -pivot.y;
    
	assert(size.width > 1 && size.height > 1);  // in case underflow
	if(size.width <= 1 && size.height <= 1)
	{
		m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
		m[0] = 0.0f; m[1] = 1.0f; m[2] = 0.0f;
		return affine;
	}

	float x = (size.width > 1)? size.width - 1 : 0;
	float y = (size.height > 1)? size.height - 1 : 0;
	std::vector<Point2f> points{ pivot, Point2f(0, 0), Point2f(x, 0), Point2f(0, y), Point2f(x, y) };
	cv::transform(points, points, affine);
	
	Rect rect = cv::boundingRect(points);
	pivot = points[0];

	// make all the stuff relative to origin
	m[2] -= rect.x;  pivot.x -= rect.x;
	m[5] -= rect.y;  pivot.y -= rect.y;
	size.width = rect.width;
	size.height= rect.height;

	return affine;
}

cv::Point2f Region::transform(const cv::Mat& affine, const cv::Point2f& point)
{
	assert(affine.rows == 2 && affine.cols == 3);
	const float* m = affine.ptr<float>();
	return Point2f(
		m[0] * point.x + m[1] * point.y + m[2],
		m[3] * point.x + m[4] * point.y + m[5]);
}

cv::Mat Region::invert(const cv::Mat& mat)
{
	assert(mat.rows == 2 && mat.cols == 3);

	cv::Mat result;
	cv::invertAffineTransform(mat, result);
	return result;
}

void Region::snake(const cv::Mat1f& image, std::vector<cv::Point2f>& points, float alpha, float beta, float gamma, float kappa, const cv::Vec3f& weight, int nb_iteration)
{
	assert(image.type() == CV_32FC1);

	Mat e_line = image;  // e_line is simply the image's intensity
	Mat e_edge = Mat::ones(image.rows, image.cols, CV_32FC1);
	Mat e_term = Mat::ones(image.rows, image.cols, CV_32FC1);

	for(int r = 1, r_end = image.rows - 1; r < r_end; ++r)
	for(int c = 1, c_end = image.cols - 1; c < c_end; ++c)
	{
		// (r-1, c-1)  (r-1, c)  (r-1, c+1)  ||  _00  _01  _02
		// (r  , c-1)  (r  , c)  (r  , c+1)  ||  _10  _11  _12
		// (r+1, c-1)  (r+1, c)  (r+1, c+1)  ||  _20  _21  _22
		const float& _00 = image.at<float>(r-1, c-1), &_01 = image.at<float>(r-1, c), &_02 = image.at<float>(r-1, c+1);
		const float& _10 = image.at<float>(r  , c-1), &_11 = image.at<float>(r  , c), &_12 = image.at<float>(r  , c+1);
		const float& _20 = image.at<float>(r+1, c-1), &_21 = image.at<float>(r+1, c), &_22 = image.at<float>(r+1, c+1);

		float gradient_x = _12 - _10;
		float gradient_y = _21 - _01;
		e_edge.at<float>(r, c) = std::sqrt(gradient_x * gradient_x + gradient_y *gradient_y);

		float dx = _12 - _11;  // [-1, 1]
		float dy = _21 - _11;  // [-1; 1]
		float dxx= (_12 - _11) - (_11 - _10);  // [-1, -2, 1]
		float dyy= (_21 - _11) - (_11 - _01);  // [-1; -2; 1]
//		float dxy= (_11 - _12) + (_22 - _21);  // [1 -1; -1 1]
		float dxy= (_22 - _02 - _20 + _00)/4;

		float dx_dx = dx * dx;
		float dy_dy = dy * dy;
		e_term.at<float>(r, c) = (dx_dx * dyy - 2*dxy*dx*dy + dy_dy * dxx) / std::pow(1 + dx_dx + dy_dy, 1.5F);
	}

	Mat e_ext;
	e_ext = (weight[0] * e_line + weight[1] * e_edge + weight[2] * e_term);

//	e_ext.convertTo(_e_ext, CV_8UC1, 255.0F);
//	cv::imshow("e_ext", _e_ext);

	Mat gradient_x, gradient_y;
	gradient(e_ext, &gradient_x, &gradient_y);

	// initializing the snake

	// populating the penta diagonal matrix
	// x″[i] = x[i-1] - 2*x[i] + x[i+1]
	// x″″[i] = x[i-2] - 4*x[i-1] + 6*x[i] - 4*x[i+1] + x[i+2]

	// ∂xt[i] / ∂t = α * x″[i] - β * x″″[i] + fx(xt[i], yt[i])
	// ∂yt[i] / ∂t = α * y″[i] + β * y″″[i] + fy(xt[i], yt[i])
	float b[3] = { 2*alpha + 6 *beta, -(alpha + 4*beta), beta };
	const int N = static_cast<int>(points.size());

	// Toeplitz matrix, or diagonal-constant matrix.
	Mat brow = Mat::zeros(N, N, CV_32FC1);
	float* brow_data = brow.ptr<float>();
	for(int i = 0; i < N; ++i)
	{
		const int _0 = i, _1  = i+1, _2 = i+2;
		brow_data[_0 % N] = b[_0 % 3];
		brow_data[_1 % N] = b[_1 % 3];
		brow_data[_2 % N] = b[_2 % 3];
		// ZERO ZERO ZERO
		brow_data[N - (_2 % N)] = b[_2 % 3];
		brow_data[N - (_1 % N)] = b[_1 % 3];
		
		brow_data += N;

		brow_data[i] += gamma;
	}

	// [L, U] = lu(brow);
	// inv_brow = inv(U) * inv(L);
	Mat inv_brow;
	cv::invert(brow, inv_brow, cv::DECOMP_CHOLESKY);

	Mat temp(N, 2, CV_32FC1, points.data());

	// moving the snake in each iteration
	for(int j = 0; j < nb_iteration; ++j)
	{
		// ssx = gamma*xs - kappa*interp2(grad_x, xs, ys);
		// ssy = gamma*ys - kappa*interp2(grad_y, xs, ys);
	
		for(int i = 0; i < N; ++i)
		{
			Point2f& point = points[i];
			float x = interp2(gradient_x, point);
			float y = interp2(gradient_y, point);

			point = gamma * point - kappa * Point2f(x, y);
			//float i_x, i_y;  // integral part
			//float t_x = modf(points[i].x, &i_x);
			//float t_y = modf(points[i].y, &i_y);
		}

		// calculating the new position of snake
		Mat product = inv_brow * temp;
		temp = product;//inv_brow * temp;
	}
}

} /* namespace venus */