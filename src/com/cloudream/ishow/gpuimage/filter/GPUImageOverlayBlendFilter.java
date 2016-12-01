package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterType;
import com.cloudream.ishow.gpuimage.GPUImageTwoInputFilter;

public class GPUImageOverlayBlendFilter extends GPUImageTwoInputFilter
{
	private static final String OVERLAY_BLEND_FRAGMENT_SHADER =
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
			"	mediump float ra, ga ,ba;\n" +
			"	if(2.0 * base.r < base.a)\n" +
			"		ra = 2.0 * overlay.r * base.r + overlay.r * (1.0 - base.a) + base.r * (1.0 - overlay.a);\n" +
			"	else\n" +
			"		ra = overlay.a * base.a - 2.0 * (base.a - base.r) * (overlay.a - overlay.r) + overlay.r * (1.0 - base.a) + base.r * (1.0 - overlay.a);\n" +
			"	\n" +
			"	if(2.0 * base.g < base.a)\n" +
			"		ga = 2.0 * overlay.g * base.g + overlay.g * (1.0 - base.a) + base.g * (1.0 - overlay.a);\n" +
			"	else\n" +
			"		ga = overlay.a * base.a - 2.0 * (base.a - base.g) * (overlay.a - overlay.g) + overlay.g * (1.0 - base.a) + base.g * (1.0 - overlay.a);\n" +
			"	\n" +
			"	if(2.0 * base.b < base.a)\n" +
			"		ba = 2.0 * overlay.b * base.b + overlay.b * (1.0 - base.a) + base.b * (1.0 - overlay.a);\n" +
			"	else\n" +
			"		ba = overlay.a * base.a - 2.0 * (base.a - base.b) * (overlay.a - overlay.b) + overlay.b * (1.0 - base.a) + base.b * (1.0 - overlay.a);\n" +
			"	\n" +
			"	gl_FragColor = vec4(ra, ga, ba, 1.0);\n" +
			"}";

	public GPUImageOverlayBlendFilter()
	{
		super(GPUImageFilterType.BLEND_OVERLAY, OVERLAY_BLEND_FRAGMENT_SHADER);
	}
}
