#include <opencv2/imgcodecs.hpp>

#include "venus/blur.h"
#include "venus/Effect.h"
#include "venus/Feature.h"
#include "venus/opencv_utility.h"
#include "venus/scalar.h"

#include "stasm/stasm_lib.h"


using namespace cv;

namespace venus {

// number of feature points detected
const size_t Feature::COUNT = 81;

// https://en.wikipedia.org/wiki/Delaunay_triangulation
// Delaunay triangle indices generated from mark() function
const std::vector<Vec3b> Feature::triangle_indices
{
	Vec3b(26, 16, 27),
	Vec3b(16, 26, 25),
	Vec3b(60, 57, 67),
	Vec3b(57, 60, 56),
	Vec3b( 1,  0, 38),
	Vec3b( 2,  1, 39),
	Vec3b(60, 55, 56),
	Vec3b(55, 60, 65),
	Vec3b( 3,  2, 63),
	Vec3b(53, 44, 52),
	Vec3b(44, 53, 34),
	Vec3b( 4,  3, 63),
	Vec3b(44, 25, 26),
	Vec3b(25, 44, 34),
	Vec3b( 5,  4, 79),
	Vec3b( 2, 40, 62),
	Vec3b(40,  2, 39),
	Vec3b( 6,  5, 78),
	Vec3b( 7,  6, 78),
	Vec3b( 8,  7, 77),
	Vec3b(35, 25, 34),
	Vec3b(25, 35, 24),
	Vec3b( 9,  8, 69),
	Vec3b(50, 58, 52),
	Vec3b(58, 50, 49),
	Vec3b(10,  9, 69),
	Vec3b(23, 33, 37),
	Vec3b(33, 23, 24),
	Vec3b(11, 10, 58),
	Vec3b(15, 27, 16),
	Vec3b(27, 15, 14),
	Vec3b(12, 11, 48),
	Vec3b(16, 20, 17),
	Vec3b(20, 16, 25),
	Vec3b(13, 12, 29),
	Vec3b(14, 13, 28),
	Vec3b(21, 23, 22),
	Vec3b(23, 21, 20),
	Vec3b(23, 38, 22),
	Vec3b(38, 23, 37),
	Vec3b(18, 17, 20),
	Vec3b(18, 20, 21),
	Vec3b(19, 18, 21),
	Vec3b( 0, 19, 22),
	Vec3b(24, 23, 20),
	Vec3b(20, 25, 24),
	Vec3b(19, 21, 22),
	Vec3b( 0, 22, 38),
	Vec3b(24, 35, 33),
	Vec3b(52, 44, 51),
	Vec3b(44, 26, 45),
	Vec3b(11, 49, 48),
	Vec3b(49, 11, 58),
	Vec3b(27, 31, 26),
	Vec3b(31, 27, 30),
	Vec3b(48, 30, 29),
	Vec3b(30, 48, 47),
	Vec3b(28, 30, 27),
	Vec3b(30, 28, 29),
	Vec3b(27, 14, 28),
	Vec3b(28, 13, 29),
	Vec3b(29, 12, 48),
	Vec3b(47, 46, 32),
	Vec3b(46, 47, 43),
	Vec3b(26, 31, 45),
	Vec3b(31, 30, 32),
	Vec3b(32, 30, 47),
	Vec3b(31, 32, 45),
	Vec3b(49, 47, 48),
	Vec3b(47, 49, 43),
	Vec3b(37, 33, 36),
	Vec3b(42, 39, 37),
	Vec3b(39, 42, 40),
	Vec3b(54, 55, 61),
	Vec3b(55, 54, 56),
	Vec3b(34, 53, 54),
	Vec3b(35, 34, 41),
	Vec3b(42, 37, 36),
	Vec3b(33, 35, 36),
	Vec3b(36, 35, 42),
	Vec3b(37, 39, 38),
	Vec3b(39,  1, 38),
	Vec3b(42, 41, 40),
	Vec3b(41, 42, 35),
	Vec3b(62, 63,  2),
	Vec3b(63, 62, 61),
	Vec3b(34, 54, 41),
	Vec3b(40, 41, 54),
	Vec3b(43, 50, 51),
	Vec3b(50, 43, 49),
	Vec3b(46, 45, 32),
	Vec3b(45, 46, 43),
	Vec3b(43, 51, 45),
	Vec3b(10, 69, 58),
	Vec3b(44, 45, 51),
	Vec3b(51, 50, 52),
	Vec3b(56, 53, 52),
	Vec3b(53, 56, 54),
	Vec3b(68, 57, 59),
	Vec3b(57, 68, 67),
	Vec3b(61, 64, 63),
	Vec3b(64, 61, 55),
	Vec3b(40, 54, 62),
	Vec3b(62, 54, 61),
	Vec3b(64, 55, 65),

	Vec3b(56, 52, 57),
	Vec3b(60, 67, 66),
	Vec3b(57, 52, 59),
	Vec3b(69,  8, 76),
	Vec3b(59, 52, 58),
	Vec3b(59, 58, 69),
	Vec3b(59, 69, 68),
        

	Vec3b( 4, 63, 80),
	Vec3b(65, 60, 66),
	Vec3b(77, 76,  8),
	Vec3b( 7, 78, 77),
	Vec3b(79,  4, 80),
	Vec3b(78,  5, 79),
/*
	// mouth region needs reconstruct subdiv
	    /64\    /66\    /68\
	   /    \65/    \67/    \   
	  /                      \
	63-----72----71----70-----69
	  \-----73---74----75-----/
	   \80\              /76/
		   \79---78---77/
*/
	// top lip
	Vec3b(63, 64, 72),
	Vec3b(64, 65, 72),
	Vec3b(65, 71, 72),
	Vec3b(65, 66, 71),
	Vec3b(66, 67, 71),
	Vec3b(71, 67, 70),
	Vec3b(67, 68, 70),
	Vec3b(68, 69, 70),

	// inner region of mouth, or teeth region
	Vec3b(69, 75, 70),
	Vec3b(70, 75, 71),
	Vec3b(71, 75, 74),
	Vec3b(71, 74, 73),
	Vec3b(72, 71, 73),
	Vec3b(73, 63, 72),

	// bottom lip
	Vec3b(63, 73, 80),
	Vec3b(80, 73, 79),
	Vec3b(79, 73, 78),
	Vec3b(73, 74, 78),
	Vec3b(78, 74, 75),
	Vec3b(78, 75, 77),
	Vec3b(77, 75, 76),
	Vec3b(76, 75, 69),
};

static constexpr HersheyFonts font_face = cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;

static inline std::string to_string(int i)
{
#ifdef ANDROID  // due to Android lack of function std::to_string()
	std::ostringstream os;
	os << i;
	const std::string text = os.str();
#else
	const std::string text = std::to_string(i);
#endif
	return text;
}

#define PRINT_TRIANGLE_INDEX 0
#if PRINT_TRIANGLE_INDEX
#include <unordered_map>

static std::vector<Vec3i> getTriangleIndex(const std::vector<Point2f>& points, const std::vector<Vec6f>& triangles)
{
	const size_t length = points.size();
	std::unordered_map<Point2f, size_t> table;  // not suitable
	table.reserve(length);
	for(size_t i = 0; i < length; ++i)
	{
		const Point2f& point = points[i];
		table[point] = i;
	}
	assert(table.size() == length);

	std::vector<Vec3i> result;
	const size_t triangle_count = triangles.size();
	result.reserve(triangle_count);

	for(const Vec6f& t: triangles)
	{
		Point2f A(t[0], t[1]), B(t[2], t[3]), C(t[4], t[5]);
		assert(table.find(A) != table.end());
		assert(table.find(B) != table.end());
		assert(table.find(C) != table.end());
		Vec3i index(table[A], table[B], table[C]);
		assert(index[0] != index[1] && index[1] != index[2]);
		result.push_back(index);
	}

	return result;
}
#endif

// used after subdiv.getTriangleList(triangles); filter those triangles that lies on border.
static std::vector<cv::Vec6f>& filter(std::vector<Vec6f>& triangles, const Rect& rect)
{
	std::vector<Vec6f>::iterator it = triangles.begin();
	while(it != triangles.end())
	{
		const Vec6f& t = *it;
		Point2i pt0(cvRound(t[0]), cvRound(t[1]));
		Point2i pt1(cvRound(t[2]), cvRound(t[3]));
		Point2i pt2(cvRound(t[4]), cvRound(t[5]));

		if(rect.contains(pt0) && rect.contains(pt1) && rect.contains(pt2))
			++it;
		else
			it = triangles.erase(it);
	}
	return triangles;
}

static void drawDelaunay(cv::Mat& image, const std::vector<cv::Vec6f>& triangles, const cv::Scalar& color)
{
	for(const cv::Vec6f& t: triangles)
	{
		cv::Point2i pt0(cvRound(t[0]), cvRound(t[1]));
		cv::Point2i pt1(cvRound(t[2]), cvRound(t[3]));
		cv::Point2i pt2(cvRound(t[4]), cvRound(t[5]));

		cv::line(image, pt0, pt1, color, 1, LINE_AA);
		cv::line(image, pt1, pt2, color, 1, LINE_AA);
		cv::line(image, pt2, pt0, color, 1, LINE_AA);
	}
}

void Feature::triangulate(cv::Mat& image, const std::vector<cv::Point2f>& points, const std::vector<cv::Vec3b>& triangles)
{
	const int width = image.cols, height = image.rows;
	const cv::Scalar line_color = CV_RGB(255, 0, 255), text_color = CV_RGB(0, 255, 0);
	
	const float font_scale = (std::min(width, height) <= 720) ? std::min(width, height)/720.0f : 1.0f;

	for(size_t i = 0, size = triangles.size(); i < size; ++i)
	{
		const Vec3i& tri = triangles[i];
		Point A(cvRound(points[tri[0]].x * width), cvRound(points[tri[0]].y * height));
		Point B(cvRound(points[tri[1]].x * width), cvRound(points[tri[1]].y * height));
		Point C(cvRound(points[tri[2]].x * width), cvRound(points[tri[2]].y * height));
		cv::line(image, A, B, line_color, 1, LINE_AA);
		cv::line(image, B, C, line_color, 1, LINE_AA);
		cv::line(image, C, A, line_color, 1, LINE_AA);

		int baseline = 0;
		const std::string text = to_string(i);
		Size text_size = getTextSize(text, font_face, font_scale, 1, &baseline);

		// align text center to triangle center
		Point center = (A + B + C) / 3;
		center.x -= text_size.width/2;
		center.x += text_size.height/2;

		cv::putText(image, text, center, font_face, font_scale, text_color, 1, LINE_AA);
	}
}

Feature::Feature(const cv::Mat& image, const std::vector<cv::Point2f>& points):
	image(image),
	points(points)
{
	// if(COUNT >= 256U) please change triangle_indices's type from std::vector<Vec3b> to std::vector<Vec3i>,
	// or at least vec3s (s is for int16_t), using vec3b here can reduce binary file size(25%).
	assert(points.size() == COUNT && COUNT < 256U);
	line = getSymmetryAxis(points);
}

/*
	Here are some useful links about hypot, I tried to use std::hypot(x, y), later changed to std::sqrt(a*a + b*b);
	http://stackoverflow.com/questions/3764978/why-hypot-function-is-so-slow
	also STL Bugs Fixed In Visual Studio 2012
	718865	STL: hypot not hoisted into the std namespace by cmath
	https://blogs.msdn.microsoft.com/vcblog/2012/06/15/stl-bugs-fixed-in-visual-studio-2012/
*/
static float angle(const Vec2f& v1, const Vec2f& v2)
{
	float numerator = v1[0]*v2[0] + v1[1]*v2[1];  // v1.dot(v2);
	float v1_length = std::sqrt(v1[0]*v1[0] + v1[1]*v1[1]);
	float v2_length = std::sqrt(v2[0]*v2[0] + v2[1]*v2[1]);
	float denominator = v1_length * v2_length;
//	assert(!isZero<T>(denominator));
	return std::acos(numerator/denominator);
}

static cv::Vec2f rotate(const cv::Vec2f& v, float angle)
{
//	std::complex<T>(x,y) * complex<T>(cos(angle), sin(angle));
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	float _x = c * v[0] - s * v[1];
	float _y = s * v[0] + c * v[1];

	return cv::Vec2f(_x, _y);
}

static void sort(std::vector<std::vector<cv::Point2f>>& faces)
{
	const size_t nb_face = faces.size();
	if(nb_face <= 1U)
		return;

	using Pair = std::pair<size_t, float>;
	std::vector<Pair> areas(nb_face);
	for(size_t i = 0; i < nb_face; ++i)
	{
		cv::Vec4f box = venus::boundingBox(faces[i]);
		float& left = box[0], &top = box[1], &right = box[2], &bottom = box[3];
		float area = (right - left) * (bottom - top);
		areas[i] = std::make_pair(i, area);
	}

	// http://stackoverflow.com/questions/9025084/sorting-a-vector-in-descending-order
	auto greater = [](const Pair& lhs, const Pair& rhs){ return lhs.second > rhs.second; };
	std::sort(areas.begin(), areas.end(), greater);

	for(size_t i = 0; i < nb_face; ++i)
	{
		size_t j = areas[i].first;
		if(i != j)
		{
			std::swap(faces[i], faces[j]);
			std::swap(areas[i].first, areas[j].first);
		}
	}
}

static std::vector<cv::Point2f> process(float landmarks[stasm_NLANDMARKS * 2])
{
	std::vector<Point2f> points;

#if 0
	points.reserve(stasm_NLANDMARKS);
    for(int i = 0; i < stasm_NLANDMARKS; i++)
	{
		printf("Point(%d, %d),\n", (int)landmarks[i*2], (int)landmarks[i*2+1]);
		points.push_back(Point2f(landmarks[i*2], landmarks[i*2+1]));
	}

#else  // add bonus feature points(use Bezier or spline curve) for better subdivision result.
	constexpr int extra_count = 4;
	points.reserve(stasm_NLANDMARKS + extra_count);

	int i = 0;
	for(; i < 13; ++i)
		points.push_back(Point2f(landmarks[i*2], landmarks[i*2+1]));
		
	{
		int ia = 13, ib = 12, ic = 11;  // triangle 11/12/13
		const Point2f& A = *reinterpret_cast<const Point2f*>(landmarks + ia*2);
		const Point2f& B = *reinterpret_cast<const Point2f*>(landmarks + ib*2);
		const Point2f& C = *reinterpret_cast<const Point2f*>(landmarks + ic*2);

		Point2f O = centerOfCircumscribedCircle(A, B, C);
		Vec2f vOA(A.x - O.x, A.y - O.y);
		Vec2f vOB(B.x - O.x, B.y - O.y);
		float theta = angle(vOA, vOB);

		Vec2f vOB_r = rotate(vOB, -theta/3);
		Vec2f vOA_l = rotate(vOA, +theta/3);
		points.push_back(Point2f(O.x + vOB_r[0], O.y + vOB_r[1]));
		points.push_back(Point2f(O.x + vOA_l[0], O.y + vOA_l[1]));
	}

	for(; i < 16; ++i)
		points.push_back(Point2f(landmarks[i*2], landmarks[i*2+1]));

	{
		int ia = 15, ib = 0, ic = 1;  // triangle 15/0/1
		const Point2f& A = *reinterpret_cast<const Point2f*>(landmarks + ia*2);
		const Point2f& B = *reinterpret_cast<const Point2f*>(landmarks + ib*2);
		const Point2f& C = *reinterpret_cast<const Point2f*>(landmarks + ic*2);

		Point2f O = centerOfCircumscribedCircle(A, B, C);
		Vec2f vOA(A.x - O.x, A.y - O.y);
		Vec2f vOB(B.x - O.x, B.y - O.y);
		float theta = angle(vOA, vOB);

		Vec2f vOA_r = rotate(vOA, -theta/3);
		Vec2f vOB_l = rotate(vOB, +theta/3);
		points.push_back(Point2f(O.x + vOA_r[0], O.y + vOA_r[1]));
		points.push_back(Point2f(O.x + vOB_l[0], O.y + vOB_l[1]));

//		points.push_back(O + vOA.rotate(-theta/3));
//		points.push_back(O + vOB.rotate(+theta/3));
	}

	for(; i < stasm_NLANDMARKS; ++i)
		points.push_back(Point2f(landmarks[i*2], landmarks[i*2+1]));

#if 0
	// add more points for better Delaunay triangulation result.
	Point2f pm1(points[22]), p0(points[21]), p1(points[20]), p2(points[25]);
	Point2f eye_brow_top_r = catmullRomSpline(0.50f, pm1, p0, p1, p2);
//	cv::circle(image, Point(cvRound(eye_brow_top_r.x), cvRound(eye_brow_top_r.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
	points.push_back(eye_brow_top_r);

	pm1 = points[29]; p0 = points[28]; p1 = points[27]; p2 = points[26];
	Point2f eye_brow_top_l = catmullRomSpline(0.50f, pm1, p0, p1, p2);
//	cv::circle(image, Point(cvRound(eye_brow_top_l.x), cvRound(eye_brow_top_l.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//	points.push_back(eye_brow_top_l);

	Point2f eye_brow_top_m = (eye_brow_top_r + eye_brow_top_l)/2;
//	cv::circle(image, Point(cvRound(eye_brow_top_m.x), cvRound(eye_brow_top_m.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//	points.push_back(eye_brow_top_m);

	Point2f eye_pupil_m = (points[42] + points[43])/2;
//	cv::circle(image, Point(cvRound(eye_pupil_m.x), cvRound(eye_pupil_m.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
	points.push_back(eye_pupil_m);

	// 3|9 62|58 63|69
	Point2f eye_cheek_1r = (points[3] + points[62] + 0.5 * points[63])/2.5F;
	Point2f eye_cheek_1l = (points[9] + points[58] + 0.5 * points[69])/2.5F;
//	cv::circle(image, Point(cvRound(eye_cheek_1r.x), cvRound(eye_cheek_1r.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//	cv::circle(image, Point(cvRound(eye_cheek_1l.x), cvRound(eye_cheek_1l.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
	points.push_back(eye_cheek_1r);
	points.push_back(eye_cheek_1l);

	// 40:50 62:58
	Point2f eye_cheek_2r = (points[40] + points[62])/2;
	Point2f eye_cheek_2l = (points[50] + points[58])/2;
//	cv::circle(image, Point(cvRound(eye_cheek_2r.x), cvRound(eye_cheek_2r.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//	cv::circle(image, Point(cvRound(eye_cheek_2l.x), cvRound(eye_cheek_2l.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
	points.push_back(eye_cheek_2r);
	points.push_back(eye_cheek_2l);

	// 39:49 1:11
	Point2f eye_cheek_3r = (points[39] + points[1])/2;
	Point2f eye_cheek_3l = (points[49] + points[11])/2;
//	cv::circle(image, Point(cvRound(eye_cheek_3r.x), cvRound(eye_cheek_3r.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//	cv::circle(image, Point(cvRound(eye_cheek_3l.x), cvRound(eye_cheek_3l.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
	points.push_back(eye_cheek_2r);
	points.push_back(eye_cheek_2l);
#endif

	return points;
}

std::vector<cv::Point2f> Feature::detectFace(const cv::Mat& image, const std::string& tag, const std::string& classifier_dir)
{
#if 0  // fallback plan use stasm_search_auto function
	auto faces = detectFaces(image, tag, classifier_dir);
	if(!faces.empty())
		return faces[0];  // the dominant face
	else
		return {};

#else  // use stasm_search_single function
	int foundface;
    float landmarks[stasm_NLANDMARKS * 2]; // x, y coords (note the 2)
	const char* image_path = tag.c_str();
    if(!stasm_search_single(&foundface, landmarks,
			reinterpret_cast<const char*>(image.data), image.cols, image.rows,
			image_path, classifier_dir.c_str()))
    {
        printf("Error in stasm_search_single: %s\n", stasm_lasterr());
		return {};
    }

    if(!foundface)
	{
		printf("No face found in %s\n", image_path);
		return {};
	}

	return process(landmarks);
#endif
}

std::vector<std::vector<cv::Point2f>> Feature::detectFaces(const cv::Mat& image, const std::string& tag, const std::string& classifier_dir)
{
	assert(image.channels() == 1);  // single channel required, namely gray image.
	std::vector<std::vector<cv::Point2f>> faces;
	
	if(!stasm_init(classifier_dir.c_str(), 0 /*trace*/))
		printf("stasm_init failed: %s\n", stasm_lasterr());

	const char* image_path = tag.c_str();
	const char* image_data = reinterpret_cast<const char*>(image.data);
	int allow_multiple_faces = 1;
	int min_width_percentage = 10;
	if(!stasm_open_image(image_data, image.cols, image.rows, image_path, allow_multiple_faces, min_width_percentage))
		printf("stasm_open_image failed: %s\n", stasm_lasterr());

	float landmarks[stasm_NLANDMARKS * 2]; // x, y coordinates
	while(true)
	{
		int found_face;
		if(!stasm_search_auto(&found_face, landmarks))
             printf("stasm_search_auto failed: %s\n", stasm_lasterr());

		if(!found_face)
			break;

		std::vector<cv::Point2f> points = process(landmarks);
		faces.push_back(std::move(points));
#endif
		// Stasm doesn't detect iris precisely, post-processing feature points for fine result.
//		correctIris(image, points);
    }

	sort(faces);  // sort multiple faces in area descending order
	return faces;
}

std::vector<std::vector<cv::Point2f>> Feature::detectFaces(cv::Size2i* size, const std::string& image_name, const std::string& classifier_dir)
{
	cv::Mat image = cv::imread(image_name, cv::IMREAD_GRAYSCALE);
	if(!image.data)
    {
        printf("Cannot load %s\n", image_name.c_str());
		return {};
    }

	if(size != nullptr)
		*size = Size2i(image.cols, image.rows);

	return detectFaces(image, image_name, classifier_dir);
}

void Feature::mark(Mat& image, const std::vector<Point2f>& points)
{
	const cv::Scalar text_color = CV_RGB(0, 255, 0);
	const float font_scale = 0.42f;
	int radius = 1;

	Rect rect = cv::boundingRect(points);
//	Region::inset(rect, 1);
	Subdiv2D subdiv(rect);

	for(size_t i = 0; i < points.size(); ++i)
	{
		const Point2f& point = points[i];
		Point2i pt(cvRound(point.x), cvRound(point.y));
		cv::circle(image, pt, radius, CV_RGB(0, 255, 0), 1, LINE_AA);

		cv::putText(image, venus::to_string(i), Point2i(pt.x + radius, pt.y), font_face, font_scale, text_color, 1, LINE_AA);
//		image.at<Vec3b>(cvRound(point.y), cvRound(point.x)) = Vec3b(0, 255, 0);

		subdiv.insert(point);
	}

	std::vector<cv::Vec6f> triangles;
	subdiv.getTriangleList(triangles);

	filter(triangles, rect);
	drawDelaunay(image, triangles, CV_RGB(0, 255, 0));

#if PRINT_TRIANGLE_INDEX  // used to cache the data in our program.
	std::vector<Vec3i> index = getTriangleIndex(points, triangles);

	printf("const std::vector<Vec3i> indices\n{\n");
	for(const Vec3i& triple: index)
		printf("\tVec3i(%2d, %2d, %2d),\n", triple[0], triple[1], triple[2]);
	printf("};\n");
#endif
}

void Feature::markWithIndices(cv::Mat& image, const std::vector<cv::Point2f>& points)
{
	const float font_scale = 0.42f;
	int offset = 2;

	const Scalar COLOR0 = CV_RGB(0, 255, 0);  // green
	const size_t point_count = points.size();
	for(size_t i = 0; i < point_count; ++i)
	{
		const Point2f& point = points[i];
		Point2i pt(cvRound(point.x), cvRound(point.y));
		cv::putText(image, to_string(i), Point2i(pt.x + offset, pt.y), font_face, font_scale, COLOR0, 1, LINE_AA);
		cv::circle(image, pt, 1, COLOR0, 1, LINE_AA);
	}

#if 1  // enable it to draw triangles
	const Scalar COLOR1 = CV_RGB(0, 0, 255);  // red
	for(const Vec3i& tri: triangle_indices)
	{
		for(int k = 0; k < 3; ++k)
			assert(0 <= tri[k] && tri[k] < static_cast<int>(point_count));

		// non-realm triangles' edge will draw twice, but it doesn't matter.
		cv::line(image, points[tri[0]], points[tri[1]], COLOR1, 1, LINE_AA);
		cv::line(image, points[tri[1]], points[tri[2]], COLOR1, 1, LINE_AA);
		cv::line(image, points[tri[2]], points[tri[0]], COLOR1, 1, LINE_AA);
	}
#endif

	Vec4f line = Feature::getSymmetryAxis(points);
	Point2f center(line[2], line[3]), down(line[0], line[1]);
	float k = down.y / down.x;  // dy/dx
	k = -1 / k;  // two mutually perpendicular line has the relationship k1*k2 = -1

#if 1  // enable it to draw horizontal/vertical lines
	const Scalar COLOR2 = CV_RGB(255, 255, 255);  // white
	Point2f pm1(points[22]), p0(points[21]), p1(points[20]), p2(points[25]);
	Point2f eye_brow_top_r = catmullRomSpline(0.50f, pm1, p0, p1, p2);
//	cv::circle(image, eye_brow_top_r, 1, COLOR2, 1, LINE_AA);

	pm1 = points[29]; p0 = points[28]; p1 = points[27]; p2 = points[26];
	Point2f eye_brow_top_l = catmullRomSpline(0.50f, pm1, p0, p1, p2);
//	cv::circle(image, eye_brow_top_l, 1, COLOR2, 1, LINE_AA);

	auto drawHorizontalLine = [](cv::Mat& image, const Point2f& point, float k, const Scalar& color)
	{
		Point2f right(0, 0), left(static_cast<float>(image.cols - 1), static_cast<float>(image.rows - 1));
		right.y = k * (right.x - point.x) + point.y;
		left.y = k * (left.x - point.x) + point.y;
		venus::drawLine(image, right, left, color, 1, LINE_AA);
	};

	// “三庭五眼”中的“三庭”，四条水平线。
	Point2f eye_brow_top_m = (eye_brow_top_r + eye_brow_top_l)/2;
//	venus::line(image, eye_brow_top_r, eye_brow_top_l, COLOR2, 1, LINE_AA);
	drawHorizontalLine(image, eye_brow_top_m, k, COLOR2);

	// top forehead index 16
	const Point2f& top_forehead = points[16];  // not to accurate
	drawHorizontalLine(image, top_forehead, k, COLOR2);

	// wing of the nose
	const Point2f& nose_wing_r = points[55];
	const Point2f& nose_wing_l = points[57];
	Point2f nose_wing_m = (nose_wing_r + nose_wing_l)/2;
	drawHorizontalLine(image, nose_wing_m, k, COLOR2);

	// chin
	const Point2f& chin = points[6];
	drawHorizontalLine(image, chin, k, COLOR2);

#if 0
	// “三庭五眼”中的“五眼”，六条竖直线。
	const Point2f& eye_right_l = points[34];
	const Point2f& eye_left_r = points[44];
//	k = (eye_left_r.y - eye_right_l.y)/(eye_left_r.x - eye_right_l.x);
	Point2f  top(0, 0), bottom(image.cols, image.rows);
	top.x = k * (top.y - eye_right_l.y) + eye_right_l.x;
	bottom.x = k * (bottom.y - eye_right_l.y) + eye_right_l.x;
	venus::line(image, top, bottom, CV_RGB(0, 255, 0), 1, LINE_AA);

	top.x = k * (top.y - eye_left_r.y) + eye_left_r.x;
	bottom.x = k * (bottom.y - eye_left_r.y) + eye_left_r.x;
	venus::line(image, top, bottom, CV_RGB(0, 255, 0), 1, LINE_AA);
#endif
	
	const Scalar COLOR3 = CV_RGB(255, 165, 0);  // orange
	for(int i = 0; i < 2; ++i)
	{
		bool is_right = (i == 0);
		std::pair<cv::Point2f, float> iris_info = Feature::calculateIrisInfo(points, is_right);
		cv::Point2f& center = iris_info.first;
		float& radius = iris_info.second;
		cv::circle(image, center, static_cast<int>(radius), COLOR3, 1, LINE_AA);

		std::vector<cv::Point2f> polygon = Feature::calculateEyePolygon(points, is_right);
		venus::drawPolygon(image, polygon, COLOR3, 1, LINE_AA);
	}

#endif

	const Scalar COLOR4 = CV_RGB(255, 0, 255);  // purple
	venus::drawLine(image, center, center + 10 * down, COLOR4);
	
	// from inner eye corner to lip point where it is close to center top point
	cv::line(image, points[34], points[65], COLOR4);
	cv::line(image, points[44], points[67], COLOR4);

	// from outer eye corner to lip corner
	cv::line(image, points[38], points[63], COLOR4);
	cv::line(image, points[48], points[69], COLOR4);
		
	// from wing of nose to lip corner
	Point2f end = points[63] * 2.0F - points[62];
	cv::line(image, points[62], end, COLOR4);
	end = points[69] * 2.0F - points[58];
	cv::line(image, points[58], end, COLOR4);
}

cv::Vec4f Feature::getSymmetryAxis(const std::vector<cv::Point2f>& points)
{
	assert(points.size() == COUNT);

#if 0  // An alternative, but not so accurate branch.
	// right eye index 42, left eye index 48.
	// Make sure that right eye is one the right side. (no pun intended)
	assert(points[48].x - points[42].x > 1/* pixel */);

	cv::Point2f middle = (points[48] + points[42].x)/2;
	cv::Vec4f line;  // (vx, vy, x0, y0)  where (vx, vy) is a normalized vector collinear to the line and (x0, y0) is a point on the line.
	line[2] = middle.x;
	line[3] = middle.y;

	float k = (points[48].y - points[42].y) / (points[48].x - points[42].x);

	// atan(x) return angle in [-pi/2, pi/2], while atan2(y, x) return angle in [-pi, pi].
	// But we know that our x > 0, we can use atan directly.
	float theta = std::atan(k);
	line[0] = std::cos(theta);
	line[1] = std::sin(theta);
#else
	
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

	// eye, currently STASM doesn't detect left eyes precisely.
	// besides, iris is not fixed in center, not suitable for computing.
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

	cv::Vec4f line;  // use Least mean squares to fit a line
	cv::fitLine(points_on_line, line, CV_DIST_L1, 0, 10.0, 0.01);
#endif

/*
	x = x0 + dx * t;
	y = y0 + dy * t;

	https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
	Point Q project on line AB, projection point is P.
	here A(x0, y0), B(x0 + dx, y0 + dy),
	vAB * vPQ = (dx, dy) * (x0 + dx * t - xq, y0 + dy * t - yq) = 0

	dx*(x0 - xq) + dx^2 + dy * (y0 - yq) + dy^2 = 0
	(dx^2 + dy^2)*t = dx*(xq - x0) + dy*(yq - y0)
	t = (dx*(xq - x0) + dy*(yq - y0)) / (dx^2 + dy^2);
*/	
	Point2f center = (points[0] + points[12]) / 2;
	float t = (line[0] * (center.x - line[2]) + line[1] * (center.y - line[3])) / (line[0] * line[0] + line[1] * line[1]);
	line[2] += line[0] * t;  // center.x = line[2] + line[0] * t;
	line[3] += line[1] * t;  // center.y = line[3] + line[1] * t;

	if(line[1] < 0)  // this makes Vec2f(line[0], line[1]) a downward vector, align to Y axis.
	{
		line[0] = -line[0];
		line[1] = -line[1];
	}
	// dx > 0, /o/o/; dx ==0, |o|o|; dx < 0 \o\o\;
	// dy is always negative in Y top down axis, need to flip for Y up axis.
	// y = k * x; y = -1/k * x;
	// atan2(y, x); atan2(x, -y);
	// atan2(x, -y) for image, inverse for makeup, namely atan2(x, y)
//	float theta = std::atan2(x, y);

	return line;
}

cv::Mat Feature::createMask(const std::vector<cv::Point2f>& points, float blur_radius/* = 0.0F */, cv::Point2i* position/* = nullptr */)
{
	assert(points.size() >= 3 && blur_radius >= 0.0F);
//	Rect rect = cv::boundingRect(points);
	Vec4f box = venus::boundingBox(points);
	Rect2f rect = box2Rect(box);
	const bool enable_blur = blur_radius > 0.0F;
	if(enable_blur)
		Region::inset(rect, blur_radius);

	Rect2i _rect = rect;
	cv::Mat mask(_rect.size(), CV_8UC1, Scalar(0));

	cv::Point2i _position = _rect.tl();
	if(position)
		*position = _position;

	const size_t length = points.size();
	std::vector<cv::Point2i> points_(length);
	for(size_t i = 0; i < length; ++i)
		points_[i] = static_cast<cv::Point2i>(points[i]) - _position;
	
	const Point2i* polygon_data[1] = { points_.data() };
	const int       point_count[1] = { static_cast<int>(points_.size()) };
	cv::fillPoly(mask, polygon_data, point_count, 1, Scalar(255));

	if(enable_blur)
		venus::gaussianBlur(mask, mask, blur_radius);
	
	return mask;
}

cv::Mat Feature::maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, int start, int length)
{
	assert(0 <= start && start < static_cast<int>(points.size()));
	assert(3 <= length&& start + length <= static_cast<int>(points.size()));

	std::vector<Point> contour(length);

	const Point2i origin = rect.tl();
	for(int i = 0; i < length; ++i)
		contour[i] = static_cast<Point2i>(points[start + i]) - origin;

	const Point* polygons[] = { contour.data() };
	const int  num_points[] = { length };
	Mat mask(rect.size(), CV_8UC1, Scalar(0));
	cv::fillPoly(mask, polygons, num_points, 1, Scalar(255));
	return mask;
}

