package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;

/**
 * Adjusts the shadows and highlights of an image
 * shadows: Increase to lighten shadows, from 0.0 to 1.0, with 0.0 as the default.
 * highlights: Decrease to darken highlights, from 0.0 to 1.0, with 1.0 as the default.
 */
public class GPUImageHighlightShadowFilter extends GPUImageFilter
{
	private static final String HIGHLIGHT_SHADOW_FRAGMENT_SHADER =
			"uniform sampler2D inputImageTexture;\n" +
			"varying highp vec2 textureCoordinate;\n" +
			" \n" +
			"uniform lowp float shadows;\n" +
			"uniform lowp float highlights;\n" +
			"\n" +
			"const mediump vec3 luminanceWeighting = vec3(0.3, 0.3, 0.3);\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	lowp vec4 source = texture2D(inputImageTexture, textureCoordinate);\n" +
			"	mediump float luminance = dot(source.rgb, luminanceWeighting);\n" +
			"\n" +
			"	mediump float shadow = clamp((pow(luminance, 1.0/(shadows+1.0)) + (-0.76)*pow(luminance, 2.0/(shadows+1.0))) - luminance, 0.0, 1.0);\n" +
			"	mediump float highlight = clamp((1.0 - (pow(1.0-luminance, 1.0/(2.0-highlights)) + (-0.8)*pow(1.0-luminance, 2.0/(2.0-highlights)))) - luminance, -1.0, 0.0);\n" +
			"	lowp vec3 result = vec3(0.0, 0.0, 0.0) + ((luminance + shadow + highlight) - 0.0) * ((source.rgb - vec3(0.0, 0.0, 0.0))/(luminance - 0.0));\n" +
			"\n" +
			"	gl_FragColor = vec4(result.rgb, source.a);\n" +
			"}";

	private int mShadowsLocation;
	private float mShadows;
	private int mHighlightsLocation;
	private float mHighlights;

	public GPUImageHighlightShadowFilter()
	{
		this(0.0F, 1.0F);
	}

	public GPUImageHighlightShadowFilter(final float shadows, final float highlights)
	{
		super(GPUImageFilterType.HIGHLIGHT_SHADOW, NO_FILTER_VERTEX_SHADER, HIGHLIGHT_SHADOW_FRAGMENT_SHADER);
		mHighlights = highlights;
		mShadows = shadows;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mHighlightsLocation = GLES20.glGetUniformLocation(getProgram(), "highlights");
		mShadowsLocation = GLES20.glGetUniformLocation(getProgram(), "shadows");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setHighlights(mHighlights);
		setShadows(mShadows);
	}

	public void setHighlights(final float highlights)
	{
		mHighlights = highlights;
		setFloat(mHighlightsLocation, mHighlights);
	}

	public void setShadows(final float shadows)
	{
		mShadows = shadows;
		setFloat(mShadowsLocation, mShadows);
	}
}
