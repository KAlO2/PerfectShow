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
	static const int COUNT;
	static const std::vector<cv::Vec3i> triangle_indices;

private:
	const cv::Mat image;
	const std::vector<cv::Point2f>& points;
	cv::Vec4f line;

private:

	static void triangulate(cv::Mat& image, const std::vector<cv::Point2f>& points, const std::vector<cv::Vec3i>& triangles);

	static Region calculateBrowRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right);
	static Region calculateEyeRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right);
	static Region calculateLipsRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line);

public:
	Feature(const cv::Mat& image, const std::vector<cv::Point2f>& points);
//	void setFeaturePoints(const std::vector<cv::Point>& points);
	
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
	cv::Vec4f getSymmetryAxis() const { return getSymmetryAxis(points); }

	static cv::Mat createMask(const std::vector<cv::Point2f> polygons[], int count, float blur_radius = 0);
	static cv::Mat createMask(const std::vector<cv::Point2f>& points, float blur_radius = 0, cv::Point2i* position = nullptr);

	static cv::Mat maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, const int indices[], int length);
	static cv::Mat maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, int start, int length);
	static cv::Mat maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points);
	static cv::Mat maskPolygonSmooth(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, const int level = 8);

	static std::vector<cv::Point2f> calculateBrowPolygon(const std::vector<cv::Point2f>& points, bool right);
	static std::vector<cv::Point2f> calculateEyePolygon(const std::vector<cv::Point2f>& points, bool right);
	static std::vector<cv::Point2f> calculateBlushPolygon(const std::vector<cv::Point2f>& points, bool right);
	static std::vector<cv::Point2f> calculateNosePolygon(const std::vector<cv::Point2f>& points);
	static std::vector<cv::Point2f> calculateTeethPolygon(const std::vector<cv::Point2f>& points);

	Region calculateBrowRegion_r() const { return calculateBrowRegion(points, line, true);  }
	Region calculateBrowRegion_l() const { return calculateBrowRegion(points, line, false); }
	Region calculateEyeRegion_r()  const { return calculateEyeRegion(points, line, true);   }
	Region calculateEyeRegion_l()  const { return calculateEyeRegion(points, line, false);  }
	Region calculateLipshRegion()  const { return calculateLipsRegion(points, line); }
	Region calculateTeethRegion()  const;

	static cv::Mat maskSkinRegion(int width, int height, const std::vector<cv::Point2f>& points);

};

} /* namespace venus */
#endif /* VENUS_FEATURE_H_ */