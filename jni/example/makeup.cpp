#define _USE_MATH_DEFINES

#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdint.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>

#include "example/makeup.h"
#include "example/utility.h"

#include "venus/Effect.h"
#include "venus/Feature.h"
#include "venus/ImageWarp.h"
#include "venus/inpaint.h"
#include "venus/Makeup.h"
#include "venus/opencv_utility.h"
#include "venus/Scalar.h"

using namespace cv;
using namespace venus;

void detectFace(const cv::Mat& image, const std::string& image_name/* = std::string()*/)
{
	Mat gray = Effect::grayscale(image);

	bool found = false;
	std::vector<std::vector<Point2f>> faces;

	// how about checking from -45 degree to 45 degree? And walk from middle to both sides.
	// 0  1  2  3  4  5  6
	// V  V  V  V  V  V  V   => f(t) = (-1)^t * ceil(t/2);
	// 0 -1  1 -2  2 -3  3
	for(int i = 0; i <= 10; ++i)
	{	
		int t = (i + 1)/2 * 9;  // every 9 degrees
		if(i%2 != 0)
			t = -t;

		std::cout << "try angle: " << std::setw(3) << t << '\n';
		float angle = t * static_cast<float>(M_PI/180);

		// top left point(0, 0), bottom right point(rows-1, cols-1)
		cv::Point2f center(gray.rows/2.0f, gray.cols/2.0f);
		Size size = gray.size();
		const Mat affine = Region::transform(size, center, angle);

		cv::Mat gray2;
		cv::warpAffine(gray, gray2, affine, size, cv::INTER_LANCZOS4, cv::BORDER_CONSTANT);  // INTER_CUBIC

		std::string tag;
		if(image_name.empty())
			tag = __FUNCTION__;
		else
			tag = image_name;

		faces = Feature::detectFaces(gray2, tag, CLASSIFIER_DIR);
		if(!faces.empty())
		{
			cv::Mat inverse = Region::invert(affine);
			for(std::vector<Point2f>& face: faces)
				cv::transform(face, face, inverse);

			found = true;
			break;  // and finally, we find a face in the image.
		}
	}

	if(!found)
	{
		std::cout << "NO face detected, tried rotated image without success.\n";
		return;
	}
}

void mark(const std::string& image_name)
{
	Mat image = cv::imread(image_name);
	
	Mat gray  = Effect::grayscale(image);
	const std::vector<std::vector<cv::Point2f>> faces = Feature::detectFaces(gray, image_name, CLASSIFIER_DIR);
	if(faces.empty())
	{
		std::cout << "no face found\n";
		return;
	}

	for(const std::vector<cv::Point2f>& face: faces)
		mark(image, face);

	cv::imshow(image_name, image);
/*
	std::vector<int> params;
	params.push_back(IMWRITE_JPEG_QUALITY);
	params.push_back(88);
	std::string dst_name = "G:/data/mark/" + basename(image_name);
	cv::imwrite(dst_name, image, params);
*/
}

