#ifndef EXAMPLE_MAKEUP_
#define EXAMPLE_MAKEUP_

#include <string>

#include <opencv2/core/mat.hpp>

void detectFace(const cv::Mat& image, const std::string& image_name = std::string());
void mark(cv::Mat& image, const std::vector<cv::Point2f>& points);
void mark(const std::string& image_name);

void detectFaceSkin(const std::string& image_name);
void judgeFaceShape(const std::string& image_name);

void createShape();
void transform(const cv::Mat& image);
void imageWarp();

void morphology(const cv::Mat& image);

/**
 * Inpaint image.
 *
 * Implementation of the exemplar based inpainting algorithm described in
 * "Object Removal by Exemplar-Based Inpainting", A. Criminisi et. al.
 * 
 * @param dst        The result
 * @param src        Image to be inpainted.
 * @param dst_mask   Region to be inpainted.
 * @param src_mask   Optional mask that specifies the region of the image to synthezise from. If left empty
 *                   the entire image without the target mask is used.
 * @param patch_size Patch size to use.
 */
void inpaint(cv::Mat& dst, const cv::Mat& src, const cv::Mat& dst_mask, const cv::Mat& src_mask, int patch_size);

void applyLip(const std::string& image_name);
void applyBlush(const std::string& image_name);
void applyEyeShadow(const std::string& image_name);
void applyEyeLash(const std::string& image_name);
void applyBrow(const std::string& image_name);
void applyIris(const std::string& image_name);

void markBlush(const std::string& image_name);

#endif /* EXAMPLE_MAKEUP_ */