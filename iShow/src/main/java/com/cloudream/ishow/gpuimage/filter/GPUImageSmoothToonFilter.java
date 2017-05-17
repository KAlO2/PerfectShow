package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterGroup;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;

/**
 *  This uses a similar process as the GPUImageToonFilter, only it precedes the toon effect
 *  with a Gaussian blur to smooth out noise.
 */
public class GPUImageSmoothToonFilter extends GPUImageFilterGroup
{
	GPUImageGaussianBlurFilter blurFilter;
	GPUImageToonFilter toonFilter;

	/**
	 * Setup and Tear down
	 */
	public GPUImageSmoothToonFilter()
	{
		// First pass: apply a variable Gaussian blur
		blurFilter = new GPUImageGaussianBlurFilter();
		addFilter(blurFilter);

		// Second pass: run the Sobel edge detection on this blurred image, along with a
		// posterization effect
		toonFilter = new GPUImageToonFilter();
		addFilter(toonFilter);

		getFilters().add(blurFilter);

		setBlurSize(0.5f);
		setThreshold(0.2f);
		setQuantizationLevels(10.0f);
		setFilterType(GPUImageFilterType.SMOOTH_TOON);
	}

	/**
	 * Accessors
	 */
	public void setTexelWidth(float value)
	{
		toonFilter.setTexelWidth(value);
	}

	public void setTexelHeight(float value)
	{
		toonFilter.setTexelHeight(value);
	}

	public void setBlurSize(float value)
	{
		blurFilter.setBlurSize(value);
	}

	public void setThreshold(float value)
	{
		toonFilter.setThreshold(value);
	}

	public void setQuantizationLevels(float value)
	{
		toonFilter.setQuantizationLevels(value);
	}

}
