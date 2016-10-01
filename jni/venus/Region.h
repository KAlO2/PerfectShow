#ifndef VENUS_REGION_H_
#define VENUS_REGION_H_

#include <opencv2/core.hpp>

namespace venus {

class Region
{
public:
	cv::Point2f pivot; //< pin point, relative to source image.
	cv::Size2f  size;  //< ROI's raw size, namely neither scaled nor rotated image size.
	cv::Mat1b   mask;

public:
	Region() = default;
	Region(cv::Point2f& pivot, cv::Size2f& size, cv::Mat1b& mask);

	static cv::Rect getRect(const cv::Point2f& pivot, const cv::Size2f& size);
	cv::Rect getRect() const;

	/**
	 * inset border for rectangle
	 *
	 * @param offset positve get smaller rectangle, negative get larger rectangle.
	 * @return rect
	 */
	static cv::Rect& inset(cv::Rect& rect, int offset);
	static cv::Mat   inset(const cv::Mat& mat, int offset);

	void inset(float offset);

	static cv::Rect2i boundingRect(const cv::Mat& mask);

	/**
	 * <gegl>/gegl/buffer/gegl-region-generic.c
	 * gegl_region_shrink(GeglRegion *region, int dx, int dy);
	 *
	 */
	static cv::Mat1b shrink(const cv::Mat1b& mask, int offset);
	static cv::Mat1b grow(const cv::Mat1b& mask, int offset);
	
	static void overlay(cv::Mat& mat, const Region& mask, const cv::Point2f& position, const cv::Mat& patch);
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
	 * @param mat a 2x3 affine transform
	 * @return the inverse of the matrix
	 */
	static cv::Mat invert(const cv::Mat& mat);

};

} /* namespace venus */
#endif /* VENUS_REGION_H_ */