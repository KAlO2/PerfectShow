package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.graphics.PointF;
import android.opengl.GLES20;

public class GPUImageBulgeDistortionFilter extends GPUImageFilter
{
	private static final String BULGE_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"\n" +
			"uniform highp float aspectRatio;\n" +
			"uniform highp vec2 center;\n" +
			"uniform highp float radius;\n" +
			"uniform highp float scale;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"highp vec2 textureCoordinateToUse = vec2(textureCoordinate.x, (textureCoordinate.y * aspectRatio + 0.5 - 0.5 * aspectRatio));\n" +
			"highp float dist = distance(center, textureCoordinateToUse);\n" +
			"textureCoordinateToUse = textureCoordinate;\n" +
			"\n" +
			"if (dist < radius)\n" +
			"{\n" +
			"textureCoordinateToUse -= center;\n" +
			"highp float percent = 1.0 - ((radius - dist) / radius) * scale;\n" +
			"percent = percent * percent;\n" +
			"\n" +
			"textureCoordinateToUse = textureCoordinateToUse * percent;\n" +
			"textureCoordinateToUse += center;\n" +
			"}\n" +
			"\n" +
			"gl_FragColor = texture2D(inputImageTexture, textureCoordinateToUse );    \n" +
			"}\n";

	private float mScale;
	private int mScaleLocation;
	private float mRadius;
	private int mRadiusLocation;
	private PointF mCenter;
	private int mCenterLocation;
	private float mAspectRatio;
	private int mAspectRatioLocation;

	public GPUImageBulgeDistortionFilter()
	{
		this(0.25F, 0.5F, new PointF(0.5F, 0.5F));
	}

	public GPUImageBulgeDistortionFilter(float radius, float scale, PointF center)
	{
		super(GPUImageFilterType.BULGE_DISTORTION, NO_FILTER_VERTEX_SHADER, BULGE_FRAGMENT_SHADER);
		mRadius = radius;
		mScale = scale;
		mCenter = center;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mScaleLocation = GLES20.glGetUniformLocation(getProgram(), "scale");
		mRadiusLocation = GLES20.glGetUniformLocation(getProgram(), "radius");
		mCenterLocation = GLES20.glGetUniformLocation(getProgram(), "center");
		mAspectRatioLocation = GLES20.glGetUniformLocation(getProgram(), "aspectRatio");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setRadius(mRadius);
		setScale(mScale);
		setCenter(mCenter);
	}

	@Override
	public void onOutputSizeChanged(int width, int height)
	{
		mAspectRatio = (float) height / width;
		setAspectRatio(mAspectRatio);
		super.onOutputSizeChanged(width, height);
	}

	private void setAspectRatio(float aspectRatio)
	{
		mAspectRatio = aspectRatio;
		setFloat(mAspectRatioLocation, aspectRatio);
	}

	/**
	 * The radius of the distortion, ranging from 0.0 to 1.0, with a default of 0.25
	 *
	 * @param radius from 0.0 to 1.0, default 0.25
	 */
	public void setRadius(float radius)
	{
		mRadius = radius;
		setFloat(mRadiusLocation, radius);
	}

	/**
	 * The amount of distortion to apply, from -1.0 to 1.0, with a default of 0.5
	 *
	 * @param scale from -1.0 to 1.0, default 0.5
	 */
	public void setScale(float scale)
	{
		mScale = scale;
		setFloat(mScaleLocation, scale);
	}

	/**
	 * The center about which to apply the distortion, with a default of (0.5, 0.5)
	 *
	 * @param center default (0.5, 0.5)
	 */
	public void setCenter(PointF center)
	{
		mCenter = center;
		setPoint(mCenterLocation, center);
	}
}
