#include "venus/region_operation.h"
#include "venus/colorspace.h"
#include "venus/Makeup.h"
#include "venus/Feature.h"
#include "venus/opencv_utility.h"

#include "platform/jni_bridge.h"

#include "stasm/stasm_lib.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>

#include <assert.h>
#include <unordered_map>

using namespace cv;

static const Scalar BACKGROUD_COLOR = CV_RGB(255, 255, 255);
static const Scalar FOREGROUD_COLOR = CV_RGB(0, 0, 0);

namespace venus {


/*
	Let (h,k) be the coordinates of the center of the circle, and r its
	radius. Then the equation of the circle is:

		 (x-h)^2 + (y-k)^2 = r^2

	Since the three points all lie on the circle, their coordinates will
	satisfy this equation. That gives you three equations:

		 (x1-h)^2 + (y1-k)^2 = r^2
		 (x2-h)^2 + (y2-k)^2 = r^2
		 (x3-h)^2 + (y3-k)^2 = r^2

	in the three unknowns h, k, and r. To solve these, subtract the first
	from the other two. That will eliminate r, h^2, and k^2 from the last
	two equations, leaving you with two simultaneous linear equations in
	the two unknowns h and k. Solve these, and you'll have the coordinates
	(h,k) of the center of the circle. Finally, set:

		 r = sqrt[(x1-h)^2+(y1-k)^2]

	and you'll have everything you need to know about the circle.

	This can all be done symbolically, of course, but you'll get some
	pretty complicated expressions for h and k. The simplest forms of
	these involve determinants, if you know what they are:

			 |x1^2+y1^2  y1  1|        |x1  x1^2+y1^2  1|
			 |x2^2+y2^2  y2  1|        |x2  x2^2+y2^2  1|
			 |x3^2+y3^2  y3  1|        |x3  x3^2+y3^2  1|
		 h = ------------------,   k = ------------------
				 |x1  y1  1|               |x1  y1  1|
			   2*|x2  y2  1|             2*|x2  y2  1|
				 |x3  y3  1|               |x3  y3  1|


	@see http://paulbourke.net/geometry/circlesphere/
	Spheres, equations and terminology written by Paul Bourke
	OpenGL/GLUT source code demonstrating the Great Circle
*/
/*
	Point A and B construct a line, the midnormal equation is 
	(B.x - A.x)*x + (B.y - A.y)*y = (B.x - A.x)*(B.x + A.x)/2 + (B.y - A.y)*(B.y + A.y)/2

	and ditto for point A and C, you can choose B and C as well.
	(C.x - A.x)*x + (C.y - A.y)*y = (C.x - A.x)*(C.x + A.x)/2 + (C.y - A.y)*(C.y + A.y)/2
	
	Two midnormal line intersect at some point O (the circumcircle) if they aren't parallel.
	[ (B.x - A.x)  (B.y - A.y) ] [ x ] = [ (B.x - A.x)*(B.x + A.x)/2 + (B.y - A.y)*(B.y + A.y)/2 ]
	[ (C.x - A.x)  (C.y - A.y) ] [ y ] = [ (C.x - A.x)*(C.x + A.x)/2 + (C.y - A.y)*(C.y + A.y)/2 ]

	By means of solving the matrix equation above, we get the circumcircle center point O.
*/
static Point2f circumcircle(const Point2f& A, const Point2f& B, const Point2f& C)
{
	float AB_x = B.x - A.x, AC_x = C.x - A.x;
	float AB_y = B.y - A.y, AC_y = C.y - A.y;

//	[ AB_x  AB_y ] [ x ] = [ AB_x*AB_mx + AB_y*AB_my ]
//	[ AC_x  AC_y ] [ y ] = [ AC_x*AC_mx + AC_y*AC_my ]

	float denorm = AB_x * AC_y - AC_x * AB_y;
	assert(std::abs(denorm) > std::numeric_limits<float>::epsilon());

	float M_x = AB_x*(B.x + A.x)/2 + AB_y*(B.y + A.y)/2;
	float M_y = AC_x*(C.x + A.x)/2 + AC_y*(C.y + A.y)/2;

	float x = (M_x*AC_y - M_y*AB_y)/denorm;
	float y = (AB_x*M_y - AC_x*M_x)/denorm;
	return Point2f(x, y);
}

/*
	here are some useful links about hypot, I tried to use std::hypot(x, y), later changed to std::sqrt(a*a + b*b);
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
	// complex<T>(x,y) * complex<T>(cos(angle), sin(angle));
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	float _x = c * v[0] - s * v[1];
	float _y = s * v[0] + c * v[1];

	return cv::Vec2f(_x, _y);
}

static void correctIris(const Mat& image, std::vector<Point2f>& points)
{
	assert(image.channels() == 1);
	RoiInfo eye_info_r = calcuateEyeRegionInfo_r(points);
	RoiInfo eye_info_l = calcuateEyeRegionInfo_l(points);

	Mat eye_r = image(Rect(eye_info_r.origin, eye_info_r.mask.size()));
	Mat eye_l = image(Rect(eye_info_l.origin, eye_info_l.mask.size()));

	// roughly estimate iris radius
	const int iris_indices_r[] = {35, 37, 39, 41};
	const int iris_indices_l[] = {45, 47, 49, 51};
	float iris_r_radius = venus::distance(points[42], points[iris_indices_r[0]]);
	for(size_t i = 1; i < NELEM(iris_indices_r); ++i)
	{
		float dist = venus::distance(points[42], points[iris_indices_r[i]]);
		if(iris_r_radius > dist)
			iris_r_radius = dist;
	}
	
	float iris_l_radius = venus::distance(points[43], points[iris_indices_l[0]]);
	for(size_t i = 1; i < NELEM(iris_indices_l); ++i)
	{
		float dist = venus::distance(points[43], points[iris_indices_l[i]]);
		if(iris_l_radius > dist)
			iris_l_radius = dist;
	}

	// Morphological opening is performed to remove glint.
	int morph_radius = 1;
	int morph_size = morph_radius * 2 + 1;
	Mat element = getStructuringElement(cv::MORPH_RECT, Size(morph_size, morph_size), Point(morph_radius, morph_radius));
	Mat image_processed;
	morphologyEx(eye_r, eye_r, cv::MORPH_OPEN, element);
	morphologyEx(eye_l, eye_l, cv::MORPH_OPEN, element);

	// http://stackoverflow.com/questions/10716464/what-are-the-correct-usage-parameter-values-for-houghcircles-in-opencv-for-iris
	const float tolerance = 0.666f;
	std::vector<cv::Vec3f> circles_r, circles_l;
	for(int i = 8; i < 20; ++i)
	{
		cv::HoughCircles(eye_r, circles_r, cv::HOUGH_GRADIENT, 1, 30, 100, i, iris_r_radius*tolerance, iris_r_radius/tolerance);
		if(circles_r.size() <= 1)
			break;
	}
	for(int i = 8; i < 20; ++i)
	{
		cv::HoughCircles(eye_l, circles_l, cv::HOUGH_GRADIENT, 1, 30, 100, i, iris_l_radius*tolerance, iris_l_radius/tolerance);
		if(circles_l.size() <= 1)
			break;
	}

	if(circles_r.size() > 1 || circles_l.size() > 1)
		LOGW("multiple circles detected, need to be only one");

/*
				36                    46
			 37    35              45    47
	right  38   42   34 -------- 44   43   48   left
			 39    41              51    49
				40                    50
*/
	auto radical_scale = [](const Point2f& center, Point2f& point, float radius)
	{
		float d = venus::distance(center, point);
		float f = radius / d;
//		assert(0.5f < f && f < 2.0f);
		point.x = center.x + (point.x - center.x) * f;
		point.y = center.y + (point.y - center.y) * f;
	};

