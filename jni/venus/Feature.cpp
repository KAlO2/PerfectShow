#include "venus/Feature.h"
#include "venus/opencv_utility.h"
#include "venus/region_operation.h"

//#include "opencv2/highgui.hpp"
//#include "opencv2/imgproc.hpp"

using namespace cv;

namespace venus {

// number of feature points detected
const int Feature::COUNT = 81;

// https://en.wikipedia.org/wiki/Delaunay_triangulation
// Delaunay triangle indices generated from mark() function
const std::vector<Vec3i> Feature::triangle_indices
{
	Vec3i(26, 16, 27),
	Vec3i(16, 26, 25),
	Vec3i(60, 57, 67),
	Vec3i(57, 60, 56),
	Vec3i( 1,  0, 38),
	Vec3i( 2,  1, 39),
	Vec3i(60, 55, 56),
	Vec3i(55, 60, 65),
	Vec3i( 3,  2, 63),
	Vec3i(53, 44, 52),
	Vec3i(44, 53, 34),
	Vec3i( 4,  3, 63),
	Vec3i(44, 25, 26),
	Vec3i(25, 44, 34),
	Vec3i( 5,  4, 79),
	Vec3i( 2, 40, 62),
	Vec3i(40,  2, 39),
	Vec3i( 6,  5, 78),
	Vec3i( 7,  6, 78),
	Vec3i( 8,  7, 77),
	Vec3i(35, 25, 34),
	Vec3i(25, 35, 24),
	Vec3i( 9,  8, 69),
	Vec3i(50, 58, 52),
	Vec3i(58, 50, 49),
	Vec3i(10,  9, 69),
	Vec3i(23, 33, 37),
	Vec3i(33, 23, 24),
	Vec3i(11, 10, 58),
	Vec3i(15, 27, 16),
	Vec3i(27, 15, 14),
	Vec3i(12, 11, 48),
	Vec3i(16, 20, 17),
	Vec3i(20, 16, 25),
	Vec3i(13, 12, 29),
	Vec3i(14, 13, 28),
	Vec3i(21, 23, 22),
	Vec3i(23, 21, 20),
	Vec3i(23, 38, 22),
	Vec3i(38, 23, 37),
	Vec3i(18, 17, 20),
	Vec3i(18, 20, 21),
	Vec3i(19, 18, 21),
	Vec3i( 0, 19, 22),
	Vec3i(24, 23, 20),
	Vec3i(20, 25, 24),
	Vec3i(19, 21, 22),
	Vec3i( 0, 22, 38),
	Vec3i(24, 35, 33),
	Vec3i(52, 44, 51),
	Vec3i(44, 26, 45),
	Vec3i(11, 49, 48),
	Vec3i(49, 11, 58),
	Vec3i(27, 31, 26),
	Vec3i(31, 27, 30),
	Vec3i(48, 30, 29),
	Vec3i(30, 48, 47),
	Vec3i(28, 30, 27),
	Vec3i(30, 28, 29),
	Vec3i(27, 14, 28),
	Vec3i(28, 13, 29),
	Vec3i(29, 12, 48),
	Vec3i(47, 46, 32),
	Vec3i(46, 47, 43),
	Vec3i(26, 31, 45),
	Vec3i(31, 30, 32),
	Vec3i(32, 30, 47),
	Vec3i(31, 32, 45),
	Vec3i(49, 47, 48),
	Vec3i(47, 49, 43),
	Vec3i(37, 33, 36),
	Vec3i(42, 39, 37),
	Vec3i(39, 42, 40),
	Vec3i(54, 55, 61),
	Vec3i(55, 54, 56),
	Vec3i(34, 53, 54),
	Vec3i(35, 34, 41),
	Vec3i(42, 37, 36),
	Vec3i(33, 35, 36),
	Vec3i(36, 35, 42),
	Vec3i(37, 39, 38),
	Vec3i(39,  1, 38),
	Vec3i(42, 41, 40),
	Vec3i(41, 42, 35),
	Vec3i(62, 63,  2),
	Vec3i(63, 62, 61),
	Vec3i(34, 54, 41),
	Vec3i(40, 41, 54),
	Vec3i(43, 50, 51),
	Vec3i(50, 43, 49),
	Vec3i(46, 45, 32),
	Vec3i(45, 46, 43),
	Vec3i(43, 51, 45),
	Vec3i(10, 69, 58),
	Vec3i(44, 45, 51),
	Vec3i(51, 50, 52),
	Vec3i(56, 53, 52),
	Vec3i(53, 56, 54),
	Vec3i(68, 57, 59),
	Vec3i(57, 68, 67),
	Vec3i(61, 64, 63),
	Vec3i(64, 61, 55),
	Vec3i(40, 54, 62),
	Vec3i(62, 54, 61),
	Vec3i(64, 55, 65),

	Vec3i(56, 52, 57),
	Vec3i(60, 67, 66),
	Vec3i(57, 52, 59),
	Vec3i(69,  8, 76),
	Vec3i(59, 52, 58),
	Vec3i(59, 58, 69),
	Vec3i(59, 69, 68),
        

	Vec3i( 4, 63, 80),
	Vec3i(65, 60, 66),
	Vec3i(77, 76,  8),
	Vec3i( 7, 78, 77),
	Vec3i(79,  4, 80),
	Vec3i(78,  5, 79),
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
	Vec3i(63, 64, 72),
	Vec3i(64, 65, 72),
	Vec3i(65, 71, 72),
	Vec3i(65, 66, 71),
	Vec3i(66, 67, 71),
	Vec3i(71, 67, 70),
	Vec3i(67, 68, 70),
	Vec3i(68, 69, 70),

	// inner region of mouth, or teeth region
	Vec3i(69, 75, 70),
	Vec3i(70, 75, 71),
	Vec3i(71, 75, 74),
	Vec3i(71, 74, 73),
	Vec3i(72, 71, 73),
	Vec3i(73, 63, 72),

	// bottom lip
	Vec3i(63, 73, 80),
	Vec3i(80, 73, 79),
	Vec3i(79, 73, 78),
	Vec3i(73, 74, 78),
	Vec3i(78, 74, 75),
	Vec3i(78, 75, 77),
	Vec3i(77, 75, 76),
	Vec3i(76, 75, 69),
};

Feature::Feature(const cv::Mat& image, const std::vector<cv::Point2f>& points):
	image(image),
	points(points)
{
	assert(points.size() == COUNT);
	line = getSymmetryAxis(points);
}

cv::Vec4f Feature::getSymmetryAxis(const std::vector<cv::Point2f>& points)
{
#if 0
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

	// dx > 0, /o/o/; dx ==0, |o|o|; dx < 0 \o\o\;
	// dy is always negative in Y top down axis, need to flip for Y up axis.
	// y = k * x; y = -1/k * x;
	// atan2(y, x); atan2(x, -y);
	// atan2(x, -y) for image, inverse for makeup, namely atan2(x, y)
//	float theta = std::atan2(x, y);

	return line;
}

/*
	@see <a href="http://140.118.9.222/publications/journal/draft.pdf">Automatic Hair Extraction from 2D Images</a>	Chuan-Kai Yang and Chia-Ning Kuo
	Department of Information Management
	National Taiwan University of Science and Technology
*/
cv::Mat Feature::maskHair() const
{
	Mat mask(image.size(), CV_8UC1);
//	Point2f center = (points[0] + points[12])/2;  // (ASM0 + ASM12)/2
	Vec4f line = getSymmetryAxis();
	Point2f center(line[2], line[3]);

	const int N = 13;
	Point2f hair[N];
	for(int i = 0; i < N; ++i)
	{
		float x = static_cast<float>(i<<1)/N - 1.0f;
		float k = 1 + 0.2f * (x*x);
		hair[i] = k *(center - points[i]) + center;
	}
	
	return mask;
}

cv::Mat1b Feature::maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, const int indices[], int length)
{
	assert(indices != nullptr && length >= 3);

	std::vector<Point> contour(length);
	for(int j = 0; j < length; ++j)
	{
		const int& i = indices[j];
		float x = points[i].x - rect.x;  // make it relative
		float y = points[i].y - rect.y;
		contour.push_back(Point(cvRound(x), cvRound(y)));
	}

	const Point* polygons[1] = { contour.data() };
	const int num_points[] = { length };
	Mat1b mask(rect.size(), 0);  // Mat mask(rect.size(), CV_8UC1, Scalar(0));
	cv::fillPoly(mask, polygons, num_points, 1, Scalar(255));
	return mask;
}

