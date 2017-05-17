package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterType;

public class GPUImageWeakPixelInclusionFilter extends GPUImage3x3TextureSamplingFilter
{
	private static final String WEAKPIXEL_FRAGMENT_SHADER =
			"precision lowp float;\n" +
			"\n" +
			"varying vec2 textureCoordinate;\n" +
			"varying vec2 leftTextureCoordinate;\n" +
			"varying vec2 rightTextureCoordinate;\n" +
			"\n" +
			"varying vec2 topTextureCoordinate;\n" +
			"varying vec2 topLeftTextureCoordinate;\n" +
			"varying vec2 topRightTextureCoordinate;\n" +
			"\n" +
			"varying vec2 bottomTextureCoordinate;\n" +
			"varying vec2 bottomLeftTextureCoordinate;\n" +
			"varying vec2 bottomRightTextureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"float bottomLeftIntensity = texture2D(inputImageTexture, bottomLeftTextureCoordinate).r;\n" +
			"float topRightIntensity = texture2D(inputImageTexture, topRightTextureCoordinate).r;\n" +
			"float topLeftIntensity = texture2D(inputImageTexture, topLeftTextureCoordinate).r;\n" +
			"float bottomRightIntensity = texture2D(inputImageTexture, bottomRightTextureCoordinate).r;\n" +
			"float leftIntensity = texture2D(inputImageTexture, leftTextureCoordinate).r;\n" +
			"float rightIntensity = texture2D(inputImageTexture, rightTextureCoordinate).r;\n" +
			"float bottomIntensity = texture2D(inputImageTexture, bottomTextureCoordinate).r;\n" +
			"float topIntensity = texture2D(inputImageTexture, topTextureCoordinate).r;\n" +
			"float centerIntensity = texture2D(inputImageTexture, textureCoordinate).r;\n" +
			"\n" +
			"float pixelIntensitySum = bottomLeftIntensity + topRightIntensity + topLeftIntensity + bottomRightIntensity + leftIntensity + rightIntensity + bottomIntensity + topIntensity + centerIntensity;\n" +
			"float sumTest = step(1.5, pixelIntensitySum);\n" +
			"float pixelTest = step(0.01, centerIntensity);\n" +
			"\n" +
			"gl_FragColor = vec4(vec3(sumTest * pixelTest), 1.0);\n" +
			"}\n";

	public GPUImageWeakPixelInclusionFilter()
	{
		super(WEAKPIXEL_FRAGMENT_SHADER);
		setFilterType(GPUImageFilterType.WEAK_PIXEL_INCLUSION);
	}
}
