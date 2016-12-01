package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterType;
import com.cloudream.ishow.gpuimage.GPUImageTwoInputFilter;

public class GPUImageSoftLightBlendFilter extends GPUImageTwoInputFilter
{
	private static final String SOFT_LIGHT_BLEND_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"varying highp vec2 textureCoordinate2;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"uniform sampler2D inputImageTexture2;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	mediump vec4 base = texture2D(inputImageTexture, textureCoordinate);\n" +
			"	mediump vec4 overlay = texture2D(inputImageTexture2, textureCoordinate2);\n" +
			"	\n" +
			"	gl_FragColor = base * (overlay.a * (base / base.a) + (2.0 * overlay * (1.0 - (base / base.a)))) + overlay * (1.0 - base.a) + base * (1.0 - overlay.a);\n" +
			"}";

	public GPUImageSoftLightBlendFilter()
	{
		super(GPUImageFilterType.BLEND_SOFT_LIGHT, SOFT_LIGHT_BLEND_FRAGMENT_SHADER);
	}
}
