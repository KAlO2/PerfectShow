package com.cloudream.ishow.util;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory.Options;
import android.graphics.BitmapFactory;
import android.graphics.BitmapRegionDecoder;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Build;
import android.provider.MediaStore;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;
import android.util.Pair;
import android.view.Display;
import android.view.View;
import android.view.WindowManager;
import android.view.View.MeasureSpec;
import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.ref.WeakReference;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import com.cloudream.ishow.App;
import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.gpuimage.OpenGLUtils;

/**
 * Utility class that deals with operations with an ImageView.
 * 
 * refer to class #{@linkcom.android.bitmap.util.BitmapUtils} and #
 * {@link com.cloudream.ishow.util.gallery3d.common.BitmapUtils}
 */
public final class BitmapUtils
{
	private static final String TAG = BitmapUtils.class.getSimpleName();
	
	/**
	 * Reusable rectangle for general internal usage
	 */
	private static final Rect EMPTY_RECT = new Rect();

	/**
	 * Used to know the max texture size allowed to be rendered
	 */
	private static int mMaxTextureSize = 0;

	/**
	 * used to save bitmaps during state save and restore so not to reload them.
	 */
	private static Pair<String, WeakReference<Bitmap>> mStateBitmap;

	private static final String FORMATS[] = { "jpg", "png", "webp" };
	
	public static final BitmapFactory.Options OPTION_RGBA8888 = new BitmapFactory.Options();
	public static final BitmapFactory.Options OPTION_A8 = new BitmapFactory.Options();
	static
	{
		// Android's Bitmap.Config.ARGB_8888 is misleading, its memory layout is RGBA, as shown in
		// JNI's macro ANDROID_BITMAP_FORMAT_RGBA_8888, and getPixel() returns ARGB format.
		OPTION_RGBA8888.inPreferredConfig = Bitmap.Config.ARGB_8888;
		OPTION_RGBA8888.inDither = false;
		OPTION_RGBA8888.inMutable = true;
		OPTION_RGBA8888.inPremultiplied = false;
		
		OPTION_A8.inPreferredConfig = Bitmap.Config.ALPHA_8;
		OPTION_A8.inDither = false;
		OPTION_A8.inMutable = true;
		OPTION_A8.inPremultiplied = false;
		
		if(BuildConfig.DEBUG && FORMATS.length > Bitmap.CompressFormat.values().length)
			throw new IndexOutOfBoundsException("More image formats need to be added to FORMATS.");
	}
	
	/**
	 * static-methods-only (utility) class
	 */
	private BitmapUtils(){}
	
	/**
	 * Rotate the given image by reading the EXIF value of the image (URI).<br>
	 * If no rotation is required the image will not be rotated.<br>
	 * New bitmap is created and the old one is recycled.
	 */
	public static RotateBitmapResult rotateBitmapByExif(Bitmap bitmap, Context context, Uri uri)
	{
		try
		{
			File file = getFileFromUri(context, uri);
			if(file.exists())
			{
				ExifInterface ei = new ExifInterface(file.getAbsolutePath());
				return rotateBitmapByExif(bitmap, ei);
			}
		}
		catch(Exception ignored)
		{
		}
		return new RotateBitmapResult(bitmap, 0);
	}

