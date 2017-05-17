#include <opencv2/highgui.hpp>

#include "example/UserData.h"



UserData::UserData(const std::string& title, const int& max, const std::vector<cv::Point2f>& points, const cv::Mat& original, cv::Mat& processed):
	title(title),
	max(max),
	points(points),
	original(original),
	mask(original.rows, original.cols, CV_8UC1, cv::Scalar(255))
{
	processed.create(original.rows, original.cols, original.type());
}

void UserData::onClick(int event, int x, int y, int flags, void* user_data)
{
	UserData& data = *reinterpret_cast<UserData*>(user_data);

	switch(event)
	{
	case cv::EVENT_LBUTTONDOWN:
		cv::imshow(data.title, data.original);
		break;
	default:
		cv::imshow(data.title, data.processed);
		break;
	}
}