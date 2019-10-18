#include <opencv2/imgproc.hpp>

#include "venus/compiler.h"
#include "venus/inpaint.h"
#include "venus/scalar.h"

namespace venus {

void TemplateMatchCandidates::setSourceImage(const cv::Mat &image)
{
	CV_Assert((image.channels() == 1 || image.channels() == 3) && (image.depth() == CV_8U));
	_image = image;
}

void TemplateMatchCandidates::initialize()
{
	std::vector<cv::Mat1b> imageChannels;
	cv::split(_image, imageChannels);
	const size_t nChannels = imageChannels.size();

	_integrals.resize(nChannels);
	for(size_t i = 0; i < nChannels; ++i)
		cv::integral(imageChannels[i], _integrals[i]);
	
	_blocks.clear();
	computeBlockRects(_templateSize, _partitionSize, _blocks);
}

void TemplateMatchCandidates::findCandidates(const cv::Mat &templ, const cv::Mat &templMask, cv::Mat &candidates,
		int maxWeakErrors, float maxMeanDifference)
{
	CV_Assert(templ.type() == CV_MAKETYPE(CV_8U, static_cast<int>(_integrals.size())) && templ.size() == _templateSize && 
			(templMask.empty() || templMask.size() == _templateSize));

	candidates.create(
			_image.size().height - templ.size().height + 1, 
			_image.size().width - templ.size().width + 1,
			CV_8UC1);
	candidates.setTo(255);

	std::vector<cv::Rect> blocks = _blocks;
	removeInvalidBlocks(templMask, blocks);

	cv::Mat1i referenceClass;
	cv::Scalar templMean;
	weakClassifiersForTemplate(templ, templMask, blocks, referenceClass, templMean);
	
	// For each channel we loop over all possible template positions and compare with classifiers.
	for(size_t i = 0; i < _integrals.size(); ++i) 
	{
		cv::Mat1i &integral = _integrals[i];
		const int *referenceClassRow = referenceClass.ptr<int>(static_cast<int>(i));

		// For all template positions ty, tx (top-left template position)
		for(int ty = 0; ty < candidates.rows; ++ty) 
		{
			uchar *outputRow = candidates.ptr<uchar>(ty);
			for(int tx = 0; tx < candidates.cols; ++tx) 
			{
				if(!outputRow[tx])
					continue;
				
				outputRow[tx] = compareWeakClassifiers(integral, tx, ty, templ.size(), blocks, referenceClassRow, 
						(float)templMean[static_cast<int>(i)], maxMeanDifference, maxWeakErrors);
			}
		}
	}
}

void TemplateMatchCandidates::weakClassifiersForTemplate(const cv::Mat &templ, const cv::Mat &templMask, const std::vector<cv::Rect> &rects, 
		cv::Mat1i &classifiers, cv::Scalar &mean)
{
	const int nChannels = templ.channels();
	const int size = static_cast<int>(rects.size());
	classifiers.create(nChannels, size);

	// Note we use cv::mean here to make use of mask.
	mean = cv::mean(templ, templMask);

	for(int x = 0; x < size; ++x)
	{
		cv::Scalar blockMean = cv::mean(templ(rects[x]), templMask.empty() ? cv::noArray() : templMask(rects[x]));
		
		for(int y = 0; y < nChannels; ++y)
			classifiers(y, x) = blockMean[y] > mean[y] ? 1 : -1;
	}
}

unsigned char TemplateMatchCandidates::compareWeakClassifiers(const cv::Mat1i &i, int x, int y, const cv::Size &templSize, 
		const std::vector<cv::Rect> &blocks, const int *compareTo, float templateMean, float maxMeanDiff, int maxWeakErrors)
{
	const int *topRow = i.ptr<int>(y);
	const int *bottomRow = i.ptr<int>(y + templSize.height); // +1 required for integrals
	
	// Mean of image under given template position
	const float posMean = (bottomRow[x + templSize.width] - bottomRow[x] - topRow[x + templSize.width] + topRow[x]) / static_cast<float>(templSize.area());

	if(std::abs(posMean - templateMean) > maxMeanDiff)
		return 0;

	// Evaluate means of sub-blocks
	int sumErrors = 0;
	for(size_t r = 0; r < blocks.size(); ++r)
	{
		const cv::Rect &b = blocks[r];

		int ox = x + b.x;
		int oy = y + b.y;

		const int *topRow = i.ptr<int>(oy);
		const int *bottomRow = i.ptr<int>(oy + b.height);

		const float blockMean = (bottomRow[ox + b.width] - bottomRow[ox] - topRow[ox + b.width] + topRow[ox]) / static_cast<float>(b.width * b.height);
		const int c = blockMean > posMean ? 1 : -1;
		sumErrors += (c != compareTo[r]) ? 1 : 0;

		if(sumErrors > maxWeakErrors)
			return 0;
	}

	return 255;
}

void TemplateMatchCandidates::computeBlockRects(cv::Size size, cv::Size partitions, std::vector<cv::Rect> &rects)
{
	rects.clear();

	const int blockWidth = size.width / partitions.width;
	const int blockHeight = size.height / partitions.height;

	if(blockWidth == 0 || blockHeight == 0)
		rects.push_back(cv::Rect(0, 0, size.width, size.height));
	else
	{
		// Note: last row/column of blocks might be of different shape to fill up entire size.
		const int lastBlockWidth = size.width - blockWidth * (partitions.width - 1);
		const int lastBlockHeight = size.height - blockHeight * (partitions.height - 1);
		
		for(int y = 0; y < partitions.height; ++y)
		{
			bool lastY = (y == partitions.height - 1);
			for(int x = 0; x < partitions.width; ++x)
			{
				bool lastX = (x == partitions.width - 1);

				rects.push_back(cv::Rect(
						x * blockWidth, y * blockHeight, 
						lastX ? lastBlockWidth : blockWidth, 
						lastY ? lastBlockHeight : blockHeight));
			}
		}
	}
}

void TemplateMatchCandidates::removeInvalidBlocks(const cv::Mat &templMask, std::vector<cv::Rect> &rects) 
{
	if(templMask.empty())
		return;
	
	auto predicate = [&templMask](const cv::Rect &r) -> bool
	{
		cv::Mat block = templMask(r);
		return cv::countNonZero(block) != r.width * r.height;
	};

	rects.erase(std::remove_if(rects.begin(), rects.end(), predicate), rects.end());
}


/** Flags for creating patch. */
enum PatchFlags
{
	PATCH_FAST   = 0,       ///< No flags. Fastest variant.
	PATCH_BOUNDS = 1 << 0,  ///< Clamp patch to bounds of image.
	PATCH_REF    = 1 << 1,  ///< Reference parent memory. Slower, but keeps the parent memory alive.
};

/** 
 * Returns a patch anchored on the given top-left corner.
 * 
 * @param Flags   Combination of flags for patch creation.
 * @param m       Underlying image
 * @param y       y-coordinate of the patch top-left corner
 * @param x       x-coordinate of the patch top-left corner
 * @param height  height of patch (extension along y-axis)
 * @param width   width of patch (extension along x-axis)
 * @return a view on the image that contains only the patch region.
 */
template<int Flags>
cv::Mat topLeftPatch(const cv::Mat &m, int y, int x, int height, int width) 
{
	// Note, compile time if's, will be optimized away by compiler.
	if(Flags & PATCH_BOUNDS)
	{
		int topx = clamp<int>(x, 0, m.cols - 1);
		int topy = clamp<int>(y, 0, m.rows - 1);
		width  -= std::abs(topx - x);
		height -= std::abs(topy - y);

		width  = clamp(width, 0, m.cols - topx);
		height = clamp(height, 0, m.rows - topy);
		x = topx;
		y = topy;
	}

	if(Flags & PATCH_REF)
		return m(cv::Rect(x, y, width, height));
	else
	{
		uchar* data = const_cast<uchar*>(m.ptr<uchar>(y, x));
		return cv::Mat(height, width, m.type(), data, m.step);
	}
}

/** 
 * Returns a patch anchored on the given top-left corner.. 
 */
inline cv::Mat topLeftPatch(const cv::Mat &m, int y, int x, int height, int width) 
{
	return topLeftPatch<PATCH_FAST>(m, y, x, height, width);
}

/** 
 * Returns a patch anchored on the given top-left corner.. 
 */
inline cv::Mat topLeftPatch(const cv::Mat &m, const cv::Rect &r) 
{
	return topLeftPatch<PATCH_FAST>(m, r.y, r.x, r.height, r.width);
}
  
/** 
 * Returns a patch centered around the given pixel coordinates.
 * 
 * @param Flags          Combination of flags for patch creation.
 * @param m              Underlying image
 * @param y              y-coordinate of the patch center
 * @param x              x-coordinate of the patch center
 * @param halfPatchSize  Half the patch size. I.e for a 3x3 patch window, set this to 1.
 * @return a view on the image that contains only the patch region.
 */
template<int Flags>
cv::Mat centeredPatch(const cv::Mat &m, int y, int x, int halfPatchSize) 
{
	int width = 2 * halfPatchSize + 1;
	int height = 2 * halfPatchSize + 1;
	x -= halfPatchSize;
	y -= halfPatchSize;

	return topLeftPatch<Flags>(m, y, x, height, width);
}

/** 
 * Returns a patch centered around the given pixel coordinates. 
 */
inline cv::Mat centeredPatch(const cv::Mat &m, int y, int x, int halfPatchSize) 
{
	return centeredPatch<PATCH_FAST>(m, y, x, halfPatchSize);
}

/** 
 * Given two centered patches in two images compute the comparable region in both images as top-left patches. 
 * 
 * @param a             first image
 * @param b             second image
 * @param ap            center in first image
 * @param bp            center in second image
 * @param halfPatchSize halfPatchSize Half the patch size. I.e for a 3x3 patch window, set this to 1.
 * @return Comparable   rectangles for first, second image. Rectangles are of same size, but anchored top-left
 *                      with respect to the given center points.
 */
inline std::pair<cv::Rect, cv::Rect> comparablePatchRegions(const cv::Mat &a, const cv::Mat &b, cv::Point ap, cv::Point bp, int halfPatchSize)
{
	int left = std::max(std::max(-halfPatchSize, -ap.x), -bp.x);
	int right = std::min(std::min(halfPatchSize + 1, -ap.x + a.cols), -bp.x + b.cols); 
	int top = std::max(std::max(-halfPatchSize, -ap.y), -bp.y);
	int bottom = std::min(std::min(halfPatchSize + 1, -ap.y + a.rows), -bp.y + b.rows); 

	std::pair<cv::Rect, cv::Rect> p;

	p.first.x = ap.x + left;
	p.first.y = ap.y + top;
	p.first.width = (right - left);
	p.first.height = (bottom - top);

	p.second.x = bp.x + left;
	p.second.y = bp.y + top;
	p.second.width = (right - left);
	p.second.height = (bottom - top);

	return p;
}

/** Test if patch goes across the boundary. */
inline bool isCenteredPatchCrossingBoundary(cv::Point p, int halfPatchSize, const cv::Mat &img)
{
	return p.x < halfPatchSize || p.x >= img.cols - halfPatchSize ||
	       p.y < halfPatchSize || p.y >= img.rows - halfPatchSize;
}

constexpr int PATCHFLAGS = PATCH_BOUNDS;

void Inpainter::initialize()
{
	CV_Assert(_input.image.channels() == 3 && _input.image.depth() == CV_8U &&
			_input.targetMask.size() == _input.image.size() &&
			(_input.sourceMask.empty() || _input.targetMask.size() == _input.sourceMask.size()) &&
			_input.patchSize > 0);

	_halfPatchSize = _input.patchSize / 2;
	_halfMatchSize = (int) (_halfPatchSize * 1.25F);

	_input.image.copyTo(_image);
	_input.targetMask.copyTo(_targetRegion);

	// Initialize regions
	cv::rectangle(_targetRegion, cv::Rect(0, 0, _targetRegion.cols, _targetRegion.rows), cv::Scalar(0), _halfMatchSize);

	_sourceRegion = 255 - _targetRegion; 
	cv::rectangle(_sourceRegion, cv::Rect(0, 0, _sourceRegion.cols, _sourceRegion.rows), cv::Scalar(0), _halfMatchSize);
	cv::erode(_sourceRegion, _sourceRegion, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(_halfMatchSize*2+1, _halfMatchSize*2+1)));