cv::Mat Feature::maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points)
{
	return maskPolygon(rect, points, 0, static_cast<int>(points.size()));
}

cv::Mat Feature::maskPolygonSmooth(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, const int level/* = 8 */)
{
	assert(level > 0);

	Point2f sum(0.0f, 0.0f);
	for(const Point2f& point : points)
		sum += point;
	Point2f center = sum / static_cast<int>(points.size());

//	Rect rect = cv::boundingRect(points);  // expose rect as parameter
	const Point2f origin = rect.tl();
	std::vector<Point2i> tmp_points = cast(points);
	Mat mask(rect.size(), CV_8UC1, Scalar(0));

	const Point* polygons[] = { tmp_points.data() };
	const int  num_points[] = { static_cast<int>(tmp_points.size()) };
	for(int j = 1; j <= level; ++j)
	{
		float FACTOR = std::sqrt(1.0F - static_cast<float>(j-1)/level);  // every step shrinks a little bit.

		for(size_t i = 0; i < points.size(); ++i)
			tmp_points[i] = (points[i] - center) * FACTOR + center - origin;

		cv::fillPoly(mask, polygons, num_points, 1, Scalar(j*255.0/level));
	}

	// find max distance from center
	float measure = 0;
	for(const Point2f& point : points)
	{
		Point2f d = point - center;
		measure += d.x*d.x + d.y*d.y;
	}
	measure /= points.size();
	measure = std::sqrt(measure);

	venus::gaussianBlur(mask, mask, measure/level);
	return mask;
}

