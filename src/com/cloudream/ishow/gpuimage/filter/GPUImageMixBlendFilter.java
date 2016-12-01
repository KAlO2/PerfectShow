package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterType;
import com.cloudream.ishow.gpuimage.GPUImageTwoInputFilter;

import android.opengl.GLES20;

public class GPUImageMixBlendFilter extends GPUImageTwoInputFilter
{
	private int mMixLocation;
	private float mMix;

	public GPUImageMixBlendFilter(GPUImageFilterType type, String fragmentShader)
	{
		this(type, fragmentShader, 0.5F);
	}

	public GPUImageMixBlendFilter(GPUImageFilterType type, String fragmentShader, float mix)
	{
		super(type, fragmentShader);
		mMix = mix;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mMixLocation = GLES20.glGetUniformLocation(getProgram(), "mixturePercent");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setMix(mMix);
	}

	/**
	 * @param mix ranges from 0.0 (only image 1) to 1.0 (only image 2), with 0.5 (half of either) as
	 *        the normal level
	 */
	public void setMix(final float mix)
	{
		mMix = mix;
		setFloat(mMixLocation, mMix);
	}
}
