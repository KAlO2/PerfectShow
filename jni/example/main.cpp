#include "example/makeup.h"
#include "example/utility.h"

#include <opencv2/highgui.hpp>

int main()
{
	// 000.jpg 001.jpg 044.jpg 110.jpg
	std::string image_name = PROJECT_DIR + "doc/image/000.jpg";
	
	detectFace(image_name);
//	mark(image_name);
	
//	detectSkin(image_name);
//	judgeFaceShape(image_name);
	
//	createShape();
//	transform(image_name);
//	imageWarp();
//	morphology(image_name);
	
//	applyLip(image_name);
//	applyBlush(image_name);
//	applyEyeShadow(image_name);
//	applyEyeLash(image_name);
//	applyBrow(image_name);

	cv::waitKey();

	return 0;
}