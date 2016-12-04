#include <opencv2/highgui.hpp>

#include "example/beauty.h"
#include "example/utility.h"
#include "platform/jni_bridge.h"

#include "venus/Beauty.h"
#include "venus/Effect.h"
#include "venus/Feature.h"

using namespace cv;
using namespace venus;

static const std::string TAG("Beauty");

void detectSkin(const cv::Mat& image)
{
	TIME_START;

	Mat mask_rgb = Beauty::calculateSkinRegion_RGB(image);
	TIME_STOP("calculateSkinRegion_RGB");

	Mat mask_ycbcr = Beauty::calculateSkinRegion_YCbCr(image);
	TIME_STOP("calculateSkinRegion_YCbCr");

	Mat mask_hsv = Beauty::calculateSkinRegion_HSV(image);
	TIME_STOP("calculateSkinRegion_HSV");

	// How about combining two methods?
	Mat mask_combined;
	double weight = 0.72;
	cv::addWeighted(mask_rgb, weight, mask_hsv, 1.0 - weight, 0.0, mask_combined);

	cv::imshow("original", image);

	cv::imshow("mask_rgb",      mask_rgb);
	cv::imshow("mask_ycbcr",    mask_ycbcr);
	cv::imshow("mask_hsv",      mask_hsv);
	cv::imshow("mask_combined", mask_combined);
	cv::waitKey();
}

void redEyeRemoval_CLI(const cv::Mat& image, float threshold)
{
	Mat gray = Effect::grayscale(image);
	std::vector<Point2f> points = Feature::detectFace(gray, __FUNCTION__, CLASSIFIER_DIR);
	// TODO detectFace only detect one face, multiple faces support will be added later.

	Mat processed = image.clone();
	if(points.empty())
	{
		// Here supposing the whole image need to be processed, outline the red eye region is better.
		const float x0 = 0, x1 = image.cols - 1;
		const float y0 = 0, y1 = image.rows - 1;
		std::vector<Point2f> whole{Point2f(x0, x0), Point2f(x1, x0), Point2f(x1, y1), Point2f(x0, y1)};
		Beauty::removeRedEye(processed, processed, whole, threshold);
	}
	else
	{
		Feature feature(image, points);
		for(int i = 0; i < 2; ++i)
		{
			std::vector<Point2f> polygon = feature.calculateEyePolygon(points, i == 0);
			Beauty::removeRedEye(processed, processed, polygon, threshold);
		}
	}

	cv::imshow("original", image);
	cv::imshow("processed", processed);
	cv::waitKey();
}

struct UserData
{
	const std::string title;            ///< window title
	const int max;                      ///< progress max
	const std::vector<Point2f> points;  ///< feature points

	Mat original;  ///< original image
	Mat mask;      ///< mask of orignal image
	Mat processed; ///< processed image
};

static void onClick(int event, int x, int y, int flags, void* user_data)
{
	UserData& data = *reinterpret_cast<UserData*>(user_data);

	switch(event)
	{
	case EVENT_LBUTTONDOWN:
		cv::imshow(data.title, data.original);
		break;
	default:
		cv::imshow(data.title, data.processed);
		break;
	}
};

void redEyeRemoval_GUI(const cv::Mat& image)
{
	Mat gray = Effect::grayscale(image);
	std::vector<Point2f> points = Feature::detectFace(gray, __FUNCTION__, CLASSIFIER_DIR);
	assert(!points.empty());  // ensure an face is found.

	const std::string title("Red Eye Removal");
	const int max = 512;

	Mat processed = image.clone();
	UserData user_data = {title, max, points, image, {}, processed};

	auto onProgressChanged = [](int progress, void* user_data)
	{
		UserData& data = *reinterpret_cast<UserData*>(user_data);
		float threshold = progress / static_cast<float>(data.max);
		
		data.original.copyTo(data.processed);
		Feature feature(data.original, data.points);
		for(int i = 0; i < 2; ++i)
		{
			std::vector<Point2f> polygon = feature.calculateEyePolygon(data.points, i == 0);
			Beauty::removeRedEye(data.processed, data.processed, polygon, threshold);
		}

		cv::imshow(data.title, data.processed);
	};

	int progress = max / 2;

	cv::namedWindow(title);
	cv::setMouseCallback(title, onClick, &user_data);
	cv::createTrackbar("amount", title, &progress, max, onProgressChanged, &user_data);

	onProgressChanged(progress, &user_data);

	cv::waitKey();
}

void skinWhiten(const cv::Mat& image)
{
	const std::string title("Skin Whiten");
	const int max = 100;

	Mat processed = image.clone();
	Mat mask(image.rows, image.cols, CV_8UC1, Scalar(255));  // whole
	UserData user_data = {title, max, {}, image, mask, processed};

	auto onProgressChanged = [](int progress, void* user_data)
	{
		UserData& data = *reinterpret_cast<UserData*>(user_data);
		float amount = progress / static_cast<float>(data.max);
		float level = amount * 8 + 2;
		Beauty::whitenSkinByLogCurve(data.processed, data.original, level);
		cv::imshow(data.title, data.processed);
	};

	int progress = max / 2;
	cv::namedWindow(title);
	cv::setMouseCallback(title, onClick, &user_data);
	cv::createTrackbar("level", title, &progress, max, onProgressChanged, &user_data);

	onProgressChanged(progress, &user_data);

	cv::waitKey();
}

void skinDermabrasion(const cv::Mat& image)
{
	const std::string title("Skin Dermabrasion");
	const int level_max = 20;

	Mat processed = image.clone();
	Mat mask = Beauty::calculateSkinRegion_RGB(image);
	cv::imshow("mask", mask);

	UserData user_data = {title, level_max, {}, image, mask, processed};

	auto onProgressChanged = [](int level, void* user_data)
	{
		UserData& data = *reinterpret_cast<UserData*>(user_data);
		float radius = 5.0F;  // can be tuned!
		Beauty::beautifySkin(data.processed, data.original, data.mask, radius, level);

		cv::imshow(data.title, data.processed);
	};

	int level = level_max / 2;
	cv::namedWindow(title);
	cv::setMouseCallback(title, onClick, &user_data);
	cv::createTrackbar("amount", title, &level, level_max, onProgressChanged, &user_data);

	onProgressChanged(level_max, &user_data);

	cv::waitKey();
}