#ifndef EXAMPLE_EFFECT_H_
#define EXAMPLE_EFFECT_H_

#include <opencv2/core/mat.hpp>


void posterize(const cv::Mat& image);

void selectiveGaussianBlur(const cv::Mat& image);
void selectiveGaussianBlur(const cv::Mat& image, const cv::Mat& mask);

#endif /* EXAMPLE_EFFECT_H_ */