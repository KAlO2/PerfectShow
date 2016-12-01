package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterType;
import com.cloudream.ishow.gpuimage.GPUImageTwoInputFilter;

public class GPUImageSubtractBlendFilter extends GPUImageTwoInputFilter
{
	private static final String SUBTRACT_BLEND_FRAGMENT_SHADER =
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
			"\n" +
			"	gl_FragColor = vec4(textureColor.rgb - textureColor2.rgb, textureColor.a);\n" +
			"}";

	public GPUImageSubtractBlendFilter()
	{
		super(GPUImageFilterType.BLEND_SUBTRACT, SUBTRACT_BLEND_FRAGMENT_SHADER);
	}
}