	if(circles_r.size() == 1)
	{
		cv::Vec3f circle = circles_r[0];
		points[42] = Point2f(circle[0] + eye_info_r.origin.x, circle[1] + eye_info_r.origin.y);
		const float r = circle[2];
		for(size_t i = 0; i < NELEM(iris_indices_r); ++i)
			radical_scale(points[42], points[iris_indices_r[i]], r);
	}

	if(circles_l.size() == 1)
	{
		cv::Vec3f circle = circles_l[0];
		points[43] = Point2f(circle[0] + eye_info_l.origin.x, circle[1] + eye_info_l.origin.y);
		const float r = circle[2];
		for(size_t i = 0; i < NELEM(iris_indices_l); ++i)
			radical_scale(points[43], points[iris_indices_l[i]], r);
	}
}

std::vector<Point2f> getFaceFeaturePoints(const std::string& face, const std::string& datadir, Point& size)
{
	cv::Mat image = cv::imread(face, CV_LOAD_IMAGE_GRAYSCALE);
	if(!image.data)
    {
        printf("Cannot load %s\n", face.c_str());
        return std::vector<Point2f>();
    }

	size = Point(image.cols, image.rows);
	cv::Mat _image = image;
	return getFaceFeaturePoints(_image, face, datadir);
}

std::vector<Point2f> getFaceFeaturePoints(const std::string& face, const std::string& datadir)
{
	cv::Mat image = cv::imread(face, CV_LOAD_IMAGE_GRAYSCALE);
	if(!image.data)
    {
        printf("Cannot load %s\n", face.c_str());
        return std::vector<Point2f>();
    }

	cv::Mat _image = image;
	return getFaceFeaturePoints(_image, face, datadir);
}

