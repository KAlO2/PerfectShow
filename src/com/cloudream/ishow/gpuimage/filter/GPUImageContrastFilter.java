package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilter;
import com.cloudream.ishow.gpuimage.GPUImageFilterType;

import android.opengl.GLES20;

/**
 * Changes the contrast of the image.<br>
 * <br>
 * contrast value ranges from 0.0 to 4.0, with 1.0 as the normal level
 */
public class GPUImageContrastFilter extends GPUImageFilter
{
	private static final String CONTRAST_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" + 
			"\n" + 
			"uniform sampler2D inputImageTexture;\n" + 
			"uniform lowp float contrast;\n" + 
			"\n" + 
			"void main()\n" + 
			"{\n" + 
			"	lowp vec4 color = texture2D(inputImageTexture, textureCoordinate);\n" + 
			"	\n" + 
			"	gl_FragColor = vec4(((color.rgb - vec3(0.5)) * contrast + vec3(0.5)), color.w);\n" + 
			"}";

	private int mContrastLocation;
	private float mContrast;

	public GPUImageContrastFilter()
	{
		this(0.0F);
	}

	public GPUImageContrastFilter(float contrast)
	{
		super(GPUImageFilterType.CONTRAST, NO_FILTER_VERTEX_SHADER, CONTRAST_FRAGMENT_SHADER);
		mContrast = map(contrast);
	}

	@Override
	public void onInit()
	{
		super.onInit();
		mContrastLocation = GLES20.glGetUniformLocation(getProgram(), "contrast");
	}

	@Override
	public void onInitialized()
	{
		super.onInitialized();
		setContrast(mContrast);
	}

	/**
	 * mapping value from [-1.0, 1.0) to [0, +INF), to make the slop is uniform.
	 * @param x input value.
	 * @return mapped value.
	 */
	private static float map(float x)
	{
		return (float)Math.tan((x + 1)*Math.PI/4);
	}
	
	public void setContrast(final float contrast)
	{
		mContrast = map(contrast);
		setFloat(mContrastLocation, mContrast);
	}
}