void mark(cv::Mat& image, const std::vector<cv::Point2f>& points)
{
/**************************** getSymmetryAxis ***************************************/
	// those points are one the vertical line.
	std::vector<cv::Point2f> points_on_line;
	points_on_line.push_back(points[16]);
	points_on_line.push_back(points[ 6]);

	// right 15 14 13 12 11 10  9  8  7
	// left  17 18 19  0  1  2  3  4  5
	// (right + left)/2 are approximately on the line
	for(int i = 15, j = 17; i >= 7; --i, j = (j+1)%20)
		points_on_line.push_back((points[i] + points[j])/2);
	
	// eye brow
	for(int i = 20; i <= 24; ++i)
		points_on_line.push_back((points[i] + points[i + 7])/2);

	// eye, currently STASM doesn't detect eyes precisely.
/*
	for(int i = 34; i <= 41; ++i)
		points_on_line.push_back((points[i] + points[i + 10])/2);
	points_on_line.push_back((points[42] + points[43])/2);
*/
	// nose
	points_on_line.push_back(points[53]);
	points_on_line.push_back((points[52] + points[54])/2);
	points_on_line.push_back(points[56]);
	points_on_line.push_back((points[62] + points[58])/2);
	points_on_line.push_back((points[55] + points[57])/2);
	points_on_line.push_back((points[61] + points[59])/2);
	points_on_line.push_back(points[60]);

	// mouth
	// mouth can varies a lot with respect to different expressions, so overlook lower lip.
	points_on_line.push_back(points[66]);
	points_on_line.push_back(points[71]);
	points_on_line.push_back((points[65] + points[67])/2);
	points_on_line.push_back((points[64] + points[68])/2);
/*******************************************************************/

	//void drawCross(cv::Mat& image, const cv::Point2f& point, float length);
	using venus::drawCross;
	float length = 8.0F;
	int radius = static_cast<int>(length / std::sqrt(8.0F));
	for(const cv::Point2f& point: points_on_line)
		drawCross(image, point, radius, CV_RGB(0, 255, 0), 1, LINE_AA);

 	cv::Vec4f line;
	cv::fitLine(points_on_line, line, CV_DIST_L1, 0, 10.0, 0.01);
//	cv::line(image, Point2f(line[2], line[3]), Point2f(line[2] + 100*line[0], line[3] + 100*line[1]),  CV_RGB(0, 255, 0), 1, LINE_AA);
	venus::drawLine(image, Point2f(line[2], line[3]), Point2f(line[2] + 100*line[0], line[3] + 100*line[1]), CV_RGB(0, 255, 0), 1, LINE_AA);
	std::cout << "angle: " << std::atan2(line[1], line[0]) * (180/M_PI) << std::endl;

	radius = 12;
	Point2f pt0(line[2], line[3]);
	drawCross(image, pt0, radius, CV_RGB(0, 255, 0), 1, LINE_AA);

	Point2f pt1 = pt0 + Point2f(line[0], line[1]) * 100.0f;
	drawCross(image, pt1, radius, CV_RGB(0, 255, 128), 1, LINE_AA);

	std::cout << "angle2: " "x " << line[0] << " y: " << line[1] << " k "<< line[0]/line[1] << std::endl;

	Point2f center = (points[0] + points[12]) / 2;
	float t = (line[0] * (center.x - line[2]) + line[1] * (center.y - line[3])) / (line[0] * line[0] + line[1] * line[1]);
	std::cout << t << std::endl;  // 576.337, 641.126
	center.x = line[2] + line[0] * t;
	center.y = line[3] + line[1] * t;
	std::cout << "center: " << center.x << ", " << center.y << std::endl;
	drawCross(image, center, radius, CV_RGB(0, 255, 128), 1, LINE_AA);

	Feature::markWithIndices(image, points);
	cv::imshow(__FUNCTION__, image);
}

void detectFaceSkin(const std::string& image_name)
{
	Mat image = imread(image_name);
	cv::imshow("image", image);

	if(image.channels() == 3)
		cvtColor(image, image, CV_BGR2RGBA);

	// use face detector to detect face.
	// CV_RGB2GRAY for Android Bitmap, CV_BG42GRAY for OpenCV Mat
	Mat gray = Effect::grayscale(image);

//	assert(image.channels() == 1);  // gray image
	const std::vector<std::vector<Point2f>> faces = Feature::detectFaces(gray, image_name, CLASSIFIER_DIR);
	assert(!faces.empty());
	const std::vector<Point2f> points = faces[0];
	
	const Point2f& point = points[56];
	cv::Vec4b skin_color = image.at<cv::Vec4b>(cvRound(point.y), cvRound(point.x));
//	float threshold = probability(skin_color[2], skin_color[1], skin_color[0]);
//	float threshold = probability(190, 108, 86);
//	Mat mask(image.size(), CV_8UC1);

	Mat mask_skin = Feature::maskSkinRegion(image.cols, image.rows, points);
	cv::imshow(__FUNCTION__, mask_skin);
}