std::vector<cv::Point2f> Feature::calculateBrowPolygon(const std::vector<cv::Point2f>& points, bool right)
{
	assert(points.size() == COUNT);
/*
	Below are eye brow feature point indices:

			  .--21--20\        /27--28--.
		  _.-`          |      |          `-._
	right 22--23--24--25`      `26--31--30--29  left
*/
#if 0  // not precise branch
	const int start = right ? 20:26, length = 6;
	std::vector<Point2f> polygon(length);

	for(int i = 0; i < length; ++i)
		polygon[i] = points[start + i];
#else
	const Point2f& _20 = points[right ? 20:27];
	const Point2f& _21 = points[right ? 21:28];
	const Point2f& _22 = points[right ? 22:29];
	const Point2f& _23 = points[right ? 23:30];
	const Point2f& _24 = points[right ? 24:31];
	const Point2f& _25 = points[right ? 25:26];
	const Point2f& _00 = points[right ?  0:12];

	std::vector<Point2f> polygon;
	polygon.push_back(_20);
	polygon.push_back(catmullRomSpline(0.25F, _25, _20, _21, _22));
	polygon.push_back(catmullRomSpline(0.50F, _25, _20, _21, _22));
	polygon.push_back(catmullRomSpline(0.75F, _25, _20, _21, _22));
	polygon.push_back(_21);

	polygon.push_back(catmullRomSpline(0.25F, _20, _21, _22, _00));
	polygon.push_back(catmullRomSpline(0.50F, _20, _21, _22, _00));
	polygon.push_back(catmullRomSpline(0.75F, _20, _21, _22, _00));
	polygon.push_back(_22);
	polygon.push_back(_23);

//	polygon.push_back(catmullRomSpline(0.50F, _22, _23, _24, _25));
	polygon.push_back(_24);
	polygon.push_back(_25);

	Point2f diff = _25 - _21;
	float r = std::sqrt(diff.x * diff.x + diff.y * diff.y);
	diff /= r;
	r = distance(_24, _25) * 0.81F;  // 0.81 seemes fine.
	polygon.push_back(_20 + r * diff);

#endif

	return polygon;
}