	if(!_input.sourceMask.empty() && cv::countNonZero(_input.sourceMask) > 0)
		_sourceRegion.setTo(0, (_input.sourceMask == 0));

	// Initialize isophote values. Deviating from the original paper here. We've found that
	// blurring the image balances the data term and the confidence term better.
	cv::Mat blurred;
	cv::blur(_image, blurred, cv::Size(3,3));
	cv::Mat_<cv::Vec3f> gradX, gradY;
	cv::Sobel(blurred, gradX, CV_32F, 1, 0, 3, 1, 0, cv::BORDER_REPLICATE);
	cv::Sobel(blurred, gradY, CV_32F, 0, 1, 3, 1, 0, cv::BORDER_REPLICATE);

	_isophoteX.create(gradX.size());
	_isophoteY.create(gradY.size());

	for(int i = 0; i < gradX.rows * gradX.cols; ++i)
	{
		// Note the isophote corresponds to the gradient rotated by 90?
		const cv::Vec3f &vx = gradX(i);
		const cv::Vec3f &vy = gradY(i);
		
		float x = (vx[0] + vx[1] + vx[2]) / (3 * 255);
		float y = (vy[0] + vy[1] + vy[2]) / (3 * 255);

		std::swap(x, y);
		x *= -1;

		_isophoteX(i) = x;
		_isophoteY(i) = y;
	}

