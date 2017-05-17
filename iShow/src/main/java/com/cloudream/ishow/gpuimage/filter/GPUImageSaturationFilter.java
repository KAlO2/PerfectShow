package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;

/**
 * saturation: The degree of saturation or desaturation to apply to the image (0.0 - 2.0, with 1.0 as the default)
 */
public class GPUImageSaturationFilter extends GPUImageFilter
{
	private static final String SATURATION_FRAGMENT_SHADER = "" +
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"uniform lowp float saturation;\n" +
			"\n" +
			"// Values from \"Graphics Shaders: Theory and Practice\" by Bailey and Cunningham\n" +
			"const mediump vec3 luminanceWeighting = vec3(0.2125, 0.7154, 0.0721);\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	owp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n" +
			"	lowp float luminance = dot(textureColor.rgb, luminanceWeighting);\n" +
			"	lowp vec3 greyScaleColor = vec3(luminance);\n" +
			"	\n" +
			"	gl_FragColor = vec4(mix(greyScaleColor, textureColor.rgb, saturation), textureColor.w);\n" +
			"	\n" +
			"}";

	private int mSaturationLocation;
	private float mSaturation;

	public GPUImageSaturationFilter()
	{
		this(1.0f);
	}

	public GPUImageSaturationFilter(final float saturation)
	{
		super(GPUImageFilterType.SATURATION, NO_FILTER_VERTEX_SHADER, SATURATION_FRAGMENT_SHADER);
		mSaturation = saturation;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mSaturationLocation = GLES20.glGetUniformLocation(getProgram(), "saturation");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setSaturation(mSaturation);
	}

	public void setSaturation(final float saturation)
	{
		mSaturation = saturation;
		setFloat(mSaturationLocation, mSaturation);
	}
}