Region Feature::calculateBrowRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right)
{
	const std::vector<Point2f> polygon = calculateBrowPolygon(points, right);
	
//	Rect2i rect = cv::boundingRect(polygon);
	Vec4f box = venus::boundingBox(polygon);
	Rect2i rect = box2Rect(box);

	Mat mask(rect.size(), CV_8UC1, Scalar(0));

	Point2i origin = rect.tl();
	std::vector<Point2i> polygon2 = venus::cast(polygon);
	for(size_t i = 0, length = polygon.size(); i < length; ++i)
		polygon2[i] -= origin;

	cv::fillConvexPoly(mask, polygon2, Scalar(255));

#if 0
	Point2f pivot = right?
		(points[20] + points[21] + points[23] + points[24]) / 4:
		(points[27] + points[28] + points[30] + points[31]) / 4;
#else
	Point2f pivot((box[0] + box[2])/2, (box[1] + box[3])/2);
#endif

	Size2f size = calculateSize(box, line);
	return Region(pivot, size, mask);
}

std::vector<cv::Point2f> Feature::calculateEyePolygon(const std::vector<cv::Point2f>& points, bool right)
{
/*
	Below are eye feature point indices:


				36                    46
			 37    35              45    47
	right  38   42   34 -------- 44   43   48   left
			 39    41              51    49
				40                    50

*/
	// right sequential index: { 34, 35, 36, 37, 38, 39, 40, 41 };
	//  left sequential index: { 44, 45, 46, 47, 48, 49, 50, 51 };
	constexpr int length = 42 - 34;  // or 52 - 44
	const int start = right?34:44;

	// Point2f implicitly cast to Point2i, which calls saturate_cast(), which calls cvRound()
#if 0
	std::vector<Point2f> polygon(length);
	for(int i = 0; i < length; ++i)
		polygon[i] = points[start + i];
#else
	std::vector<Point2f> polygon;
	for(int i = 0; i < length; ++i)
	{
		const Point2f& _0 = points[start + (i+length-1)%length];
		const Point2f& _1 = points[start + i];
		const Point2f& _2 = points[start + (i+length+1)%length];
		const Point2f& _3 = points[start + (i+length+2)%length];
		polygon.push_back(_1);
		polygon.push_back(catmullRomSpline(1.0F/3, _0, _1, _2, _3));
		polygon.push_back(catmullRomSpline(2.0F/3, _0, _1, _2, _3));
	}
#endif
	return polygon;
}