	// Initialize confidence values
	_confidence.create(_image.size());
	_confidence.setTo(1);
	_confidence.setTo(0, _targetRegion);

	// Configure valid image region considered during algorithm
	_startX = _halfMatchSize;
	_startY = _halfMatchSize;
	_endX = _image.cols - _halfMatchSize - 1;
	_endY = _image.rows - _halfMatchSize - 1;

	// Setup template match performance improvement
	_tmc.setSourceImage(_image);
	_tmc.setTemplateSize(cv::Size(_halfMatchSize * 2 + 1, _halfMatchSize * 2 + 1));
	_tmc.setPartitionSize(cv::Size(3,3));
	_tmc.initialize();
}

bool Inpainter::hasMoreSteps() const
{
	return cv::countNonZero(_targetRegion) > 0;
}

void Inpainter::step()
{	
	// We also need an updated knowledge of gradients in the border region
	updateFillFront();
	
	// Next, we need to select the best target patch on the boundary to be inpainted.
	cv::Point targetPatchLocation = findTargetPatchLocation();

	// Determine the best matching source patch from which to inpaint.
	cv::Point sourcePatchLocation = findSourcePatchLocation(targetPatchLocation, true);
	if(sourcePatchLocation.x == -1)
		sourcePatchLocation = findSourcePatchLocation(targetPatchLocation, false);

	// Copy values
	propagatePatch(targetPatchLocation, sourcePatchLocation);
}

