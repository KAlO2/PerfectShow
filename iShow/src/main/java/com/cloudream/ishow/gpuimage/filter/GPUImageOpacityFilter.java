package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;

import android.opengl.GLES20;

/**
 * Adjusts the alpha channel of the incoming image
 * opacity: The value to multiply the incoming alpha channel for each pixel by (0.0 - 1.0, with 1.0 as the default)
*/
public class GPUImageOpacityFilter extends GPUImageFilter
{
	private static final String OPACITY_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"uniform lowp float opacity;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n" +
			"	\n" +
			"	gl_FragColor = vec4(textureColor.rgb, textureColor.a * opacity);\n" +
			"}\n";

	private int mOpacityLocation;
	private float mOpacity;

	public GPUImageOpacityFilter()
	{
		this(1.0F);
	}

	public GPUImageOpacityFilter(final float opacity)
	{
		super(NO_FILTER_VERTEX_SHADER, OPACITY_FRAGMENT_SHADER);
		mOpacity = opacity;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mOpacityLocation = GLES20.glGetUniformLocation(getProgram(), "opacity");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setOpacity(mOpacity);
	}

	public void setOpacity(final float opacity)
	{
		mOpacity = opacity;
		setFloat(mOpacityLocation, mOpacity);
	}
}