/*
	L 越接近 rect_length 则越可能是方脸
	face_size.x /face_size.y -> 4/6 ，三庭五眼的分割距离均等， 标准脸
	圆形脸（娃娃脸） 1:1 脸部有肉，两腮丰满，给人单纯可爱的感觉
	方形脸（国字脸） 1:1 正方形，额头宽大，棱角突出。额头、颧骨、下颌的宽度基本相同
	长方形脸（马脸） 发际线接近水平，额头高，面颊线条较直
	倒三角（心型脸） 额部较宽，下巴窄而尖，上大下小的比例。
	正三角（梨型脸）额头窄，两腮宽大
	菱形脸（申子脸）清瘦，颧骨突出，尖下额，发际线较窄

	面积一定，周长越小则越光滑，越接近圆形。
	S = pi*r*r, L = 2*pi*r, rho = L^2 / (4*pi*S)
	圆形： rho = 1.0
	正方形：rho = pi/4 = 0.79
	正三角形 rho = pi*sqrt(3)/9 = 0.60

	rho = S/S_circle
	S 为区域面积，S_inner 为区域最小外接圆面积

	rho = S/(width*width)
	S 为区域面积，width 为长轴长度

	矩特征是建立在对一个区域内部会对值分布的统计分析基础上的，是一种统计平均的描述，
	可以从全局观点描述对象的整体特征。矩是一种线性特征，对图像的旋转、缩放、平移具有不变性。

	shape factor 有关形状定量测度（矩，面积，周长）
	圆度 偏心率
*/
void judgeFaceShape(const std::string& image_name)
{
	Mat image = imread(image_name);
	Mat gray  = Effect::grayscale(image);
	const std::vector<Point2f> points = Feature::detectFace(gray, image_name, CLASSIFIER_DIR);
	assert(!points.empty());

	float L = 0.0f;  // circumference
	for(int i = 0; i < 19; ++i)
		L += distance(points[i], points[i+1]);
	L += distance(points[19], points[0]);

	float diff_0_12 = points[12].x - points[0].x;
	float diff_1_11 = points[11].x - points[1].x;
	float diff_2_10 = points[10].x - points[2].x;
	float width = std::max(std::max(diff_0_12, diff_1_11), diff_2_10);

	float height = points[6].y - points[16].y;
	Point2f face_size(width, height);

	float radius = std::min(face_size.x, face_size.y);
	float S = static_cast<float>(M_PI) * radius * radius;
	float roundness = 4 * static_cast<float>(M_PI) * S / (L*L);
	printf("roundness: %f\n", roundness);
	float rect_length = (face_size.x + face_size.y) * 2;
}

void createShape()
{
	float R = 168;  // pixels

	// draw a star, https://en.wikipedia.org/wiki/Pentagon
	const int N = 10;
	std::vector<Point2f> star(N);

	// sine law, sin(180° - 108°/2)/R = sin(36°/2)/r
	// sin(pi - theta) = sin(theta)
	float r = R * static_cast<float>(std::sin(M_PI/10) / std::sin(3*M_PI/10));
	for(int i = 0; i < N; ++i)
	{
		float& L = (i%2 == 0) ? R:r;
		float t = static_cast<float>(i * 2*M_PI/N - M_PI/2);
		star[i] = Point2f(R + L * std::cos(t), R + L * std::sin(t));
	}

//	Mat mask = Feature::createMask(polygon, R/18);
	Rect rect_star = cv::boundingRect(star);
	Mat mask_star = Feature::maskPolygonSmooth(rect_star, star);

	// draw a heart
	float angle = static_cast<float>(M_PI/10);
	std::vector<Point2f> heart = Makeup::createHeartShape(Point2f(R, R), R, angle);
	Rect rect_heart = cv::boundingRect(heart);
	Mat mask_heart = Feature::maskPolygonSmooth(rect_heart, heart, 8);

	uint32_t color = 0xFF0000FF;  // red
//	cv::imwrite(PROJECT_DIR + "star.png", mask_star);
	cv::imshow("star", mask_star);
	cv::imshow("heart", mask_heart);
}

