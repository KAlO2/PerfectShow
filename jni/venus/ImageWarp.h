#ifndef VENUS_IMAGE_WARP_H_
#define VENUS_IMAGE_WARP_H_

#include <vector>

#include <opencv2/core.hpp>

namespace venus {

/**
 * The base class for Moving Least Square image warping.
 * Choose one of the subclasses, the easiest interface to generate an output is to use setAllAndGenerate function.
 */
class ImageWarp
{
private:

public:
    ImageWarp();
	virtual ~ImageWarp(){}

	/**
	 * Set all and generate an output.
	 * @param src        the image to be warped.
	 * @param src_points A list of "from" points.
	 * @param dst_points A list of "target" points.
	 * @param outW       The width of the output image.
	 * @param outH       The height of the output image.
	 * @param transRatio 1 means warp to target points, 0 means no warping
	 *
	 * This will do all the initialization and generate a warped image. After calling this, one can later call 
	 * genNewImage with different transRatios to generate a warping animation.
	 */
	cv::Mat setAllAndGenerate(const cv::Mat& src,
			const std::vector<cv::Point2f> &src_points, const std::vector<cv::Point2f> &dst_points,
			const int outW, const int outH,
			const float transRatio = 1);

	/**
	 * Generate the warped image.
	 * This function generate a warped image using PRE-CALCULATED data.
	 * DO NOT CALL THIS AT FIRST! Call this after at least one call of
	 * setAllAndGenerate.
	 */
    cv::Mat genNewImage(const cv::Mat& src, float transRatio);

	/**
	 * Calculate delta value which will be used for generating the warped image.
	 */
    virtual void calcDelta() = 0;

    float alpha;  //< Parameter for MLS.
    int gridSize; //< Parameter for MLS.

	/**
	 * @param[in] dst_points Set the list of target points
	 * @param[in] src_points Set the list of source points
	 */
	void setMappingPoints(const std::vector<cv::Point2f>& dst_points, const std::vector<cv::Point2f>& src_points);

	//! The size of the original image. For precalculation.
 //   void setSize(int width, int height) { src.width = width; src.height = height; }

	//! The size of output image
//    void setTargetSize(const int width, const int height) { dst.width = width; dst.height = height; }

	void setSize(int w, int h){srcW=w, srcH=h;}
    void setTargetSize(const int outW, const int outH){
        tarW = outW;
        tarH = outH;
    }

protected:

    std::vector<cv::Point2f> oldDotL, newDotL;

    int nPoint;

    cv::Mat_<float> /*! \brief delta_x */rDx, /*! \brief delta_y */rDy;

//	Size2i src, dst;
    int srcW, srcH;
    int tarW, tarH;
};

/**
 * The class for MLS Rigid transform.
 * It will try to keep the image rigid. You can set preScale if you
 * can accept uniform transform.
 */
class ImageWarp_Rigid : public ImageWarp
{
public:
    bool preScale;  //< Whether do unify scale on the points before deformation
    
    ImageWarp_Rigid();
    void calcDelta();
};


//! The class for MLS Similarity transform.
class ImgWarp_MLS_Similarity : public ImageWarp
{
public:
    void calcDelta();
};

#if 0
class ImageWarp_PiecewiseAffine : public ImageWarp
{
public:
    //! How to deal with the background.
    /*!
        BGNone: No background is reserved.
        BGMLS: Use MLS to deal with the background.
        BGPieceWise: Use the same scheme for the background.
    */
    enum BGFill
	{
			BGNone, //! No background is reserved.
            BGMLS,  //! Use MLS to deal with the background.
			BGPiecewise //! Use the same scheme for the background.
    };
	ImageWarp_PiecewiseAffine();
	~ImageWarp_PiecewiseAffine();

    void calcDelta();
    BGFill backGroundFillAlg;
private:
    cv::Point2f getMLSDelta(int x, int y);
};
#endif


} /* namespace venus */
#endif /* VENUS_IMAGE_WARP_H_ */
