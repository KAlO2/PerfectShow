package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterType;
import com.cloudream.ishow.gpuimage.GPUImageTwoInputFilter;

public class GPUImageLightenBlendFilter extends GPUImageTwoInputFilter
{
	private static final String LIGHTEN_BLEND_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"varying highp vec2 textureCoordinate2;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"uniform sampler2D inputImageTexture2;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n" +
			"	lowp vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinate2);\n" +
			"	\n" +
			"	gl_FragColor = max(textureColor, textureColor2);\n" +
			"}";

	public GPUImageLightenBlendFilter()
	{
		super(GPUImageFilterType.BLEND_LIGHTEN, LIGHTEN_BLEND_FRAGMENT_SHADER);
	}
}