std::vector<Point2f> getFaceFeaturePoints(const Mat& image, const std::string& image_name, const std::string& datadir)
{
	assert(image.channels() == 1);  // single channel required, namely gray image.

	std::vector<Point2f> points;

    int foundface;
    float landmarks[stasm_NLANDMARKS * 2]; // x, y coords (note the 2)
	const char* imagepath = image_name.c_str();
    if(!stasm_search_single(&foundface, landmarks,
			reinterpret_cast<const char*>(image.data), image.cols, image.rows,
			imagepath, datadir.c_str()))
    {
        printf("Error in stasm_search_single: %s\n", stasm_lasterr());
        return points;
    }

    if(!foundface)
		printf("No face found in %s\n", imagepath);
    else
    {
        // draw the landmarks on the image as white dots (image is monochrome)
        stasm_force_points_into_image(landmarks, image.cols, image.rows);
#if 0
		points.reserve(stasm_NLANDMARKS);
        for(int i = 0; i < stasm_NLANDMARKS; i++)
		{
			printf("Point(%d, %d),\n", (int)landmarks[i*2], (int)landmarks[i*2+1]);
			points.push_back(Point2f(landmarks[i*2], landmarks[i*2+1]));
		}

#else  // add bonus feature points(use Bezier or spline curve) for better subdivision result.
		const int extra_count = 4;
		points.reserve(stasm_NLANDMARKS + extra_count);

		int i = 0;
		for(; i < 13; ++i)
			points.push_back(Point2f(landmarks[i*2], landmarks[i*2+1]));
		
		{
			int ia = 13, ib = 12, ic = 11;  // triangle 11/12/13
			const Point2f& A = *reinterpret_cast<const Point2f*>(landmarks + ia*2);
			const Point2f& B = *reinterpret_cast<const Point2f*>(landmarks + ib*2);
			const Point2f& C = *reinterpret_cast<const Point2f*>(landmarks + ic*2);

			Point2f O = circumcircle(A, B, C);
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

			Point2f O = circumcircle(A, B, C);
			Vec2f vOA(A.x - O.x, A.y - O.y);
			Vec2f vOB(B.x - O.x, B.y - O.y);
			float theta = angle(vOA, vOB);

			Vec2f vOA_r = rotate(vOA, -theta/3);
			Vec2f vOB_l = rotate(vOB, +theta/3);
			points.push_back(Point2f(O.x + vOA_r[0], O.y + vOA_r[1]));
			points.push_back(Point2f(O.x + vOB_l[0], O.y + vOB_l[1]));

//			points.push_back(O + vOA.rotate(-theta/3));
//			points.push_back(O + vOB.rotate(+theta/3));
		}

		for(; i < stasm_NLANDMARKS; ++i)
			points.push_back(Point2f(landmarks[i*2], landmarks[i*2+1]));
#if 0
		// add more points for better Delaunay triangulation result.
		Point2f pm1(points[22]), p0(points[21]), p1(points[20]), p2(points[25]);
		Point2f eye_brow_top_r = catmullRomSpline(0.50f, pm1, p0, p1, p2);
//		cv::circle(image, Point(cvRound(eye_brow_top_r.x), cvRound(eye_brow_top_r.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
		points.push_back(eye_brow_top_r);

		pm1 = points[29]; p0 = points[28]; p1 = points[27]; p2 = points[26];
		Point2f eye_brow_top_l = catmullRomSpline(0.50f, pm1, p0, p1, p2);
//		cv::circle(image, Point(cvRound(eye_brow_top_l.x), cvRound(eye_brow_top_l.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//		points.push_back(eye_brow_top_l);

		Point2f eye_brow_top_m = (eye_brow_top_r + eye_brow_top_l)/2;
//		cv::circle(image, Point(cvRound(eye_brow_top_m.x), cvRound(eye_brow_top_m.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//		points.push_back(eye_brow_top_m);

		Point2f eye_pupil_m = (points[42] + points[43])/2;
//		cv::circle(image, Point(cvRound(eye_pupil_m.x), cvRound(eye_pupil_m.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
		points.push_back(eye_pupil_m);

		// 3|9 62|58 63|69
		Point2f eye_cheek_1r = (points[3] + points[62] + 0.5 * points[63])/2.5f;
		Point2f eye_cheek_1l = (points[9] + points[58] + 0.5 * points[69])/2.5f;
//		cv::circle(image, Point(cvRound(eye_cheek_1r.x), cvRound(eye_cheek_1r.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//		cv::circle(image, Point(cvRound(eye_cheek_1l.x), cvRound(eye_cheek_1l.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
		points.push_back(eye_cheek_1r);
		points.push_back(eye_cheek_1l);

		// 40:50 62:58
		Point2f eye_cheek_2r = (points[40] + points[62])/2;
		Point2f eye_cheek_2l = (points[50] + points[58])/2;
//		cv::circle(image, Point(cvRound(eye_cheek_2r.x), cvRound(eye_cheek_2r.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//		cv::circle(image, Point(cvRound(eye_cheek_2l.x), cvRound(eye_cheek_2l.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
		points.push_back(eye_cheek_2r);
		points.push_back(eye_cheek_2l);

		// 39:49 1:11
		Point2f eye_cheek_3r = (points[39] + points[1])/2;
		Point2f eye_cheek_3l = (points[49] + points[11])/2;
//		cv::circle(image, Point(cvRound(eye_cheek_3r.x), cvRound(eye_cheek_3r.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
//		cv::circle(image, Point(cvRound(eye_cheek_3l.x), cvRound(eye_cheek_3l.y)), 1, CV_RGB(0, 255, 0), 1, LINE_AA);
		points.push_back(eye_cheek_2r);
		points.push_back(eye_cheek_2l);
#endif
#endif
    }

//	correctIris(image, points);

	return points;
}

// Detection and Estimation of Iris Centre Estimation Using Constrained Kalman Filtering
// https://arxiv.org/ftp/arxiv/papers/1506/1506.04843.pdf
void calcuateIrisInfo(const cv::Mat& image, const std::vector<cv::Point2f>& points, float skew_angle, cv::Point3f& iris_r, cv::Point3f& iris_l)
{
	// TODO
	calcuateEyeRegionInfo_r(points);
}

void calcuateIrisInfo(const cv::Mat& image, const std::vector<cv::Point2f>& points, cv::Point3f& iris_r, cv::Point3f& iris_l)
{
	Vec4f line = Feature::getSymmetryAxis(points);
	float skew_angle = std::atan2(line[1], line[0]);  // with vertical line being -pi/2 radian.

	calcuateIrisInfo(image, points, skew_angle, iris_r, iris_l);
}


