#ifndef VENUS_EXTRACTOR_H_
#define VENUS_EXTRACTOR_H_

#include "opencv2/core.hpp"

#include <vector>

namespace venus {

/**
 * Extract face ROI info from color image
 */
class Extractor
{
public:
	static const std::vector<cv::Vec3i> triangle_indices;

private:
	const cv::Mat image;
	const std::vector<cv::Point2f>& points;



public:
	Extractor(const cv::Mat& image, const std::vector<cv::Point2f>& points);
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
#endif /* VENUS_EXTRACTOR_H_ */