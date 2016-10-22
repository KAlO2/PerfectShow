#ifndef REGION_OPERATION_H_
#define REGION_OPERATION_H_

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <stdint.h>
#include <string>
#include <vector>

namespace venus {

// ROI (region Of Interest), these values must match up with the enum in FaceDetector.java
enum Roi
{
	FACE,  // the whole face
	EYE_BROW_L,
	EYE_BROW_R,
	EYE_LASH_L,
	EYE_LASH_R,
	EYE_SHADOW_L,
	EYE_SHADOW_R,
	IRIS_L,
	IRIS_R,
	BLUSH_L,
	BLUSH_R,
	NOSE,
	LIP_T,  // upper lip
	LIP_B,  // lower lip

	REGION_COUNT
};

class RoiInfo
{
public:
	cv::Point2f origion;
	cv::Point2f pivot;
	cv::Mat     mask;

public:
	RoiInfo() = default;
	RoiInfo(cv::Point2f& origion, cv::Point2f& pivot, cv::Mat& mask)
		:origion(origion)
		,pivot(pivot)
		,mask(mask)
	{}
};


std::vector<cv::Point2f> getFaceFeaturePoints(const std::string& face, const std::string& datadir);
std::vector<cv::Point2f> getFaceFeaturePoints(const std::string& face, const std::string& datadir, cv::Point& size);
std::vector<cv::Point2f> getFaceFeaturePoints(const cv::Mat& image, const std::string& image_name, const std::string& datadir);

/**
 * stasm doesn't detect eyes precisely. since we already get rough region, we tune it for fine result.
 *
 * @param[in] image the image with face in it.
 * @param[in] points feature points detected from image.
 * @param[in] skew_angle in degree, calculated from #getSymmetryAxis. reduce computation if it has been calculated.
 * @param[out] iris_r right iris info, Vec3f(x0, y0, r), where x0, y0 pair for center, r for iris radius.
 * @param[out] iris_l left iris info, ditto.
 */
void calcuateIrisInfo(const cv::Mat& image, const std::vector<cv::Point2f>& points, float skew_angle, cv::Point3f& iris_r, cv::Point3f& iris_l);
void calcuateIrisInfo(const cv::Mat& image, const std::vector<cv::Point2f>& points, cv::Point3f& iris_r, cv::Point3f& iris_l);


/**
 * generate single channel CV_8UC1 mask
 */
//cv::Mat polygonMask(const cv::Rect& rect, const std::vector<cv::Point2f>& points, const int indices[], int length);

// calcuate xxx RoiInfo series, some organs have left and right part regions,
// and the left or right orientation is from aspect of human in the image, not you!
RoiInfo calcuateFaceRegionInfo(const std::vector<cv::Point2f>& points);
RoiInfo calcuateEyeBrowRegionInfo(const std::vector<cv::Point2f>& points, bool right = true);
RoiInfo calcuateEyeRegionInfo_r(const std::vector<cv::Point2f>& points);
RoiInfo calcuateEyeRegionInfo_l(const std::vector<cv::Point2f>& points);
RoiInfo calcuateBlusherRegionInfo(const std::vector<cv::Point2f>& points, bool right = true);
RoiInfo calcuateLipsRegionInfo(const std::vector<cv::Point2f>& points, int radius = 0, const cv::Scalar& color = cv::Scalar(255, 255, 255, 255));

cv::Mat stretchImage(const cv::Mat& image, const cv::Size& src_size, const cv::Size& dst_size, const std::vector<cv::Point2f>& src_points, const std::vector<cv::Point2f>& dst_points, const std::vector<cv::Vec3i>& indices);
cv::Mat& stretchImage(const cv::Mat& src_image, cv::Mat& dst_image, const std::vector<cv::Point2f>& src_points, const std::vector<cv::Point2f>& dst_points, const std::vector<cv::Vec3i>& indices);
cv::Mat& stretchImageWithAlpha(const cv::Mat& src_image, cv::Mat& dst_image, const std::vector<cv::Point2f>& src_points, const std::vector<cv::Point2f>& dst_points, const std::vector<cv::Vec3i>& indices);

cv::Mat cloneFace(const std::string& user_image_path, const std::string& model_image_path, const std::string& datadir);

// color is Android's ARGB byte order, not OpenCV's BGRA
cv::Mat& blendColor(cv::Mat& src, const cv::Mat& mask, uint32_t color, float alpha);
cv::Mat overlay(const cv::Mat& src, cv::Mat& dst, const cv::Point& orgin, int alpha);




void blendIris(cv::Mat& image, const cv::Mat& iris, const std::vector<cv::Point2f>& points, float amount);



/* NOTES
 *
 * The method used here is similar to the lighting invariant correction
 * method but slightly different: we do not divide the RGB components,
 * but subtract them I2 = I0 - I1, where I0 is the sample image to be
 * corrected, I1 is the reference pattern. Then we solve DeltaI=0
 * (Laplace) with I2 Dirichlet conditions at the borders of the
 * mask. The solver is a red/black checker Gauss-Seidel with over-relaxation.
 * It could benefit from a multi-grid evaluation of an initial solution
 * before the main iteration loop.
 *
 * I reduced the convergence criteria to 0.1% (0.001) as we are
 * dealing here with RGB integer components, more is overkill.
 *
 * Jean-Yves Couleaud cjyves@free.fr
 */
//heal_motion()

} /* namespace venus */
#endif /* REGION_OPERATION_H_ */
