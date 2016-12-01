package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;

/**
 * exposure: The adjusted exposure (-10.0 - 10.0, with 0.0 as the default)
 */
public class GPUImageExposureFilter extends GPUImageFilter
{
	private static final String EXPOSURE_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"uniform highp float exposure;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	highp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n" +
			"	\n" +
			"	gl_FragColor = vec4(textureColor.rgb * pow(2.0, exposure), textureColor.w);\n" +
			"} ";

	private int mExposureLocation;
	private float mExposure;

	public GPUImageExposureFilter()
	{
		this(1.0F);
	}

	public GPUImageExposureFilter(final float exposure)
	{
		super(GPUImageFilterType.EXPOSURE, NO_FILTER_VERTEX_SHADER, EXPOSURE_FRAGMENT_SHADER);
		mExposure = exposure;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mExposureLocation = GLES20.glGetUniformLocation(getProgram(), "exposure");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setExposure(mExposure);
	}

	public void setExposure(final float exposure)
	{
		mExposure = exposure;
		setFloat(mExposureLocation, mExposure);
	}
}
