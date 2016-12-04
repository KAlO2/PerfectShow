#ifndef EXAMPLE_BEAUTY_H_
#define EXAMPLE_BEAUTY_H_

#include <opencv2/core/mat.hpp>


void detectSkin(const cv::Mat& image);

// Command Line Interface (CLI) or Graphical User Interface (GUI)
void redEyeRemoval_CLI(const cv::Mat& image, float threshold);
void redEyeRemoval_GUI(const cv::Mat& image);

void skinWhiten(const cv::Mat& image);
void skinDermabrasion(const cv::Mat& image);

#endif /* EXAMPLE_BEAUTY_H_ */