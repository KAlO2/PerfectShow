#ifndef VENUS_REGION_H_
#define VENUS_REGION_H_

#include <stdint.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace venus {

/**
 * color selecting criterion
 */
enum class SelectCriterion
{
	COMPOSITE,  ///< weight of all the channels

	RED,        ///< The red channel of an RGB/RGBA color.
	GREEN,      ///< The green channel of an RGB/RGBA color.
	BLUE,       ///< The blue channel of an RGB/RGBA color.
	ALPHA,      ///< The alpha channel of an RGBA color.

	HUE,        ///< The hue channel of an HSV color.
	SATURATION, ///< The saturation channel of an HSV color.
	VALUE,      ///< The value channel of an HSV color.
};

class Region
{
public:
	cv::Point2f pivot;  ///< pin point, relative to source image.
	cv::Size2f  size;   ///< ROI's raw size, namely neither scaled nor rotated image size.
	cv::Mat     mask;   ///< mask of ROI(Region Of Interest).

public:
	Region() = default;
	Region(cv::Point2f& pivot, cv::Size2f& size, cv::Mat& mask);

	static cv::Rect getRect(const cv::Point2f& pivot, const cv::Size2f& size);
	cv::Rect getRect() const;

	static Region merge(const Region& region1, const Region& region2);

	/**
	 * <pre>
	 * _________________
	 * |  LT |    LR    |    top
	 * |-----p----------|
	 * |  LB |    RB    |    bottom
	 * |_____|__________|
	 *  left  right
	 * </pre>
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
	 * @param[in,out] rect
	 * @param[in]     offset  The amount to add/subtract from the rectangle's left/right, top/bottom.
	 */
	template<typename T>
	static void inset(cv::Rect_<T>& rect, T offset);

	static cv::Mat inset(const cv::Mat& mat, int offset);

	void inset(float offset);

	static cv::Rect2i boundingRect(const cv::Mat& mask, int tolerance = 0);

	/**@{
	 * Finding pixels within the specified threshold from the given RGBA values. If antialiasing is on, You got smooth result. 
	 *
	 * @param[out] mask       Mask of the source @a image, with the same type (as called depth in OpenCV).
	 * @param[in]  image      The source image.
	 * @param[in]  color      RGBA format, need to be same type as image.
	 * @param[in]  threshold  range [0.0, 1.0] for floating point type, or range [0, 255] for integer type.
	 * @param[in]  criterion  @enum SelectCriterion
	 * @param[in]  select_transparent  Take alpha channel into consideration or not?
	 * @param[in]  antialias  enable it if you want smooth edge.
	 */
	static void selectContiguousRegionByColor(cv::Mat& mask, const cv::Mat& image, const cv::Vec4b& color, uint8_t threshold,
			SelectCriterion criterion, bool select_transparent, bool antialias);

	static void selectContiguousRegionByColor(cv::Mat& mask, const cv::Mat& image, const cv::Vec4f& color, float threshold,
			SelectCriterion criterion, bool select_transparent, bool antialias);
	/**@}*/

	/**
	 * Contract the selection, the opposite operation of @a grow.
	 *
	 * @param[out] dst    The resulting mask.
	 * @param[in]  src    The mask image to be operate.
	 * @param[in]  offset 
	 */
	static void shrink(cv::Mat& dst, const cv::Mat& src, int offset);

	/**
	 * Enlarge the selection, the opposite operation of @a shrink.
	 *
	 * @param[out] dst    The resulting mask.
	 * @param[in]  src    The mask image to be operate.
	 * @param[in]  offset 
	 */
	static void grow(cv::Mat& dst, const cv::Mat& src, int offset);
	
	static void overlay(cv::Mat& dst, const cv::Mat& patch, const cv::Point2i& position, const cv::Mat& mask);
	void overlay(cv::Mat& mat, const cv::Mat& patch) const;

	/**
	 * first translating, then rotating, last scaling.
	 * matrix = scale * rotation * translation * position;
	 *
	 * @param[in,out] size   Source image size.
	 * @param[in,out] pivot  Center of the rotation in the source image, and transformed image pivot after transformed.
	 * @param[in]     angle  Angle in radians. Positive value means clockwise rotation.
	 * @param[in]     scale  Scaling factor in X/Y directions, default to no scaling.
	 */
	static cv::Mat transform(cv::Size& size, cv::Point2f& pivot, const float& angle, const cv::Vec2f& scale = cv::Vec2f(1.0F, 1.0F));

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

	/**
	 * @param[in] image         The source image, of type CV_32FC1
	 * @param[in] points        The initial snake coordinates
	 * @param[in] alpha         Controls tension
	 * @param[in] beta          Controls rigidity
	 * @param[in] gamma         Step size
	 * @param[in] kappa         Controls enegry term
	 * @param[in] weight        Vec3f(wl, we, wt), Weights for line, edge and terminal enegy components
	 * @param[in] nb_iteration  Number of iteration for which snake is to be moved
	 */
	static void snake(const cv::Mat1f& image, std::vector<cv::Point2f>& points, float alpha, float beta, float gamma, float kappa,
			const cv::Vec3f& weight, int nb_iteration);

};


template<typename T>
void Region::inset(cv::Rect_<T>& rect, T offset)
{
	assert(rect.width >= 0 && rect.height >= 0);
//	assert(width >= 0 || (rect.width > -width && rect.height > -width));

	rect.x      += offset;
	rect.y      += offset;
	rect.width  -= offset * 2;
	rect.height -= offset * 2;

	if(rect.width  < 0)  rect.width  = 0;
	if(rect.height < 0)  rect.height = 0;
}

} /* namespace venus */
#endif /* VENUS_REGION_H_ */