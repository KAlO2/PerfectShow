package com.cloudream.ishow.algorithm;

import com.cloudream.ishow.BuildConfig;

import android.graphics.Bitmap;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.RectF;
import android.support.annotation.Nullable;
import android.widget.Toast;
import android.graphics.Paint.Style;



public class Effect
{
	private static final boolean NATIVE = false;
	private final static ColorMatrix matrix = new ColorMatrix();
	
	public enum DistortionType
	{
		SHRINK_GROW,
		SWIRL_CW_CCW,
		SMEAR,
	}

	public enum SelectCriterion
	{
		COMPOSITE,

		RED,
		GREEN,
		BLUE,
		ALPHA,

		HUE,
		SATURATION,
		VALUE,
	};
	
	public static void applyWhirlPinch(Bitmap image, Point pivot, float whirl, float pinch, float radius)
	{
		if(image == null || image.getConfig() != Bitmap.Config.ARGB_8888)
			throw new IllegalArgumentException("bad image");  // 0xBAD13A9E
		
		nativeApplyWhirlPinch(image, pivot, whirl, pinch, radius);
	}
	
	public static void applyWhirlPinch(Bitmap image, float whirl, float pinch, float radius)
	{
		if(image == null || image.getConfig() != Bitmap.Config.ARGB_8888)
			throw new IllegalArgumentException("bad image");  // 0xBAD13A9E
		nativeApplyWhirlPinch2(image, whirl, pinch, radius);
	}
	
	public static void distort(Bitmap image, PointF point0, PointF point1, float strength, DistortionType type)
	{
		if(image == null || image.getConfig() != Bitmap.Config.ARGB_8888)
			throw new IllegalArgumentException("bad image");  // 0xBAD13A9E
		
		nativeDistort(image, point0, point1, strength, type.ordinal());
	}
	
	public static void catmullRomSpline(PointF result, float t, PointF p0, PointF p1, PointF p2, PointF p3)
	{
		if(BuildConfig.DEBUG && (result == null || p0 == null || p1 == null || p2 == null || p3 == null))
			throw new NullPointerException("PointF is null");
		
		nativeCatmullRomSpline(result, t, p0, p1, p2, p3);
	}
	
	/**
	 * Tone is a color term commonly used by painters. 
	 * tone a bitmap with specified color, it's like mixing the color pixel by pixel,
	 * the color resulting in layering a color dst RGB with a src color RGB is:
	 * 
	 * <pre>
	 * new_R = dst_R + (src_R - dst_R) * amount
	 * new_G = dst_G + (src_G - dst_G) * amount
	 * new_B = dst_B + (src_B - dst_B) * amount
	 * </pre>
	 * 
	 * @see {@link https://en.wikipedia.org/wiki/Tints_and_shades}
	 * 
	 * @param bitmap
	 * @param color RGB
	 * @param amount color blending amount, with 0 being unchangeable, with 1 being the pure color.
	 * @return
	 */
	public static Bitmap tone(Bitmap bitmap, int color, float amount)
	{
		if(BuildConfig.DEBUG && (amount < 0.0f || amount > 1.0f))
			throw new IllegalArgumentException("amount is out of range [0, 1]");

		if(NATIVE)
		{
			// FIXME work not as expected
			Bitmap result = bitmap.copy(Bitmap.Config.ARGB_8888, true);
			nativeTone(result, color, amount);
			return result;
		}
		else
		{
			final float c = 1.0f - amount;
			matrix.setScale(c, c, c, 1.0f);
			
			final float array[] = matrix.getArray();
			array[ 4] = Color.red(color)   * amount;
			array[ 9] = Color.green(color) * amount;
			array[14] = Color.blue(color)  * amount;
//			array[19] = Color.alpha(color) * amount;
			
			ColorMatrixColorFilter filter = new ColorMatrixColorFilter(matrix);
			Paint paint = new Paint();  // (Paint.ANTI_ALIAS_FLAG);
			paint.setColorFilter(filter);
			
			Bitmap result = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Bitmap.Config.ARGB_8888);
//			result.eraseColor(color & 0x00ffffff);
			Canvas canvas = new Canvas(result);
			canvas.drawBitmap(bitmap, 0, 0, paint);
			
			return result;
		}
	}
	
	public static Bitmap tone(Bitmap bitmap, int color)
	{
		float amount = Color.alpha(color)/255.0f;
		return tone(bitmap, color, amount);
	}
	
	/**
	 * http://stackoverflow.com/questions/6615002/given-an-rgb-value-how-do-i-create-a-tint-or-shade
	 * A shade is produced by "darkening" a hue or "adding black", to shade:
	 * 
	 * <pre>
	 * R' = R * (1 - shade_factor)
	 * G' = G * (1 - shade_factor)
	 * B' = B * (1 - shade_factor)
	 * </pre>
	 * 
	 * @param bitmap
	 * @param color
	 * @param amount
	 */
	public static Bitmap shade(Bitmap bitmap, float amount)
	{
		return tone(bitmap, Color.BLACK, amount);
	}
	
	/**
	 * A tint is produced by "lightening" a hue or "adding white", to tint:
	 * 
	 * <pre>
	 * R' = R + (255 - R) * tint_factor
	 * G' = G + (255 - G) * tint_factor
	 * B' = B + (255 - B) * tint_factor
	 * </pre>
	 * 
	 * @param bitmap
	 * @param color
	 * @param amount
	 */
	public static Bitmap tint(Bitmap bitmap, int color, float amount)
	{
		return tone(bitmap, Color.WHITE, amount);
	}
	
