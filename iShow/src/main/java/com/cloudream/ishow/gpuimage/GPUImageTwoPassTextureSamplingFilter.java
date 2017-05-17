package com.cloudream.ishow.gpuimage;

import android.opengl.GLES20;

public class GPUImageTwoPassTextureSamplingFilter extends GPUImageTwoPassFilter
{
	public GPUImageTwoPassTextureSamplingFilter(
			String firstVertexShader, String firstFragmentShader,
			String secondVertexShader, String secondFragmentShader)
	{
		super(firstVertexShader, firstFragmentShader,
				secondVertexShader, secondFragmentShader);
	}

	@Override
	public void onInit()
	{
		super.onInit();
		initTexelOffsets();
	}

	protected void initTexelOffsets()
	{
		float ratio = getHorizontalTexelOffsetRatio();
		GPUImageFilter filter = mFilters.get(0);
		int texelWidthOffsetLocation = GLES20.glGetUniformLocation(filter.getProgram(), "texelWidthOffset");
		int texelHeightOffsetLocation = GLES20.glGetUniformLocation(filter.getProgram(), "texelHeightOffset");
		filter.setFloat(texelWidthOffsetLocation, ratio / mOutputWidth);
		filter.setFloat(texelHeightOffsetLocation, 0);

		ratio = getVerticalTexelOffsetRatio();
		filter = mFilters.get(1);
		texelWidthOffsetLocation = GLES20.glGetUniformLocation(filter.getProgram(), "texelWidthOffset");
		texelHeightOffsetLocation = GLES20.glGetUniformLocation(filter.getProgram(), "texelHeightOffset");
		filter.setFloat(texelWidthOffsetLocation, 0);
		filter.setFloat(texelHeightOffsetLocation, ratio / mOutputHeight);
	}

	@Override
	public void onOutputSizeChanged(int width, int height)
	{
		super.onOutputSizeChanged(width, height);
		initTexelOffsets();
	}

	public float getVerticalTexelOffsetRatio()
	{
		return 1.0F;
	}

	public float getHorizontalTexelOffsetRatio()
	{
		return 1.0F;
	}
}
