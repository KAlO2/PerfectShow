#ifndef VENUS_FEATURE_H_
#define VENUS_FEATURE_H_

#include <stdint.h>
#include <vector>

#include <opencv2/core.hpp>

#include "venus/Region.h"

namespace venus {

/**
 * Extract face ROI info from color image
 */
class Feature
{
public:
	static const size_t COUNT;
	static const std::vector<cv::Vec3b> triangle_indices;

private:
	const cv::Mat image;
	const std::vector<cv::Point2f>& points;
	cv::Vec4f line;

private:

	static void triangulate(cv::Mat& image, const std::vector<cv::Point2f>& points, const std::vector<cv::Vec3b>& triangles);

	static Region calculateBrowRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right);
	static Region calculateEyeRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right);
	static Region calculateLipsRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line);

	static cv::Vec4f calcuateEyeRadius(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right);

public:
	Feature(const cv::Mat& image, const std::vector<cv::Point2f>& points);
	
	/**
	 * When you are positive that there will be only one face (or dominant face, namely has the biggest size) in the @p image,
	 * consider use this function for faster speed.
	 *
	 * @param[in] image          The @p gray image to be detected.
	 * @param[in] tag            Nullable, for debugging usage, usually the image name.
	 * @param[in] classifier_dir The classifiers (haarcascade_frontalface_alt2.xml and so on) directory.
	 * @return feature points of the face detected.
	 */
	static std::vector<cv::Point2f> detectFace(const cv::Mat& image, const std::string& tag, const std::string& classifier_dir);

	/**
	 * Detect feature points from an image if there are face(s) in the image.
	 *
	 * @param[in] image          The @p gray image to be detected.
	 * @param[in] tag            Nullable, for debugging usage, usually the image name.
	 * @param[in] classifier_dir The classifiers (haarcascade_frontalface_alt2.xml and so on) directory.
	 * @return Faces detected, each face has the same amount of feature points.
	 */
	static std::vector<std::vector<cv::Point2f>> detectFaces(const cv::Mat& image, const std::string& tag, const std::string& classifier_dir);

	/**
	 * Detect feature points from given path.
	 * @param[out] size          The image size.
	 * @param[in] image_name     The image name.
	 * @param[in] classifier_dir The classifiers (haarcascade_frontalface_alt2.xml and so on) directory.
	 * @return Faces detected, each face has the same amount of feature points.
	 */
	static std::vector<std::vector<cv::Point2f>> detectFaces(cv::Size2i* size, const std::string& image_name, const std::string& classifier_dir);

	static void mark(cv::Mat& image, const std::vector<cv::Point2f>& points);

	static void markWithIndices(cv::Mat& image, const std::vector<cv::Point2f>& points);

	/**
	 * Generally speaking, the result is a vertical line. For skew faces, it's not coarsely calculated by two eyes,
	 * but use <a href="https://en.wikipedia.org/wiki/Least_squares">least squares</a> method for more accurate result.
	 *
	 * @param points the feature points detected from an image.
	 * @return Vec4f(vx, vy, x0, y0), where (vx, vy) is a normalized vector collinear to the line and
	 *          (x0, y0) is a point on the line, or to be more exact, center of the detected face.
	 */
	static cv::Vec4f getSymmetryAxis(const std::vector<cv::Point2f>& points);
	inline cv::Vec4f getSymmetryAxis() const { return line; }

	static cv::Mat createMask(const std::vector<cv::Point2f>& points, float blur_radius = 0.0F, cv::Point2i* position = nullptr);

	static cv::Mat maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, int start, int length);
	static cv::Mat maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points);
	static cv::Mat maskPolygonSmooth(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, const int level = 8);

	static std::vector<cv::Point2f> calculateBrowPolygon (const std::vector<cv::Point2f>& points, bool right);
	static std::vector<cv::Point2f> calculateEyePolygon  (const std::vector<cv::Point2f>& points, bool right);
	static std::vector<cv::Point2f> calculateBlushPolygon(const std::vector<cv::Point2f>& points, bool right);
	static std::vector<cv::Point2f> calculateNosePolygon (const std::vector<cv::Point2f>& points);
	static std::vector<cv::Point2f> calculateLipPolygon  (const std::vector<cv::Point2f>& points, bool upper);
	static std::vector<cv::Point2f> calculateTeethPolygon(const std::vector<cv::Point2f>& points);

	/**
	 * @param[in] points feature points
	 * @param[in] right  right or left side of iris.
	 * return position(x, y) and radius(r) of iris, previously stored in Vec3f(x, y, r).
	 */
	static std::pair<cv::Point2f, float> calculateIrisInfo(const std::vector<cv::Point2f>& points, bool right);

	static cv::RotatedRect calculateBlushRectangle(const std::vector<cv::Point2f>& points, bool right);

	/**
	 * @param points Feature points of detected face.
	 * @param angle  Skew angle, measured in radians.
	 * @param right  Right or left side of face.
	 *
	 * Note that angle of cv::RotatedRect is in degrees, not in radians.
	 */
	static cv::RotatedRect calculateBlushRectangle(const std::vector<cv::Point2f>& points, float angle, bool right);

	Region calculateBrowRegion(bool right) const { return calculateBrowRegion(points, line, right);  }
	Region calculateEyeRegion(bool right)  const { return calculateEyeRegion(points, line, right);   }
	Region calculateLipshRegion()          const { return calculateLipsRegion(points, line); }
	Region calculateTeethRegion()          const;

	static cv::RotatedRect calculateRectangle(float angle, const cv::Point2f& left, const cv::Point2f& top, const cv::Point2f& right, const cv::Point2f& bottom);

	/**
	 * Given four points, calculate the intersection point of the two lines (left-right and top-bottom).
	 * @see <a href="https://en.wikipedia.org/wiki/Line–line_intersection">Line–line intersection</a>
	 */
	static cv::Vec4f calcuateDistance(cv::Point2f& pivot, const cv::Point2f& left, const cv::Point2f& top, const cv::Point2f& right, const cv::Point2f& bottom);

	cv::Vec4f calcuateEyeRadius(bool right) const { return calcuateEyeRadius(points, line, right); }

	/**
	 * Mask skin region through the feature points detected.
	 *
	 * @param[in] width  The source image width.
	 * @param[in] height The source image height.
	 * @param[in] points feature points detected from the source image.
	 * @return skin mask with the specified size(width x height), and type is CV_8UC1.
	 */
	static cv::Mat maskSkinRegion(int width, int height, const std::vector<cv::Point2f>& points);

};

} /* namespace venus */
#endif /* VENUS_FEATURE_H_ */