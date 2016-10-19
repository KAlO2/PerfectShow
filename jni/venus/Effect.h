#ifndef VENUS_EFFECT_H_
#define VENUS_EFFECT_H_

#include <opencv2/core.hpp>

namespace venus {

class Effect
{
private:

public:

	static cv::Mat gaussianBlur(const cv::Mat& image, float radius);

	
};

} /* namespace venus */
#endif /* VENUS_EFFECT_H_ */