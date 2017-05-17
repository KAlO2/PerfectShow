package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.graphics.PointF;
import android.opengl.GLES20;

/**
 * Creates a swirl distortion on the image.
 */
public class GPUImageSwirlFilter extends GPUImageFilter
{
	private static final String SWIRL_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"\n" +
			"uniform highp vec2 center;\n" +
			"uniform highp float radius;\n" +
			"uniform highp float angle;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	highp vec2 textureCoordinateToUse = textureCoordinate;\n" +
			"	highp float dist = distance(center, textureCoordinate);\n" +
			"	if(dist < radius)\n" +
			"	{\n" +
			"		textureCoordinateToUse -= center;\n" +
			"		highp float percent = (radius - dist) / radius;\n" +
			"		highp float theta = percent * percent * angle * 8.0;\n" +
			"		highp float s = sin(theta);\n" +
			"		highp float c = cos(theta);\n" +
			"		textureCoordinateToUse = vec2(dot(textureCoordinateToUse, vec2(c, -s)), dot(textureCoordinateToUse, vec2(s, c)));\n" +
			"		textureCoordinateToUse += center;\n" +
			"	}\n" +
			"\n" +
			"	gl_FragColor = texture2D(inputImageTexture, textureCoordinateToUse );\n" +
			"}\n";

	private float mAngle;
	private int mAngleLocation;
	private float mRadius;
	private int mRadiusLocation;
	private PointF mCenter;
	private int mCenterLocation;

	public GPUImageSwirlFilter()
	{
		this(0.5F, 1.0F, new PointF(0.5F, 0.5F));
	}

	public GPUImageSwirlFilter(float radius, float angle, PointF center)
	{
		super(GPUImageFilterType.SWIRL, NO_FILTER_VERTEX_SHADER, SWIRL_FRAGMENT_SHADER);
		mRadius = radius;
		mAngle = angle;
		mCenter = center;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mAngleLocation = GLES20.glGetUniformLocation(getProgram(), "angle");
		mRadiusLocation = GLES20.glGetUniformLocation(getProgram(), "radius");
		mCenterLocation = GLES20.glGetUniformLocation(getProgram(), "center");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setRadius(mRadius);
		setAngle(mAngle);
		setCenter(mCenter);
	}

	/**
	 * The radius of the distortion, ranging from 0.0 to 1.0, with a default of 0.5.
	 *
	 * @param radius from 0.0 to 1.0, default 0.5
	 */
	public void setRadius(float radius)
	{
		mRadius = radius;
		setFloat(mRadiusLocation, radius);
	}

	/**
	 * The amount of distortion to apply, with a minimum of 0.0 and a default of 1.0.
	 *
	 * @param angle minimum 0.0, default 1.0
	 */
	public void setAngle(float angle)
	{
		mAngle = angle;
		setFloat(mAngleLocation, angle);
	}

	/**
	 * The center about which to apply the distortion, with a default of (0.5, 0.5).
	 *
	 * @param center default (0.5, 0.5)
	 */
	public void setCenter(PointF center)
	{
		mCenter = center;
		setPoint(mCenterLocation, center);
	}
}
