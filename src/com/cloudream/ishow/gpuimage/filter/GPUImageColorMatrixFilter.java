package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImage;
import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;

/**
 * Applies a ColorMatrix to the image.
 */
public class GPUImageColorMatrixFilter extends GPUImageFilter
{
	private static final String COLOR_MATRIX_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"\n" +
			"uniform lowp mat4 colorMatrix;\n" +
			"uniform lowp float intensity;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n" +
			"	lowp vec4 outputColor = textureColor * colorMatrix;\n" +
			"	\n" +
			"	gl_FragColor = (intensity * outputColor) + ((1.0 - intensity) * textureColor);\n" +
			"}";

	private float mIntensity;
	private float[] mColorMatrix;
	private int mColorMatrixLocation;
	private int mIntensityLocation;

	public GPUImageColorMatrixFilter()
	{
		this(1.0F, new float[]
		{
			1.0F, 0.0F, 0.0F, 0.0F,
			0.0F, 1.0F, 0.0F, 0.0F,
			0.0F, 0.0F, 1.0F, 0.0F,
			0.0F, 0.0F, 0.0F, 1.0F,
		});
	}

	public GPUImageColorMatrixFilter(final float intensity, final float[] colorMatrix)
	{
		super(NO_FILTER_VERTEX_SHADER, COLOR_MATRIX_FRAGMENT_SHADER);
		mIntensity = intensity;
		mColorMatrix = colorMatrix;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mColorMatrixLocation = GLES20.glGetUniformLocation(getProgram(), "colorMatrix");
		mIntensityLocation = GLES20.glGetUniformLocation(getProgram(), "intensity");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setIntensity(mIntensity);
		setColorMatrix(mColorMatrix);
	}

	public void setIntensity(final float intensity)
	{
		mIntensity = intensity;
		setFloat(mIntensityLocation, intensity);
	}

	public void setColorMatrix(final float[] colorMatrix)
	{
		mColorMatrix = colorMatrix;
		setUniformMatrix4f(mColorMatrixLocation, colorMatrix);
	}
}