cv::Mat1b Feature::maskPolygon(const cv::Rect2i& rect, const std::vector<cv::Point2f>& points, int start, int length)
{
	assert(0 <= start && start < static_cast<int>(points.size()));
	assert(3 <= length&& start + length < static_cast<int>(points.size()));

	std::vector<Point> contour(length);

	const Point2i origion = rect.tl();
	for(int i = 0; i < length; ++i)
		contour[i] = static_cast<Point2i>(points[start + i]) - origion;

	const Point* polygons[1] = { contour.data() };
	const int num_points[] = { length };
	Mat1b mask(rect.size(), 0);
	cv::fillPoly(mask, polygons, num_points, 1, Scalar(255));
	return mask;
}

Region Feature::calcuateBrowRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right)
{
/*
	Below are eye brow feature point indices:

			  .--21--20\        /27--28--.
		  _.-`          |      |          `-._
	right 22--23--24--25`      `26--31--30--29  left
*/
#if 0  // not so good
	const int start = right ? 20:26, length = 6;
	std::vector<Point2f> polygon(length);

	for(int i = 0; i < length; ++i)
		polygon[i] = points[start + i];
	
#else
	const Point2f& _0(right ? points[20]:points[27]);
	const Point2f& _1(right ? points[21]:points[28]);
	const Point2f& _2(right ? points[22]:points[29]);
	const Point2f& _3(right ? points[23]:points[30]);
	const Point2f& _4(right ? points[24]:points[31]);
	const Point2f& _5(right ? points[25]:points[26]);
	const Point2f& _p(right ? points[ 0]:points[12]);

	std::vector<Point2f> polygon;
	polygon.push_back(_0);
	polygon.push_back(catmullRomSpline(0.25f, _5, _0, _1, _2));
	polygon.push_back(catmullRomSpline(0.50f, _5, _0, _1, _2));
	polygon.push_back(catmullRomSpline(0.75f, _5, _0, _1, _2));
	polygon.push_back(_1);

	polygon.push_back(catmullRomSpline(0.25f, _0, _1, _2, _p));
	polygon.push_back(catmullRomSpline(0.50f, _0, _1, _2, _p));
	polygon.push_back(catmullRomSpline(0.75f, _0, _1, _2, _p));
	polygon.push_back(_2);
	polygon.push_back(_3);

//	polygon.push_back(catmullRomSpline(0.50f, _2, _3, _4, _5));
	polygon.push_back(_4);
	polygon.push_back(_5);

	Point2f diff = _5 - _1;
	float r = std::sqrt(diff.x * diff.x + diff.y * diff.y);
	diff /= r;
	r = distance(_4, _5) * 0.81f;  // 0.81 is OK
	polygon.push_back(_0 + r * diff);

	int length = static_cast<int>(polygon.size());
#endif

//	Rect2i rect = cv::boundingRect(polygon);
	Vec4f box = venus::boundingBox(polygon);
	Rect2i rect = box2Rect(box);

	Mat1b mask(rect.size(), 0);

	Point2i origion = rect.tl();
	std::vector<Point2i> polygon2 = venus::cast(polygon);
	for(int i = 0; i < length; ++i)
		polygon2[i] -= origion;

	cv::fillConvexPoly(mask, polygon2, Scalar(255));

	// TODO need to come up with a more suitable pivot
	Point2f pivot = right?
		(points[20] + points[21] + points[23] + points[24]) / 4.0f:
		(points[27] + points[28] + points[30] + points[31]) / 4.0f;

	Size2f size = calculateSize(box, line);
	return Region(pivot, size, mask);
}