static Mat polygonMask(const Rect& rect, const std::vector<Point2f>& points, const int indices[], int length)
{
	assert(indices != nullptr && length >= 3);
	std::vector<Point> contour;
	contour.reserve(length);
	for(int j = 0; j < length; ++j)
	{
		const int& i = indices[j];
		float x = points[i].x - rect.x;  // make it relative
		float y = points[i].y - rect.y;
		contour.push_back(Point(cvRound(x), cvRound(y)));
	}

	const Point* polygons[1] = { contour.data() };
	const int num_points[] = { length };
	Mat mask(rect.size(), CV_8UC1, BACKGROUD_COLOR);
	cv::fillPoly(mask, polygons, num_points, 1, FOREGROUD_COLOR);
	return mask;
}

RoiInfo calcuateFaceRegionInfo(const std::vector<cv::Point2f>& points)
{
	Rect rect = cv::boundingRect(points);

	Point2f origin = rect.tl();
	Point2f pivot(rect.x + rect.width/2.0f, rect.y + rect.height/2.0f);

	Mat mask(rect.size(), CV_8UC3, BACKGROUD_COLOR);
	Point polygon[20];
	for(int i = 0; i < 20; ++i)
		polygon[i] = Point(cvRound(points[i].x - rect.x), cvRound(points[i].y - rect.y));
	cv::fillConvexPoly(mask, polygon, NELEM(polygon), FOREGROUD_COLOR);

	return RoiInfo(origin, pivot, mask);
}

RoiInfo calcuateEyeBrowRegionInfo(const std::vector<cv::Point2f>& points, bool right/* = true*/)
{
	const int count = 6;
	std::vector<Point> polygon(count);

	int start = right ? 20:26;
	for(int i = start; i < start + count; ++i)
		polygon[i] = Point(cvRound(points[i].x), cvRound(points[i].y));
	
	Rect rect = cv::boundingRect(polygon);
	Point2f origin = rect.tl();
	Point2f pivot;
	if(right)
	{
		pivot.x = (points[20].x +  points[21].x +  points[23].x +  points[24].x) / 4.0f;
		pivot.y = (points[20].y +  points[21].y +  points[23].y +  points[24].y) / 4.0f;
	}
	else
	{
		pivot.x = (points[27].x +  points[28].x +  points[30].x +  points[31].x) / 4.0f;
		pivot.y = (points[27].y +  points[28].y +  points[30].y +  points[31].y) / 4.0f;
	}

	Mat mask(rect.size(), CV_8UC3, BACKGROUD_COLOR);
	for(int i = 0; i < count; ++i)
	{
		polygon[i].x -= cvRound(origin.x);
		polygon[i].y -= cvRound(origin.y);
	}

	cv::fillConvexPoly(mask, polygon, FOREGROUD_COLOR);
	return RoiInfo(origin, pivot, mask);
}

// note that these pivot points are chose according to the sample image.
RoiInfo calcuateEyeRegionInfo_r(const std::vector<cv::Point2f>& points)
{
	float left = points[38].x, right = points[34].x;
	float top = points[36].y, bottom = points[40].y;
	Point2f origin(left, top);
	Point2f pivot((points[37].x + points[39].x)/2, (points[37].y + points[39].y)/2);

	int rows = cvRound(bottom - top);
	int cols = cvRound(right - left);
	const int indices[] = { 38, 39, 40, 41, 34, 35, 36, 37 };
	Rect rect(cvRound(left), cvRound(top), cols, rows);
	Mat mask = polygonMask(rect, points, indices, NELEM(indices));

	return RoiInfo(origin, pivot, mask);
}

RoiInfo calcuateEyeRegionInfo_l(const std::vector<cv::Point2f>& points)
{
	float left = points[44].x, right = points[48].x;
	float top = points[46].y, bottom = points[50].y;
	Point2f origin(left, top);
	Point2f pivot((points[47].x + points[49].x)/2, (points[47].y + points[49].y)/2);

	int rows = cvRound(bottom - top);
	int cols = cvRound(right - left);
	const int indices[] = { 44, 45, 46, 47, 48, 49, 50, 51 };
	Rect rect(cvRound(left), cvRound(top), cols, rows);
	Mat mask = polygonMask(rect, points, indices, NELEM(indices));

	return RoiInfo(origin, pivot, mask);
}

RoiInfo calcuateBlusherRegionInfo(const std::vector<Point2f>& points, bool right/* = true*/)
{
	Point2f origin;
	Mat mask;
	if(right)
	{
		origin = Point2f(points[1].x, points[1].y);

		// blush_index_l = {1, 2, 3, 62, (34+62)/2, 62, (62+41)/2 };
		Point polygon_r[] = 
		{
			Point(cvRound(points[1].x), cvRound(points[1].y)),
			Point(cvRound(points[2].x), cvRound(points[2].y)),
			Point(cvRound(points[3].x), cvRound(points[3].y)),
			Point(cvRound((points[3].x + points[64].x)/2), cvRound((points[3].y + points[64].y)/2)),
			Point(cvRound(points[62].x), cvRound(points[62].y)),
			Point(cvRound((points[62].x + points[41].x)/2), cvRound((points[62].y + points[41].y)/2)),
		};

		cv::fillConvexPoly(mask, polygon_r, NELEM(polygon_r), Scalar(255,255,255));

	}
	else
	{
		origin = Point2f(points[1].x, points[1].y);

		Point polygon_l[] = 
		{
			Point(cvRound(points[11].x), cvRound(points[11].y)),
			Point(cvRound(points[10].x), cvRound(points[10].y)),
			Point(cvRound(points[9].x),  cvRound(points[9].y)),
			Point(cvRound((points[9].x) + points[68].x)/2, cvRound((points[9].y + points[68].y)/2)),
			Point(cvRound(points[58].x), cvRound(points[58].y)),
			Point(cvRound((points[58].x + points[51].x)/2), cvRound((points[58].y + points[51].y)/2)),
		};
		
		cv::fillConvexPoly(mask, polygon_l, NELEM(polygon_l), Scalar(255, 255, 255));
	}

	Point2f pivot = origin;
	return RoiInfo(origin, pivot, mask);
}