std::pair<cv::Point2f, float> Feature::calculateIrisInfo(const std::vector<cv::Point2f>& points, bool right)
{
	assert(points.size() == COUNT);

	// TODO tweaking needed
	const Point2f center = points[right?42:43];
	float d[4];
	d[0] = distance(center, points[right?35:45]);
	d[1] = distance(center, points[right?37:47]);
	d[2] = distance(center, points[right?39:49]);
	d[3] = distance(center, points[right?41:51]);

	float radius = (d[0] + d[1] + d[2] + d[3])/4;  // std::accumulate(d, d + 4, 0)/4;
//	float radius = *std::min_element(d, d + 4);

	return std::make_pair(center, radius);
}

cv::RotatedRect Feature::calculateBlushRectangle(const std::vector<cv::Point2f>& points, bool right)
{
	Vec4f line = Feature::getSymmetryAxis(points);
	float angle = std::atan2(line[1], line[0]) - static_cast<float>(M_PI/2);

	return calculateBlushRectangle(points, angle, right);
}

cv::RotatedRect Feature::calculateBlushRectangle(const std::vector<cv::Point2f>& points, float angle, bool right)
{
	// symmetric point
	const Point2f& _54 = points[right? 54:52];
	const Point2f& _00 = points[right?  0:12];
	const Point2f& _01 = points[right?  1:11];
	const Point2f& _02 = points[right?  2:10];
	const Point2f& _03 = points[right?  3: 9];
	const Point2f& _63 = points[right? 63:69];
	const Point2f& _62 = points[right? 62:58];
	const Point2f& _34 = points[right? 34:44];

	// keep those formula synchronized with member function below
	Point2f t0 = (_54 + _34 + _62)/3;
	Point2f t1 = (_00 + _01 + _54*2)/4;
	Point2f& t = (t0.y < t1.y) ? t0 : t1;

	Point2f b = (_03 + _63 + _62)/3;
	Point2f l = _02;
	Point2f r = _62;
	
	return calculateRectangle(angle, l, t, r, b);
}

