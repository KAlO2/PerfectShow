#include "example/beauty.h"
#include "example/makeup.h"
#include "example/utility.h"

#include <opencv2/imgcodecs.hpp>

using namespace cv;
using namespace venus;

int main()
{
	// 000.jpg 001.jpg 044.jpg 110.jpg
	std::string dir = PROJECT_DIR + "doc/image/";
	std::string image_name = dir + "110.jpg";
	Mat image = cv::imread(image_name);
//	detectFace(image_name);
//	mark(image_name);
	
	testSamples(dir, detectSkin);
	
//	Mat red_eye_sample = cv::imread(PROJECT_DIR + "red_eye.jpg");
//	redEyeRemoval_CLI(red_eye_sample, 0.72F);
//	redEyeRemoval_GUI(red_eye_sample);
	
//	skinDermabrasion(image);

//	judgeFaceShape(image_name);
	
//	colorize(image_name);
	
//	createShape();
//	transform(image);
//	imageWarp();
//	morphology(image);
	
//	applyLip(image_name);
//	applyBlush(image_name);
//	applyEyeShadow(image_name);
//	applyEyeLash(image_name);
//	applyBrow(image_name);

	return 0;
}