static void subdivLine(std::vector<Point>& contour, const Point2f& p0, const Point2f& p1, const Point2f& p2, const Point2f& p3)
{
	contour.push_back(catmullRomSpline(1.0f/3, p0, p1, p2, p3));
	contour.push_back(catmullRomSpline(2.0f/3, p0, p1, p2, p3));
	contour.push_back(p2);
}

RoiInfo calcuateLipsRegionInfo(const std::vector<Point2f>& points, int radius/* = 0*/, const Scalar& color/*= Scalar(255, 255, 255, 255)*/)
{
	// Due to the speed factor, mouth haven't been taken skew into consideration.
	float left = points[63].x, right = points[69].x;
	float top = std::min(points[65].y, points[67].y), bottom = points[78].y;

	Rect2f rect;
	left -= radius; right += radius;
	top -= radius; bottom += radius;

	Point2f origin(left, top);
	Point2f pivot(points[74].x + radius, points[74].y + radius);
	Rect region(cvRound(left), cvRound(top), cvRound(right - left), cvRound(bottom - top));
	Size size = region.size();
	Mat mask(size, CV_8UC1, Scalar::all(0));  // Bitmap.Config.A8 transparent
#if 0
	const int index_lips[] =
	{
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72,  // upper
		63, 73, 74, 75, 69, 76, 77, 78, 79, 80,  // lower
	};

	std::vector<Point> contour;
	contour.reserve(NELEM(index_lips));
	for(size_t j = 0; j < NELEM(index_lips); ++j)
	{
		const int& i = index_lips[j];
		float x = points[i].x - left;  // make it relative
		float y = points[i].y - top;
		contour.push_back(Point(cvRound(x), cvRound(y)));
	}
	
	const Point* polygons[2] = { contour.data(), contour.data() + contour.size()/2 };
	const int num_points[] = { static_cast<int>(contour.size()/2), static_cast<int>(contour.size()/2) };
	cv::fillPoly(mask, polygons, num_points, 2, color);  // white
#else
	std::vector<Point> contour;
	auto push = [&contour, &left, &top](const Point2f& point) { contour.push_back(Point(cvRound(point.x - left), cvRound(point.y - top))); };
	// upper lip
	for(int i = 63; i <= 72; ++i)
		push(points[i]);

	// lower lip
	push(points[63]);
	push(points[73]);
	push(catmullRomSpline(1.0f/3, points[63], points[73], points[74], points[75]));
	push(catmullRomSpline(2.0f/3, points[63], points[73], points[74], points[75]));
	push(points[74]);

	push(catmullRomSpline(1.0f/3, points[73], points[74], points[75], points[69]));
	push(catmullRomSpline(2.0f/3, points[73], points[74], points[75], points[69]));
	push(points[75]);

	push(points[69]);
	push(points[76]);

	for(int i = 76; i <= 79; ++i)
	{
		int i0 = i == 76 ? 69 : i-1;
		int i1 = i;
		int i2 = i + 1;
		int i3 = i == 79 ? 63 : i+2;
		push(catmullRomSpline(1.0f/3, points[i0], points[i1], points[i2], points[i3]));
		push(catmullRomSpline(2.0f/3, points[i0], points[i1], points[i2], points[i3]));
		push(points[i2]);
	}

	int upper_length = 73 - 63, lower_length = static_cast<int>(contour.size()) - upper_length;
	const Point* polygons[2] = { contour.data(), contour.data() + upper_length };
	const int num_points[] = { upper_length, lower_length };
	cv::fillPoly(mask, polygons, num_points, 2, color);

	if(radius > 0)
		cv::blur(mask, mask, Size(radius, radius));
#endif
//	imshow("mouth_mask", mask);
	return RoiInfo(origin, pivot, mask);
}

Mat stretchImage(const Mat& image, const Size& src_size, const Size& dst_size, const std::vector<Point2f>& src_points, const std::vector<Point2f>& dst_points, const std::vector<Vec3b>& indices)
{
	assert(src_points.size() == dst_points.size());
	Mat result(dst_size, image.type());

	Rect src_rect(Point(0, 0), src_size);
	Rect dst_rect(Point(0, 0), dst_size);
#ifndef NDEBUG
	for(const Point2f& point: src_points)
		assert(point.inside(src_rect));

	for(const Point2f& point: dst_points)
		assert(point.inside(dst_rect));
#endif

	Mat image_f;
	int type = image.type();
	if(type != CV_32F)
        image.convertTo(image_f, CV_32F);
	assert(false);  // TODO
	return image;
}

