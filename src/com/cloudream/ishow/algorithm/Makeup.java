package com.cloudream.ishow.algorithm;

import com.cloudream.ishow.util.ColorUtils;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.support.annotation.NonNull;
import android.util.Log;

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

	public Bitmap markFeaturePoints()
	{
		return feature.mark();
	}
	
	public void applyBrow(final Bitmap eye_brow, int color, float amount)
	{
		final PointF points[] = feature.getFeaturePoints();
/*
		int tile_width = Math.round(points[25].x - points[20].x);
		int tile_height = 8;
		int tile_x = Math.round(points[20].x);
		int tile_y = Math.round(points[20].y - tile_height);
		Bitmap pattern = Bitmap.createBitmap(bmp_stop, tile_x, tile_y, tile_width, tile_height);
		BitmapShader shader = new BitmapShader(pattern, Shader.TileMode.MIRROR, Shader.TileMode.MIRROR);

		Path path_eye_brow_r = Feature.getBrowPath(points, 22, 21, 20 ,25, 24 ,23);
		Path path_eye_brow_l = Feature.getBrowPath(points, 29, 28, 27, 26, 31, 30);
		
		// TODO not so good, need to come up with a method skin color
		int x = Math.round((points[20].x + points[27].x)/2);
		int y = Math.round((points[20].y + points[27].y)/2);
		int reference_color = bmp_stop.getPixel(x, y);
		float width = (points[23].y - points[21].y)/2;
		Canvas canvas = new Canvas(bmp_step);
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		paint.setColor(reference_color);
		paint.setStyle(Paint.Style.FILL_AND_STROKE);
		paint.setStrokeWidth(width);
		paint.setShader(shader);
		paint.setMaskFilter(new BlurMaskFilter(tile_height*2, BlurMaskFilter.Blur.NORMAL));
		canvas.drawPath(path_eye_brow_r, paint);
		canvas.drawPath(path_eye_brow_l, paint);
*/
		nativeApplyBrow(bmp_step, bmp_stop, points, eye_brow, color, amount);
	}
	
	public void applyEyeLash(Bitmap mask, int color, float amount)
	{
		PointF points[] = feature.getFeaturePoints();
		nativeApplyEyeLash(bmp_step, bmp_stop, points, mask, color, amount);
	}
	
	// tried to use #LayerDrawable
	private static Bitmap mergeLayers(final Bitmap layers[])
	{
		if(layers == null || layers.length <= 0)
			return null;
		
		Bitmap bitmap = layers[0].copy(Bitmap.Config.ARGB_8888, true);
		Canvas canvas = new Canvas(bitmap);
		
		// merge down these layers, here I use default blending mode, change mode if necessary.
		for(int i = 1; i < layers.length; ++i)
			canvas.drawBitmap(layers[i], 0, 0, null);
		
		return bitmap;
	}
	
	public void applyEyeShadow(@NonNull final Bitmap masks[], @NonNull final int colors[], float amount)
	{
		PointF points[] = feature.getFeaturePoints();
		
		if(true)  // merge layers in Java side
		{
			final int count = masks.length;
			Bitmap layers[] = new Bitmap[count];
			
			for(int i = 0; i < count; ++i)
				layers[i] = Effect.tone(masks[i], colors[i]);
	
			Bitmap eye_shadow = mergeLayers(layers);
			nativeApplyEye(bmp_step, bmp_stop, points, eye_shadow, amount);
		}
		else
			nativeApplyEyeShadow(bmp_step, bmp_stop, points, masks, colors, amount);
	}
	
	public void applyIris(Bitmap iris, Bitmap mask, float amount)
	{
		PointF points[] = feature.getFeaturePoints();
		nativeApplyIris(bmp_step, bmp_stop, points, iris, mask, amount);
	}
	
	public void applyBlush(BlushShape shape, int color, float amount)
	{
		PointF points[] = feature.getFeaturePoints();
		nativeApplyBlush(bmp_step, bmp_stop, points, shape.ordinal(), color, amount);
	}
	
	/**
	 * Lip gloss and lipstick can make your lips look fuller, glossier and better! See how to 
	 * <a href="http://www.wikihow.com/Apply-Lip-Color">apply lip color</a>.
	 * 
	 * @param color an #ARGB integer, @see #Color
	 * @param amount 0 means no change, and 1 means fully applied.
	 */
	public void applyLip(int color, float amount)
	{
		final PointF position = new PointF();

		Path path = feature.getLipPath();
		Bitmap mask = Feature.createMask(path, color, 8, position);
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
	
	
	private static native void nativeApplyBrow     (Bitmap dst, Bitmap src, final PointF points[], Bitmap mask, int color, float amount);
	private static native void nativeApplyEye      (Bitmap dst, Bitmap src, final PointF points[], Bitmap cosmetic, float amount);
	private static native void nativeApplyEyeLash  (Bitmap dst, Bitmap src, final PointF points[], Bitmap mask, int color, float amount);
	private static native void nativeApplyEyeShadow(Bitmap dst, Bitmap src, final PointF points[], Bitmap masks[], int colors[], float amount);
	private static native void nativeApplyIris     (Bitmap dst, Bitmap src, final PointF points[], Bitmap iris, Bitmap mask, float amount);
	private static native void nativeApplyBlush    (Bitmap dst, Bitmap src, final PointF points[], int shape, int color, float amount);
	private static native void nativeApplyLip      (Bitmap dst, Bitmap src, final PointF points[], int color, float amount);

	
}
