#ifndef VENUS_REGION_H_
#define VENUS_REGION_H_

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace venus {

class Region
{
public:
	cv::Point2f pivot;  //< pin point, relative to source image.
	cv::Size2f  size;   //< ROI's raw size, namely neither scaled nor rotated image size.
	cv::Mat     mask;   //< mask of ROI(Region Of Interest).

public:
	Region() = default;
	Region(cv::Point2f& pivot, cv::Size2f& size, cv::Mat& mask);

	static cv::Rect getRect(const cv::Point2f& pivot, const cv::Size2f& size);
	cv::Rect getRect() const;

	static Region merge(const Region& region1, const Region& region2);

	/**_________________
	 * |  LT |    LR    |    top
	 * |-----p----------|
	 * |  LB |    RB    |    bottom
	 * |_____|__________|
	 *  left  right
	 *
	 * Scale the four parts separately, then assembly them together.
	 * Makeup image like eye lash can use this function to tune image for proper size.
	 */
	static cv::Mat resize(const cv::Mat& image, const cv::Point2f& pivot,
		float left_scale, float top_scale, float right_scale, float bottom_scale,
		int interpolation = cv::INTER_LINEAR);

	static inline cv::Mat resize(const cv::Mat& image, const cv::Point2f& pivot, const cv::Vec4f& scale, int interpolation = cv::INTER_LINEAR)
	{
		return resize(image, pivot, scale[0], scale[1], scale[2], scale[3], interpolation);
	}

	/**
     * Inset the rectangle by offset. If offset is positive, then the sides are moved inwards, making 
	 * the rectangle narrower. If offset is negative, then the sides are moved outwards, making the 
	 * rectangle wider.
	 *
	 * @param offset The amount to add(subtract) from the rectangle's left(right), top(bottom).
	 */
	template<typename T>
	static void inset(cv::Rect_<T>& rect, T offset)
	{
		assert(rect.width >= 0 && rect.height >= 0);
//		assert(width >= 0 || (rect.width > -width && rect.height > -width));

		rect.x      += offset;
		rect.y      += offset;
		rect.width  -= offset * 2;
		rect.height -= offset * 2;

		if(rect.width  < 0)  rect.width = 0;
		if(rect.height < 0)  rect.height = 0;
	}

	static cv::Mat inset(const cv::Mat& mat, int offset);

	void inset(float offset);

	static cv::Rect2i boundingRect(const cv::Mat& mask, int tolerance = 0);

	/**
	 * <gegl>/gegl/buffer/gegl-region-generic.c
	 * gegl_region_shrink(GeglRegion *region, int dx, int dy);
	 *
	 */
	static void shrink(cv::Mat& dst, const cv::Mat& mask, int offset);
	static void grow(cv::Mat& dst, const cv::Mat& mask, int offset);
	
	static void overlay(cv::Mat& dst, const cv::Mat& patch, const cv::Point2i& position, const cv::Mat& mask);
	void overlay(cv::Mat& mat, const cv::Mat& patch) const;

	/**
	 * first translating, then rotating, last scaling.
	 * matrix = scale * rotation * translation * position;
	 *
	 * @param[in,out] size Source image size.
	 * @param[in,out] pivot Center of the rotation in the source image, and transformed image pivot after transformed.
	 * @param[in] rotation Angle in radians. Positive value means clockwise rotation.
	 * @param[in] scale Scaling factor in X/Y directions, default to no scaling.
	 */
	static cv::Mat transform(cv::Size& size, cv::Point2f& pivot, float angle, const cv::Vec2f& scale = cv::Vec2f(1.0f, 1.0f));

	/**
	 * Like void cv::transform(InputArray src, OutputArray dst, InputArray m);, but operate on cv::Point2f,
	 * the result will be matrix multiplication (affine * point).
	 *
	 * @param[in] affine Affine matrix.
	 * @param[in] point  Position of the point.
	 * @return           Transformed position.
	 */
	static cv::Point2f transform(const cv::Mat& affine, const cv::Point2f& point);

	/**
	 * @param mat a 2x3 affine transform
	 * @return the inverse of the matrix
	 */
	static cv::Mat invert(const cv::Mat& mat);

};

} /* namespace venus */
#endif /* VENUS_REGION_H_ */