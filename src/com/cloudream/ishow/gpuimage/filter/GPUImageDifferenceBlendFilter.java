package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterType;
import com.cloudream.ishow.gpuimage.GPUImageTwoInputFilter;

public class GPUImageDifferenceBlendFilter extends GPUImageTwoInputFilter
{
	private static final String DIFFERENCE_BLEND_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"varying highp vec2 textureCoordinate2;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"uniform sampler2D inputImageTexture2;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	mediump vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);\n" +
			"	mediump vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinate2);\n" +
			"	gl_FragColor = vec4(abs(textureColor2.rgb - textureColor.rgb), textureColor.a);\n" +
			"}";

	public GPUImageDifferenceBlendFilter()
	{
		super(GPUImageFilterType.BLEND_DIFFERENCE, DIFFERENCE_BLEND_FRAGMENT_SHADER);
	}
}
