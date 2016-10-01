#ifndef VENUS_FEATURE_H_
#define VENUS_FEATURE_H_

#include <vector>
#include <opencv2/core.hpp>
#include <venus/Region.h>

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
	static Region calcuateBrowRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right);
	static Region calcuateEyeRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right);

public:
	Feature(const cv::Mat& image, const std::vector<cv::Point2f>& points);
//	void setFeaturePoints(const std::vector<cv::Point>& points);
	std::vector<cv::Point> getPolygon(int region);
	
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

	static cv::Mat1b maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, const int indices[], int length);
	static cv::Mat1b maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, int start, int length);

	Region calcuateBrowRegion_r() const { return calcuateBrowRegion(points, line, true);  }
	Region calcuateBrowRegion_l() const { return calcuateBrowRegion(points, line, false); }
	Region calcuateEyeRegion_r()  const { return calcuateEyeRegion(points, line, true);   }
	Region calcuateEyeRegion_l()  const { return calcuateEyeRegion(points, line, false);  }
	
	static Region maskLips(const std::vector<cv::Point2f>& points, const cv::Vec4f& line);
	Region maskLips() const { return maskLips(points, line); }
	cv::Mat maskHair() const;
protected:
	void assignRegion();

	std::vector<cv::Point> m_facePolygon;
	std::vector<cv::Point> m_leftEyePolygon;
	std::vector<cv::Point> m_rightEyePolygon;
	std::vector<cv::Point> m_upperLipPolygon;
	std::vector<cv::Point> m_lowerLipPolygon;
	std::vector<cv::Point> m_mouthPolygon;

	std::vector<cv::Point> m_nosePolygon;
	std::vector<cv::Point> m_leftEyeBrowPolygon;
	std::vector<cv::Point> m_rightEyeBrowPolygon;
	std::vector<cv::Point> m_leftBlusherPolygon;
	std::vector<cv::Point> m_rightBlusherPolygon;

	cv::Point m_leftNoseHole;
	cv::Point m_rightNoseHole;

};

} /* namespace venus */
#endif /* VENUS_FEATURE_H_ */