void Inpainter::updateFillFront()
{
	// 2nd order derivative used to find border.
	cv::Laplacian(_targetRegion, _borderRegion, CV_8U, 3, 1, 0, cv::BORDER_REPLICATE);

	// Update confidence values along fill front.
	for(int y = _startY; y < _endY; ++y)
	{
		const uchar *bRow = _borderRegion.ptr(y);
		for(int x = _startX; x < _endX; ++x)
		{
			if(bRow[x] > 0)
			{
				// Update confidence for border item
				cv::Point p(x, y);
				_confidence(p) = confidenceForPatchLocation(p);
			}
		}
	}
}

cv::Point Inpainter::findTargetPatchLocation()
{
	// Sweep over all pixels in the border region and priorize them based on 
	// a confidence term (i.e how many pixels are already known) and a data term that prefers
	// border pixels on strong edges running through them.

	float maxPriority = 0;
	cv::Point bestLocation(0, 0);

	_borderGradX.create(_targetRegion.size());
	_borderGradY.create(_targetRegion.size());
	cv::Sobel(_targetRegion, _borderGradX, CV_32F, 1, 0, 3, 1, 0, cv::BORDER_REPLICATE);
	cv::Sobel(_targetRegion, _borderGradY, CV_32F, 0, 1, 3, 1, 0, cv::BORDER_REPLICATE);

	for(int y = _startY; y < _endY; ++y)
	{
		const uchar *bRow  = _borderRegion.ptr(y);
		const float *gxRow = _borderGradX.ptr<float>(y);
		const float *gyRow = _borderGradY.ptr<float>(y);
		const float *ixRow = _isophoteX.ptr<float>(y);
		const float *iyRow = _isophoteY.ptr<float>(y);
		const float *cRow  = _confidence.ptr<float>(y);

		for(int x = _startX; x < _endX; ++x)
		{
			if(bRow[x] > 0)
			{
				// Data term
				cv::Vec2f grad(gxRow[x], gyRow[x]);
				float dot = grad.dot(grad);

				if(dot == 0)
					grad *= 0;
				else
					grad /= std::sqrt(dot);
		
				const float d = std::abs(grad[0] * ixRow[x] + grad[1] * iyRow[x]) + 0.0001F;

				// Confidence term
				const float c = cRow[x];

				// Priority of patch
				const float prio = c * d;
				if(prio > maxPriority)
				{
					maxPriority = prio;
					bestLocation = cv::Point(x,y);
				}
			}
		}
	}

	return bestLocation;
}