std::vector<cv::Point2f> Feature::calculateBlushPolygon(const std::vector<cv::Point2f>& points, bool right)
{
	assert(points.size() == COUNT);

	// symmetric point
	const Point2f& _54 = points[right? 54:52];
	const Point2f& _00 = points[right?  0:12];
	const Point2f& _01 = points[right?  1:11];
	const Point2f& _02 = points[right?  2:10];
	const Point2f& _03 = points[right?  3: 9];
	const Point2f& _04 = points[right?  4: 8];
	const Point2f& _63 = points[right? 63:69];
	const Point2f& _62 = points[right? 62:58];
	const Point2f& _34 = points[right? 34:44];

	// keep consistent with member function above
	std::vector<Point2f> polygon
	{
		(_54 + _34 + _62)/3,
		(_00 + _01 + _54*2)/4,
		(_00 + _01*2)/3,
		_01,
		catmullRomSpline(1.0F/3, _00, _01, _02, _03),
		catmullRomSpline(2.0F/3, _00, _01, _02, _03),
		_02,
		catmullRomSpline(1.0F/3, _01, _02, _03, _04),
		catmullRomSpline(2.0F/3, _01, _02, _03, _04),
		_03,
		(_03 + _63 + _62)/3,
		_62,
	};

	return polygon;
}

