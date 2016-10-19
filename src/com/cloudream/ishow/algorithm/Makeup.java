package com.cloudream.ishow.algorithm;

import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.util.BitmapUtils;

import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.Shader;

public class Makeup extends BitmapWrapper
{
	public static enum BlushShape
	{
		DEFAULT,
		DISK,
		OVAL,
		TRIANGLE,
		HEART,
		SEAGULL,
	};
	
	private Feature feature;
	
	public Makeup(Bitmap image, final PointF points[])
	{
		super(image);
		
		feature = new Feature(image, points);
	}

	public Bitmap mark()
	{
		return feature.mark();
	}
	
	/**
	 * Lip gloss and lipstick can make your lips look fuller, glossier and better! See how to 
	 * <a href="http://www.wikihow.com/Apply-Lip-Color">apply lip color</a>.
	 * 
	 * @param color an #ARGB integer, @see #Color
	 * @param amount 0 means no change, and 1 means fully applied.
	 */
	public void applyLipColor(int color, float amount)
	{
		final PointF position = new PointF();

		Path path = feature.getLipPath();
		Bitmap mask = Effect.createMask(path, color, 8, position);
		mask = Effect.tint(mask, color, amount);

//		bmp_modified = Effect.tint(bmp_step, color, t);
//		Bitmap mask, PointF mask_poisition,
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC));
//		bmp_modified = bmp_step.copy(Bitmap.Config.ARGB_8888, true);
		final Rect rect = new Rect(Math.round(position.x), Math.round(position.y), mask.getWidth(), mask.getHeight());
//		canvas.drawColor(Color.TRANSPARENT);
		Canvas canvas = new Canvas(bmp_step);
		canvas.drawBitmap(bmp_stop, 0, 0, null);
//		canvas.drawBitmap(bmp_step, rect, rect, paint);
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));
		canvas.drawBitmap(mask, position.x, position.y, paint);
	}
	
	public void applyBlush(BlushShape shape, int color, float amount)
	{
		PointF points[] = feature.getFeaturePoints();
		int _color = BitmapUtils.bgra2rgba(color);
		nativeApplyBlush(bmp_step, bmp_stop, points, shape.ordinal(), _color, amount);
	}
	
	private Bitmap blendEyeBrow(Bitmap image, final Bitmap eye_brow, float amount)
	{
		PointF points[] = feature.getFeaturePoints();
		
		Bitmap result = image.copy(Bitmap.Config.ARGB_8888, true);
		int tile_width = Math.round(points[25].x - points[20].x);
		int tile_height = 8;
		int tile_x = Math.round(points[20].x);
		int tile_y = Math.round(points[20].y - tile_height);
		Bitmap pattern = Bitmap.createBitmap(image, tile_x, tile_y, tile_width, tile_height);
		BitmapShader shader = new BitmapShader(pattern, Shader.TileMode.MIRROR, Shader.TileMode.MIRROR);

		Path path_eye_brow_r = Feature.getEyeBrowPath(points, 22, 21, 20 ,25, 24 ,23);
		Path path_eye_brow_l = Feature.getEyeBrowPath(points, 29, 28, 27, 26, 31, 30);
		
		// TODO not so good, need to come up with a method skin color
		int x = Math.round((points[20].x + points[27].x)/2);
		int y = Math.round((points[20].y + points[27].y)/2);
		int color = image.getPixel(x, y);
		float width = (points[23].y - points[21].y)/2;
		Canvas canvas = new Canvas(result);
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		paint.setColor(color);
		paint.setStyle(Paint.Style.FILL_AND_STROKE);
		paint.setStrokeWidth(width);
		paint.setShader(shader);
		paint.setMaskFilter(new BlurMaskFilter(tile_height*2, BlurMaskFilter.Blur.NORMAL));
		canvas.drawPath(path_eye_brow_r, paint);
		canvas.drawPath(path_eye_brow_l, paint);

//		nativeBlendEyeBrow(result, eye_brow, points, null/*eye_brow_points*/, amount);
		return result;
	}
	
	private static native void nativeApplyLipColor(int color, float amount);
	private static native void nativeApplyBlush(Bitmap dst, Bitmap src, final PointF points[], int shape, int color, float amount);
	
	private static native void nativeBlendIris(Bitmap image, final Bitmap iris, final PointF points[], float amount);
	private static native void nativeBlendIris2(Bitmap image, final Bitmap iris, final Bitmap iris_mask, final PointF points[], int color, float amount);
	private static native void nativeBlendBlusher(Bitmap image, final Bitmap blusher, final PointF points[], int color, float amount);
	private static native void nativeBlendEyeBrow(Bitmap image, final Bitmap eye_brow, final PointF points[], final PointF[] eye_brow_points, float amount);
	private static native void nativeBlendEyeLash(Bitmap image, final Bitmap eye_lash, final PointF points[], int color, float amount);
	
}
