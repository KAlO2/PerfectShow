#include "venus/ImageWarp.h"
#include "venus/opencv_utility.h"
#include "venus/scalar.h"
#include <opencv2/imgproc.hpp>

using namespace cv;

namespace venus {

ImageWarp::ImageWarp(int grid_size):
	grid_size(grid_size)
{
}

ImageWarp::ImageWarp():
ImageWarp(5)
{
}

inline float bilinear_interp(float x, float y, float v11, float v12, float v21, float v22)
{
	return (v11*(1-y) + v12*y) * (1-x) + (v21*(1-y) + v22*y) * x;
}

cv::Mat ImageWarp::setAllAndGenerate(const cv::Mat& src,
		const std::vector<Point2f>& src_points,
		const std::vector<Point2f>& dst_points,
		const cv::Size& target, float alpha, float amount/* = 1.0F */)
{
	setSourceSize(src.cols, src.rows);
	setTargetSize(target);
	setMappingPoints(dst_points, src_points);
	calculateDelta(alpha);
	return genNewImage(src, amount);
}

cv::Mat ImageWarp::genNewImage(const cv::Mat& src, float amount)
{
	Mat dst(dst_size.height, dst_size.width, src.type());
	for(int i = 0; i < dst_size.height; i += grid_size)
	for(int j = 0; j < dst_size.width; j += grid_size)
	{
		int ni = i + grid_size;
		int nj = j + grid_size;
		float w = static_cast<float>(grid_size), h = w;
		if(ni >= dst_size.height) ni = dst_size.height - 1, h = static_cast<float>(ni - i + 1);
		if(nj >= dst_size.width)  nj = dst_size.width - 1,  w = static_cast<float>(nj - j + 1);

		for(int di = 0; di < h; ++di)
		for(int dj = 0; dj < w; ++dj)
		{
			float deltaX = bilinear_interp(di/h, dj/w, rDx(i,j),rDx(i, nj), rDx(ni, j), rDx(ni, nj));
			float deltaY = bilinear_interp(di/h, dj/w, rDy(i,j),rDy(i, nj), rDy(ni, j), rDy(ni, nj));
			float nx = j + dj + deltaX * amount;
			float ny = i + di + deltaY * amount;

			nx = clamp(nx, 0.0F, static_cast<float>(src_size.width - 1));
			ny = clamp(ny, 0.0F, static_cast<float>(src_size.height - 1));
   
			int x0 = int(nx), y0 = int(ny);
			int x1 = std::ceil(nx), y1 = std::ceil(ny);
			
			switch(src.channels())
			{
			case 1:
				dst.at<uchar>(i + di, j + dj) = bilinear_interp(ny - y0, nx - x0,
						src.at<uchar>(y0, x0), src.at<uchar>(y0, x1),
						src.at<uchar>(y1, x0), src.at<uchar>(y1, x1));
				break;
			case 3:
				for(int b=0; b<3; ++b)
					dst.at<Vec3b>(i + di, j + dj)[b] = bilinear_interp(ny - y0, nx - x0,
						src.at<Vec3b>(y0, x0)[b], src.at<Vec3b>(y0, x1)[b],
						src.at<Vec3b>(y1, x0)[b], src.at<Vec3b>(y1, x1)[b]);
				break;
			case 4:
			{
				for(int b=0; b<4; ++b)
					dst.at<Vec4b>(i + di, j + dj)[b] = bilinear_interp(ny - y0, nx - x0,
						src.at<Vec4b>(y0, x0)[b], src.at<Vec4b>(y0, x1)[b],
						src.at<Vec4b>(y1, x0)[b], src.at<Vec4b>(y1, x1)[b]);
			}
				break;
			default:
				assert(false);
				break;
			}
		}
	}
	return dst;
}

void ImageWarp::setMappingPoints(const std::vector<cv::Point2f>& dst_points, const std::vector<cv::Point2f>& src_points)
{
	assert(src_points.size() == dst_points.size());

	this->src_points = dst_points;
	this->dst_points = src_points;
}

ImageWarp_Rigid::ImageWarp_Rigid()
{
	prescale = false;
}

static float calculateArea(const std::vector<cv::Point2f>& points)
{
	Vec4f box = venus::boundingBox(points);
//	Vec4f(left, top, right, bottom);
	return (box[2] - box[0])*(box[3] - box[1]);
}

void ImageWarp_Rigid::calculateDelta(float alpha)
{
	const int N = static_cast<int>(src_points.size());

	float ratio;
	if(prescale)
	{
		// TODO use cv::contourArea(), the area is computed using the Green formula.
		float src_area = calculateArea(src_points);
		float dst_area = calculateArea(dst_points);
		ratio = std::sqrt(dst_area / src_area);

		for(int i = 0; i < N; ++i)
			dst_points[i] /= ratio;
	}

	std::vector<float> w(N);

	rDx.create(dst_size);
	rDy.create(dst_size);

	if(N < 2)
	{
		rDx.setTo(0);
		rDy.setTo(0);
		return;
	}

	for(int i = 0; ; i += grid_size)
	{
		if(i >= dst_size.width && i < dst_size.width + grid_size - 1)
			i = dst_size.width - 1;
		else if(i >= dst_size.width)
			break;

		for(int j = 0; ; j += grid_size)
		{
			if (j >= dst_size.height && j < dst_size.height + grid_size - 1)
				j = dst_size.height - 1;
			else if (j >= dst_size.height)
				break;

			float sw = 0;
			Point2f swp(0, 0), swq(0, 0), newP(0, 0), curV(i, j);

			int k;
			for(k = 0; k < N; ++k)
			{
				if((i == src_points[k].x) && j == src_points[k].y)
					break;

//				float denorm = distance(Point2f(i, j), src_points[k]);
				float denorm = ((i - src_points[k].x)*(i - src_points[k].x) +
								(j - src_points[k].y)*(j - src_points[k].y));
				if(alpha == 1.0F)
					w[k] = 1.0F / denorm;
				else
					w[k] = std::pow(denorm, -alpha);

				sw  = sw  + w[k];
				swp = swp + w[k] * src_points[k];
				swq = swq + w[k] * dst_points[k];
			}

			if(k == N)
			{
				Point2f pstar = swp / sw;
				Point2f qstar = swq / sw;

				// Calc miu_r
				// miu_s = 0;
				float s1 = 0, s2 = 0;
				for(k = 0; k < N; ++k)
				{
					if (i == src_points[k].x && j == src_points[k].y)
						continue;

					Point2f Pi = src_points[k] - pstar;
					Point2f PiJ;
					PiJ.x = -Pi.y, PiJ.y = Pi.x;
					Point2f Qi = dst_points[k] - qstar;
					s1 += w[k] * Qi.dot(Pi);
					s2 += w[k] * Qi.dot(PiJ);
				}

				float miu_r = std::sqrt(s1*s1 + s2*s2);
				curV -= pstar;
				Point2f curVJ;
				curVJ.x = -curV.y, curVJ.y = curV.x;

				for(k = 0; k < N; ++k)
				{
					if(i==src_points[k].x && j==src_points[k].y)
						continue;

					Point2f Pi = src_points[k] - pstar;
					Point2f PiJ;
					PiJ.x = -Pi.y, PiJ.y = Pi.x;

					Point2f tmpP;
					tmpP.x = Pi.dot(curV) * dst_points[k].x - PiJ.dot(curV) * dst_points[k].y;
					tmpP.y = -Pi.dot(curVJ) * dst_points[k].x + PiJ.dot(curVJ) * dst_points[k].y;
					tmpP *= w[k]/miu_r;
					newP += tmpP;
				}
				newP += qstar;
			}
			else
			{
				newP = dst_points[k];
			}

			if(prescale)
			{
				rDx(j, i) = newP.x * ratio - i;
				rDy(j, i) = newP.y * ratio - j;
			}
			else
			{
				rDx(j, i) = newP.x - i;
				rDy(j, i) = newP.y - j;
			}
		}
	}
	
	if(prescale)
	{
		for(int i = 0; i < N; ++i)
			dst_points[i] *= ratio;
	}
}

void ImageWarp_Similarity::calculateDelta(float alpha)
{
	const int N = static_cast<int>(src_points.size());
	std::vector<float> w(N);

	rDx.create(dst_size);
	rDy.create(dst_size);

	if (N < 2)
	{
		rDx.setTo(0);
		rDy.setTo(0);
		return;
	}

	for(int i = 0; ; i += grid_size)
	{
		if(i >= dst_size.width && i < dst_size.width + grid_size - 1)
			i = dst_size.width - 1;
		else if(i >= dst_size.width)
			break;

		for(int j = 0; ; j += grid_size)
		{
			if(j >= dst_size.height && j < dst_size.height + grid_size - 1)
				j = dst_size.height - 1;
			else if(j >= dst_size.height)
				break;

			float sw = 0;
			Point2f swp(0, 0), swq(0, 0), newP(0, 0), curV(i, j);

			int k;
			for(k = 0; k < N; ++k)
			{
				if(i == src_points[k].x && j==src_points[k].y)
					break;

				float denorm = (i - src_points[k].x)*(i - src_points[k].x)+
				               (j - src_points[k].y)*(j - src_points[k].y);
				// w[k] = std::pow(denorm, -alpha);
				w[k] = 1 / denorm;
				sw  = sw  + w[k];
				swp = swp + w[k] * src_points[k];
				swq = swq + w[k] * dst_points[k];
			}

			if(k == N)
			{
				Point2f pstar = swp / sw;
				Point2f qstar = swq / sw;

				// Calc miu_s
				float miu_s = 0;
				for(k = 0; k < N; ++k)
				{
					if(i == src_points[k].x && j == src_points[k].y)
						continue;

					Point2f Pi = src_points[k] - pstar;
					miu_s += w[k] * Pi.dot(Pi);
				}

				curV -= pstar;
				Point2f curVJ;
				curVJ.x = -curV.y;
				curVJ.y = curV.x;

				for(k = 0; k < N; ++k)
				{
					if(i == src_points[k].x && j == src_points[k].y)
						continue;

					Point2f Pi = src_points[k] - pstar;
					Point2f PiJ, tmpP;
					PiJ.x = -Pi.y;
					PiJ.y = Pi.x;

					tmpP.x = Pi.dot(curV) * dst_points[k].x - PiJ.dot(curV) * dst_points[k].y;
					tmpP.y = -Pi.dot(curVJ) * dst_points[k].x + PiJ.dot(curVJ) * dst_points[k].y;
					tmpP *= w[k]/miu_s;
					newP += tmpP;
				}
				newP += qstar;
			}
			else
			{
				newP = dst_points[k];
			}

			rDx(j, i) = newP.x - i;
			rDy(j, i) = newP.y - j;
		}
	}
}
#if 0
ImageWarp_PiecewiseAffine::ImageWarp_PiecewiseAffine():
	fill_mood(BackgroundFillMode::NONE)
{
}

ImageWarp_PiecewiseAffine::~ImageWarp_PiecewiseAffine()
{
}

cv::Point2f ImageWarp_PiecewiseAffine::getMLSDelta(int x, int y)
{
	const int N = static_cast<int>(src_points.size());

	std::vector<float> w;
	w.resize(N);

	static Point2f  curV;
	float miu_s;

	int i = x;
	int j = y;
	int k;

	float sw = 0;
	Point2f swp(0, 0), swq(0, 0), newP(0, 0), curV(i, j);
	for(k = 0; k < N; k++)
	{
		if ((i==src_points[k].x) && j==src_points[k].y)
			break;

		float denorm = (i - src_points[k].x)*(i - src_points[k].x)+
		               (j - src_points[k].y)*(j - src_points[k].y);
		// w[k] = std::pow(denorm, -alpha);
		w[k] = 1 / denorm;

		sw  = sw  + w[k];
		swp += w[k] * src_points[k];
		swq += w[k] * dst_points[k];
	}

	if(k == N)
	{
		Point2f pstar = swp / sw;
		Point2f qstar = swq / sw;

		// Calc miu_s
		miu_s = 0;
		for(k = 0; k < N; k++)
		{
			if (i==src_points[k].x && j==src_points[k].y)
				continue;

			Point2f Pi = src_points[k] - pstar;
			miu_s += w[k] * Pi.dot(Pi);
		}

		curV -= pstar;
		Point2f curVJ;
		curVJ.x = -curV.y, curVJ.y = curV.x;

		for (k = 0; k < N; k++)
		{
			if(i == src_points[k].x && j == src_points[k].y)
				continue;

			Point2f Pi = src_points[k] - pstar;
			Point2f PiJ, tmpP;
			PiJ.x = -Pi.y, PiJ.y = Pi.x;

			tmpP.x = Pi.dot(curV) * dst_points[k].x
					 - PiJ.dot(curV) * dst_points[k].y;
			tmpP.y = -Pi.dot(curVJ) * dst_points[k].x
					 + PiJ.dot(curVJ) * dst_points[k].y;
			tmpP *= w[k] / miu_s;
			newP += tmpP;
		}
		newP += qstar;
	}
	else
	{
		newP = dst_points[k];
	}
	
	newP.x -= i;
	newP.y -= j;
	return newP;
}

struct Triangle
{
	Point_< int > v[3];
};

void ImageWarp_PiecewiseAffine::calculateDelta(float alpha)
{
	const int N = static_cast<int>(src_points.size());

	Mat_<int> imgLabel = Mat_<int>::zeros(dst_size.height, dst_size.width);

	rDx = Mat_<float>::zeros(dst_size);
	rDy = Mat_<float>::zeros(dst_size);
	for(int i = 0; i < N; ++i)
	{
		Point2f& point = src_points[i];
		//! Ignore points outside the target image
		point.x = clamp<float>(point.x, 0, dst_size.width - 1);
		point.y = clamp<float>(point.y, 0, dst_size.height - 1);
		
		rDx(point) = dst_points[i].x - point.x;
		rDy(point) = dst_points[i].y - point.y;
	}

	rDx(0, 0) = rDy(0, 0) = 0;
	rDx(dst_size.height - 1, 0) = rDy(0, dst_size.width-1) = 0;
	rDy(dst_size.height - 1, 0) = rDy(dst_size.height-1, dst_size.width-1) = src_size.height - dst_size.height;
	rDx(0, dst_size.width - 1)  = rDx(dst_size.height-1, dst_size.width-1) = src_size.width - dst_size.width;
	
	std::vector<Triangle>::iterator it;
	cv::Rect2i boundRect(0, 0, dst_size.width, dst_size.height);
	std::vector<Triangle> oL1 = src_points;
	if(fill_mood == BackgroundFillMode::PIECEWISE)
	{
		oL1.push_back(Point2f(0,0));
		oL1.push_back(Point2f(0,dst_size.height-1));
		oL1.push_back(Point2f(dst_size.width-1, 0));
		oL1.push_back(Point2f(dst_size.width-1,dst_size.height-1));
	}
	// In order preserv the background
	std::vector<Point2i> V = delaunayDiv(oL1, boundRect);
	
	Mat_<uint8_t> imgTmp = Mat_<uint8_t>::zeros(dst_size.height, dst_size.width);
	for(it = V.begin(); it != V.end(); ++it)
	{
		cv::line(imgTmp, it->v[0], it->v[1], 255, 1, CV_AA);
		cv::line(imgTmp, it->v[0], it->v[2], 255, 1, CV_AA);
		cv::line(imgTmp, it->v[2], it->v[1], 255, 1, CV_AA);
		
		// Not interested in points outside the region.
		if(!(it->v[0].inside(boundRect) && it->v[1].inside(boundRect) && it->v[2].inside(boundRect)))
			continue;
		
		cv::fillConvexPoly(imgLabel, it->v, 3, cv::Scalar_<int>(it-V.begin()+1));
	}

	for(int i = 0; ; i += grid_size)
	{
		if(i >= dst_size.width && i < dst_size.width + grid_size - 1)
			i = dst_size.width - 1;
		else if(i >= dst_size.width)
			break;

		for(int j = 0; ; j += grid_size)
		{
			if(j >= dst_size.height && j < dst_size.height + grid_size - 1)
				j = dst_size.height - 1;
			else if(j >= dst_size.height)
				break;

			int id = imgLabel(j, i) - 1;
			if(id < 0)
			{
				if (fill_mood == BackgroundFillMode::MLS)
				{
					Point_<float> dV = getMLSDelta(i, j);
					rDx(j, i) = dV.x;
					rDy(j, i) = dV.y;
				}
				else
				{
					rDx(j, i) = -i;
					rDy(j, i) = -j;
				}
				continue;
			}
			Point2i v1 = V[id].v[1] - V[id].v[0];
			Point2i v2 = V[id].v[2] - V[id].v[0];
			Point2i curV(i, j);
			curV -= V[id].v[0];

			float d2 = float(v1.x * curV.y - curV.x * v1.y)/(v1.x*v2.y-v2.x*v1.y);
			float d1 = float(v2.x * curV.y - curV.x * v2.y)/(v2.x*v1.y-v1.x*v2.y);
			float d0 = 1-d1-d2;
			rDx(j, i) = d0*rDx(V[id].v[0]) + d1*rDx(V[id].v[1]) + d2*rDx(V[id].v[2]);
			rDy(j, i) = d0*rDy(V[id].v[0]) + d1*rDy(V[id].v[1]) + d2*rDy(V[id].v[2]);
		}
	}
}
#endif
} /* namespace venus */