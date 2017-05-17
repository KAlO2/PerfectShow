package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterType;

/**
 * Applies an emboss effect to the image.<br>
 * <br>
 * Intensity ranges from 0.0 to 4.0, with 1.0 as the normal level
 */
public class GPUImageEmbossFilter extends GPUImage3x3ConvolutionFilter
{
	private float mIntensity;

	public GPUImageEmbossFilter()
	{
		this(1.0F);
	}

	public GPUImageEmbossFilter(final float intensity)
	{
		super();
		setFilterType(GPUImageFilterType.EMBOSS);
		mIntensity = intensity;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		setIntensity(mIntensity);
	}

	public void setIntensity(final float intensity)
	{
		mIntensity = intensity;
		setConvolutionKernel(new float[]
		{
			intensity * (-2.0F), -intensity, 0.0F,
			-intensity, 1.0F, intensity, 0.0F,
			intensity, intensity * 2.0F,
		});
	}

	public float getIntensity()
	{
		return mIntensity;
	}
}
