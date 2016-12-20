package com.cloudream.ishow.gpuimage;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.PointF;
import android.opengl.GLES20;

import java.io.InputStream;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.LinkedList;

public class GPUImageFilter
{
	protected static final String NO_FILTER_VERTEX_SHADER =
			"attribute vec4 position;\n" +
			"attribute vec4 inputTextureCoordinate;\n" +
			"\n" +
			"varying vec2 textureCoordinate;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	gl_Position = position;\n" +
			"	textureCoordinate = inputTextureCoordinate.xy;\n" +
			"}";
	protected static final String NO_FILTER_FRAGMENT_SHADER =
			"varying highp vec2 textureCoordinate;\n" +
			"\n" +
			"uniform sampler2D inputImageTexture;\n" +
			"\n" +
			"void main()\n" +
			"{\n" +
			"	gl_FragColor = texture2D(inputImageTexture, textureCoordinate);\n" +
			"}";

	private final LinkedList<Runnable> mRunOnDraw;
	private final String mVertexShader;
	private final String mFragmentShader;
	protected int mGLProgramId;
	protected int mGLAttribPosition;
	protected int mGLUniformTexture;
	protected int mGLAttribTextureCoordinate;
	protected int mOutputWidth;
	protected int mOutputHeight;
	private boolean mIsInitialized;
	protected GPUImageFilterType type;
	
	public GPUImageFilter()
	{
		this(GPUImageFilterType.RAW, NO_FILTER_VERTEX_SHADER, NO_FILTER_FRAGMENT_SHADER);
	}

	public GPUImageFilter(final String vertexShader, final String fragmentShader)
	{
		this(null, vertexShader, fragmentShader);
	}
	
	public GPUImageFilter(GPUImageFilterType type, final String vertexShader, final String fragmentShader)
	{
		mRunOnDraw = new LinkedList<Runnable>();
		mVertexShader = vertexShader;
		mFragmentShader = fragmentShader;
		this.type = type;
	}

	public final void init()
	{
		onInit();
		mIsInitialized = true;
		onInitialized();
	}

	public void onInit()
	{
		mGLProgramId = OpenGLUtils.loadProgram(mVertexShader, mFragmentShader);
		mGLAttribPosition = GLES20.glGetAttribLocation(mGLProgramId, "position");
		mGLUniformTexture = GLES20.glGetUniformLocation(mGLProgramId, "inputImageTexture");
		mGLAttribTextureCoordinate = GLES20.glGetAttribLocation(mGLProgramId, "inputTextureCoordinate");
		mIsInitialized = true;
	}

	public void onInitialized()
	{
	}

	public final void destroy()
	{
		mIsInitialized = false;
		GLES20.glDeleteProgram(mGLProgramId);
		onDestroy();
	}

	protected void onDestroy()
	{
	}

	public void onOutputSizeChanged(final int width, final int height)
	{
		mOutputWidth = width;
		mOutputHeight = height;
	}