void transform(const cv::Mat& image)
{
	Mat image2;
	cvtColor(image, image2, CV_BGR2BGRA);

	constexpr int radius = 8;
	Point2f pivot(image.cols/2.0F, image.rows/2.0F);
	drawCross(image2, pivot, radius, CV_RGB(0, 255, 0));
	Size size = image.size();
	float angle = static_cast<float>(M_PI/3);
	Mat affine = Region::transform(size, pivot, angle, Vec2f(0.5F, 0.32F));

	cv::Mat target;
	cv::warpAffine(image, target, affine, size, cv::INTER_CUBIC, cv::BORDER_REFLECT101);

	drawCross(target, pivot, radius, CV_RGB(0, 255, 0));
//	cv::imwrite(PROJECT_DIR + "transformed.png", target);
	cv::imshow("source", image2);
	cv::imshow("target", target);
}

void imageWarp()
{
	// monalisa's smile
	const std::string image_name(PROJECT_DIR + "doc/monalisa.jpg");
	Mat image = imread(image_name, IMREAD_UNCHANGED);

	ImageWarp_Rigid warp;
	const std::vector<Point2f> src
	{
		Point2f(186, 140),
		Point2f(295, 135),
		Point2f(208, 181),
		Point2f(261, 181),
		Point2f(184, 203),
		Point2f(304, 202),
		Point2f(213, 225),
		Point2f(243, 225),
		Point2f(211, 244),
		Point2f(253, 244),
		Point2f(195, 254),
		Point2f(232, 281),
		Point2f(285, 252),
	};

	const std::vector<Point2f> dst
	{
		Point2f(186, 140),
		Point2f(295, 135),
		Point2f(208, 181),
		Point2f(261, 181),
		Point2f(184, 203),
		Point2f(304, 202),
		Point2f(213, 225),
		Point2f(243, 225),
		Point2f(207, 238),
		Point2f(261, 237),
		Point2f(199, 253),
		Point2f(232, 281),
		Point2f(279, 249),
	};

//	warp.setMappingPoints(src, dst);  // reverse effect
	warp.setMappingPoints(dst, src);
	warp.setSourceSize(image.cols, image.rows);
	warp.setTargetSize(image.cols, image.rows);
	warp.calculateDelta(0.56F);
	Mat target = warp.genNewImage(image, 1.0F);
	cv::imshow("target", target);
//	imwrite(PROJECT_DIR + "target.png", target);
}

// http://docs.opencv.org/2.4/doc/tutorials/imgproc/opening_closing_hats/opening_closing_hats.html
void morphology(const cv::Mat& image)
{
	int morph_radius = 1;
	int morph_size = morph_radius * 2 + 1;
	Mat element = getStructuringElement(cv::MORPH_RECT, Size(morph_size, morph_size), Point(morph_radius, morph_radius));

	// Apply the specified morphology operation
	Mat image_processed;
#if 0  // functionally equivalent
	erode(image, image_processed, element, Point(-1, -1), 1);
	dilate(image_processed, image_processed, element, Point(-1, -1), 1);
#else
	// dst = close(src, element) = dilate(erode(src, element))
	morphologyEx(image, image_processed, cv::MORPH_OPEN, element);
#endif
	imshow(__FUNCTION__, image_processed);
}


/**
 * Find candidate positions for template matching.
 *
 * This is a convinience method for using TemplateMatchCandidates.
 *
 * @param image Image to search in
 * @param templ Template image
 * @param templMask Optional template mask
 * @param candidate A mask of possible candidates. If image size is W,H and template size is w,h
 *         the size of candidate will be W - w + 1, H - h + 1.
 * @param partitionSize Number of blocks to subdivide template into
 * @param maxWeakErrors Max classification mismatches per channel.
 * @param maxMeanDifference Max difference of patch / template mean before rejecting a candidate.
 */