std::vector<cv::Point2f> Feature::calculateNosePolygon(const std::vector<cv::Point2f>& points)
{
	assert(points.size() == COUNT);
	return std::vector<Point2f>
	{
		catmullRomSpline(1.0F/3, points[18], points[25], points[54], points[62]),
		catmullRomSpline(2.0F/3, points[18], points[25], points[54], points[62]),
		                                                 points[54], points[62],
		catmullRomSpline(1.0F/3,                         points[54], points[62], points[61], points[55]),
		catmullRomSpline(2.0F/3,                         points[54], points[62], points[61], points[55]),
		                                                                         points[61], points[55],
		
		points[60],  // middle point
		
		                         points[57], points[59],
		catmullRomSpline(1.0F/3, points[57], points[59], points[58], points[52]),
		catmullRomSpline(2.0F/3, points[57], points[59], points[58], points[52]),
		                                                 points[58], points[52],
		catmullRomSpline(1.0F/3,                         points[58], points[52], points[26], points[14]),
		catmullRomSpline(2.0F/3,                         points[58], points[52], points[26], points[14]),
	};
}

std::vector<cv::Point2f> Feature::calculateLipPolygon(const std::vector<cv::Point2f>& points, bool upper)
{
	assert(points.size() == COUNT);
/*
	Below are lips feature point indices:
	
                  /64\    /66\    /68\
                 /    \65/    \67/    \   
                /                      \
              63-----72----71----70-----69
                \-----73---74----75-----/
                 \80\              /76/
                     \79---78---77/

*/
/*
	const int lip_indices[] =
	{
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72,  // upper lip
		63, 73, 74, 75, 69, 76, 77, 78, 79, 80,  // lower lip
	};
*/
	if(upper)  // for upper lip
	{
#if 0
		constexpr int BEGIN = 63, END = 73;
		std::vector<Point2f> polygon(END - BEGIN);
		for(int i = BEGIN; i < END; ++i)
			polygon[i - BEGIN] = points[i];

		return polygon;
#else
		// I almost forgot that std::vector has range [first, last) constructor.
		return std::vector<Point2f>(points.begin() + 63, points.begin() + 73);
#endif
	}
	else  // for lower lip
	{
		constexpr int N = 10 + (79 - 76 + 1)*3;
		std::vector<Point2f> polygon;
		polygon.reserve(N);

		polygon.push_back(points[63]);

		polygon.push_back(points[73]);
		polygon.push_back(catmullRomSpline(1.0F/3, points[63], points[73], points[74], points[75]));
		polygon.push_back(catmullRomSpline(2.0F/3, points[63], points[73], points[74], points[75]));
		polygon.push_back(points[74]);
		polygon.push_back(catmullRomSpline(1.0F/3, points[73], points[74], points[75], points[69]));
		polygon.push_back(catmullRomSpline(2.0F/3, points[73], points[74], points[75], points[69]));
		polygon.push_back(points[75]);

		polygon.push_back(points[69]);
		polygon.push_back(points[76]);

		for(int i = 76; i <= 79; ++i)
		{
			int i0 = i == 76 ? 69 : i-1;
			int i1 = i;
			int i2 = i + 1;
			int i3 = i == 79 ? 63 : i+2;
			polygon.push_back(catmullRomSpline(1.0F/3, points[i0], points[i1], points[i2], points[i3]));
			polygon.push_back(catmullRomSpline(2.0F/3, points[i0], points[i1], points[i2], points[i3]));
			polygon.push_back(points[i2]);
		}

		// if assertion failed, change value N according to contour.size();
		assert(polygon.size() == N);
		return polygon;
	}
}

std::vector<cv::Point2f> Feature::calculateTeethPolygon(const std::vector<cv::Point2f>& points)
{
	assert(points.size() == COUNT);
	return std::vector<Point2f>
	{
		points[63], points[72], points[71], points[70], points[69],  // upper lip

		points[75],
		catmullRomSpline(2.0F/3, points[73], points[74], points[75], points[69]),
		catmullRomSpline(1.0F/3, points[73], points[74], points[75], points[69]),
		points[74],
		catmullRomSpline(2.0F/3, points[63], points[73], points[74], points[75]),
		catmullRomSpline(1.0F/3, points[63], points[73], points[74], points[75]),
		points[73], 
	};
}

Region Feature::calculateEyeRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right)
{
	std::vector<Point2f> polygon = calculateEyePolygon(points, right);
	std::vector<Point2i> polygon2 = venus::cast(polygon);
	Rect rect = cv::boundingRect(polygon);
	Mat mask = maskPolygon(rect, polygon);

	Point2f pivot = right?
		(points[36] + points[40])/2:
		(points[46] + points[50])/2;

	Vec4f box = right?
		Vec4f(points[38].x, points[36].y, points[34].x, points[40].y):
	    Vec4f(points[44].x, points[46].y, points[48].x, points[50].y);

	Size2f size = calculateSize(box, line);

	return Region(pivot, size, mask);
}

Region Feature::calculateLipsRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line)
{
	std::vector<cv::Point2f> polygon_t = calculateLipPolygon(points, true);
	std::vector<cv::Point2f> polygon_b = calculateLipPolygon(points, false);

	const int length_t = static_cast<int>(polygon_t.size());
	const int length_b = static_cast<int>(polygon_b.size());
	std::vector<cv::Point2f> polygon(length_t + length_b);
	std::copy(polygon_t.begin(), polygon_t.end(), polygon.begin());
	std::copy(polygon_b.begin(), polygon_b.end(), polygon.begin() + length_t);

	Vec4f box = venus::boundingBox(polygon);
	Rect2i rect = box2Rect(box);
//	Rect2i rect_t = cv::boundingRect(polygon_t);
//	Rect2i rect_b = cv::boundingRect(polygon_b);
//	Rect2i rect = rect_t & rect_b;

	Point2f pivot = (rect.tl() + rect.br())/2.0f;
	Size2f size = calculateSize(box, line);

	std::vector<cv::Point2i> _polygon = cast(polygon);
	const Point2i position = rect.tl();
	for(Point2i& point : _polygon)
		point -= position;

	const Point2i* polygons[] = { _polygon.data(), _polygon.data() + length_t };
	const int    num_points[] = { length_t, length_b };
	Mat mask(rect.size(), CV_8UC1, Scalar(0));
	cv::fillPoly(mask, polygons, num_points, 2, Scalar(255));

	return Region(pivot, size, mask);
}

