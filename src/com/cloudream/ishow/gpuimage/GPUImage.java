package com.cloudream.ishow.gpuimage;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.hardware.Camera;
import android.media.ExifInterface;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.provider.MediaStore;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.List;

import com.cloudream.ishow.util.BitmapUtils;

/**
 * The main accessor for GPUImage functionality. This class helps to do common tasks through a
 * simple interface.
 */
public class GPUImage
{
	private final Context mContext;
	private final GPUImageRenderer mRenderer;
	private GLSurfaceView mGlSurfaceView;
	private GPUImageFilter mFilter;
	private Bitmap mCurrentBitmap;
	private ScaleType mScaleType = ScaleType.CENTER_CROP;

	/**
	 * Instantiates a new GPUImage object.
	 *
	 * @param context the context
	 */
	public GPUImage(final Context context)
	{
		if(!OpenGLUtils.isOpenGLES2Supported(context))
			throw new IllegalStateException("OpenGL ES 2.0 is not supported on this phone.");

		mContext = context;
		mFilter = new GPUImageFilter();
		mRenderer = new GPUImageRenderer(mFilter);
	}

	/**
	 * Sets the GLSurfaceView which will display the preview.
	 *
	 * @param view the GLSurfaceView
	 */
	public void setGLSurfaceView(final GLSurfaceView view)
	{
		mGlSurfaceView = view;
		mGlSurfaceView.setEGLContextClientVersion(2);
		mGlSurfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
		mGlSurfaceView.getHolder().setFormat(PixelFormat.RGBA_8888);
		mGlSurfaceView.setRenderer(mRenderer);
		mGlSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
		mGlSurfaceView.requestRender();
	}

	/**
	 * Sets the background color
	 *
	 * @param red red color value
	 * @param green green color value
	 * @param blue red color value
	 */
	public void setBackgroundColor(float red, float green, float blue)
	{
		mRenderer.setBackgroundColor(red, green, blue);
	}

	/**
	 * Request the preview to be rendered again.
	 */
	public void requestRender()
	{
		if(mGlSurfaceView != null)
		{
			mGlSurfaceView.requestRender();
		}
	}

	/**
	 * Sets the up camera to be connected to GPUImage to get a filtered preview.
	 *
	 * @param camera the camera
	 */
	public void setUpCamera(final Camera camera)
	{
		setUpCamera(camera, 0, false, false);
	}

	/**
	 * Sets the up camera to be connected to GPUImage to get a filtered preview.
	 *
	 * @param camera the camera
	 * @param degrees by how many degrees the image should be rotated
	 * @param flipHorizontal if the image should be flipped horizontally
	 * @param flipVertical if the image should be flipped vertically
	 */
	public void setUpCamera(final Camera camera, final int degrees, final boolean flipHorizontal,
			final boolean flipVertical)
	{
		mGlSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
		mRenderer.setUpSurfaceTexture(camera);
		
		int rotation = degrees / 90;
		mRenderer.setRotation(rotation, flipHorizontal, flipVertical);
	}

	/**
	 * Sets the filter which should be applied to the image which was (or will be) set by
	 * setImage(...).
	 *
	 * @param filter the new filter
	 */
	public void setFilter(final GPUImageFilter filter)
	{
		mFilter = filter;
		mRenderer.setFilter(mFilter);
		requestRender();
	}

	/**
	 * Sets the image on which the filter should be applied.
	 *
	 * @param bitmap the new image
	 */
	public void setImage(final Bitmap bitmap)
	{
		mCurrentBitmap = bitmap;
		mRenderer.setImageBitmap(bitmap, false);
		requestRender();
	}

	/**
	 * This sets the scale type of GPUImage. This has to be run before setting the image. If image
	 * is set and scale type changed, image needs to be reset.
	 *
	 * @param scaleType The new ScaleType
	 */
	public void setScaleType(ScaleType scaleType)
	{
		mScaleType = scaleType;
		mRenderer.setScaleType(scaleType);
		mRenderer.deleteImage();
		mCurrentBitmap = null;
		requestRender();
	}

