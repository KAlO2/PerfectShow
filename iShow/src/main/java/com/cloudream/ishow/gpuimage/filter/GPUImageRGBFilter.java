package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;

/**
 * Adjusts the individual RGB channels of an image
 * red: Normalized values by which each color channel is multiplied. The range is from 0.0 up, with 1.0 as the default.
 * green:
 * blue:
 */
public class GPUImageRGBFilter extends GPUImageFilter
{
	private static final String RGB_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"uniform highp float red;\n" +
			"uniform highp float green;\n" +
			"uniform highp float blue;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	highp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n" +
			"	\n" +
			"	gl_FragColor = vec4(textureColor.r * red, textureColor.g * green, textureColor.b * blue, 1.0);\n" +
			"}\n";

	private int mRedLocation;
	private float mRed;
	private int mGreenLocation;
	private float mGreen;
	private int mBlueLocation;
	private float mBlue;
	private boolean mIsInitialized = false;

	public GPUImageRGBFilter()
	{
		this(1.0F, 1.0F, 1.0F);
	}

	public GPUImageRGBFilter(final float red, final float green, final float blue)
	{
		super(GPUImageFilterType.RGB, NO_FILTER_VERTEX_SHADER, RGB_FRAGMENT_SHADER);
		mRed = red;
		mGreen = green;
		mBlue = blue;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mRedLocation = GLES20.glGetUniformLocation(getProgram(), "red");
		mGreenLocation = GLES20.glGetUniformLocation(getProgram(), "green");
		mBlueLocation = GLES20.glGetUniformLocation(getProgram(), "blue");
		mIsInitialized = true;
		setRed(mRed);
		setGreen(mGreen);
		setBlue(mBlue);
	}

	public void setRed(final float red)
	{
		mRed = red;
		if(mIsInitialized)
		{
			setFloat(mRedLocation, mRed);
		}
	}

	public void setGreen(final float green)
	{
		mGreen = green;
		if(mIsInitialized)
		{
			setFloat(mGreenLocation, mGreen);
		}
	}

	public void setBlue(final float blue)
	{
		mBlue = blue;
		if(mIsInitialized)
		{
			setFloat(mBlueLocation, mBlue);
		}
	}
}