/*		
	int width = 640, height = 640;
	Path path = new Path();
	path.moveTo(160, 160);
	path.lineTo(160, 480);
	path.lineTo(480, 480);
	path.lineTo(480, 160);
	path.close();  //	path.lineTo(160, 160);
	Bitmap bitmap = Effect.createMask(width, height, path, Color.RED, 80);
	SettingsActivity.saveImage(this, null, bitmap);
*/	
	public static Bitmap createMask(int width, int height, final Path path, int color, int blurRadius)
	{
		Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);  // mutable
		Canvas canvas = new Canvas(bitmap);
		canvas.drawColor(Color.TRANSPARENT);
		
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
//		https://github.com/chiuki/android-graphics-demo/tree/master/app/src/main/java/com/sqisland/android/graphics_demo
//		 if(Build.VERSION.SDK_INT >= 11)
//			 view.setLayerType(View.LAYER_TYPE_SOFTWARE, null);
		paint.setMaskFilter(new BlurMaskFilter(blurRadius, BlurMaskFilter.Blur.NORMAL));
		paint.setColor(color);
		paint.setStyle(Style.FILL);
		
		canvas.drawPath(path, paint);
		return bitmap;
	}
	
	/**
	 * 
	 * @param path [in]
	 * @param color [in]
	 * @param blur_radius [in]
	 * @param position [out]
	 * @return
	 */
	public static Bitmap createMask(final Path path, int color, float blur_radius, @Nullable PointF position)
	{
		if(path == null || path.isEmpty())
			return null;
		
		RectF bounds = new RectF();
		path.computeBounds(bounds, true);
//		final float left = bounds.left, top = bounds.top;
		bounds.inset(-blur_radius, -blur_radius);
		
		int width = (int)bounds.width();
		int height = (int)bounds.height();
		Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);  // mutable
		Canvas canvas = new Canvas(bitmap);
		canvas.drawColor(Color.TRANSPARENT);
		
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		paint.setMaskFilter(new BlurMaskFilter(blur_radius, BlurMaskFilter.Blur.NORMAL));
		paint.setColor(color);
		paint.setStyle(Style.FILL);
		path.offset(-bounds.left, -bounds.top);
		canvas.drawPath(path, paint);
		
		if(position != null)
		{
			position.x = bounds.left;
			position.y = bounds.top;
		}
			
		return bitmap;
	}
	
	public static Bitmap grayToAlpha(final Bitmap bitmap)
	{
		if(bitmap == null)
			return null;
		
//		Bitmap result = bitmap.copy(Bitmap.Config.ARGB_8888, true);
		Bitmap result = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Bitmap.Config.ARGB_8888);
		result.setHasAlpha(true);
		nativeGrayToAlpha(bitmap, result);
		return result;
	}
	
	public static Bitmap colorToAlpha(int color, final Bitmap bitmap)
	{
		if(bitmap == null)
			return null;
		
		Bitmap result = bitmap.copy(Bitmap.Config.ARGB_8888, true);
		nativeColorToAlpha(color, bitmap, result);
		return result;
	}

	public static Bitmap selectContiguousRegionByColor(Bitmap image, int color, SelectCriterion criterion,
			float threshold, boolean antialias, boolean select_transparent)
	{
		Bitmap mask = Bitmap.createBitmap(image.getWidth(), image.getHeight(), Bitmap.Config.ALPHA_8);
		nativeSelectContiguousRegionByColor(mask, image, color, criterion.ordinal(), threshold, antialias, select_transparent);
		return mask;
	}
	
	private static final DashPathEffect DASH_PATH_EFFECT = new DashPathEffect(new float[] {5, 5}, 0.0f);
	public static Bitmap markSelection(Bitmap bitmap, Bitmap mask, int color)
	{
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		paint.setColor(color);
		paint.setStyle(Style.STROKE);
		paint.setPathEffect(DASH_PATH_EFFECT);
		
		if(!bitmap.isMutable())
			bitmap = bitmap.copy(bitmap.getConfig(), true);
		Canvas canvas = new Canvas(bitmap);
		canvas.drawBitmap(mask, 0, 0, paint);
		return bitmap;
	}
	
	// native functions might as well not have the same name, or the parameter's full qualified type will join in name mangling.
	// native_function => native_1function
	private static native void nativeApplyWhirlPinch(Bitmap image, Point pivot, float whirl, float pinch, float radius);
	private static native void nativeApplyWhirlPinch2(Bitmap image, float whirl, float pinch, float radius);

	private static native void nativeDistort(Bitmap image, PointF point0, PointF point1, float strength, int type);
	
	private static native void nativeCatmullRomSpline(PointF result, float t, PointF p0, PointF p1, PointF p2, PointF p3);
	
	private static native void nativeTone(Bitmap bitmap, int color, float amount);
	
	private static native void nativeGrayToAlpha(Bitmap src, Bitmap dst);
	private static native void nativeColorToAlpha(int color, Bitmap src, Bitmap dst);
	
	private static native void nativeSelectContiguousRegionByColor(Bitmap mask, Bitmap image, int color, int select_criterion,
			float threshold, boolean antialias, boolean select_transparent);
	
	static
	{
		// https://developer.android.com/ndk/guides/cpp-support.html
//		System.loadLibrary("c++_shared");
		System.loadLibrary("opencv_java3");
		System.loadLibrary("venus");
	}
}
