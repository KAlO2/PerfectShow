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
	 * Detect feature points from an image if there are face(s) in the image.
	 *
	 * @param[in] image          The <em>gray</em> image to be detected.
	 * @param[in] tag            Nullable, for debugging usage, usually the image name.
	 * @param[in] classifier_dir The classifiers (haarcascade_frontalface_alt2.xml and so on) directory.
	 * @return                   Feature points.
	 */
	static std::vector<cv::Point2f> detectFace(const cv::Mat& image, const std::string& tag, const std::string& classifier_dir);

	/**
	 * Detect feature points from given path.
	 * @param[out] size          The image size.
	 * @param[in] image_name     The image name.
	 * @param[in] classifier_dir The classifiers (haarcascade_frontalface_alt2.xml and so on) directory.
	 * @return                   Feature points.
	 */
	static std::vector<cv::Point2f> detectFace(cv::Size2i* size, const std::string& image_name, const std::string& classifier_dir);

	cv::Mat mark() const;
	static void mark(cv::Mat& image, const std::vector<cv::Point2f>& points);
	
	cv::Mat markWithIndices() const;
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

	Region calculateBrowRegion(bool right) const { return calculateBrowRegion(points, line, right);  }
	Region calculateEyeRegion(bool right)  const { return calculateEyeRegion(points, line, right);   }
	Region calculateLipshRegion()          const { return calculateLipsRegion(points, line); }
	Region calculateTeethRegion()          const;

	/**
	 * Given four points, calculate the intersection point of the two lines (left-right and top-bottom).
	 * @see https://en.wikipedia.org/wiki/Line–line_intersection
	 */
	static cv::Vec4f calcuateDistance(cv::Point2f& pivot, const cv::Point2f& left, const cv::Point2f& top, const cv::Point2f& right, const cv::Point2f& bottom);

	cv::Vec4f calcuateEyeRadius(bool right) const { return calcuateEyeRadius(points, line, right); }

	static cv::Mat maskSkinRegion(int width, int height, const std::vector<cv::Point2f>& points);

};

} /* namespace venus */
#endif /* VENUS_FEATURE_H_ */