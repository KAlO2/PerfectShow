#ifndef EXAMPLE_MAKEUP_
#define EXAMPLE_MAKEUP_

#include <string>

#include <opencv2/core/mat.hpp>

void detectFace(const cv::Mat& image, const std::string& image_name = std::string());
void mark(const std::string& image_name);

void detectFaceSkin(const std::string& image_name);
void judgeFaceShape(const std::string& image_name);

void createShape();
void transform(const cv::Mat& image);
void imageWarp();

void morphology(const cv::Mat& image);

void inpaint(cv::Mat& dst, const cv::Mat& src, const cv::Mat& dst_mask, const cv::Mat& src_mask, int patch_size);

void applyLip(const std::string& image_name);
void applyBlush(const std::string& image_name);
void applyEyeShadow(const std::string& image_name);
void applyEyeLash(const std::string& image_name);
void applyBrow(const std::string& image_name);

#endif /* EXAMPLE_MAKEUP_ */