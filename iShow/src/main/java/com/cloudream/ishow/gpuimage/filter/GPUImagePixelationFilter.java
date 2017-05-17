package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;
/**
 * Applies a grayscale effect to the image.
 */
public class GPUImagePixelationFilter extends GPUImageFilter
{
	private static final String PIXELATION_FRAGMENT_SHADER =
			"precision highp float;\n" +
			
			"varying vec2 textureCoordinate;\n" +
			
			"uniform float imageWidthFactor;\n" +
			"uniform float imageHeightFactor;\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"uniform float pixel;\n" +
			
			"void main()\n" +
			"{\n" +
			"  vec2 uv  = textureCoordinate.xy;\n" +
			"  float dx = pixel * imageWidthFactor;\n" +
			"  float dy = pixel * imageHeightFactor;\n" +
			"  vec2 coord = vec2(dx * floor(uv.x / dx), dy * floor(uv.y / dy));\n" +
			"  vec3 tc = texture2D(inputImageTexture, coord).xyz;\n" +
			"  gl_FragColor = vec4(tc, 1.0);\n" +
			"}";

	private int mImageWidthFactorLocation;
	private int mImageHeightFactorLocation;
	private float mPixel;
	private int mPixelLocation;

	public GPUImagePixelationFilter()
	{
		super(GPUImageFilterType.PIXELATION, NO_FILTER_VERTEX_SHADER, PIXELATION_FRAGMENT_SHADER);
		mPixel = 1.0F;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mImageWidthFactorLocation = GLES20.glGetUniformLocation(getProgram(), "imageWidthFactor");
		mImageHeightFactorLocation = GLES20.glGetUniformLocation(getProgram(), "imageHeightFactor");
		mPixelLocation = GLES20.glGetUniformLocation(getProgram(), "pixel");
		setPixel(mPixel);
	}

	@Override
	public void onOutputSizeChanged(final int width, final int height)
	{
		super.onOutputSizeChanged(width, height);
		setFloat(mImageWidthFactorLocation, 1.0f / width);
		setFloat(mImageHeightFactorLocation, 1.0f / height);
	}

	public void setPixel(final float pixel)
	{
		mPixel = pixel;
		setFloat(mPixelLocation, mPixel);
	}
}