	public void onDraw(final int textureId, final FloatBuffer cubeBuffer, final FloatBuffer textureBuffer)
	{
		GLES20.glUseProgram(mGLProgramId);
		runPendingOnDrawTasks();
		if(!mIsInitialized)
		{
			return;
		}

		cubeBuffer.position(0);
		GLES20.glVertexAttribPointer(mGLAttribPosition, 2, GLES20.GL_FLOAT, false, 0, cubeBuffer);
		GLES20.glEnableVertexAttribArray(mGLAttribPosition);
		textureBuffer.position(0);
		GLES20.glVertexAttribPointer(mGLAttribTextureCoordinate, 2, GLES20.GL_FLOAT, false, 0, textureBuffer);
		GLES20.glEnableVertexAttribArray(mGLAttribTextureCoordinate);
		if(textureId != OpenGLUtils.NO_TEXTURE)
		{
			GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);
			GLES20.glUniform1i(mGLUniformTexture, 0);
		}
		onDrawArraysPre();
		GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
		GLES20.glDisableVertexAttribArray(mGLAttribPosition);
		GLES20.glDisableVertexAttribArray(mGLAttribTextureCoordinate);
		GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
	}

	protected void onDrawArraysPre()
	{
	}

	protected void runPendingOnDrawTasks()
	{
		while(!mRunOnDraw.isEmpty())
		{
			mRunOnDraw.removeFirst().run();
		}
	}

	public boolean isInitialized()
	{
		return mIsInitialized;
	}

	public int getOutputWidth()
	{
		return mOutputWidth;
	}

	public int getOutputHeight()
	{
		return mOutputHeight;
	}
	
	protected void setFilterType(GPUImageFilterType type)
	{
		this.type = type;
	}
	
	public GPUImageFilterType getFilterType()
	{
		return type;
	}
	
	public int getProgram()
	{
		return mGLProgramId;
	}

	public int getAttribPosition()
	{
		return mGLAttribPosition;
	}

	public int getAttribTextureCoordinate()
	{
		return mGLAttribTextureCoordinate;
	}

	public int getUniformTexture()
	{
		return mGLUniformTexture;
	}

	protected void setInteger(final int location, final int intValue)
	{
		runOnDraw(new Runnable()
		{
			@Override
			public void run()
			{
				GLES20.glUniform1i(location, intValue);
			}
		});
	}

	protected void setIntegerArray(final int location, final int[] arrayValue)
	{
		runOnDraw(new Runnable()
		{
			@Override
			public void run()
			{
				GLES20.glUniform1iv(location, arrayValue.length, IntBuffer.wrap(arrayValue));
			}
		});
	}

	protected void setFloat(final int location, final float floatValue)
	{
		runOnDraw(new Runnable()
		{
			@Override
			public void run()
			{
				GLES20.glUniform1f(location, floatValue);
			}
		});
	}

	protected void setFloatVec2(final int location, final float[] arrayValue)
	{
		runOnDraw(new Runnable()
		{
			@Override
			public void run()
			{
				GLES20.glUniform2fv(location, 1, FloatBuffer.wrap(arrayValue));
			}
		});
	}

	protected void setFloatVec3(final int location, final float[] arrayValue)
	{
		runOnDraw(new Runnable()
		{
			@Override
			public void run()
			{
				GLES20.glUniform3fv(location, 1, FloatBuffer.wrap(arrayValue));
			}
		});
	}

	protected void setFloatVec4(final int location, final float[] arrayValue)
	{
		runOnDraw(new Runnable()
		{
			@Override
			public void run()
			{
				GLES20.glUniform4fv(location, 1, FloatBuffer.wrap(arrayValue));
			}
		});
	}

	protected void setFloatArray(final int location, final float[] arrayValue)
	{
		runOnDraw(new Runnable()
		{
			@Override
			public void run()
			{
				GLES20.glUniform1fv(location, arrayValue.length, FloatBuffer.wrap(arrayValue));
			}
		});
	}

	protected void setPoint(final int location, final PointF point)
	{
		runOnDraw(new Runnable()
		{

			@Override
			public void run()
			{
				float[] vec2 = new float[2];
				vec2[0] = point.x;
				vec2[1] = point.y;
				GLES20.glUniform2fv(location, 1, vec2, 0);
			}
		});
	}

	protected void setUniformMatrix3f(final int location, final float[] matrix)
	{
		runOnDraw(new Runnable()
		{

			@Override
			public void run()
			{
				GLES20.glUniformMatrix3fv(location, 1, false, matrix, 0);
			}
		});
	}

	protected void setUniformMatrix4f(final int location, final float[] matrix)
	{
		runOnDraw(new Runnable()
		{

			@Override
			public void run()
			{
				GLES20.glUniformMatrix4fv(location, 1, false, matrix, 0);
			}
		});
	}

	protected void runOnDraw(final Runnable runnable)
	{
		synchronized(mRunOnDraw)
		{
			mRunOnDraw.addLast(runnable);
		}
	}

	public static String loadShader(String file, Context context)
	{
		try
		{
			AssetManager assetManager = context.getAssets();
			InputStream ims = assetManager.open(file);

			String re = convertStreamToString(ims);
			ims.close();
			return re;
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}

		return "";
	}

	public static String convertStreamToString(java.io.InputStream is)
	{
		java.util.Scanner s = new java.util.Scanner(is).useDelimiter("\\A");
		return s.hasNext() ? s.next() : "";
	}
}