	/**
	 * Sets the rotation of the displayed image.
	 *
	 * @param rotation new rotation
	 */
	public void setRotation(int rotation)
	{
		if(rotation < Surface.ROTATION_0 || rotation > Surface.ROTATION_270)
			throw new IllegalArgumentException("rotation must be Surface.ROTATION_{0|90|180|270}");
		mRenderer.setRotation(rotation);
	}

	/**
	 * Sets the rotation of the displayed image with flip options.
	 *
	 * @param rotation new rotation
	 */
	public void setRotation(int rotation, boolean flipHorizontal, boolean flipVertical)
	{
		mRenderer.setRotation(rotation, flipHorizontal, flipVertical);
	}

	/**
	 * Deletes the current image.
	 */
	public void deleteImage()
	{
		mRenderer.deleteImage();
		mCurrentBitmap = null;
		requestRender();
	}

	/**
	 * Sets the image on which the filter should be applied from a Uri.
	 *
	 * @param uri the uri of the new image
	 */
	public void setImage(final Uri uri)
	{
		new LoadImageUriTask(this, uri).execute();
	}

	/**
	 * Sets the image on which the filter should be applied from a File.
	 *
	 * @param file the file of the new image
	 */
	public void setImage(final File file)
	{
		new LoadImageFileTask(this, file).execute();
	}

	private String getPath(final Uri uri)
	{
		final String[] projection = { MediaStore.Images.Media.DATA };
		Cursor cursor = mContext.getContentResolver().query(uri, projection, null, null, null);
		int pathIndex = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
		String path = null;
		if(cursor.moveToFirst())
		{
			path = cursor.getString(pathIndex);
		}
		cursor.close();
		return path;
	}

	/**
	 * Gets the current displayed image with applied filter as a Bitmap.
	 *
	 * @return the current image with filter applied
	 */
	public Bitmap getBitmapWithFilterApplied()
	{
		return getBitmapWithFilterApplied(mCurrentBitmap);
	}

	/**
	 * Gets the given bitmap with current filter applied as a Bitmap.
	 *
	 * @param bitmap the bitmap on which the current filter should be applied
	 * @return the bitmap with filter applied
	 */
	public Bitmap getBitmapWithFilterApplied(final Bitmap bitmap)
	{
		if(mGlSurfaceView != null)
		{
			mRenderer.deleteImage();
			mRenderer.runOnDraw(new Runnable()
			{
				@Override
				public void run()
				{
					synchronized(mFilter)
					{
						mFilter.destroy();
						mFilter.notify();
					}
				}
			});
			synchronized(mFilter)
			{
				requestRender();
				try
				{
					mFilter.wait();
				}
				catch(InterruptedException e)
				{
					e.printStackTrace();
				}
			}
		}

		GPUImageRenderer renderer = new GPUImageRenderer(mFilter);
		renderer.setRotation(Surface.ROTATION_0, mRenderer.isFlippedHorizontally(), mRenderer.isFlippedVertically());
		renderer.setScaleType(mScaleType);
		PixelBuffer buffer = new PixelBuffer(bitmap.getWidth(), bitmap.getHeight());
		buffer.setRenderer(renderer);
		renderer.setImageBitmap(bitmap, false);
		Bitmap result = buffer.getBitmap();
		mFilter.destroy();
		renderer.deleteImage();
		buffer.destroy();

		mRenderer.setFilter(mFilter);
		if(mCurrentBitmap != null)
		{
			mRenderer.setImageBitmap(mCurrentBitmap, false);
		}
		requestRender();

		return result;
	}

	/**
	 * Gets the images for multiple filters on a image. This can be used to quickly get thumbnail
	 * images for filters. <br>
	 * Whenever a new Bitmap is ready, the listener will be called with the bitmap. The order of the
	 * calls to the listener will be the same as the filter order.
	 *
	 * @param bitmap the bitmap on which the filters will be applied
	 * @param filters the filters which will be applied on the bitmap
	 * @param listener the listener on which the results will be notified
	 */
	public static void getBitmapForMultipleFilters(final Bitmap bitmap, final List<GPUImageFilter> filters,
			final ResponseListener<Bitmap> listener)
	{
		if(filters.isEmpty())
		{
			return;
		}
		GPUImageRenderer renderer = new GPUImageRenderer(filters.get(0));
		renderer.setImageBitmap(bitmap, false);
		PixelBuffer buffer = new PixelBuffer(bitmap.getWidth(), bitmap.getHeight());
		buffer.setRenderer(renderer);

		for(GPUImageFilter filter : filters)
		{
			renderer.setFilter(filter);
			listener.response(buffer.getBitmap());
			filter.destroy();
		}
		renderer.deleteImage();
		buffer.destroy();
	}