void findTemplateMatchCandidates(
    const cv::Mat& image,
    const cv::Mat& templ,
    const cv::Mat& templMask,
    cv::Mat& candidates,
    cv::Size partitionSize = cv::Size(3,3),
    int maxWeakErrors = 3,
    float maxMeanDifference = 20)
{
    TemplateMatchCandidates tmc;
    tmc.setSourceImage(image);
    tmc.setPartitionSize(partitionSize);
    tmc.setTemplateSize(templ.size());
    tmc.initialize();

    candidates.create(
        image.size().height - templ.size().height + 1,
        image.size().width - templ.size().width + 1,
        CV_8UC1);

    tmc.findCandidates(templ, templMask, candidates, maxWeakErrors, maxMeanDifference);
}

void inpaint(cv::Mat& dst, const cv::Mat& src, const cv::Mat& dst_mask, const cv::Mat& src_mask, int patch_size)
{
	Inpainter inpainter;
	inpainter.setSourceImage(src);
	inpainter.setSourceMask(src_mask);
	inpainter.setTargetMask(dst_mask);
	inpainter.setPatchSize(patch_size);
	inpainter.initialize();
	
	while(inpainter.hasMoreSteps())
		inpainter.step();

	inpainter.image().copyTo(dst);
}

void applyLip(const std::string& image_name)
{
	Mat image = cv::imread(image_name, cv::IMREAD_UNCHANGED);
	Mat gray  = Effect::grayscale(image);

	const std::vector<Point2f> points = Feature::detectFace(gray, image_name, CLASSIFIER_DIR);
	assert(!points.empty());

	float amount = 0.80F;
	const uint32_t color = 0x556722BF;
#if 0
	RoiInfo lips = calcuateLipsRegionInfo(points);

	Rect lips_region(lips.origin, lips.mask.size());

	blendColor(image(lips_regin), lips.mask, color, amount);
#else
	cvtColor(image, image, CV_RGB2RGBA);
	Feature feature(image, points);
	Region region = feature.calculateLipshRegion();
	Mat& mask = region.mask;
	cv::imshow("lips", region.mask);
	Point2i origin = static_cast<Point2i>(region.pivot - Point2f(mask.cols, mask.rows)/2.0F);
	Mat result;
	Makeup::applyLip(result, image, points, color, amount);
//	imwrite(PROJECT_DIR + "lip.png", result);
//	cvtColor(image, image, CV_RGBA2RGB);
#endif
	cv::imshow(__FUNCTION__, image);
}

void applyBlush(const std::string& image_name)
{
	Mat image = cv::imread(image_name, cv::IMREAD_UNCHANGED);
	Mat gray  = Effect::grayscale(image);

	if(image.type() == CV_8UC3)
		cvtColor(image, image, CV_BGR2BGRA);

	const std::vector<Point2f> points = Feature::detectFace(gray, image_name, CLASSIFIER_DIR);
	assert(!points.empty());

	Mat result;
	uint32_t color = 0xFEEDBEEF;
	Makeup::applyBlush(result, image, points, Makeup::BlushShape::HEART, color, 0.99F);
	cv::imshow(__FUNCTION__, result);
}

void applyEyeShadow(const std::string& image_name)
{
	Mat image = cv::imread(image_name, cv::IMREAD_UNCHANGED);
	Mat gray = Effect::grayscale(image);

	const std::vector<Point2f> points = Feature::detectFace(gray, image_name, CLASSIFIER_DIR);
	assert(!points.empty());

	if(image.channels() == 3)
		cv::cvtColor(image, image, CV_BGR2BGRA);
	assert(image.type() == CV_8UC4);

	// Input makeup parameters description:
	// Eye shadow use 3 layers for color blending, each layer is a composition of alpha mask and a primary color.
	// maybe I can store 3 mask into one RGB image? It sounds like a novel idea.
	int index = 0; // 00 ~ 09
	uint32_t color[3] = { 0xFF0000FF, 0xFF008FD2, 0xFFFF8A00 };
	float amount = 0.99F;  // [0.0, 1.0]

	std::ostringstream stream;
	stream << PROJECT_DIR << "res/drawable-nodpi/eye_shadow_" << std::setw(3) << std::setfill('0') << index << ".png";
	std::string filename = stream.str();
	char& ch = filename[filename.length() - 5/* "0.png" */];

	Mat mask[3];
	for(int i = 0; i < 3; ++i)
	{
		++ch;
		mask[i] = imread(filename, CV_LOAD_IMAGE_UNCHANGED);
		assert(!mask[i].empty());
		assert(mask[i].type() == CV_8UC1);
		assert(mask[0].size() == mask[i].size());
	}

	uint32_t color2 = 0;
	Mat result;
	Mat eye_shadow = Makeup::createEyeShadow(mask, color);

	Makeup::applyEyeShadow(result, image, points, mask, color, amount);

	cv::imshow(__FUNCTION__, result);
}

