package com.cloudream.ishow.gpuimage;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import android.graphics.Bitmap;
import android.opengl.GLES20;
import android.view.Surface;

public class GPUImageTwoInputFilter extends GPUImageFilter
{
	private static final String VERTEX_SHADER =
			"attribute vec4 position;\n" +
			"attribute vec4 inputTextureCoordinate;\n" +
			"attribute vec4 inputTextureCoordinate2;\n" +
			" \n" +
			"varying vec2 textureCoordinate;\n" +
			"varying vec2 textureCoordinate2;\n" +
			" \n" +
			"void main()\n" +
			"{\n" +
			"	gl_Position = position;\n" +
			"	textureCoordinate = inputTextureCoordinate.xy;\n" +
			"	textureCoordinate2 = inputTextureCoordinate2.xy;\n" +
			"}";

	public int mFilterSecondTextureCoordinateAttribute;
	public int mFilterInputTextureUniform2;
	public int mFilterSourceTexture2 = OpenGLUtils.NO_TEXTURE;
	private ByteBuffer mTexture2CoordinatesBuffer;
	private Bitmap mBitmap;

	public GPUImageTwoInputFilter(GPUImageFilterType type, String fragmentShader)
	{
		this(type, VERTEX_SHADER, fragmentShader);
	}

	public GPUImageTwoInputFilter(GPUImageFilterType type, String vertexShader, String fragmentShader)
	{
		super(type, vertexShader, fragmentShader);
		setRotation(Surface.ROTATION_0, false, false);
	}

	@Override
	public void onInit()
	{
		super.onInit();

		mFilterSecondTextureCoordinateAttribute = GLES20.glGetAttribLocation(getProgram(), "inputTextureCoordinate2");
		mFilterInputTextureUniform2 = GLES20.glGetUniformLocation(getProgram(), "inputImageTexture2"); // This
																										// does
																										// assume
																										// a
																										// name
																										// of
																										// "inputImageTexture2"
																										// for
																										// second
																										// input
																										// texture
																										// in
																										// the
																										// fragment
																										// shader
		GLES20.glEnableVertexAttribArray(mFilterSecondTextureCoordinateAttribute);

		if(mBitmap != null && !mBitmap.isRecycled())
		{
			setBitmap(mBitmap);
		}
	}

	public void setBitmap(final Bitmap bitmap)
	{
		if(bitmap != null && bitmap.isRecycled())
		{
			return;
		}
		mBitmap = bitmap;
		if(mBitmap == null)
		{
			return;
		}
		runOnDraw(new Runnable()
		{
			public void run()
			{
				if(mFilterSourceTexture2 == OpenGLUtils.NO_TEXTURE)
				{
					if(bitmap == null || bitmap.isRecycled())
					{
						return;
					}
					GLES20.glActiveTexture(GLES20.GL_TEXTURE3);
					mFilterSourceTexture2 = OpenGLUtils.loadTexture(bitmap, OpenGLUtils.NO_TEXTURE, false);
				}
			}
		});
	}

	public Bitmap getBitmap()
	{
		return mBitmap;
	}

	public void recycleBitmap()
	{
		if(mBitmap != null && !mBitmap.isRecycled())
		{
			mBitmap.recycle();
			mBitmap = null;
		}
	}

	public void onDestroy()
	{
		super.onDestroy();
		GLES20.glDeleteTextures(1, new int[]
		{ mFilterSourceTexture2 }, 0);
		mFilterSourceTexture2 = OpenGLUtils.NO_TEXTURE;
	}

	@Override
	protected void onDrawArraysPre()
	{
		GLES20.glEnableVertexAttribArray(mFilterSecondTextureCoordinateAttribute);
		GLES20.glActiveTexture(GLES20.GL_TEXTURE3);
		GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mFilterSourceTexture2);
		GLES20.glUniform1i(mFilterInputTextureUniform2, 3);

		mTexture2CoordinatesBuffer.position(0);
		GLES20.glVertexAttribPointer(mFilterSecondTextureCoordinateAttribute, 2, GLES20.GL_FLOAT, false, 0,
				mTexture2CoordinatesBuffer);
	}

	public void setRotation(final int rotation, final boolean flipHorizontal, final boolean flipVertical)
	{
		float[] buffer = TextureRotationUtil.getRotation(rotation, flipHorizontal, flipVertical);

		ByteBuffer bBuffer = ByteBuffer.allocateDirect(32).order(ByteOrder.nativeOrder());
		FloatBuffer fBuffer = bBuffer.asFloatBuffer();
		fBuffer.put(buffer);
		fBuffer.flip();

		mTexture2CoordinatesBuffer = bBuffer;
	}
}