Region Feature::calculateTeethRegion() const
{
	const std::vector<Point2f> polygon = calculateTeethPolygon(points);
	Rect rect = cv::boundingRect(polygon);
	Mat mask = maskPolygon(rect, polygon);

	Mat gray = Effect::grayscale(image(rect));
	
	Scalar sum = cv::sum(gray);
	double threshold = sum[0] / (image.rows * image.cols);
/*
	Mat roi = image(rect).clone();
	Scalar sum = cv::sum(roi);
	Mat rgba[4];
	assert(image.channels() <= 4);
	cv::split(image, rgba);
*/
	// TODO come up with a robust method
//	Mat mask;
	assert(image.depth() == CV_8U);
	cv::threshold(image, mask, threshold, 255, THRESH_BINARY);

	Vec4f box = venus::boundingBox(polygon);
	Size2f size = calculateSize(box, line);
	Point2f pivot((box[0] + box[2])/2, (box[1] + box[3])/2);

	return Region(pivot, size, mask);
}

cv::RotatedRect Feature::calculateRectangle(float angle, const cv::Point2f& left, const cv::Point2f& top, const cv::Point2f& right, const cv::Point2f& bottom)
{
	const float cosa = std::cos(angle);
	const float sina = std::sin(angle);
	const Point2f horizontal(cosa, sina), vertical(-sina, cosa);

	Point2f tl, br;
	lineIntersection(top, top + horizontal, left, left + vertical, &tl);
	lineIntersection(bottom, bottom + horizontal, right, right + vertical, &br);
	
	Point2f center = (tl + br) / 2;
	Vec2f direction = br - tl;
	if(direction.dot(vertical) < 0)
		direction = -direction;

	Vec2f along = vertical.dot(direction) * vertical;
	Vec2f normal = direction - along;
	Size2f size(std::sqrt(normal.dot(normal)), std::sqrt(along.dot(along)));

	return RotatedRect(center, size, rad2deg(angle));
}

cv::Vec4f Feature::calcuateDistance(cv::Point2f& pivot, const cv::Point2f& left, const cv::Point2f& top, const cv::Point2f& right, const cv::Point2f& bottom)
{
	assert(left.x < right.x && top.y < bottom.y);
	assert(left.x <= top.x && top.x <= right.x && left.x <= bottom.x && bottom.x <= right.x);
	assert(top.y <= left.y && left.y <= bottom.y && top.y <= right.y && right.y <= bottom.y);
	
#if 0
	// non-skew version
	pivot = Point2f((top.x + bottom.x)/2, (left.y + right.y)/2);
	return Vec4f(pivot.x - left.x, right.x - pivot.x, pivot.y - top.y, bottom.y - pivot.y);
#else
	float denorm = (left.x - right.x)*(top.y - bottom.y) - (left.y - right.y)*(top.x - bottom.x);
	assert(std::abs(denorm) > std::numeric_limits<float>::epsilon());

	float z = left.x*right.y - left.y*right.x;
	float w = top.x*bottom.y - top.y*bottom.x;
	float x = z*(top.x - bottom.x) - w*(left.x - right.x);
	float y = z*(top.y - bottom.y) - w*(left.y - right.y);
	pivot.x = x / denorm;
	pivot.y = y / denorm;

	x = venus::distance(pivot, left);
	y = venus::distance(pivot, top);
	z = venus::distance(pivot, right);
	w = venus::distance(pivot, bottom);
	return Vec4f(x, y, z, w);
#endif
}

cv::Vec4f Feature::calcuateEyeRadius(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right)
{
	const Point2f& eye_left   = points[right ? 38:44];
	const Point2f& eye_top    = points[right ? 36:46];
	const Point2f& eye_right  = points[right ? 34:48];
	const Point2f& eye_bottom = points[right ? 40:50];

	(void)line;  // TODO need line for tune top and bottom point
	Point2f pivot;
	Vec4f distance = calcuateDistance(pivot, eye_left, eye_top, eye_right, eye_bottom);
	return distance;
}

cv::Mat Feature::maskSkinRegion(int width, int height, const std::vector<cv::Point2f>& points)
{
	assert(width > 0 && height > 0);

	const Scalar TRANSPARENT(0), OPAQUE(255);
	Mat mask(height, width, CV_8UC1, TRANSPARENT);

	const int N = 20;
	Point polygon[3*N];
	for(int i = 0; i < N; ++i)
	{
		int _0 = (i + (N-1))%N, _1 = i, _2 = (i + 1)%N, _3 = (i + 2)%N;
		polygon[3*i + 0] = catmullRomSpline(0.25F, points[_0], points[_1], points[_2], points[_3]);
		polygon[3*i + 1] = catmullRomSpline(0.50F, points[_0], points[_1], points[_2], points[_3]);
		polygon[3*i + 2] = catmullRomSpline(0.75F, points[_0], points[_1], points[_2], points[_3]);
	}
	cv::fillConvexPoly(mask, polygon, 3*N, OPAQUE);

#if 0 // less accurate branch
	auto fillConvex = [&mask, &points, &polygon, &TRANSPARENT](int start, int stop/* exclude */)
	{
		for(int i = start; i < stop; ++i)
			polygon[i - start] = Point(cvRound(points[i].x), cvRound(points[i].y));
		cv::fillConvexPoly(mask, polygon, stop - start, TRANSPARENT);
	};

	fillConvex(20, 26);  // right eye brow
	fillConvex(26, 32);  // left eye brow
	fillConvex(34, 42);  // right eye region
	fillConvex(44, 52);  // left eye region
#else
	for(int i = 0; i <= 1; ++i)
	{
		const bool is_right = (i == 0);
		std::vector<Point2f> poly = Feature::calculateBrowPolygon(points, is_right);
		cv::fillConvexPoly(mask, cast(poly).data(), poly.size(), TRANSPARENT);

		poly = Feature::calculateEyePolygon(points, is_right);
		cv::fillConvexPoly(mask, cast(poly).data(), poly.size(), TRANSPARENT);
	}
#endif

	int radius = cvRound(venus::distance(points[57], points[59]));
	circle(mask, points[55], radius, Scalar(0), CV_FILLED, LINE_AA);
	circle(mask, points[57], radius, Scalar(0), CV_FILLED, LINE_AA);

	// mouth region
	int j = 0;
	for(int i = 63; i <= 69; ++i, ++j)
		polygon[j] = Point(cvRound(points[i].x), cvRound(points[i].y));
	for(int i = 76; i <= 80; ++i, ++j)
		polygon[j] = Point(cvRound(points[i].x), cvRound(points[i].y));
	cv::fillConvexPoly(mask, polygon, j, TRANSPARENT);

//	Rect rect = cv::boundingRect(points);
#if 0
	float radius = std::max(width, height) * 0.01F;
	Effect::gaussianBlur(mask, mask, radius);
#endif
	// TODO kick out hair region

	// TODO kick out glasses if found.

	return mask;
}

} /* namespace venus */