void applyEyeLash(const std::string& image_name)
{
	Mat image = cv::imread(image_name, cv::IMREAD_UNCHANGED);
	Mat gray  = Effect::grayscale(image);

	const std::vector<Point2f> points = Feature::detectFace(gray, image_name, CLASSIFIER_DIR);
	assert(!points.empty());

	if(image.channels() == 3)
		cv::cvtColor(image, image, CV_BGR2BGRA);  // add alpha channel

	int index = 6;  // 0 ~ 9
	std::ostringstream stream;
	stream << PROJECT_DIR << "res/drawable-nodpi/eye_lash_" << std::setw(2) << std::setfill('0') << index << ".png";
	std::string eye_lash_filename = stream.str();

	std::cout << eye_lash_filename << std::endl;
	Mat eye_lash = cv::imread(eye_lash_filename, cv::IMREAD_UNCHANGED);
	assert(!eye_lash.empty() && eye_lash.channels() == 1);

	uint32_t color = 0xFFE3003E;
	float amount = 0.81F;
	
	Mat result;
	Makeup::applyEyeLash(result, image, points, eye_lash, color, amount);
	cv::imshow(__FUNCTION__, result);
}

void applyBrow(const std::string& image_name)
{
	Mat image = cv::imread(image_name, cv::IMREAD_UNCHANGED);
	Mat gray  = Effect::grayscale(image);

	const std::vector<Point2f> points = Feature::detectFace(gray, image_name, CLASSIFIER_DIR);
	assert(!points.empty());

	if(image.channels() == 3)
		cv::cvtColor(image, image, CV_BGR2BGRA);
	
#if 0
	int index = 6;  // 0 ~ 15
	std::ostringstream stream;
	stream << PROJECT_DIR << "res/drawable-nodpi/eye_brow_mask_" << std::setw(2) << std::setfill('0') << index << ".png";
	std::string target_brow_filename = stream.str();

	Mat mask = cv::imread(target_brow_filename, cv::IMREAD_UNCHANGED);
//	assert(!target_brow.empty() && target_brow.channels() == 4);
	uint32_t color = 0xEE070909;
	Mat target_brow = Makeup::pack(mask, color);

	// Sighs, OpenCV doesn't handle alpha channel correctly in cv::imshow, I've got to use other image viewers to check the result.
//	cv::imshow("target_brow", target_brow);
//	cv::imwrite(PROJECT_DIR + "target_brow.png", target_brow);

	Rect rect = Region::boundingRect(target_brow);
	std::cout << "left: " << rect.x << ", right: " << (rect.x + rect.width)  << '\n'
	          << "top: "  << rect.y << ", bottom: "<< (rect.y + rect.height) << '\n';
#else
	int index = 5;  // 0 ~ 15
	std::ostringstream stream;
	stream << PROJECT_DIR << "res/drawable-nodpi/eye_brow_" << std::setw(2) << std::setfill('0') << index << ".png";
	std::string target_brow_filename = stream.str();

	Mat mask = cv::imread(target_brow_filename, cv::IMREAD_UNCHANGED);
	uint32_t color = 0;  // useless paramter
#endif
	std::cout << target_brow_filename << std::endl;

	Feature feature(image, points);
	Vec4f line = feature.getSymmetryAxis();
	float angle = std::atan2(line[1], line[0]) - static_cast<float>(M_PI/2);
	std::cout << "skew angle: " << rad2deg(angle) << '\n';

	Mat result;
	Makeup::applyBrow(result, image, points, mask, color, 0.81F， 0.0F);

#if 0
	for(int i = 0; i <= 1; ++i)
	{
		const bool right = (i == 0);
		std::vector<Point2f> polygon = Feature::calculateBrowPolygon(points, right);
		Point2i position;
		Mat mask = Feature::createMask(polygon, 0.0F, &position);

		int out_count = 0;
		Vec4f out_sum(0.0F, 0.0F, 0.0F, 0.0F);

		for(int r = 0; r < mask.rows; ++r)
		for(int c = 0; c < mask.cols; ++c)
		{
			if(mask.at<uint8_t>(r, c) != 0)
				continue;

			// ouside the brow region
			const Vec4b& color = image.at<Vec4b>(r + position.y, c + position.x);
			out_sum += color;
			++out_count;
		}
	
		Vec4f out_avg = (out_count == 0) ? out_sum : (out_sum / out_count);
		Mat roi(mask.size(), CV_8UC4, out_avg);

		constexpr int L = 8;
		for(int r = 0; r < mask.rows; ++r)
		for(int c = 0; c < mask.cols; ++c)
		{
			const Vec4b& image_color = image.at<Vec4b>(r + position.y, c + position.x);
			Vec4b& region_color = roi.at<Vec4b>(r, c);
			for(int i = 0; i < 3; ++i)
				region_color[i] = saturate_cast<uint8_t>(region_color[i] + (image_color[i]/L) - (255/(L+1)));
		}

		int offset = mask.rows >> 1;  // can be tuned!
#if 0	
		position.x -= offset;
		position.y -= offset;
		mask = Region::inset(mask, -offset);
		Region::grow(mask, mask, mask.rows >> 3);
#endif

		Rect rect(position, mask.size());
		Effect::gaussianBlur(mask, offset);
		roi.copyTo(image(rect), mask);

		Region region = feature.calculateBrowRegion(right);
		Size2f& region_size = region.size;
		Size target_size = rect.size();
		Vec2f scale(region_size.width / target_size.width, region_size.height / target_size.height);
		
		Mat affine = Region::transform(target_size, region.pivot, angle, scale);

		if(!right)
			cv::flip(target_brow, target_brow, 1/* horizontally */);

		cv::Mat affined_brow;
		cv::warpAffine(target_brow, affined_brow, affine, target_size, cv::INTER_LANCZOS4, cv::BORDER_CONSTANT);
		Size2i affined_brow_size(affined_brow.cols, affined_brow.rows);
		Point2i origin = region.pivot - Point2f(affined_brow_size)/2;
		rect = Rect(origin, affined_brow_size);
//		affined_brow.copyTo(image(rect), affined_brow);
	}
#endif
#if 0

//	float offset = 9.0f;
//	brow_r.inset(offset);
//	brow_l.inset(offset);
	const Point2f& pivot_r = region_brow_r.pivot, &pivot_l = region_brow_l.pivot;
//	Size2f& size = brow_r.size;
	Mat& brow_mask_r = region_brow_r.mask, brow_mask_l = region_brow_l.mask;
	Size& size_r = brow_mask_r.size(), size_l = brow_mask_l.size();

	imshow("brow_r.mask", region_brow_r.mask);

//	Mat mask2 = Region::inset(brow_mask_r, radius);

	
//	Mat brow_grow_r = Region::grow(mask2, radius);
	imshow("mask", brow_mask_r);
//	imshow("mask2", mask2);
	Rect rect_r(static_cast<int>(pivot_r.x - size_r.width/2), static_cast<int>(pivot_r.y - size_r.height/2),
		cvCeil(size_r.width), cvCeil(size_r.height));
	Rect rect_l(static_cast<int>(pivot_l.x - size_l.width/2), static_cast<int>(pivot_l.y - size_l.height/2),
		cvCeil(size_l.width), cvCeil(size_l.height));
	Mat brow_roi_r = image(rect_r).clone();
	Mat brow_roi_l = image(rect_l).clone();

//	imshow("brow_roi", brow_roi_r);

	// [130 x 96]  [109 x 81]
//	std::cout << brow_roi.size() << "  " << mask.size() << '\n';
//	blend(image(rect_r), brow_roi, mask, rect_r.tl(), 1.0f);
	brow_roi_r.copyTo(image(rect_r), brow_mask_r);//Region::grow(mask, 5));
	brow_roi_l.copyTo(image(rect_l), brow_mask_l);

	
//	Size2f makeup_brow_size(148 - 7, 52 - 10);  // left 7, right 148, top 10, bottom 52
	Size makeup_brow_size_r = makeup_brow.size();
	Size makeup_brow_size_l = makeup_brow.size();
	
//	makeup_brow_r.copyTo(image(Region::getRect(pivot_r, size_r)), makeup_brow_r);
//	makeup_brow_l.copyTo(image(Region::getRect(pivot_l, size_l)), makeup_brow_l);
#endif
	cv::imshow(__FUNCTION__, result);
}