// http://www.learnopencv.com/face-morph-using-opencv-cpp-python/
// https://www.learnopencv.com/warp-one-triangle-to-another-using-opencv-c-python/
// affine src image to dst image with respect to some feature points.
Mat& stretchImage(const Mat& src_image, Mat& dst_image, const std::vector<Point2f>& src_points, const std::vector<Point2f>& dst_points, const std::vector<Vec3b>& indices)
{
	assert(src_points.size() == dst_points.size());
	assert(src_image.type() == dst_image.type() && src_image.depth() == CV_32F);
	int type = src_image.type();
	
	std::vector<Point2f> src_triangle(3), dst_triangle(3);
	for(const Vec3i& tri: indices)
	{
		assert(0 <= tri[0] && tri[0] < static_cast<int>(dst_points.size()));
		assert(0 <= tri[1] && tri[1] < static_cast<int>(dst_points.size()));
		assert(0 <= tri[2] && tri[2] < static_cast<int>(dst_points.size()));

		for(int i = 0; i < 3; ++i)
		{
			src_triangle[i] = src_points[tri[i]];
			dst_triangle[i] = dst_points[tri[i]];
		}

		// Find bounding rectangle for each triangle
		Rect src_rect = cv::boundingRect(src_triangle);
		Rect dst_rect = cv::boundingRect(dst_triangle);

		// Offset points by left top corner of the respective rectangles
		std::vector<Point2f> src_cropped, dst_cropped;
		std::vector<Point> dst_cropped_int;

		for(int i = 0; i < 3; ++i)
		{
			src_cropped.push_back(Point2f(src_triangle[i].x - src_rect.x, src_triangle[i].y - src_rect.y));
			dst_cropped.push_back(Point2f(dst_triangle[i].x - dst_rect.x, dst_triangle[i].y - dst_rect.y));
 
			// fillConvexPoly needs a vector of Point and not Point2f
			dst_cropped_int.push_back(Point(cvRound(dst_triangle[i].x - dst_rect.x), cvRound(dst_triangle[i].y - dst_rect.y)) );
		}
 
		// Apply warpImage to small rectangular patches
		Mat img1Cropped;
		src_image(src_rect).copyTo(img1Cropped);

		// Given a pair of triangles, find the affine transform.
		Mat transform = cv::getAffineTransform(src_cropped, dst_cropped);
		
		// Apply the affine transform just found to the src image
		Mat img2Cropped = Mat::zeros(dst_rect.size(), img1Cropped.type());
		cv::warpAffine(img1Cropped, img2Cropped, transform, img2Cropped.size(), INTER_LINEAR, BORDER_REFLECT_101);  // INTER_CUBIC

		// Get mask by filling triangle
		Mat mask(dst_rect.size(), type, Scalar::all(0.0));
		cv::fillConvexPoly(mask, dst_cropped_int, Scalar::all(1.0), LINE_AA);
		
		// Copy triangular region of the rectangular patch to the output image
		Mat dst_roi = dst_image(dst_rect);
		multiply(img2Cropped, mask, img2Cropped);
		multiply(dst_roi, Scalar::all(1.0) - mask, dst_roi);
		dst_roi = dst_roi + img2Cropped;

#if 0  // used for debugging intermediate modification
		cv::imshow("affine patch by patch", dst_image);
		cv::waitKey();
#endif
	}

	return dst_image;
}

void splitColorAndAlpha(const Mat& image, Mat& color, Mat& alpha)
{
	assert(image.type() == CV_8UC4);
	color = Mat(image.size(), CV_8UC3);
	alpha = Mat(image.size(), CV_8UC1);

//	Mat image_bgr;
//	cvtColor(image, image_bgr, CV_BGRA2BGR);
	for(int r = 0; r < image.rows; ++r)
	for(int c = 0; c < image.cols; ++c)
	{
//#ifdef ANDROID
//		const cv::Vec4b& argb = image.at<cv::Vec4b>(r, c);
//		// Config.ARGB_8888 means ANDROID_BITMAP_FORMAT_RGBA_8888
//		// Android use ARGB, different name, damn it! https://groups.google.com/d/topic/android-ndk/XBGh8LZvkls
//		alpha.at<uchar>(r, c) = argb[0];
//		color.at<cv::Vec3b>(r, c) = *reinterpret_cast<const cv::Vec3b*>(reinterpret_cast<const uchar*>(&argb) + 1);
//#else
		// OpenCV use BGRA, what a mess world!
		const cv::Vec4b& bgra = image.at<cv::Vec4b>(r, c);
		color.at<cv::Vec3b>(r, c) = *reinterpret_cast<const cv::Vec3b*>(&bgra);
		alpha.at<uchar>(r, c) = bgra[3];
//#endif
	}
}

