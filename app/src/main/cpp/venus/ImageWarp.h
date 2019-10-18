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

protected:
    int grid_size; ///< Parameter for MLS.

    std::vector<cv::Point2f> src_points;
	std::vector<cv::Point2f> dst_points;

    cv::Mat_<float> rDx, rDy;

	cv::Size2i src_size;
	cv::Size2i dst_size;

public:
	ImageWarp();
    ImageWarp(int grid_size);
	virtual ~ImageWarp(){}

	/**
	 * Set all and generate an output.
	 * @param src        The input image to be warped.
	 * @param src_points A list of "from" points.
	 * @param dst_points A list of "target" points.
	 * @param target     The output image size.
	 * @param amount     1 means warp to target points, 0 means no warping.
	 *
	 * This will do all the initialization and generate a warped image. After calling this, one can later call 
	 * genNewImage with different transRatios to generate a warping animation.
	 */
	cv::Mat setAllAndGenerate(const cv::Mat& src,
			const std::vector<cv::Point2f> &src_points, const std::vector<cv::Point2f> &dst_points,
			const cv::Size2i& target, float alpha, float amount = 1.0F);

	/**
	 * Generate the warped image.
	 * This function generate a warped image using PRE-CALCULATED data.
	 * DO NOT CALL THIS AT FIRST! Call this after at least one call of setAllAndGenerate.
	 */
    cv::Mat genNewImage(const cv::Mat& src, float transRatio);

	/**
	 * Calculate delta value which will be used for generating the warped image.
	 */
    virtual void calculateDelta(float alpha) = 0;



	/**
	 * @param[in] dst_points Set the list of target points
	 * @param[in] src_points Set the list of source points
	 */
	void setMappingPoints(const std::vector<cv::Point2f>& dst_points, const std::vector<cv::Point2f>& src_points);
//	void setMappingPoints(const std::vector<cv::Point2f>&& dst_points, const std::vector<cv::Point2f>&& src_points);

	void setSourceSize(int width, int height) { src_size = cv::Size2i(width, height); }
	void setTargetSize(int width, int height) { dst_size = cv::Size2i(width, height); }

	void setSourceSize(const cv::Size2i& size) { src_size = size; }
	void setTargetSize(const cv::Size2i& size) { dst_size = size; }
};

/**
 * The class for MLS Rigid transform.
 * It will try to keep the image rigid. You can set preScale if you
 * can accept uniform transform.
 */
class ImageWarp_Rigid : public ImageWarp
{
private:
	bool prescale;  ///< Whether unify scaling the points before deformation

public:    
    ImageWarp_Rigid();
    virtual void calculateDelta(float alpha) override;

	void set(bool prescale);
};


//! The class for MLS Similarity transform.
class ImageWarp_Similarity: public ImageWarp
{
public:
	virtual void calculateDelta(float alpha) override;
};
#if 0
class ImageWarp_PiecewiseAffine: public ImageWarp
{
public:
	/**
	 * How to deal with the background.
	 */
	enum class BackgroundFillMode
	{
		NONE,      ///< No background is reserved.
		MLS,       ///< Use MLS to deal with the background.
		PIECEWISE, ///< Use the same scheme for the background.
	};

private:
	BackgroundFillMode fill_mood;

	cv::Point2f getMLSDelta(int x, int y);


public:
	ImageWarp_PiecewiseAffine();
	~ImageWarp_PiecewiseAffine();

	virtual void calculateDelta(float alpha) override;
};
#endif

} /* namespace venus */
#endif /* VENUS_IMAGE_WARP_H_ */
