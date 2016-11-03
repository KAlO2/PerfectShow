#include "venus/ImageWarp.h"
#include "venus/opencv_utility.h"
#include "venus/scalar.h"
#include <opencv2/imgproc.hpp>

using namespace cv;

namespace venus {

ImageWarp::ImageWarp()
{
    gridSize = 5;
}

inline float bilinear_interp(float x, float y, float v11, float v12, float v21, float v22)
{
    return (v11*(1-y) + v12*y) * (1-x) + (v21*(1-y) + v22*y) * x;
}

Mat ImageWarp::setAllAndGenerate(const Mat & src,
		const std::vector<Point2f> &src_points,
		const std::vector<Point2f> &dst_points,
		const int outW, const int outH, 
		const float transRatio)
{
	setSize(src.cols, src.rows);
	setTargetSize(outW, outH);
	setMappingPoints(dst_points, src_points);
	calcDelta();
	return genNewImage(src, transRatio);
}

Mat ImageWarp::genNewImage(const Mat& src, float transRatio)
{
    Mat dst(tarH, tarW, src.type());
    for(int i = 0; i < tarH; i += gridSize)
    for(int j = 0; j < tarW; j += gridSize)
	{
        int ni = i+gridSize;
		int nj = j+gridSize;
        float w = gridSize, h = gridSize;
        if (ni >= tarH) ni = tarH-1, h = ni-i+1;
        if (nj >= tarW) nj = tarW-1, w = nj-j+1;

        for(int di=0; di<h; ++di)
        for(int dj=0; dj<w; ++dj)
		{
            float deltaX = bilinear_interp(di/h, dj/w, rDx(i,j),rDx(i, nj), rDx(ni, j), rDx(ni, nj));
            float deltaY = bilinear_interp(di/h, dj/w, rDy(i,j),rDy(i, nj), rDy(ni, j), rDy(ni, nj));
            float nx = j + dj + deltaX * transRatio;
            float ny = i + di + deltaY * transRatio;

			nx = clamp<float>(nx, 0, srcW - 1);
			ny = clamp<float>(ny, 0, srcH - 1);
   
            int nxi = int(nx);
            int nyi = int(ny);
            int nxi1 = ceil(nx);
            int nyi1 = ceil(ny);
            
			switch(src.channels())
			{
			case 1:
				dst.at<uchar>(i+di, j+dj) =
                    bilinear_interp(ny-nyi, nx-nxi,
                            src.at<uchar>(nyi, nxi), src.at<uchar>(nyi, nxi1),
                            src.at<uchar>(nyi1, nxi), src.at<uchar>(nyi1, nxi1));
				break;
			case 3:
				for (int b=0; b<3; ++b)
                    dst.at<Vec3b>(i+di, j+dj)[b] =
                        bilinear_interp(ny-nyi, nx-nxi,
                            src.at<Vec3b>(nyi, nxi)[b], src.at<Vec3b>(nyi, nxi1)[b],
                            src.at<Vec3b>(nyi1, nxi)[b], src.at<Vec3b>(nyi1, nxi1)[b]);
				break;

			case 4:
				for (int b=0; b<4; ++b)
                    dst.at<Vec4b>(i+di, j+dj)[b] =
                        bilinear_interp(ny-nyi, nx-nxi,
                            src.at<Vec4b>(nyi, nxi)[b],  src.at<Vec4b>(nyi, nxi1)[b],
                            src.at<Vec4b>(nyi1, nxi)[b], src.at<Vec4b>(nyi1, nxi1)[b]);
				break;
			default:
				assert(false);
				break;
			}
		}
	}
    return dst;
}

// Set source points and prepare transformation matrices
void ImageWarp::setMappingPoints(const std::vector<cv::Point2f>& dst_points, const std::vector<cv::Point2f>& src_points)
{
	nPoint = dst_points.size();
	oldDotL = dst_points;
	newDotL = src_points;
}

ImageWarp_Rigid::ImageWarp_Rigid()
{
    preScale = false;
}

float calculateArea(const std::vector<cv::Point2f>& points)
{
	Vec4f box = venus::boundingBox(points);
//	Vec4f(left, top, right, bottom);
	return (box[2] - box[0])*(box[3] - box[1]);
}

void ImageWarp_Rigid::calcDelta()
{
    float ratio;

    if (preScale)
	{
        ratio = std::sqrt(calculateArea(newDotL) / calculateArea(oldDotL));
        for(int i = 0; i< nPoint; i++)
            newDotL[i] *= 1/ratio;
    }

	std::vector<float> w(nPoint);

    rDx.create(tarH, tarW);
    rDy.create(tarH, tarW);

	if (nPoint < 2)
	{
		rDx.setTo(0);
		rDy.setTo(0);
		return;
	}
    Point2f curV, curVJ, PiJ, Qi;

    for(int i = 0; ; i += gridSize)
	{
        if (i>=tarW && i<tarW+gridSize - 1)
            i=tarW-1;
        else if (i>=tarW)
            break;
        for (int j = 0; ; j+=gridSize){
            if (j>=tarH && j<tarH+gridSize - 1)
                j = tarH - 1;
            else if (j>=tarH)
                break;
            float sw = 0;
			Point2f swp(0, 0), swq(0, 0);

            Point2f newP(0, 0);
            curV.x = i;
            curV.y = j;

			int k;
            for (k = 0; k < nPoint; k++){
                if ((i==oldDotL[k].x) && j==oldDotL[k].y)
                    break;
                if (alpha==1)
                    w[k] = 1/((i-oldDotL[k].x)*(i-oldDotL[k].x)+
                        (j-oldDotL[k].y)*(j-oldDotL[k].y));
                else
                    w[k] = pow((i-oldDotL[k].x)*(i-oldDotL[k].x)+
                        (j-oldDotL[k].y)*(j-oldDotL[k].y), -alpha);
                sw = sw + w[k];
                swp = swp + w[k] * oldDotL[k];
                swq = swq + w[k] * newDotL[k];
            }
            if ( k == nPoint ) {
                Point2f pstar = (1 / sw) * swp ;
                Point2f qstar = 1/sw * swq;
    //            qDebug("pstar: (%f, %f)", pstar[0], pstar[1]);

                // Calc miu_r
                //miu_s = 0;
                float s1=0, s2=0;
                for (k = 0; k < nPoint; k++){
                    if (i==oldDotL[k].x && j==oldDotL[k].y)
                        continue;

                    Point2f Pi = oldDotL[k] - pstar;
                    PiJ.x = -Pi.y, PiJ.y = Pi.x;
                    Qi = newDotL[k] - qstar;
                    s1 += w[k] * Qi.dot(Pi);
                    s2 += w[k] * Qi.dot(PiJ);
                }
                float miu_r = sqrt(s1*s1 + s2*s2);

                curV -= pstar;
                curVJ.x = -curV.y, curVJ.y = curV.x;

                for (k = 0; k < nPoint; k++){
                    if (i==oldDotL[k].x && j==oldDotL[k].y)
                        continue;

                    Point2f Pi = oldDotL[k] - pstar;
                    PiJ.x = -Pi.y, PiJ.y = Pi.x;

					Point2f tmpP;
                    tmpP.x = Pi.dot(curV) * newDotL[k].x
                             - PiJ.dot(curV) * newDotL[k].y;
                    tmpP.y = -Pi.dot(curVJ) * newDotL[k].x
                             + PiJ.dot(curVJ) * newDotL[k].y;
                    tmpP *= w[k]/miu_r;
                    newP += tmpP;
                }
                newP += qstar;
            }
            else {
                newP = newDotL[k];
            }

            if (preScale){
                rDx(j, i) = newP.x * ratio - i;
                rDy(j, i) = newP.y * ratio - j;
            }
            else {
                rDx(j, i) = newP.x - i;
                rDy(j, i) = newP.y - j;
            }
        }
    }
    
    if(preScale)
	{
        for (int i = 0; i < nPoint; i++)
            newDotL[i] *= ratio;
    }
}


void ImgWarp_MLS_Similarity::calcDelta()
{
    Point2f swq, qstar, newP, tmpP;
    float sw;

	std::vector<float> w(nPoint);

    rDx.create(tarH, tarW);
    rDy.create(tarH, tarW);

	if (nPoint < 2){
		rDx.setTo(0);
		rDy.setTo(0);
		return;
	}

    Point2f swp, pstar, curV, curVJ, Pi, PiJ;
    float miu_s;

    for(int i = 0; ; i+=gridSize)
	{
        if (i>=tarW && i<tarW+gridSize - 1)
            i=tarW-1;
        else if (i>=tarW)
            break;
        for(int j = 0; ; j+=gridSize){
            if (j>=tarH && j<tarH+gridSize - 1)
                j = tarH - 1;
            else if (j>=tarH)
                break;
            sw = 0;
            swp.x = swp.y = 0;
            swq.x = swq.y = 0;
            newP.x = newP.y = 0;
            curV.x = i;
            curV.y = j;
			int k;
            for(k = 0; k < nPoint; k++){
                if ((i==oldDotL[k].x) && j==oldDotL[k].y)
                    break;
               /* w[k] = pow((i-oldDotL[k].x)*(i-oldDotL[k].x)+
                        (j-oldDotL[k].y)*(j-oldDotL[k].y), -alpha);*/
                w[k] = 1/((i-oldDotL[k].x)*(i-oldDotL[k].x)+
                        (j-oldDotL[k].y)*(j-oldDotL[k].y));
                sw = sw + w[k];
                swp = swp + w[k] * oldDotL[k];
                swq = swq + w[k] * newDotL[k];
            }
            if ( k == nPoint ) {
                pstar = (1 / sw) * swp ;
                qstar = 1/sw * swq;
    //            qDebug("pstar: (%f, %f)", pstar[0], pstar[1]);

                // Calc miu_s
                miu_s = 0;
                for (k = 0; k < nPoint; k++){
                    if (i==oldDotL[k].x && j==oldDotL[k].y)
                        continue;

                    Pi = oldDotL[k] - pstar;
                    miu_s += w[k] * Pi.dot(Pi);
                }

                curV -= pstar;
                curVJ.x = -curV.y, curVJ.y = curV.x;

                for (k = 0; k < nPoint; k++){
                    if (i==oldDotL[k].x && j==oldDotL[k].y)
                        continue;

                    Pi = oldDotL[k] - pstar;
                    PiJ.x = -Pi.y, PiJ.y = Pi.x;

                    tmpP.x = Pi.dot(curV) * newDotL[k].x
                             - PiJ.dot(curV) * newDotL[k].y;
                    tmpP.y = -Pi.dot(curVJ) * newDotL[k].x
                             + PiJ.dot(curVJ) * newDotL[k].y;
                    tmpP *= w[k]/miu_s;
                    newP += tmpP;
                }
                newP += qstar;
            }
            else {
                newP = newDotL[k];
            }

            rDx(j, i) = newP.x - i;
            rDy(j, i) = newP.y - j;
        }
    }
}

#if 0
ImageWarp_PiecewiseAffine::ImageWarp_PiecewiseAffine(void)
{
    backGroundFillAlg = BGNone;
}

ImageWarp_PiecewiseAffine::~ImageWarp_PiecewiseAffine(void)
{
}

Point2f ImageWarp_PiecewiseAffine::getMLSDelta(int x, int y) {
    static Point2f swq, qstar, newP, tmpP;
    float sw;

    static std::vector<float> w;
    w.resize(nPoint);

    static Point2f swp, pstar, curV, curVJ, Pi, PiJ;
    float miu_s;

    int i = x;
    int j = y;
    int k;

    sw = 0;
    swp.x = swp.y = 0;
    swq.x = swq.y = 0;
    newP.x = newP.y = 0;
    curV.x = i;
    curV.y = j;
    for (k = 0; k < nPoint; k++) {
        if ((i==oldDotL[k].x) && j==oldDotL[k].y)
            break;
        /* w[k] = pow((i-oldDotL[k].x)*(i-oldDotL[k].x)+
                 (j-oldDotL[k].y)*(j-oldDotL[k].y), -alpha);*/
        w[k] = 1/((i-oldDotL[k].x)*(i-oldDotL[k].x)+
                  (j-oldDotL[k].y)*(j-oldDotL[k].y));
        sw = sw + w[k];
        swp = swp + w[k] * oldDotL[k];
        swq = swq + w[k] * newDotL[k];
    }
    if ( k == nPoint ) {
        pstar = (1 / sw) * swp ;
        qstar = 1/sw * swq;
        //            qDebug("pstar: (%f, %f)", pstar[0], pstar[1]);

        // Calc miu_s
        miu_s = 0;
        for (k = 0; k < nPoint; k++) {
            if (i==oldDotL[k].x && j==oldDotL[k].y)
                continue;

            Pi = oldDotL[k] - pstar;
            miu_s += w[k] * Pi.dot(Pi);
        }

        curV -= pstar;
        curVJ.x = -curV.y, curVJ.y = curV.x;

        for (k = 0; k < nPoint; k++) {
            if (i==oldDotL[k].x && j==oldDotL[k].y)
                continue;

            Pi = oldDotL[k] - pstar;
            PiJ.x = -Pi.y, PiJ.y = Pi.x;

            tmpP.x = Pi.dot(curV) * newDotL[k].x
                     - PiJ.dot(curV) * newDotL[k].y;
            tmpP.y = -Pi.dot(curVJ) * newDotL[k].x
                     + PiJ.dot(curVJ) * newDotL[k].y;
            tmpP *= w[k]/miu_s;
            newP += tmpP;
        }
        newP += qstar;
    }
    else {
        newP = newDotL[k];
    }
    
    newP.x -= i;
    newP.y -= j;
    return newP;
}

struct Triangle
{
	Point_< int > v[3];
};

void ImageWarp_PiecewiseAffine::calcDelta(){
	Mat_< int > imgLabel = Mat_< int >::zeros(tarH, tarW);

	rDx = rDx.zeros(tarH, tarW);
	rDy = rDy.zeros(tarH, tarW);
	for (int i=0;i<this->nPoint;i++){
		//! Ignore points outside the target image
        if (oldDotL[i].x<0)
            oldDotL[i].x = 0;
        if (oldDotL[i].y<0)
            oldDotL[i].y = 0;
        if (oldDotL[i].x >= tarW)
            oldDotL[i].x = tarW - 1;
        if (oldDotL[i].y >= tarH)
            oldDotL[i].y = tarH - 1;
		
		rDx(oldDotL[i]) = newDotL[i].x-oldDotL[i].x;
		rDy(oldDotL[i]) = newDotL[i].y-oldDotL[i].y;
	}
	rDx(0, 0) = rDy(0, 0) = 0;
    rDx(tarH-1, 0) = rDy(0, tarW-1) = 0;
    rDy(tarH-1, 0) = rDy(tarH-1, tarW-1) = srcH-tarH;
    rDx(0, tarW-1) = rDx(tarH-1, tarW-1) = srcW-tarW;


	
	std::vector<Triangle>::iterator it;
	cv::Rect2i boundRect(0, 0, tarW, tarH);
    std::vector<Triangle> oL1 = oldDotL;
    if (backGroundFillAlg == BGPiecewise)
	{
        oL1.push_back(Point2f(0,0));
        oL1.push_back(Point2f(0,tarH-1));
        oL1.push_back(Point2f(tarW-1, 0));
        oL1.push_back(Point2f(tarW-1,tarH-1));
    }
    // In order preserv the background
	std::vector<Point2i> V = delaunayDiv(oL1, boundRect);
    
    Mat_<uchar> imgTmp = Mat_<uchar>::zeros(tarH, tarW);
	for (it=V.begin(); it!=V.end(); it++){
        cv::line(imgTmp, it->v[0], it->v[1], 255, 1, CV_AA);
        cv::line(imgTmp, it->v[0], it->v[2], 255, 1, CV_AA);
        cv::line(imgTmp, it->v[2], it->v[1], 255, 1, CV_AA);
        
		// Not interested in points outside the region.
		if (!(it->v[0].inside(boundRect) && it->v[1].inside(boundRect) && it->v[2].inside(boundRect)))
			continue;
			
		cv::fillConvexPoly(imgLabel, it->v, 3, cv::Scalar_<int>(it-V.begin()+1));
	}

    Point2i v1, v2, curV;

    for (int i = 0; ; i+=gridSize){
        if (i>=tarW && i<tarW+gridSize - 1)
            i=tarW-1;
        else if (i>=tarW)
            break;
        for (int j = 0; ; j+=gridSize){
            if (j>=tarH && j<tarH+gridSize - 1)
                j = tarH - 1;
            else if (j>=tarH)
                break;
			int tId = imgLabel(j, i) - 1;
			if (tId<0){
                if (backGroundFillAlg == BGMLS){
                    Point_<float> dV = getMLSDelta(i, j);
                    rDx(j, i) = dV.x;
                    rDy(j, i) = dV.y;
                }
                else{
                    rDx(j, i) = -i;
                    rDy(j, i) = -j;
                }
				continue;
			}
			v1 = V[tId].v[1] - V[tId].v[0];
			v2 = V[tId].v[2] - V[tId].v[0];
			curV.x = i, curV.y = j;
			curV -= V[tId].v[0];

			float d0, d1, d2;
			d2 = float(v1.x * curV.y - curV.x * v1.y)/(v1.x*v2.y-v2.x*v1.y);
			d1 = float(v2.x * curV.y - curV.x * v2.y)/(v2.x*v1.y-v1.x*v2.y);
			//d1=d2=0;
			d0 = 1-d1-d2;
			rDx(j, i) = d0*rDx(V[tId].v[0]) + d1*rDx(V[tId].v[1]) + d2*rDx(V[tId].v[2]);
			rDy(j, i) = d0*rDy(V[tId].v[0]) + d1*rDy(V[tId].v[1]) + d2*rDy(V[tId].v[2]);
        }
    }

}
#endif

} /* namespace venus */