cv::Mat& stretchImageWithAlpha(const Mat& src_image, Mat& dst_image, const std::vector<cv::Point2f>& src_points, const std::vector<cv::Point2f>& dst_points, const std::vector<cv::Vec3b>& indices)
{
	Mat src_image_bgr, src_image_a;
	splitColorAndAlpha(src_image, src_image_bgr, src_image_a);
	src_image_bgr.convertTo(src_image_bgr, CV_32FC3, 1/255.0);
	src_image_a.convertTo(src_image_a, CV_32FC1, 1/255.0);

	Mat dst_image_bgr, dst_image_a;
	splitColorAndAlpha(dst_image, dst_image_bgr, dst_image_a);
	dst_image_bgr.convertTo(dst_image_bgr, CV_32FC3, 1/255.0);
	dst_image_a.convertTo(dst_image_a, CV_32FC1, 1/255.0);

	Mat result(dst_image.size(), CV_32FC4);
	stretchImage(src_image_bgr, dst_image_bgr, src_points, dst_points, indices);
	stretchImage(src_image_a, dst_image_a, src_points, dst_points, indices);

	// merge color and alpha
	for(int r = 0; r < dst_image.rows; ++r)
	for(int c = 0; c < dst_image.cols; ++c)
	{
		float& a = dst_image_a.at<float>(r, c);
		cv::Vec3f& bgr = dst_image_bgr.at<cv::Vec3f>(r, c);
		result.at<cv::Vec4f>(r, c) = cv::Vec4f(bgr[0], bgr[1], bgr[2], a);
	}

	// CV_32FC4 => CV_8UC4
	result.convertTo(dst_image, CV_8UC4, 255.0);
	
	return dst_image;
}

cv::Mat cloneFace(const std::string& user_image_path, const std::string& model_image_path, const std::string& datadir)
{
	TIME_START;

	Mat user = imread(user_image_path);
	Mat model = imread(model_image_path);
	assert(user.data && model.data);  // make sure image does exist and path is correct.
	TIME_STOP("cloneFace > imread");
	
	Mat user_image_gray, model_image_gray;
	cv::cvtColor(user, user_image_gray, CV_BGR2GRAY);
	cv::cvtColor(model, model_image_gray, CV_BGR2GRAY);
	TIME_STOP("cloneFace > cvtColor");

	std::vector<Point2f> user_points = getFaceFeaturePoints(user_image_gray, user_image_path, datadir);
	std::vector<Point2f> model_points = getFaceFeaturePoints(model_image_gray, model_image_path, datadir);
	TIME_STOP("cloneFace > getFaceFeaturePoints");
	if(user_points.empty() || model_points.empty())
	{
		LOGE("no face detectd, return raw image");
		return user;
	}

	RoiInfo user_face_info = calcuateFaceRegionInfo(user_points);
	RoiInfo model_face_info = calcuateFaceRegionInfo(model_points);
	TIME_STOP("cloneFace > calcuateFaceRoiInfo");
	
	// make all the stuff relative to face, not entire image.
	Rect user_face_rect(cvRound(user_face_info.origin.x), cvRound(user_face_info.origin.y), user_face_info.mask.cols, user_face_info.mask.rows);
	Rect model_face_rect(cvRound(model_face_info.origin.x), cvRound(model_face_info.origin.y), model_face_info.mask.cols, model_face_info.mask.rows);
	Mat user_face = user(user_face_rect);
	Mat model_face = model(model_face_rect);

	for(Point2f& point: user_points)
		point -= user_face_info.origin;
	for(Point2f& point: model_points)
		point -= model_face_info.origin;
	TIME_STOP("cloneFace > cropFace");
	
	user_face.convertTo(user_face, CV_32FC3, 1/255.0);
	model_face.convertTo(model_face, CV_32FC3, 1/255.0);
	stretchImage(model_face, user_face, model_points, user_points, Feature::triangle_indices);
	user_face.convertTo(user_face, CV_8UC3, 255.0);
	TIME_STOP("cloneFace > convert image from CV_8UC3 to CV_32FC3, stretchImage");
	
	cv::seamlessClone(user_face, user, user_face_info.mask, user_face_info.pivot, user, MIXED_CLONE);
	TIME_STOP("cloneFace > seamlessClone");
	return user;
}

Mat& blendColor(Mat& src, const Mat& mask, uint32_t color, float alpha)
{
	assert(0.0f <= alpha && alpha <= 1.0f);
	assert(src.size() == mask.size() && mask.type() == CV_8UC1);
	
	Mat mask2;
	cvtColor(mask, mask2, CV_GRAY2RGB);

	uint32_t red = (color >> 16) & 0xff;
	uint32_t green = (color >> 8) & 0xff;
	uint32_t blue = color & 0xff;
#if USE_OPENCV_BGRA_LAYOUT
	Scalar rgb(blue, green, red);
#else
	Scalar rgb(red, green, blue);
#endif
	Mat blend(src.size(), CV_8UC3, rgb);
	cv::addWeighted(src, alpha, blend, (1.0 - alpha), 0.0, blend);
	cv::bitwise_and(blend, mask2, blend);

	// http://docs.opencv.org/master/d0/d86/tutorial_py_image_arithmetics.html
	// http://docs.opencv.org/3.0-beta/modules/core/doc/operations_on_arrays.html
	// dst = src&(mask) | dst&(~mask)
	Mat mask_inv;
	cv::bitwise_not(mask2, mask_inv);
	cv::bitwise_and(src, mask_inv, src);
	
	cv::add(src, blend, src);
	return src;
}