void applyIris(const std::string& image_name)
{
	Mat image = cv::imread(image_name, cv::IMREAD_UNCHANGED);
	Mat gray  = Effect::grayscale(image);
	std::cout << "channels: " << image.channels() << '\n';

	const std::vector<Point2f> points = Feature::detectFace(gray, image_name, CLASSIFIER_DIR);
	assert(!points.empty());

	Mat mask[2];
	std::string filename[2];
	int index = 7;  // 0 ~ 15
	index *= 10;
	for(int i = 0; i < 2; ++i)
	{
		std::ostringstream stream;
		stream << PROJECT_DIR << "res/drawable-nodpi/iris_" << std::setw(3) << std::setfill('0') << index+i << ".png";
		std::string filename = stream.str();
		mask[i] = imread(filename, IMREAD_UNCHANGED);
		assert(mask[i].type() == CV_8UC4 && mask[i].rows == mask[i].cols);
	}
	cv::imshow("mask", mask[0]);
	Feature feature(image, points);

#if 1
	Makeup::applyIris(image, image, points, mask[0], 0.0F);
#else
	for(int i = 0; i < 2; ++i)
	{
		bool is_right = (i == 0);
		const std::pair<cv::Point2f, float> iris_info = Feature::calculateIrisInfo(points, is_right);
		const cv::Point2f& center = iris_info.first;
		const float& radius = iris_info.second;
//		cv::circle(image, center, radius, CV_RGB(0, 255, 0), 1, LINE_AA);
		Mat scaled;
		printf("radius: %f, image: %d\n", radius, mask[0].rows);
		float scale = radius*2 / mask[0].rows;
		cv::resize(mask[0], scaled, cv::Size(/*cvRound(radius), cvRound(radius)*/), scale, scale, cv::INTER_LINEAR);
//		cv::imshow("scaled" + std::to_string(i), scaled);

		// blend color
		Point2f origin = center - Point2f(scaled.cols, scaled.rows)/2;
		
		Region region = feature.calculateEyeRegion(is_right);
		Makeup::blend(image, image, scaled, region.mask, origin, 1.0F);
	}
#endif
	cv::imshow(__FUNCTION__, image);
}
void markBlush(const std::string& image_name)
{
	Mat image = cv::imread(image_name, cv::IMREAD_UNCHANGED);
	Mat gray  = Effect::grayscale(image);

	const std::vector<Point2f> points = Feature::detectFace(gray, image_name, CLASSIFIER_DIR);
	assert(!points.empty());

	Vec4f line = Feature::getSymmetryAxis(points);
	float angle = std::atan2(line[1], line[0]) - static_cast<float>(M_PI/2);
	std::cout << __FUNCTION__ << " angle: " << angle << '\n';
	for(int i = 0; i < 2; ++i)
	{
		RotatedRect rotated_rect = Feature::calculateBlushRectangle(points, angle, i == 0);
		venus::drawRotatedRect(image, rotated_rect, Scalar(0, 255, 0));
	}

	cv::imshow(__FUNCTION__, image);
}