	/**
	 * Runs the given Runnable on the OpenGL thread.
	 *
	 * @param runnable The runnable to be run on the OpenGL thread.
	 */
	void runOnGLThread(Runnable runnable)
	{
		mRenderer.runOnDrawEnd(runnable);
	}

	private Point getOutputSize()
	{
		Point size = new Point();
		if(mRenderer != null && mRenderer.getFrameWidth() != 0 && mRenderer.getFrameHeight() != 0)
		{
			size.x = mRenderer.getFrameWidth();
			size.y = mRenderer.getFrameHeight();
		}
		else if(mCurrentBitmap != null)
		{
			size.x = mCurrentBitmap.getWidth();
			size.y = mCurrentBitmap.getHeight();
		}
		else
		{
			WindowManager windowManager = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
			Display display = windowManager.getDefaultDisplay();
			size.x = display.getWidth();
			size.y = display.getHeight();
		}
		
		return size;
	}

	public interface OnPictureSavedListener
	{
		void onPictureSaved(Uri uri);
	}

	private class LoadImageUriTask extends LoadImageTask
	{

		private final Uri mUri;

		public LoadImageUriTask(GPUImage gpuImage, Uri uri)
		{
			super(gpuImage);
			mUri = uri;
		}

		@Override
		protected Bitmap decode(BitmapFactory.Options options)
		{
			return BitmapUtils.getBitmapFromUri(mContext, mUri, options);
		}

		@Override
		protected int getImageOrientation()
		{
			Cursor cursor = mContext.getContentResolver().query(mUri, new String[]
			{ MediaStore.Images.ImageColumns.ORIENTATION }, null, null, null);

			if(cursor == null || cursor.getCount() != 1)
				return 0;

			cursor.moveToFirst();
			int orientation = cursor.getInt(0);
			cursor.close();
			return orientation;
		}
	}

	private class LoadImageFileTask extends LoadImageTask
	{

		private final File mImageFile;

		public LoadImageFileTask(GPUImage gpuImage, File file)
		{
			super(gpuImage);
			mImageFile = file;
		}

		@Override
		protected Bitmap decode(BitmapFactory.Options options)
		{
			return BitmapFactory.decodeFile(mImageFile.getAbsolutePath(), options);
		}

		@Override
		protected int getImageOrientation()
		{
			ExifInterface exif = null;
			try
			{
				exif = new ExifInterface(mImageFile.getAbsolutePath());
			}
			catch(IOException e)
			{
				e.printStackTrace();
				return 0;
			}
			
			int orientation = exif.getAttributeInt(ExifInterface.TAG_ORIENTATION, 0);
			switch(orientation)
			{
			case ExifInterface.ORIENTATION_NORMAL:
				return 0;
			case ExifInterface.ORIENTATION_ROTATE_90:
				return 90;
			case ExifInterface.ORIENTATION_ROTATE_180:
				return 180;
			case ExifInterface.ORIENTATION_ROTATE_270:
				return 270;
			default:
				return 0;
			}
		}
	}

	private abstract class LoadImageTask extends AsyncTask<Void, Void, Bitmap>
	{

		private final GPUImage mGPUImage;
		private int mOutputWidth;
		private int mOutputHeight;

		public LoadImageTask(final GPUImage gpuImage)
		{
			mGPUImage = gpuImage;
		}

		@Override
		protected Bitmap doInBackground(Void... params)
		{
			if(mRenderer != null && mRenderer.getFrameWidth() == 0)
			{
				try
				{
					synchronized(mRenderer.mSurfaceChangedWaiter)
					{
						mRenderer.mSurfaceChangedWaiter.wait(3000);
					}
				}
				catch(InterruptedException e)
				{
					e.printStackTrace();
				}
			}
			Point size = getOutputSize();
			mOutputWidth = size.x;
			mOutputHeight = size.y;
			return loadResizedImage();
		}

