#ifndef VENUS_INPAINT_H_
#define VENUS_INPAINT_H_

#include <opencv2/core.hpp>

namespace venus {

/**
	Find candidate positions for template matching.

	Implementation is based on 
	"Speed-up Template Matching through Integral Image based Weak Classifiers", Tirui Wu et. al.

	This method subdivides the template into a set of blocks. For each block the mean (of each
	channel) is calculated and compared to the mean of the entire template. A binary decision is
	based on whether the block mean is bigger than the template mean. Such a decision is made for
	each block.

	Then for each template position in the source image, the same procedure as described above is
	applied (replace "template" with "region of image") and the binary decisions are compared. If
	too many descisions mismatch, the current position is said not to be a candidate.

	What makes the method fast is the use of integral images for calculating means. Using integral
	images reduces the calculated of the mean to two additions, two subtractions and a division, 
	independent of the size of the rectangle to calculate the mean for.

	Changes made by the author with respect to the original paper:
		- The algorithm works with 1 or 3 channel images. If multiple image channels are present
			each channel is treated seperately.

		- A mask can be passed with the template. The mask defines the valid areas within the 
			template. Only those areas are considered during classification. A block is rejected
			from the decision process if not all its pixels are masked. If no mask is passed, all
			blocks are considered valid.
*/
class TemplateMatchCandidates
{
private:
	cv::Mat _image;
    std::vector<cv::Mat1i> _integrals;
    std::vector<cv::Rect>  _blocks;
    cv::Size _templateSize;
    cv::Size _partitionSize;

private:

    /** Subdivides a size into a rectangle of blocks. */
    void computeBlockRects(cv::Size size, cv::Size partitions, std::vector<cv::Rect> &rects);

    /** Reject blocks depending on the template mask. */
    void removeInvalidBlocks(const cv::Mat &templMask, std::vector<cv::Rect> &rects);

    /** Calculate the weak classifiers for the template, taking the mask into account. */
    void weakClassifiersForTemplate(const cv::Mat &templ, const cv::Mat &templMask, const std::vector<cv::Rect> &rects, cv::Mat1i &classifiers, cv::Scalar &mean);

    /** Compare the template classifiers to the classifiers generated from the given template position. */
    unsigned char compareWeakClassifiers(const cv::Mat1i &i, int x, int y, const cv::Size &templSize, const std::vector<cv::Rect> &blocks, const int *compareTo, float templateMean, float maxMeanDiff, int maxWeakErrors);

public:
	/** Set the source image. */
	void setSourceImage(const cv::Mat &image);
        
	/** Set the template size. */
	inline void setTemplateSize(const cv::Size &templateSize) { _templateSize = templateSize; }

	/** Set the partition size. Specifies the number of blocks in x and y direction. */
	inline void setPartitionSize(const cv::Size& size) { _partitionSize = size; }

	/** Initialize candidate search. */
	void initialize();

	/** 
	 * Find candidates.
	 * 
	 * @param templ Template image.
	 * @param templMask Optional template mask.
	 * @param candidates Computed candidates mask.
	 * @param maxWeakErrors Max classification mismatches per channel.
	 * @param maxMeanDifference Max difference of patch / template mean before rejecting a candidate.
	 * @return Candidate mask.
	 */
	void findCandidates(const cv::Mat &templ, const cv::Mat &templMask, cv::Mat &candidates, int maxWeakErrors = 3, float maxMeanDifference = 20);
};


/** 
	Implementation of the exemplar based inpainting algorithm described in
	"Object Removal by Exemplar-Based Inpainting", A. Criminisi et. al. 
            
	Changes made by the author with respect to the original paper:
		- the template match error is calculated based on larger patch sizes than those
			used to infill. The reason behind this is to compare a larger portion of source
			and target regions and thus to avoid visual artefacts.
      
		- the search for the best matching spot of the patch position to be inpainted
			is accelerated by TemplateMatchCandidates.

	Please note edge cases (i.e regions on the image border) are crudely handled by simply 
	discarding them.

*/
class Inpainter
{
private:
	struct UserSpecified
	{
        cv::Mat image;
        cv::Mat sourceMask;
        cv::Mat targetMask;
        int patchSize;

        UserSpecified():
			patchSize(9)
		{
		}
    };

    UserSpecified _input;
	
    TemplateMatchCandidates _tmc;
	cv::Mat _image, _candidates;
	cv::Mat1b _targetRegion, _borderRegion, _sourceRegion;
	cv::Mat1f _isophoteX, _isophoteY, _confidence, _borderGradX, _borderGradY;
	int _halfPatchSize, _halfMatchSize;
	int _startX, _startY, _endX, _endY;

private:

	/** Updates the fill-front which is the border between filled and unfilled regions. */
	void updateFillFront();

	/** Find patch on fill front with highest priortiy. This will be the patch to be inpainted in this step. */
	cv::Point findTargetPatchLocation();

	/** For a given patch to inpaint, search for the best matching source patch to use for inpainting. */
	cv::Point findSourcePatchLocation(const cv::Point& targetPatchLocation, bool useCandidateFilter);

	/** Calculate the confidence for the given patch location. */
	float confidenceForPatchLocation(const cv::Point& point) const;
	
	/** Given that we know the source and target patch, propagate associated values from the source into the target region. */
	void propagatePatch(const cv::Point& target, const cv::Point& source);

public:

	/** Empty constructor */
	Inpainter() = default;
	
	/** Set the image to be inpainted. */
	inline void setSourceImage(const cv::Mat &bgrImage) { _input.image = bgrImage; }

    /** Set the mask that describes the region inpainting can copy from. */
	inline void setSourceMask(const cv::Mat &mask) { _input.sourceMask = mask; }

	/** Set the mask that describes the region to be inpainted. */
	inline void setTargetMask(const cv::Mat &mask) { _input.targetMask = mask; }

	/** Set the patch size. */
	inline void setPatchSize(int size) { _input.patchSize = size; }

	/** Initialize inpainting. */
	void initialize();

	/** True if there are more steps to perform. */
	bool hasMoreSteps() const;

	/** Perform a single step (i.e fill one patch) and return the updated information. */
	void step();

	/** Access the current state of the inpainted image. */
	const cv::Mat& image() const { return _image; }

    /** Access the current state of the target region. */
	const cv::Mat& targetRegion() const { return _targetRegion; }
};

} /* namespace venus */
#endif /* VENUS_INPAINT_H_ */