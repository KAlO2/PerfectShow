package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;

/**
 * The haze filter can be used to add or remove haze.
 *
 * This is similar to a UV filter.
 */
public class GPUImageHazeFilter extends GPUImageFilter
{
	private static final String HAZE_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"\n" +
			"uniform lowp float distance;\n" +
			"uniform highp float slope;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	//todo reconsider precision modifiers	 \n" +
			"	 highp vec4 color = vec4(1.0);//todo reimplement as a parameter\n" +
			"\n" +
			"	 highp float  d = textureCoordinate.y * slope  +  distance; \n" +
			"\n" +
			"	 highp vec4 c = texture2D(inputImageTexture, textureCoordinate) ; // consider using unpremultiply\n" +
			"\n" +
			"	 c = (c - d * color) / (1.0 -d);\n" +
			"\n" +
			"	 gl_FragColor = c; //consider using premultiply(c);\n" +
			"}\n";

	private float mDistance;
	private int mDistanceLocation;
	private float mSlope;
	private int mSlopeLocation;

	public GPUImageHazeFilter()
	{
		this(0.2F, 0.0F);
	}

	public GPUImageHazeFilter(float distance, float slope)
	{
		super(GPUImageFilterType.HAZE, NO_FILTER_VERTEX_SHADER, HAZE_FRAGMENT_SHADER);
		mDistance = distance;
		mSlope = slope;
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mDistanceLocation = GLES20.glGetUniformLocation(getProgram(), "distance");
		mSlopeLocation = GLES20.glGetUniformLocation(getProgram(), "slope");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setDistance(mDistance);
		setSlope(mSlope);
	}

	/**
	 * Strength of the color applied. Default 0. Values between -.3 and .3 are best.
	 *
	 * @param distance -0.3 to 0.3 are best, default 0
	 */
	public void setDistance(float distance)
	{
		mDistance = distance;
		setFloat(mDistanceLocation, distance);
	}

	/**
	 * Amount of color change. Default 0. Values between -.3 and .3 are best.
	 *
	 * @param slope -0.3 to 0.3 are best, default 0
	 */
	public void setSlope(float slope)
	{
		mSlope = slope;
		setFloat(mSlopeLocation, slope);
	}
}
