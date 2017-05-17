package com.cloudream.ishow.gpuimage;

import android.annotation.SuppressLint;
import android.opengl.GLES20;
import android.view.Surface;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.List;

import static com.cloudream.ishow.gpuimage.GPUImageRenderer.CUBE;

/**
 * Resembles a filter that consists of multiple filters applied after each other.
 */
public class GPUImageFilterGroup extends GPUImageFilter
{

	protected List<GPUImageFilter> mFilters;
	protected List<GPUImageFilter> mMergedFilters;
	private int[] mFrameBuffers;
	private int[] mFrameBufferTextures;

	private final FloatBuffer mGLCubeBuffer;
	private final FloatBuffer mGLTextureBuffer;
	private final FloatBuffer mGLTextureFlipBuffer;

	/**
	 * Instantiates a new GPUImageFilterGroup with no filters.
	 */
	public GPUImageFilterGroup()
	{
		this(null);
	}

	/**
	 * Instantiates a new GPUImageFilterGroup with the given filters.
	 *
	 * @param filters the filters which represent this filter
	 */
	public GPUImageFilterGroup(List<GPUImageFilter> filters)
	{
		mFilters = filters;
		if(mFilters == null)
		{
			mFilters = new ArrayList<GPUImageFilter>();
		}
		else
		{
			updateMergedFilters();
		}

		mGLCubeBuffer = ByteBuffer.allocateDirect(CUBE.length * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();
		mGLCubeBuffer.put(CUBE).position(0);

		final int rotation = Surface.ROTATION_0;
		final float texcoord[] = TextureRotationUtil.TEXCOORD_ARRAY[rotation];
		mGLTextureBuffer = ByteBuffer.allocateDirect(texcoord.length * 4).order(ByteOrder.nativeOrder())
				.asFloatBuffer();
		mGLTextureBuffer.put(texcoord).position(0);

		float[] flipTexture = TextureRotationUtil.getRotation(rotation, false, true);
		mGLTextureFlipBuffer = ByteBuffer.allocateDirect(flipTexture.length * 4).order(ByteOrder.nativeOrder())
				.asFloatBuffer();
		mGLTextureFlipBuffer.put(flipTexture).position(0);
	}

	public void addFilter(GPUImageFilter aFilter)
	{
		if(aFilter == null)
		{
			return;
		}
		mFilters.add(aFilter);
		updateMergedFilters();
	}

	@Override
	public void onInit()
	{
		super.onInit();
		for(GPUImageFilter filter : mFilters)
		{
			filter.init();
		}
	}

	@Override
	public void onDestroy()
	{
		destroyFramebuffers();
		for(GPUImageFilter filter : mFilters)
		{
			filter.destroy();
		}
		super.onDestroy();
	}

	private void destroyFramebuffers()
	{
		if(mFrameBufferTextures != null)
		{
			GLES20.glDeleteTextures(mFrameBufferTextures.length, mFrameBufferTextures, 0);
			mFrameBufferTextures = null;
		}
		if(mFrameBuffers != null)
		{
			GLES20.glDeleteFramebuffers(mFrameBuffers.length, mFrameBuffers, 0);
			mFrameBuffers = null;
		}
	}

	@Override
	public void onOutputSizeChanged(final int width, final int height)
	{
		super.onOutputSizeChanged(width, height);
		if(mFrameBuffers != null)
		{
			destroyFramebuffers();
		}

		int size = mFilters.size();
		for(int i = 0; i < size; i++)
		{
			mFilters.get(i).onOutputSizeChanged(width, height);
		}

		if(mMergedFilters != null && mMergedFilters.size() > 0)
		{
			size = mMergedFilters.size();
			mFrameBuffers = new int[size - 1];
			mFrameBufferTextures = new int[size - 1];

			for(int i = 0; i < size - 1; i++)
			{
				GLES20.glGenFramebuffers(1, mFrameBuffers, i);
				GLES20.glGenTextures(1, mFrameBufferTextures, i);
				GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mFrameBufferTextures[i]);
				GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, width, height, 0, GLES20.GL_RGBA,
						GLES20.GL_UNSIGNED_BYTE, null);
				GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
				GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
				GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
				GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);

				GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFrameBuffers[i]);
				GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D,
						mFrameBufferTextures[i], 0);

				GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
				GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
			}
		}
	}

	@SuppressLint("WrongCall")
	@Override
	public void onDraw(final int textureId, final FloatBuffer cubeBuffer, final FloatBuffer textureBuffer)
	{
		runPendingOnDrawTasks();
		if(!isInitialized() || mFrameBuffers == null || mFrameBufferTextures == null)
		{
			return;
		}
		if(mMergedFilters != null)
		{
			int size = mMergedFilters.size();
			int previousTexture = textureId;
			for(int i = 0; i < size; i++)
			{
				GPUImageFilter filter = mMergedFilters.get(i);
				boolean isNotLast = i < size - 1;
				if(isNotLast)
				{
					GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFrameBuffers[i]);
					GLES20.glClearColor(0, 0, 0, 0);
				}

				if(i == 0)
				{
					filter.onDraw(previousTexture, cubeBuffer, textureBuffer);
				}
				else if(i == size - 1)
				{
					filter.onDraw(previousTexture, mGLCubeBuffer,
							(size % 2 == 0) ? mGLTextureFlipBuffer : mGLTextureBuffer);
				}
				else
				{
					filter.onDraw(previousTexture, mGLCubeBuffer, mGLTextureBuffer);
				}

				if(isNotLast)
				{
					GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
					previousTexture = mFrameBufferTextures[i];
				}
			}
		}
	}

	/**
	 * Gets the filters.
	 *
	 * @return the filters
	 */
	public List<GPUImageFilter> getFilters()
	{
		return mFilters;
	}

	public List<GPUImageFilter> getMergedFilters()
	{
		return mMergedFilters;
	}

	public void updateMergedFilters()
	{
		if(mFilters == null)
		{
			return;
		}

		if(mMergedFilters == null)
		{
			mMergedFilters = new ArrayList<GPUImageFilter>();
		}
		else
		{
			mMergedFilters.clear();
		}

		List<GPUImageFilter> filters;
		for(GPUImageFilter filter : mFilters)
		{
			if(filter instanceof GPUImageFilterGroup)
			{
				((GPUImageFilterGroup) filter).updateMergedFilters();
				filters = ((GPUImageFilterGroup) filter).getMergedFilters();
				if(filters == null || filters.isEmpty())
					continue;
				mMergedFilters.addAll(filters);
				continue;
			}
			mMergedFilters.add(filter);
		}
	}
}
