package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;

/**
 * Converts the image to a single-color version, based on the luminance of each pixel
 * intensity: The degree to which the specific color replaces the normal image color (0.0 - 1.0, with 1.0 as the default)
 * color: The color to use as the basis for the effect, with (0.6, 0.45, 0.3, 1.0) as the default.
 */
public class GPUImageMonochromeFilter extends GPUImageFilter
{
	private static final String MONOCHROME_FRAGMENT_SHADER =
			"precision lowp float;\n" +
			"\n" +
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"uniform float intensity;\n" +
			"uniform vec3 filterColor;\n" +
			"\n" +
			"const mediump vec3 luminanceWeighting = vec3(0.2125, 0.7154, 0.0721);\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			" 	//desat, then apply overlay blend\n" +
			" 	lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n" +
			" 	float luminance = dot(textureColor.rgb, luminanceWeighting);\n" +
			" 	\n" +
			" 	lowp vec4 desat = vec4(vec3(luminance), 1.0);\n" +
			" 	\n" +
			" 	//overlay\n" +
			" 	lowp vec4 outputColor = vec4(\n" +
			"		(desat.r < 0.5 ? (2.0 * desat.r * filterColor.r) : (1.0 - 2.0 * (1.0 - desat.r) * (1.0 - filterColor.r))),\n" +
			"		(desat.g < 0.5 ? (2.0 * desat.g * filterColor.g) : (1.0 - 2.0 * (1.0 - desat.g) * (1.0 - filterColor.g))),\n" +
			"		(desat.b < 0.5 ? (2.0 * desat.b * filterColor.b) : (1.0 - 2.0 * (1.0 - desat.b) * (1.0 - filterColor.b))),\n" +
			"		1.0);\n" +
			"	\n" +
			"	//which is better, or are they equal?\n" +
			"	gl_FragColor = vec4( mix(textureColor.rgb, outputColor.rgb, intensity), textureColor.a);\n" +
			"}";

	private int mIntensityLocation;
	private float mIntensity;
	private int mFilterColorLocation;
	private float[] mColor;

	public GPUImageMonochromeFilter()
	{
		this(1.0f, new float[]{ 0.6F, 0.45F, 0.3F, 1.0F });
	}

	public GPUImageMonochromeFilter(final float intensity, final float[] color)
	{
		super(GPUImageFilterType.MONOCHROME, NO_FILTER_VERTEX_SHADER, MONOCHROME_FRAGMENT_SHADER);
		mIntensity = intensity;
		mColor = color;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mIntensityLocation = GLES20.glGetUniformLocation(getProgram(), "intensity");
		mFilterColorLocation = GLES20.glGetUniformLocation(getProgram(), "filterColor");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setIntensity(1.0f);
		setColor(new float[]{ 0.6F, 0.45F, 0.3F, 1.0F });
	}

	public void setIntensity(final float intensity)
	{
		mIntensity = intensity;
		setFloat(mIntensityLocation, mIntensity);
	}

	public void setColor(final float[] color)
	{
		mColor = color;
		setColorRed(mColor[0], mColor[1], mColor[2]);
	}

	public void setColorRed(final float red, final float green, final float blue)
	{
		setFloatVec3(mFilterColorLocation, new float[]{ red, green, blue });
	}
}