float Inpainter::confidenceForPatchLocation(const cv::Point& point) const
{
	cv::Mat1f c = centeredPatch<PATCHFLAGS>(_confidence, point.y, point.x, _halfPatchSize);
	return (float)cv::sum(c)[0] / c.size().area();
}

cv::Point Inpainter::findSourcePatchLocation(const cv::Point& targetPatchLocation, bool useCandidateFilter)
{	
	cv::Point bestLocation(-1, -1);
	float bestError = std::numeric_limits<float>::max();

	cv::Mat3b targetImagePatch = centeredPatch<PATCHFLAGS>(_image, targetPatchLocation.y, targetPatchLocation.x, _halfMatchSize);
	cv::Mat1b targetMask = centeredPatch<PATCHFLAGS>(_targetRegion, targetPatchLocation.y, targetPatchLocation.x, _halfMatchSize);
	
	cv::Mat invTargetMask = (targetMask == 0);
	if(useCandidateFilter)
		_tmc.findCandidates(targetImagePatch, invTargetMask, _candidates, 3, 10);
	
	int count = 0;
	for(int y = _startY; y < _endY; ++y)
	for(int x = _startX; x < _endX; ++x)
	{
		// Note, candidates need to be corrected. Centered patch locations used here, top-left used with candidates.
		const bool shouldTest = (!useCandidateFilter || _candidates.at<uchar>(y - _halfMatchSize, x - _halfMatchSize)) &&
				_sourceRegion.at<uchar>(y, x) > 0;

		if(shouldTest)
		{
			++count;
			cv::Mat1b sourceMask = centeredPatch<PATCHFLAGS>(_sourceRegion, y, x, _halfMatchSize);
			cv::Mat3b sourceImagePatch = centeredPatch<PATCHFLAGS>(_image, y, x, _halfMatchSize);

			float error = (float)cv::norm(targetImagePatch, sourceImagePatch, cv::NORM_L1, invTargetMask);
			
			if(error < bestError)
			{
				bestError = error;
				bestLocation = cv::Point(x, y);
			}
		}
	}

	return bestLocation;
}

void Inpainter::propagatePatch(const cv::Point& target, const cv::Point& source)
{
	cv::Mat1b copyMask = centeredPatch<PATCHFLAGS>(_targetRegion, target.y, target.x, _halfPatchSize);

	centeredPatch<PATCHFLAGS>(_image, source.y, source.x, _halfPatchSize).copyTo(
	centeredPatch<PATCHFLAGS>(_image, target.y, target.x, _halfPatchSize), copyMask);

	centeredPatch<PATCHFLAGS>(_isophoteX, source.y, source.x, _halfPatchSize).copyTo(
	centeredPatch<PATCHFLAGS>(_isophoteX, target.y, target.x, _halfPatchSize), copyMask);		

	centeredPatch<PATCHFLAGS>(_isophoteY, source.y, source.x, _halfPatchSize).copyTo(
	centeredPatch<PATCHFLAGS>(_isophoteY, target.y, target.x, _halfPatchSize), copyMask);

	float cPatch = _confidence.at<float>(target);
	centeredPatch<PATCHFLAGS>(_confidence, target.y, target.x, _halfPatchSize).setTo(cPatch, copyMask);
	
	copyMask.setTo(0);
}

} /* namespace venus */