Region Feature::calcuateEyeRegion(const std::vector<cv::Point2f>& points, const cv::Vec4f& line, bool right)
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
	int start = right?34:44;

	// Point2f implicitly cast to Point2i, which calls saturate_cast(), which calls cvRound()
	std::vector<Point> polygon(length);
	for(int i = 0; i < length; ++i)
		polygon[i] = points[start + i];

	Rect rect = cv::boundingRect(polygon);
	Mat1b mask = maskPolygon(rect, points, start, length);

	Point2f pivot = (points[start+3] + points[start+5])/2;

	Vec4f box = right?
		Vec4f(points[38].x, points[36].y, points[34].x, points[40].y):
	    Vec4f(points[44].x, points[46].y, points[48].x, points[50].y);
	Size2f size = calculateSize(box, line);

	return Region(pivot, size, mask);
}

Region Feature::maskLips(const std::vector<cv::Point2f>& points, const cv::Vec4f& line)
{
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
//	float left = points[63].x, right = points[69].x;
//	float top = std::min(points[65].y, points[67].y), bottom = points[78].y;
	Vec4f box = boundingBox(points, 63, 80 - 63 + 1);
	Rect2i rect = box2Rect(box);

	Point2f pivot = points[71];  // or (points[71] + points[74])/2;
	Size2f size = calculateSize(box, line);
/*
	const int lip_indices[] =
	{
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72,  // upper lip
		63, 73, 74, 75, 69, 76, 77, 78, 79, 80,  // lower lip
	};
*/
	std::vector<Point2i> contour(32);
	
	// upper lip
	for(int i = 63; i <= 72; ++i)
		contour.push_back(points[i]);

	// lower lip
	contour.push_back(points[63]);
	contour.push_back(points[73]);
	contour.push_back(catmullRomSpline(1.0f/3, points[63], points[73], points[74], points[75]));
	contour.push_back(catmullRomSpline(2.0f/3, points[63], points[73], points[74], points[75]));
	contour.push_back(points[74]);

	contour.push_back(catmullRomSpline(1.0f/3, points[73], points[74], points[75], points[69]));
	contour.push_back(catmullRomSpline(2.0f/3, points[73], points[74], points[75], points[69]));
	contour.push_back(points[75]);
	contour.push_back(points[69]);
	contour.push_back(points[76]);

	for(int i = 76; i <= 79; ++i)
	{
		int i0 = i == 76 ? 69 : i-1;
		int i1 = i;
		int i2 = i + 1;
		int i3 = i == 79 ? 63 : i+2;
		contour.push_back(catmullRomSpline(1.0f/3, points[i0], points[i1], points[i2], points[i3]));
		contour.push_back(catmullRomSpline(2.0f/3, points[i0], points[i1], points[i2], points[i3]));
		contour.push_back(points[i2]);
	}

	int upper_length = 72 - 63 + 1;  // fixed length
	int lower_length = static_cast<int>(contour.size()) - upper_length;  // the rest
	const Point* polygons[2] = { contour.data(), contour.data() + upper_length };
	const int num_points[] = { upper_length, lower_length };
	Mat1b mask(rect.size(), 0);  // Bitmap.Config.A8 transparent
	cv::fillPoly(mask, polygons, num_points, 2, Scalar(255));

	return Region(pivot, size, mask);
}