		@Override
		protected void onPostExecute(Bitmap bitmap)
		{
			super.onPostExecute(bitmap);
			mGPUImage.deleteImage();
			mGPUImage.setImage(bitmap);
		}

		protected abstract Bitmap decode(BitmapFactory.Options options);

		private Bitmap loadResizedImage()
		{
			BitmapFactory.Options options = new BitmapFactory.Options();
			options.inJustDecodeBounds = true;
			decode(options);
			int scale = 1;
			while(checkSize(options.outWidth / scale > mOutputWidth, options.outHeight / scale > mOutputHeight))
			{
				scale++;
			}

			scale--;
			if(scale < 1)
			{
				scale = 1;
			}
			options = new BitmapFactory.Options();
			options.inSampleSize = scale;
			options.inPreferredConfig = Bitmap.Config.RGB_565;
			options.inPurgeable = true;
			options.inTempStorage = new byte[32 * 1024];
			Bitmap bitmap = decode(options);
			if(bitmap == null)
			{
				return null;
			}
			bitmap = rotateImage(bitmap);
			bitmap = scaleBitmap(bitmap);
			return bitmap;
		}

		private Bitmap scaleBitmap(Bitmap bitmap)
		{
			// resize to desired dimensions
			int width = bitmap.getWidth();
			int height = bitmap.getHeight();
			int[] newSize = getScaleSize(width, height);
			Bitmap workBitmap = Bitmap.createScaledBitmap(bitmap, newSize[0], newSize[1], true);
			if(workBitmap != bitmap)
			{
				bitmap.recycle();
				bitmap = workBitmap;
				System.gc();
			}

			if(mScaleType == ScaleType.CENTER_CROP)
			{
				// Crop it
				int diffWidth = newSize[0] - mOutputWidth;
				int diffHeight = newSize[1] - mOutputHeight;
				workBitmap = Bitmap.createBitmap(bitmap, diffWidth / 2, diffHeight / 2, newSize[0] - diffWidth,
						newSize[1] - diffHeight);
				if(workBitmap != bitmap)
				{
					bitmap.recycle();
					bitmap = workBitmap;
				}
			}

			return bitmap;
		}

		/**
		 * Retrieve the scaling size for the image dependent on the ScaleType.<br>
		 * <br>
		 * If CROP: sides are same size or bigger than output's sides<br>
		 * Else : sides are same size or smaller than output's sides
		 */
		private int[] getScaleSize(int width, int height)
		{
			float newWidth;
			float newHeight;

			float widthRatio = (float) width / mOutputWidth;
			float heightRatio = (float) height / mOutputHeight;

			boolean adjustWidth = mScaleType == ScaleType.CENTER_CROP ? widthRatio > heightRatio
					: widthRatio < heightRatio;

			if(adjustWidth)
			{
				newHeight = mOutputHeight;
				newWidth = (newHeight / height) * width;
			}
			else
			{
				newWidth = mOutputWidth;
				newHeight = (newWidth / width) * height;
			}
			return new int[]{ Math.round(newWidth), Math.round(newHeight) };
		}

		private boolean checkSize(boolean widthBigger, boolean heightBigger)
		{
			if(mScaleType == ScaleType.CENTER_CROP)
			{
				return widthBigger && heightBigger;
			}
			else
			{
				return widthBigger || heightBigger;
			}
		}

		private Bitmap rotateImage(final Bitmap bitmap)
		{
			if(bitmap == null)
				return null;
			
			Bitmap rotatedBitmap = bitmap;

			int orientation = getImageOrientation();
			if(orientation != 0)
			{
				Matrix matrix = new Matrix();
				matrix.postRotate(orientation);
				rotatedBitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
				bitmap.recycle();
			}

			return rotatedBitmap;
		}

		protected abstract int getImageOrientation();
	}

	public interface ResponseListener<T>
	{
		void response(T item);
	}

	public enum ScaleType
	{
		CENTER_INSIDE, CENTER_CROP
	}
}