	/**
	 * Rotate the given image by given Exif value.<br>
	 * If no rotation is required the image will not be rotated.<br>
	 * New bitmap is created and the old one is recycled.
	 */
	public static RotateBitmapResult rotateBitmapByExif(Bitmap bitmap, ExifInterface exif)
	{
		int degrees;
		int orientation = exif.getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL);
		switch(orientation)
		{
		case ExifInterface.ORIENTATION_ROTATE_90:
			degrees = 90;
			break;
		case ExifInterface.ORIENTATION_ROTATE_180:
			degrees = 180;
			break;
		case ExifInterface.ORIENTATION_ROTATE_270:
			degrees = 270;
			break;
		default:
			degrees = 0;
			break;
		}
		return new RotateBitmapResult(bitmap, degrees);
	}

	/**
	 * Decode bitmap from stream using sampling to get bitmap with the requested limit.
	 */
	public static DecodeBitmapResult decodeSampledBitmap(Context context, Uri uri, int reqWidth, int reqHeight)
	{
		try
		{
			ContentResolver resolver = context.getContentResolver();

			// First decode with inJustDecodeBounds=true to check dimensions
			BitmapFactory.Options options = decodeImageForOption(resolver, uri);

			// Calculate inSampleSize
			options.inSampleSize = Math.max(
					calculateInSampleSize(options.outWidth, options.outHeight, reqWidth, reqHeight),
					calculateInSampleSizeByMaxTextureSize(options.outWidth, options.outHeight));

			// Decode bitmap with inSampleSize set
			Bitmap bitmap = decodeImage(resolver, uri, options);

			return new DecodeBitmapResult(bitmap, options.inSampleSize);

		}
		catch(Exception e)
		{
			throw new RuntimeException("Failed to load sampled bitmap: " + uri + "\r\n" + e.getMessage(), e);
		}
	}

	/**
	 * Crop image bitmap from given bitmap using the given points in the original bitmap and the
	 * given rotation.<br>
	 * if the rotation is not 0,90,180 or 270 degrees then we must first crop a larger area of the
	 * image that contains the requires rectangle, rotate and then crop again a sub rectangle.
	 */
	public static Bitmap cropBitmap(Bitmap bitmap, float[] points, int degreesRotated, boolean fixAspectRatio,
			int aspectRatioX, int aspectRatioY)
	{

		// get the rectangle in original image that contains the required cropped area (larger for
		// non rectangular crop)
		Rect rect = getRectFromPoints(points, bitmap.getWidth(), bitmap.getHeight(), fixAspectRatio, aspectRatioX,
				aspectRatioY);

		// crop and rotate the cropped image in one operation
		Matrix matrix = new Matrix();
		matrix.setRotate(degreesRotated, bitmap.getWidth() / 2, bitmap.getHeight() / 2);
		Bitmap result = Bitmap.createBitmap(bitmap, rect.left, rect.top, rect.width(), rect.height(), matrix, true);

		if(result == bitmap)
		{
			// corner case when all bitmap is selected, no worth optimizing for it
			result = bitmap.copy(bitmap.getConfig(), false);
		}

		// rotating by 0, 90, 180 or 270 degrees doesn't require extra cropping
		if(degreesRotated % 90 != 0)
		{

			// extra crop because non rectengular crop cannot be done directly on the image without
			// rotating first
			result = cropForRotatedImage(result, points, rect, degreesRotated, fixAspectRatio, aspectRatioX,
					aspectRatioY);
		}

		return result;
	}

	/**
	 * Crop image bitmap from URI by decoding it with specific width and height to down-sample if
	 * required.
	 */
	public static Bitmap cropBitmap(Context context, Uri loadedImageUri, float[] points, int degreesRotated,
			int orgWidth, int orgHeight, boolean fixAspectRatio, int aspectRatioX, int aspectRatioY, int reqWidth,
			int reqHeight)
	{

		// get the rectangle in original image that contains the required cropped area (larger for
		// non rectangular crop)
		Rect rect = getRectFromPoints(points, orgWidth, orgHeight, fixAspectRatio, aspectRatioX, aspectRatioY);

		int width = reqWidth > 0 ? reqWidth : rect.width();
		int height = reqHeight > 0 ? reqHeight : rect.height();
	
		// decode only the required image from URI, optionally sub-sampling if
		// reqWidth/reqHeight is given.
		Bitmap result = decodeSampledBitmapRegion(context, loadedImageUri, rect, width, height);

		if(result != null)
		{
			// rotate the decoded region by the required amount
			result = rotateBitmapInt(result, degreesRotated);

			// rotating by 0, 90, 180 or 270 degrees doesn't require extra cropping
			if(degreesRotated % 90 != 0)
			{
				// extra crop because non rectangular crop cannot be done directly on the image
				// without rotating first
				result = cropForRotatedImage(result, points, rect, degreesRotated, fixAspectRatio,
						aspectRatioX, aspectRatioY);
			}
		}
		else
		{

			// failed to decode region, may be skia issue, try full decode and then crop
			try
			{
				BitmapFactory.Options options = new BitmapFactory.Options();
				options.inSampleSize = calculateInSampleSize(rect.width(), rect.height(), reqWidth,
						reqHeight);

				Bitmap fullBitmap = decodeImage(context.getContentResolver(), loadedImageUri, options);
				if(fullBitmap != null)
				{
					result = cropBitmap(fullBitmap, points, degreesRotated, fixAspectRatio, aspectRatioX, aspectRatioY);
					fullBitmap.recycle();
				}
			}
			catch(Exception e)
			{
				throw new RuntimeException("Failed to load sampled bitmap: " + loadedImageUri + "\r\n" + e.getMessage(),
						e);
			}
		}

		return result;
	}

	/**
	 * Get a rectangle for the given 4 points (x0,y0,x1,y1,x2,y2,x3,y3) by finding the min/max 2
	 * points that contains the given 4 points and is a stright rectangle.
	 */
	public static Rect getRectFromPoints(float[] points, int imageWidth, int imageHeight, boolean fixAspectRatio,
			int aspectRatioX, int aspectRatioY)
	{
		int left = Math.round(Math.max(0, Math.min(Math.min(Math.min(points[0], points[2]), points[4]), points[6])));
		int top = Math.round(Math.max(0, Math.min(Math.min(Math.min(points[1], points[3]), points[5]), points[7])));
		int right = Math
				.round(Math.min(imageWidth, Math.max(Math.max(Math.max(points[0], points[2]), points[4]), points[6])));
		int bottom = Math
				.round(Math.min(imageHeight, Math.max(Math.max(Math.max(points[1], points[3]), points[5]), points[7])));

		Rect rect = new Rect(left, top, right, bottom);
		if(fixAspectRatio)
		{
			fixRectForAspectRatio(rect, aspectRatioX, aspectRatioY);
		}

		return rect;
	}

	/**
	 * Fix the given rectangle if it doesn't confirm to aspect ration rule.<br>
	 * Make sure that width and height are equal if 1:1 fixed aspect ratio is requested.
	 */
	public static void fixRectForAspectRatio(Rect rect, int aspectRatioX, int aspectRatioY)
	{
		if(aspectRatioX == aspectRatioY && rect.width() != rect.height())
		{
			if(rect.height() > rect.width())
			{
				rect.bottom -= rect.height() - rect.width();
			}
			else
			{
				rect.right -= rect.width() - rect.height();
			}
		}
	}

	/**
	 * Write the given bitmap to the given uri using the given compression.
	 */
	public static void writeBitmapToUri(Context context, Bitmap bitmap, Uri uri, Bitmap.CompressFormat compressFormat,
			int compressQuality) throws FileNotFoundException
	{
		OutputStream outputStream = null;
		try
		{
			outputStream = context.getContentResolver().openOutputStream(uri);
			bitmap.compress(compressFormat, compressQuality, outputStream);
		}
		finally
		{
			closeSafe(outputStream);
		}
	}

	// region: Private methods

	/**
	 * Decode image from uri using "inJustDecodeBounds" to get the image dimensions.
	 */
	private static BitmapFactory.Options decodeImageForOption(ContentResolver resolver, Uri uri)
			throws FileNotFoundException
	{
		InputStream stream = null;
		try
		{
			stream = resolver.openInputStream(uri);
			BitmapFactory.Options options = new BitmapFactory.Options();
			options.inJustDecodeBounds = true;
			BitmapFactory.decodeStream(stream, EMPTY_RECT, options);
			options.inJustDecodeBounds = false;
			return options;
		}
		finally
		{
			closeSafe(stream);
		}
	}

	/**
	 * Decode image from uri using given "inSampleSize", but if failed due to out-of-memory then
	 * raise the inSampleSize until success.
	 */
	private static Bitmap decodeImage(ContentResolver resolver, Uri uri, BitmapFactory.Options options)
			throws FileNotFoundException
	{
		do
		{
			InputStream stream = null;
			try
			{
				stream = resolver.openInputStream(uri);
				return BitmapFactory.decodeStream(stream, EMPTY_RECT, options);
			}
			catch(OutOfMemoryError e)
			{
				options.inSampleSize *= 2;
			}
			finally
			{
				closeSafe(stream);
			}
		} while(options.inSampleSize <= 512);
		throw new RuntimeException("Failed to decode image: " + uri);
	}

	/**
	 * Decode specific rectangle bitmap from stream using sampling to get bitmap with the requested
	 * limit.
	 */
	private static Bitmap decodeSampledBitmapRegion(Context context, Uri uri, Rect rect, int reqWidth, int reqHeight)
	{
		InputStream stream = null;
		BitmapRegionDecoder decoder = null;
		try
		{
			BitmapFactory.Options options = new BitmapFactory.Options();
			options.inSampleSize = calculateInSampleSize(rect.width(), rect.height(), reqWidth, reqHeight);

			stream = context.getContentResolver().openInputStream(uri);
			decoder = BitmapRegionDecoder.newInstance(stream, false);
			do
			{
				try
				{
					return decoder.decodeRegion(rect, options);
				}
				catch(OutOfMemoryError e)
				{
					options.inSampleSize *= 2;
				}
			} while(options.inSampleSize <= 512);
		}
		catch(Exception e)
		{
			throw new RuntimeException("Failed to load sampled bitmap: " + uri + "\r\n" + e.getMessage(), e);
		}
		finally
		{
			closeSafe(stream);
			if(decoder != null)
			{
				decoder.recycle();
			}
		}
		return null;
	}

	/**
	 * Special crop of bitmap rotated by not stright angle, in this case the original crop bitmap
	 * contains parts beyond the required crop area, this method crops the already cropped and
	 * rotated bitmap to the final rectangle.<br>
	 * Note: rotating by 0, 90, 180 or 270 degrees doesn't require extra cropping.
	 */
	private static Bitmap cropForRotatedImage(Bitmap bitmap, float[] points, Rect rect, int degreesRotated,
			boolean fixAspectRatio, int aspectRatioX, int aspectRatioY)
	{
		if(degreesRotated % 90 != 0)
		{

			int adjLeft = 0, adjTop = 0, width = 0, height = 0;
			double rads = Math.toRadians(degreesRotated);
			int compareTo = degreesRotated < 90 || (degreesRotated > 180 && degreesRotated < 270) ? rect.left
					: rect.right;
			for(int i = 0; i < points.length; i += 2)
			{
				if(((int) points[i]) == compareTo)
				{
					adjLeft = (int) Math.abs(Math.sin(rads) * (rect.bottom - points[i + 1]));
					adjTop = (int) Math.abs(Math.cos(rads) * (points[i + 1] - rect.top));
					width = (int) Math.abs((points[i + 1] - rect.top) / Math.sin(rads));
					height = (int) Math.abs((rect.bottom - points[i + 1]) / Math.cos(rads));
					break;
				}
			}

			rect.set(adjLeft, adjTop, adjLeft + width, adjTop + height);
			if(fixAspectRatio)
			{
				fixRectForAspectRatio(rect, aspectRatioX, aspectRatioY);
			}

			Bitmap bitmapTmp = bitmap;
			bitmap = Bitmap.createBitmap(bitmap, rect.left, rect.top, rect.width(), rect.height());
			bitmapTmp.recycle();
		}
		return bitmap;
	}

	/**
	 * Calculate the largest inSampleSize value that is a power of 2 and keeps both height and width
	 * smaller than max texture size allowed for the device.
	 */
	private static int calculateInSampleSizeByMaxTextureSize(int width, int height)
	{
		int inSampleSize = 1;
		if(mMaxTextureSize == 0)
			mMaxTextureSize = OpenGLUtils.getMaxTextureSize();
		
		if(mMaxTextureSize > 0)
		{
			while((height / inSampleSize) > mMaxTextureSize || (width / inSampleSize) > mMaxTextureSize)
			{
				inSampleSize *= 2;
			}
		}
		return inSampleSize;
	}

	/**
	 * Get {@link File} object for the given Android URI.<br>
	 * Use content resolver to get real path if direct path doesn't return valid file.
	 */
	private static File getFileFromUri(Context context, Uri uri)
	{
		// first try by direct path
		File file = new File(uri.getPath());
		if(file.exists())
		{
			return file;
		}

		// try reading real path from content resolver (gallery images)
		Cursor cursor = null;
		try
		{
			String[] proj = { MediaStore.Images.Media.DATA };
			cursor = context.getContentResolver().query(uri, proj, null, null, null);
			int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
			cursor.moveToFirst();
			String realPath = cursor.getString(column_index);
			file = new File(realPath);
		}
		catch(Exception ignored)
		{
		}
		finally
		{
			closeSafe(cursor);
		}

		return file;
	}

	/**
	 * Rotate the given bitmap by the given degrees.<br>
	 * New bitmap is created and the old one is recycled.
	 */
	private static Bitmap rotateBitmapInt(Bitmap bitmap, int degrees)
	{
		if(degrees > 0)
		{
			Matrix matrix = new Matrix();
			matrix.setRotate(degrees);
			Bitmap newBitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, false);
			if(newBitmap != bitmap)
			{
				bitmap.recycle();
			}
			return newBitmap;
		}
		else
		{
			return bitmap;
		}
	}

	public static Bitmap transformBitmap(@NonNull Bitmap bitmap, @NonNull Matrix matrix)
	{
		if(matrix.isIdentity())
			return bitmap;
		
		try
		{
			return Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
		}
		catch(OutOfMemoryError error)
		{
			Log.e(TAG, "transformBitmap: ", error);
		}
		return bitmap;
	}

	public static void decodeBitmapInBackground(@NonNull Context context, @NonNull Uri uri, @Nullable Uri outputUri,
			int requiredWidth, int requiredHeight, BitmapLoadTask.BitmapLoadCallback loadCallback)
	{

		new BitmapLoadTask(context, uri, outputUri, requiredWidth, requiredHeight, loadCallback).execute();
	}

	public static int calculateInSampleSize(@NonNull BitmapFactory.Options options, int reqWidth, int reqHeight)
	{
		// Raw height and width of image
		final int height = options.outHeight;
		final int width = options.outWidth;
		int inSampleSize = 1;

		if(height > reqHeight || width > reqWidth)
		{
			// Calculate the largest inSampleSize value that is a power of 2 and keeps both
			// height and width lower or equal to the requested height and width.
			while((height / inSampleSize) > reqHeight || (width / inSampleSize) > reqWidth)
			{
				inSampleSize *= 2;
			}
		}
		return inSampleSize;
	}

	public static int getExifOrientation(@NonNull Context context, @NonNull Uri imageUri)
	{
		int orientation = ExifInterface.ORIENTATION_UNDEFINED;
		try
		{
			InputStream stream = context.getContentResolver().openInputStream(imageUri);
			if(stream == null)
			{
				return orientation;
			}
			orientation = new ImageHeaderParser(stream).getOrientation();
			BitmapUtils.closeSafe(stream);
		}
		catch(IOException e)
		{
			Log.e(TAG, "getExifOrientation: " + imageUri.toString(), e);
		}
		return orientation;
	}

	public static int exifToDegrees(int exifOrientation)
	{
		int rotation;
		switch(exifOrientation)
		{
		case ExifInterface.ORIENTATION_ROTATE_90:
		case ExifInterface.ORIENTATION_TRANSPOSE:
			rotation = 90;
			break;
		case ExifInterface.ORIENTATION_ROTATE_180:
		case ExifInterface.ORIENTATION_FLIP_VERTICAL:
			rotation = 180;
			break;
		case ExifInterface.ORIENTATION_ROTATE_270:
		case ExifInterface.ORIENTATION_TRANSVERSE:
			rotation = 270;
			break;
		default:
			rotation = 0;
		}
		return rotation;
	}

	public static int exifToTranslation(int exifOrientation)
	{
		int translation;
		switch(exifOrientation)
		{
		case ExifInterface.ORIENTATION_FLIP_HORIZONTAL:
		case ExifInterface.ORIENTATION_FLIP_VERTICAL:
		case ExifInterface.ORIENTATION_TRANSPOSE:
		case ExifInterface.ORIENTATION_TRANSVERSE:
			translation = -1;
			break;
		default:
			translation = 1;
		}
		return translation;
	}

	/**
	 * This method calculates maximum size of both width and height of bitmap. It is twice the
	 * device screen diagonal for default implementation (extra quality to zoom image). Size cannot
	 * exceed max texture size.
	 *
	 * @return - max bitmap size in pixels.
	 */
	@SuppressWarnings({ "SuspiciousNameCombination", "deprecation" })
	public static int calculateMaxBitmapSize(@NonNull Context context)
	{
		WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
		Display display = wm.getDefaultDisplay();

		Point size = new Point();
		int width, height;
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR2)
		{
			display.getSize(size);
			width = size.x;
			height = size.y;
		}
		else
		{
			width = display.getWidth();
			height = display.getHeight();
		}

		// Twice the device screen diagonal as default
		int maxBitmapSize = (int) Math.sqrt(Math.pow(width, 2) + Math.pow(height, 2));

		// Check for max texture size via Canvas
		Canvas canvas = new Canvas();
		final int maxCanvasSize = Math.min(canvas.getMaximumBitmapWidth(), canvas.getMaximumBitmapHeight());
		if(maxCanvasSize > 0)
			maxBitmapSize = Math.min(maxBitmapSize, maxCanvasSize);

		// Check for max texture size via GL
		final int maxTextureSize = OpenGLUtils.getMaxTextureSize();
		if(maxTextureSize > 0)
			maxBitmapSize = Math.min(maxBitmapSize, maxTextureSize);

		Log.d(TAG, "maxBitmapSize: " + maxBitmapSize);
		return maxBitmapSize;
	}

	/**
	 * Close the given closeable object (Stream) in a safe way: check if it is null and catch-log
	 * exception thrown.
	 *
	 * @param closeable the closable object to close
	 */
	public static void closeSafe(Closeable closeable)
	{
		if(closeable == null)
			return;
	
		try
		{
			closeable.close();
		}
		catch(IOException ignored)
		{
		}
	}

	/**
	 * The result of
	 * {@link #decodeSampledBitmap(android.content.Context, android.net.Uri, int, int)}.
	 */
	public static final class DecodeBitmapResult
	{

		/**
		 * The loaded bitmap
		 */
		public final Bitmap bitmap;

		/**
		 * The sample size used to load the given bitmap
		 */
		public final int sampleSize;

		public DecodeBitmapResult(Bitmap bitmap, int sampleSize)
		{
			this.sampleSize = sampleSize;
			this.bitmap = bitmap;
		}
	}

	public static byte[] bitmapToByteArray(final Bitmap bitmap, final boolean needRecycle)
	{
		ByteArrayOutputStream output = new ByteArrayOutputStream();
		bitmap.compress(CompressFormat.PNG, 100, output);
		if(needRecycle)
			bitmap.recycle();

		byte[] result = output.toByteArray();
		try
		{
			output.close();
		}
		catch(IOException e)
		{
			e.printStackTrace();
		}

		return result;
	}

	/**
	 * Calculate the largest inSampleSize value that is a power of 2 and keeps both height and width
	 * larger than the requested height and width.
	 */
	private static int calculateInSampleSize(int height, int width, int reqWidth, int reqHeight)
	{
		int inSampleSize = 1;

		if(height > reqHeight || width > reqWidth)
		{
			final int halfHeight = height / 2;
			final int halfWidth = width / 2;

			// Calculate the largest inSampleSize value that is a power of 2 and keeps both
			// height and width larger than the requested height and width.
			while((halfHeight / inSampleSize) > reqHeight && (halfWidth / inSampleSize) > reqWidth)
			{
				// Note: A power of two value is calculated because the decoder uses a final value
				// by rounding down to the nearest power of two, as per the inSampleSize documentation.
				inSampleSize *= 2;
			}
		}

		return inSampleSize;
	}

	// https://developer.android.com/training/displaying-bitmaps/load-bitmap.html
	public static Bitmap decodeSampledBitmap(Resources res, int resId, int reqWidth, int reqHeight)
	{
		// First decode with inJustDecodeBounds = true to check dimensions
		final BitmapFactory.Options options = new BitmapFactory.Options();
		options.inJustDecodeBounds = true;
		BitmapFactory.decodeResource(res, resId, options);

		// Calculate inSampleSize
		options.inSampleSize = calculateInSampleSize(options.outWidth, options.outHeight, reqWidth, reqHeight);

		// Decode bitmap with inSampleSize set
		options.inJustDecodeBounds = false;
		return BitmapFactory.decodeResource(res, resId, options);
	}

	public static Bitmap decodeSampledBitmap(String filename, int reqWidth, int reqHeight)
	{
		// First decode with inJustDecodeBounds = true to check dimensions
		final BitmapFactory.Options options = new BitmapFactory.Options();
		options.inJustDecodeBounds = true;
		BitmapFactory.decodeFile(filename, options);

		// Calculate inSampleSize
		options.inSampleSize = calculateInSampleSize(options.outWidth, options.outHeight, reqWidth, reqHeight);

		// Decode bitmap with inSampleSize set
		options.inJustDecodeBounds = false;
		return BitmapFactory.decodeFile(filename, options);
	}

	public static Rect getOpaqueRegion(final Bitmap image)
	{
		return getOpaqueRegion(image, 0);
	}

	public static Rect getOpaqueRegion(final Bitmap image, int threshold)
	{
		if(image == null)
			throw new NullPointerException("image is null");

		final int width = image.getWidth();
		final int height = image.getHeight();
		Rect rect = new Rect(width, height, 0, 0);
		for(int j = 0; j < height; ++j)
		{
			for(int i = 0; i < width; ++i)
			{
				int color = image.getPixel(i, j);
				int alpha = Color.alpha(color);
				if(alpha > threshold)
					rect.union(i, j);
			}
		}

		return rect;
	}

	/**
	 * turn true color image into gray image.
	 * 
	 * @param image
	 * @return gray image, format ARGB_8888 as before.
	 */
	public Bitmap gray(Bitmap image)
	{
		Bitmap result = Bitmap.createBitmap(image.getWidth(), image.getHeight(), Bitmap.Config.ARGB_8888);
		Canvas canvas = new Canvas(result);
		Paint paint = new Paint();
		ColorMatrix transform = new ColorMatrix();
		transform.setSaturation(0);
		ColorMatrixColorFilter filter = new ColorMatrixColorFilter(transform);
		paint.setColorFilter(filter);
		canvas.drawBitmap(image, 0, 0, paint);
		return result;
	}

	// convert View to Bitmap, it can be archived to SD card later.
	public static Bitmap getBitmapFromView(View view)
	{
		// force the view size to be calculated in order to get the correct view size
		view.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
		int width = view.getMeasuredWidth();
		int height = view.getMeasuredHeight();
		Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);

		Canvas canvas = new Canvas(bitmap);
		view.layout(0, 0, width, height);
		view.draw(canvas);
		return bitmap;
	}

	public static String getImageSuffix(Bitmap.CompressFormat format)
	{
		return FORMATS[format.ordinal()];
	}
	
	public static boolean saveImage(Bitmap bitmap, File file, Bitmap.CompressFormat format, int quality)
	{
		try
		{
			FileOutputStream out = new FileOutputStream(file);
			bitmap.compress(format, quality, out);
//			bitmap.recycle();
			out.flush();
			out.close();
		}
		catch(FileNotFoundException e)
		{
			e.printStackTrace();
			return false;
		}
		catch(IOException e)
		{
			e.printStackTrace();
			return false;
		}
		
		return true;
	}
	
	public static Bitmap getBitmapFromUri(Context context, final Uri uri, BitmapFactory.Options options)
	{
//		Uri uri = data.getData();
		String scheme = uri.getScheme();
		InputStream inputStream = null;
		try
		{
			if(scheme != null && (scheme.startsWith("http") || scheme.startsWith("https")))
				inputStream = new URL(uri.toString()).openStream();
			else
				inputStream = context.getContentResolver().openInputStream(uri);
		}
		catch(IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

//		if(inputStream == null)  return null;  // this is handled in decodeStream()
		return BitmapFactory.decodeStream(inputStream, null, options);
	}
	
	public static synchronized String createUniversalFilename()
	{
		// generate a unique filename based on time
		// yyyy-MM-dd HH:mm:ss.SSSZ 1970-01-01 00:00:00.000+0000
		SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd-HHmmss", Locale.getDefault());
//		sdf.setTimeZone(TimeZone.getTimeZone("UTC"));  // TODO take care of time zone?
		String time = sdf.format(new Date());
		
		return time;
	}
	
	public static synchronized File createImageFile(String name, String category, Bitmap.CompressFormat format)
	{
//		String root = Environment.getExternalStorageDirectory().toString();
		String path = App.getWorkingDirectory();
		if(category != null)
			path += File.separator + category;
		File dir = new File(path);
		dir.mkdirs();
		
		final String suffix = FORMATS[format.ordinal()];
		String filename = String.format("IMG_%s.%s", name, suffix);
		File file = new File(dir, filename);
		
		if(file.exists())  // edge case, unlikely happen
		{
			for(int i = 1; i < Integer.MAX_VALUE; ++i)
			{
				filename = String.format("IMG_%s (%d).%s", name, i, suffix);
				file = new File(dir, filename);
				if(!file.exists())
					break;
			}
		}
		
		return file;
	}
	
	// bilinear interpolation
	public static int getColor(Bitmap bitmap, float x, float y)
	{
		assert (0 <= x && x < bitmap.getWidth());
		assert (0 <= y && y < bitmap.getHeight());

		int x0 = (int) x;
		int y0 = (int) y;

		int c00 = bitmap.getPixel(x0, y0);
		int c01 = bitmap.getPixel(x0, y0 + 1);
		int c10 = bitmap.getPixel(x0 + 1, y0);
		int c11 = bitmap.getPixel(x0 + 1, y0 + 1);

		final float wx = x - x0, one_minus_wx = 1.0f - wx;
		final float wy = y - y0, one_minus_wy = 1.0f - wy;

		float r0 = Color.red(c00) * wx + Color.red(c01) * one_minus_wx;
		float r1 = Color.red(c10) * wx + Color.red(c11) * one_minus_wx;
		float r = r0 * wy + r1 * one_minus_wy;

		float g0 = Color.green(c00) * wx + Color.green(c01) * one_minus_wx;
		float g1 = Color.green(c10) * wx + Color.green(c11) * one_minus_wx;
		float g = g0 * wy + g1 * one_minus_wy;

		float b0 = Color.blue(c00) * wx + Color.blue(c01) * one_minus_wx;
		float b1 = Color.blue(c10) * wx + Color.blue(c11) * one_minus_wx;
		float b = b0 * wy + b1 * one_minus_wy;

		int red = Math.round(r), green = Math.round(g), blue = Math.round(b);
		if(bitmap.getConfig() == Bitmap.Config.RGB_565)
			return Color.rgb(red, green, blue);

		float a0 = Color.alpha(c00) * wx + Color.alpha(c01) * one_minus_wx;
		float a1 = Color.alpha(c10) * wx + Color.alpha(c11) * one_minus_wx;
		float a = a0 * wy + a1 * one_minus_wy;

		int alpha = Math.round(a);
		return Color.argb(red, green, blue, alpha);
	}

	/**
	 * http://stackoverflow.com/questions/3035692/how-to-convert-a-drawable-to-a-bitmap
	 * 
	 * @param drawable
	 * @return
	 */
	public static Bitmap drawableToBitmap(@NonNull Drawable drawable)
	{
		Bitmap bitmap = null;

		if(drawable instanceof BitmapDrawable)
		{
			BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
			if(bitmapDrawable.getBitmap() != null)
				return bitmapDrawable.getBitmap();
		}

		int width = drawable.getIntrinsicWidth();
		int height = drawable.getIntrinsicHeight();
		if(width <= 0 || height <= 0) // Single color bitmap will be created of 1x1 pixel
			bitmap = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888);
		else
			bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);

		Canvas canvas = new Canvas(bitmap);
		drawable.setBounds(0, 0, width, height);
		drawable.draw(canvas);
		return bitmap;
	}
	
	/**
	 * It's like BitmapFactory#decodeFile(String, Options), but takes EXIF information into account.
	 * 
	 * @param pathName Complete path name for the file to be decoded.
	 * @param opts     Image Decoding options.
	 * @return         The decoded bitmap.
	 * 
	 * @see BitmapFactory#decodeFile(String, Options)
	 */
	public static Bitmap decodeFile(@NonNull String pathName, @Nullable Options opts)
	{
		Bitmap bitmap = BitmapFactory.decodeFile(pathName, opts);
		
		final String lowerPathName = pathName.toLowerCase();
		boolean isJPEG = lowerPathName.endsWith(".jpg") || lowerPathName.endsWith(".jpeg");
		if(isJPEG)
		{
			Matrix matrix = getOrientationFromJPEGFile(pathName);
			Bitmap rotated = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
			
			bitmap.recycle();
			return rotated;
		}
		
		return bitmap;
	}
	
	public static Matrix getOrientationFromJPEGFile(String path)
	{
		int orientation = ExifInterface.ORIENTATION_UNDEFINED;
		Matrix matrix = new Matrix();  // identity matrix
		try
		{
			ExifInterface exif = new ExifInterface(path);
			orientation = exif.getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_UNDEFINED); 
		}
		catch(IOException e)
		{
			e.printStackTrace();
			return matrix;
		}
		
		switch (orientation)
		{
		case ExifInterface.ORIENTATION_FLIP_HORIZONTAL:
			matrix.setScale(-1, 1);
			break;
		case ExifInterface.ORIENTATION_ROTATE_180:
			matrix.setRotate(180);
			break;
		case ExifInterface.ORIENTATION_FLIP_VERTICAL:
			matrix.setRotate(180);
			matrix.postScale(-1, 1);
			break;
		case ExifInterface.ORIENTATION_TRANSPOSE:
			matrix.setRotate(90);
			matrix.postScale(-1, 1);
			break;
		case ExifInterface.ORIENTATION_ROTATE_90:
			matrix.setRotate(90);
			break;
		case ExifInterface.ORIENTATION_TRANSVERSE:
			matrix.setRotate(-90);
			matrix.postScale(-1, 1);
			break;
		case ExifInterface.ORIENTATION_ROTATE_270:
			matrix.setRotate(-90);
			break;
		case ExifInterface.ORIENTATION_NORMAL:
//			return bitmap;
//			[[fallthrough]];
		default:
			break;
		}
		
		return matrix;
	}
	
	/**
	 * The result of
	 * {@link #rotateBitmapByExif(android.graphics.Bitmap, android.media.ExifInterface)}.
	 */
	public static final class RotateBitmapResult
	{

		/**
		 * The loaded bitmap
		 */
		public final Bitmap bitmap;

		/**
		 * The degrees the image was rotated
		 */
		public final int degrees;

		RotateBitmapResult(Bitmap bitmap, int degrees)
		{
			this.bitmap = bitmap;
			this.degrees = degrees;
		}
	}

}