void Feature::assignRegion()
{
	m_facePolygon.reserve(20);
	for(int i = 0; i < 20; ++i)
		m_facePolygon.push_back(Point(cvRound(points[i].x), cvRound(points[i].y)));

	const int eye_brow_point_count = 6;
	m_rightEyeBrowPolygon.reserve(eye_brow_point_count);
	m_leftEyeBrowPolygon.reserve(eye_brow_point_count);
	for(int i = 20; i < (20 + eye_brow_point_count); ++i)
		m_rightEyeBrowPolygon.push_back(Point(cvRound(points[i].x), cvRound(points[i].y)));
	for(int i = 26; i < (26 + eye_brow_point_count); ++i)
		m_leftEyeBrowPolygon.push_back(Point(cvRound(points[i].x), cvRound(points[i].y)));

	const int eye_point_count = 8;
	m_rightEyePolygon.reserve(eye_point_count);
	m_leftEyePolygon.reserve(eye_point_count);
	for(int i = 34; i < (34 + eye_point_count); ++i)
		m_rightEyePolygon.push_back(Point(cvRound(points[i].x), cvRound(points[i].y)));
	for(int i = 44; i < (34 + eye_point_count); ++i)
		m_leftEyePolygon.push_back(Point(cvRound(points[i].x), cvRound(points[i].y)));

	m_upperLipPolygon.reserve(10);
	for(int i = 63; i < 73; ++i)
		m_upperLipPolygon.push_back(Point(cvRound(points[i].x), cvRound(points[i].y)));

	m_lowerLipPolygon.reserve(9);
	m_lowerLipPolygon.push_back(Point(cvRound(points[63].x), cvRound(points[63].y)));
	for(int i = 73; i < 81; ++i)
		m_lowerLipPolygon.push_back(Point(cvRound(points[i].x), cvRound(points[i].y)));
	
	m_mouthPolygon.reserve(12);
	for(int i = 63; i < 70; ++i)
		m_mouthPolygon.push_back(Point(cvRound(points[i].x), cvRound(points[i].y)));
	for(int i = 76; i < 81; ++i)
		m_mouthPolygon.push_back(Point(cvRound(points[i].x), cvRound(points[i].y)));

	const int nose_point_count = 10;
	m_nosePolygon.reserve(nose_point_count);
	int nose_indices[nose_point_count] = {52, 53, 54, 62, 61, 55, 60, 57, 59, 58};
	for(int i = 0; i < nose_point_count; ++i)
	{
		int j = nose_indices[i];
		m_nosePolygon.push_back(Point(cvRound(points[j].x), cvRound(points[j].y)));
	}

	m_rightBlusherPolygon.push_back(Point(cvRound(points[1].x), cvRound(points[1].y)));
	m_rightBlusherPolygon.push_back(Point(cvRound(points[2].x), cvRound(points[2].y)));
	m_rightBlusherPolygon.push_back(Point(cvRound(points[3].x), cvRound(points[3].y)));
	m_rightBlusherPolygon.push_back(Point(cvRound((points[3].x + points[64].x)/2), cvRound((points[3].y + points[64].y)/2)));
	m_rightBlusherPolygon.push_back(Point(cvRound(points[62].x), cvRound(points[62].y)));
	m_rightBlusherPolygon.push_back(Point(cvRound((points[62].x + points[41].x)/2), cvRound((points[62].y + points[41].y)/2)));

	m_leftBlusherPolygon.push_back(Point(cvRound(points[11].x), cvRound(points[11].y)));
	m_leftBlusherPolygon.push_back(Point(cvRound(points[10].x), cvRound(points[10].y)));
	m_leftBlusherPolygon.push_back(Point(cvRound(points[9].x),  cvRound(points[9].y)));
	m_leftBlusherPolygon.push_back(Point(cvRound((points[9].x) + points[68].x)/2, cvRound((points[9].y + points[68].y)/2)));
	m_leftBlusherPolygon.push_back(Point(cvRound(points[58].x), cvRound(points[58].y)));
	m_leftBlusherPolygon.push_back(Point(cvRound((points[58].x + points[51].x)/2), cvRound((points[58].y + points[51].y)/2)));
}

std::vector<Point> Feature::getPolygon(int region)
{
	switch(region)
	{
	case FACE:         return m_facePolygon;
	case EYE_BROW_L:   return m_leftEyeBrowPolygon;
	case EYE_BROW_R:   return m_rightEyeBrowPolygon;
	case EYE_LASH_L:   return m_leftEyePolygon;  // TODO
	case EYE_LASH_R:   return m_rightEyePolygon; // TODO
	case EYE_SHADOW_L: return m_leftEyePolygon;	 // TODO
	case EYE_SHADOW_R: return m_rightEyePolygon; // TODO
	case BLUSH_L:	   return m_leftBlusherPolygon;
	case BLUSH_R:	   return m_rightBlusherPolygon;
	case NOSE:		   return m_nosePolygon;
	case LIP_B:	       return m_mouthPolygon;
	default: assert(false); return std::vector<Point>();
	}
}

} /* namespace venus */