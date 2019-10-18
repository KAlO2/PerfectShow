#include <opencv2/highgui.hpp>

#include "example/effect.h"
#include "example/UserData.h"
#include "example/utility.h"

#include "platform/jni_bridge.h"
#include "venus/scalar.h"

#include "venus/Beauty.h"
#include "venus/Effect.h"

using namespace cv;
using namespace venus;

static const std::string TAG("Effect");


void posterize(const cv::Mat& image)
{
	const std::string title("Posterize");
	const int max = 1024;

	Mat processed = image.clone();
	UserData user_data(title, max, {}, image, processed);

	auto onProgressChanged = [](int level, void* user_data)
	{
		UserData& data = *reinterpret_cast<UserData*>(user_data);
		float x = level / static_cast<float>(data.max);
		x *= 256;  // [0, 1] => [0, 256]
		x = clamp(x, 1.0F, 256.0F);
		
		Effect::posterize(data.processed, data.original, x);
		cv::imshow(data.title, data.processed);
	};

	int level = 256;
	cv::namedWindow(title);
	cv::setMouseCallback(title, UserData::onClick, &user_data);
	cv::createTrackbar("amount", title, &level, max, onProgressChanged, &user_data);
	
	onProgressChanged(level, &user_data);
}

void pixelize(const cv::Mat& image)
{
	const std::string title("Pixelize");
	const int max = 52;

	Mat processed = image.clone();
	UserData user_data(title, max, {}, image, processed);

	auto onProgressChanged = [](int block_size, void* user_data)
	{
		UserData& data = *reinterpret_cast<UserData*>(user_data);
		if(block_size <= 1)
			data.original.copyTo(data.processed);
		else
			Effect::pixelize(data.processed, data.original, block_size);

		cv::imshow(data.title, data.processed);
	};
	
	int block_size = 4;
	cv::namedWindow(title);
	cv::setMouseCallback(title, UserData::onClick, &user_data);
	cv::createTrackbar("amount", title, &block_size, max, onProgressChanged, &user_data);

	onProgressChanged(block_size, &user_data);
	cv::waitKey();
}

void selectiveGaussianBlur(const cv::Mat& image)
{
#if 0
	Mat mask(image.rows, image.cols, CV_8UC1, 255);
#else
	Mat mask = Beauty::calculateSkinRegion_RGB(image);
#endif
	selectiveGaussianBlur(image, mask);
}

void selectiveGaussianBlur(const cv::Mat& image, const cv::Mat& mask)
{
	const std::string title("blur");
	const int max = 255;
	
	Mat processed = image.clone();
	UserData user_data(title, max, {}, image, processed);
	user_data.setMask(mask);

	auto onProgressChanged = [](int threshold, void* user_data)
	{
		UserData& data = *reinterpret_cast<UserData*>(user_data);
		float radius = 7.0F;

		Effect::gaussianBlurSelective(data.processed, data.original, data.mask, radius, threshold);
		cv::imshow(data.title, data.processed);
	};
	
	int threshold = 12;
	cv::namedWindow(title);
	cv::setMouseCallback(title, UserData::onClick, &user_data);
	cv::createTrackbar("amount", title, &threshold, max, onProgressChanged, &user_data);

	onProgressChanged(threshold, &user_data);
	cv::waitKey();
}