#ifndef EXAMPLE_USER_DATA_H_
#define EXAMPLE_USER_DATA_H_

#include <string>
#include <opencv2/core/mat.hpp>

struct UserData
{
	const std::string title;            ///< window title
	const int max;                      ///< progress max
	const std::vector<cv::Point2f> points;  ///< feature points

	cv::Mat original;  ///< original image
	cv::Mat mask;      ///< mask of orignal image
	cv::Mat processed; ///< processed image

//	float radius;

public:
	UserData(const std::string& title, const int& max, const std::vector<cv::Point2f>& points, const cv::Mat& original, cv::Mat& processed);

	void setMask(const cv::Mat& mask) { this->mask = mask; }
	
	static void onClick(int event, int x, int y, int flags, void* user_data);
	
};

#endif /* EXAMPLE_USER_DATA_H_ */