Mat overlay(const Mat& src, Mat& dst, const Point& origin, int alpha)
{
	assert(src.channels() == 4 && dst.channels() == 4);  // use alpha channel to blend
	assert(0 <= alpha && alpha < 256);

	Size size(src.cols, src.rows);
	Rect rect1(origin, size);
	Rect rect2(0, 0, dst.cols, dst.rows);
	
	// http://docs.opencv.org/2.4/modules/core/doc/basic_structures.html?highlight=rect#Rect_
	rect1 &= rect2;
	if(rect1.area() <= 0)  // not overlap
		return dst;
	
	// https://en.wikipedia.org/wiki/Alpha_compositing
	// if dst is BGR format, treat A as 255, namely opaque.
	Mat roi = dst(rect1);
	for(int r = 0; r < roi.rows; ++r)
	for(int c = 0; c < roi.cols; ++c)
	{
		const cv::Vec4b& src_bgra = src.at<cv::Vec4b>(r, c);
		cv::Vec4b& dst_bgra = roi.at<cv::Vec4b>(r, c);
//		cv::Vec3b& dst_bgr = dst_has_alpha? *reinterpret_cast<cv::Vec3b*>(&roi.at<cv::Vec4b>(r, c)) : roi.at<cv::Vec3b>(r, c);
		
//		cv::Vec3i src_bgr(src_bgra[0], src_bgra[1], src_bgra[2]);

		const uchar& src_a = src_bgra[3];

		//(src_bgr * src_a + dst_bgr * 255 * (1 - src_a)) / (src_a + 255 * (1 - src_a));
		//dst_color = dst_color * 255 +
		// https://docs.gimp.org/en/gimp-concepts-layer-modes.html
		// overlay
		if(src_a == 0)
			continue;

		float t = static_cast<float>(alpha * src_a)/(255 * 255);
		float one_minuse_t = 1.0f - t;

        dst_bgra[0] = cvRound((one_minuse_t * dst_bgra[0] + t * src_bgra[0]));
        dst_bgra[1] = cvRound((one_minuse_t * dst_bgra[1] + t * src_bgra[1]));
        dst_bgra[2] = cvRound((one_minuse_t * dst_bgra[2] + t * src_bgra[2]));
//		for(int i = 0; i < 3; ++i)
//		{
//			int value = cvRound((1.0f - t) * dst_bgra[i] + t * src_bgra[i]);
			
//			float value = dst_bgra[i]/255.0f * (dst_bgra[i] + 2* src_bgra[i]/255.0f*(255.0f - dst_bgra[i]));
//			assert(0 <= value && value <= 255.0f);
//			dst_bgr[i] = static_cast<uchar>(value);
//			int value = ((255 - src_a) * dst_bgra[i] + src_a * src_bgra[i]) / 255;
//			float value = (1.0f - alpha) * dst_bgra[i] + alpha * src_bgra[i];
//			assert(0 <= value && value <= 255);
//			dst_bgra[i] = static_cast<uchar>(value);
//		}

	}

	return dst;
}

void blendIris(cv::Mat& image, const cv::Mat& iris, const std::vector<cv::Point2f>& points, float amount)
{
	// iris right index 42， left index 43
	const int iris_indices_r[] = {35, 37, 39, 41};
	const int iris_indices_l[] = {45, 47, 49, 51};
	float iris_radius_r = distance(points[42], points[iris_indices_r[0]]);
	for(size_t i = 1; i < NELEM(iris_indices_r); ++i)
	{
		float dist = distance(points[42], points[iris_indices_r[i]]);
		if(iris_radius_r > dist)
			iris_radius_r = dist;
	}
				
	float iris_radius_l = distance(points[43], points[iris_indices_l[0]]);
	for(size_t i = 1; i < NELEM(iris_indices_l); ++i)
	{
		float dist = distance(points[43], points[iris_indices_l[i]]);
		if(iris_radius_l > dist)
			iris_radius_l = dist;
	}

	cv::Mat iris2;
	if(iris.channels() == 3)
		cvtColor(iris, iris2, CV_BGR2BGRA);
	iris2.convertTo(iris2, CV_32FC4, 1/255.0);

	const Vec3f FROM_COLOR(1.0f, 1.0f, 1.0f);  // white
	for(int r = 0; r < iris2.rows; ++r)
	for(int c = 0; c < iris2.cols; ++c)
	{
		Vec4f& color = iris2.at<Vec4f>(r, c);
		color = venus::color2alpha(color, FROM_COLOR);
	}
	iris2.convertTo(iris2, CV_8UC4, 255.0);
//	cv::imwrite("color2alpha.png", iris2);

	// TODO it seems this value fit every iris image in res/drawable/
	const float iris_image_radius = (84.0f - 12.0f)/2;

	Mat iris_r, iris_l;
	float iris_ratio_r = iris_radius_r / iris_image_radius;
	float iris_ratio_l = iris_radius_l / iris_image_radius;
	cv::resize(iris2, iris_r, Size(), iris_ratio_r, iris_ratio_r);
	cv::resize(iris2, iris_l, Size(), iris_ratio_l, iris_ratio_l);

	const Point2f& iris_center_r = points[42];
	const Point2f& iris_center_l = points[43];
	Point2i iris_position_r(iris_center_r.x - (iris_r.cols >> 1), iris_center_r.y - (iris_r.rows >> 1));
	Point2i iris_position_l(iris_center_l.x - (iris_l.cols >> 1), iris_center_l.y - (iris_l.rows >> 1));

	RoiInfo eye_info_r = calcuateEyeRegionInfo_r(points);
	RoiInfo eye_info_l = calcuateEyeRegionInfo_l(points);

	Makeup::blend(iris_r, image, eye_info_r.mask, iris_position_r, amount);
	Makeup::blend(iris_l, image, eye_info_l.mask, iris_position_l, amount);
}




} /* namespace venus */