package com.wonderful.ishow.makeup;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.support.annotation.ColorInt;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class Makeup extends BitmapLayer {
	
	// keep in consistent with native side in venus/Makeup.cpp
	public static final int BLUSH_SHAPE_DEFAULT  = 0;
	public static final int BLUSH_SHAPE_DISK     = 1;
	public static final int BLUSH_SHAPE_OVAL     = 2;
	public static final int BLUSH_SHAPE_TRIANGLE = 3;
	public static final int BLUSH_SHAPE_HEART    = 4;
	public static final int BLUSH_SHAPE_SEAGULL  = 5;
	public static final int BLUSH_SHAPE_COUNT    = 6;
	
	@IntDef({
		BLUSH_SHAPE_DEFAULT ,
		BLUSH_SHAPE_DISK    ,
		BLUSH_SHAPE_OVAL    ,
		BLUSH_SHAPE_TRIANGLE,
		BLUSH_SHAPE_HEART   ,
		BLUSH_SHAPE_SEAGULL ,
		BLUSH_SHAPE_COUNT   ,  // internal usage
	})
	@Retention(RetentionPolicy.SOURCE)
	public @interface BlushShape {}
	
	private Feature feature;
	
	public Makeup(Bitmap image, final PointF[] points) {
		super(image);
		
		feature = new Feature(image, points);
	}

	public Bitmap markFeaturePoints() {
		return feature.mark();
	}
	
	public void applyBrow(final Bitmap eyeBrow, @ColorInt int color, float amount) {
		final PointF[] points = feature.getFeaturePoints();
/*
		int tile_width = Math.round(points[25].x - points[20].x);
		int tile_height = 8;
		int tile_x = Math.round(points[20].x);
		int tile_y = Math.round(points[20].y - tile_height);
		Bitmap pattern = Bitmap.createBitmap(outputImage, tile_x, tile_y, tile_width, tile_height);
		BitmapShader shader = new BitmapShader(pattern, Shader.TileMode.MIRROR, Shader.TileMode.MIRROR);

		Path path_eye_brow_r = Feature.getBrowPath(points, 22, 21, 20 ,25, 24 ,23);
		Path path_eye_brow_l = Feature.getBrowPath(points, 29, 28, 27, 26, 31, 30);
		
		// TODO not so good, need to come up with a method skin color
		int x = Math.round((points[20].x + points[27].x)/2);
		int y = Math.round((points[20].y + points[27].y)/2);
		int reference_color = outputImage.getPixel(x, y);
		float width = (points[23].y - points[21].y)/2;
		Canvas canvas = new Canvas(intermediateImage);
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		paint.setColor(reference_color);
		paint.setStyle(Paint.Style.FILL_AND_STROKE);
		paint.setStrokeWidth(width);
		paint.setShader(shader);
		paint.setMaskFilter(new BlurMaskFilter(tile_height*2, BlurMaskFilter.Blur.NORMAL));
		canvas.drawPath(path_eye_brow_r, paint);
		canvas.drawPath(path_eye_brow_l, paint);
*/
		nativeApplyBrow(intermediateImage, outputImage, points, eyeBrow, color, amount);
	}
	
	public void applyEyeLash(Bitmap mask, int color, float amount) {
		PointF[] points = feature.getFeaturePoints();
		nativeApplyEyeLash(intermediateImage, outputImage, points, mask, color, amount);
	}
	
	// tried to use #LayerDrawable
	private static Bitmap mergeLayers(@NonNull final Bitmap[] layers) {
		if(layers == null || layers.length <= 0)
			return null;
		
		Bitmap bitmap = layers[0].copy(Bitmap.Config.ARGB_8888, true);
		Canvas canvas = new Canvas(bitmap);
		
		// merge down these layers, here I use default blending mode, change mode if necessary.
		for(int i = 1; i < layers.length; ++i)
			canvas.drawBitmap(layers[i], 0, 0, null);
		
		return bitmap;
	}
	
	public void applyEyeShadow(@NonNull final Bitmap[] masks, @NonNull final int[] colors, float amount) {
		PointF[] points = feature.getFeaturePoints();
		
		if(true) {  // merge layers in Java side
			final int count = masks.length;
			Bitmap[] layers = new Bitmap[count];
			
			for(int i = 0; i < count; ++i)
				layers[i] = Effect.tone(masks[i], colors[i]);
	
			Bitmap eye_shadow = mergeLayers(layers);
			nativeApplyEye(intermediateImage, outputImage, points, eye_shadow, amount);
		} else {
			nativeApplyEyeShadow(intermediateImage, outputImage, points, masks, colors, amount);
		}
	}
	
	public void applyIris(Bitmap iris, float amount) {
		PointF[] points = feature.getFeaturePoints();
		nativeApplyIris(intermediateImage, outputImage, points, iris, amount);
	}
	
	public void applyBlush(@BlushShape int shape, int color, float amount) {
		PointF[] points = feature.getFeaturePoints();
		nativeApplyBlush(intermediateImage, outputImage, points, shape, color, amount);
	}
	
	/**
	 * Lip gloss and lipstick can make your lips look fuller, glossier and better! See how to 
	 * <a href="http://www.wikihow.com/Apply-Lip-Color">apply lip color</a>.
	 * 
	 * @param color color.
	 * @param amount 0 means no change, and 1 means fully applied.
	 */
	public void applyLip(@ColorInt int color, float amount) {
		final PointF position = new PointF();

		Path path = feature.getLipsPath();
		Bitmap mask = Feature.createMask(path, color, 8, position);
		mask = Effect.tone(mask, color, amount);

//		bmp_modified = Effect.tint(intermediateImage, color, t);
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC));
		final Rect rect = new Rect(Math.round(position.x), Math.round(position.y), mask.getWidth(), mask.getHeight());
//		canvas.drawColor(Color.TRANSPARENT);
		Canvas canvas = new Canvas(intermediateImage);
		canvas.drawBitmap(outputImage, 0, 0, null);
//		canvas.drawBitmap(intermediateImage, rect, rect, paint);
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));
		canvas.drawBitmap(mask, position.x, position.y, paint);
	}
	
	
	private static native void nativeApplyBrow     (Bitmap dst, Bitmap src, final PointF[] points, Bitmap mask, int color, float amount);
	private static native void nativeApplyEye      (Bitmap dst, Bitmap src, final PointF[] points, Bitmap cosmetic, float amount);
	private static native void nativeApplyEyeLash  (Bitmap dst, Bitmap src, final PointF[] points, Bitmap mask, int color, float amount);
	private static native void nativeApplyEyeShadow(Bitmap dst, Bitmap src, final PointF[] points, Bitmap[] masks, int[] colors, float amount);
	private static native void nativeApplyIris     (Bitmap dst, Bitmap src, final PointF[] points, Bitmap iris, float amount);
	private static native void nativeApplyBlush    (Bitmap dst, Bitmap src, final PointF[] points, int shape, int color, float amount);
	private static native void nativeApplyLip      (Bitmap dst, Bitmap src, final PointF[] points, int color, float amount);
	
	static {
		System.loadLibrary("opencv_java4");
		System.loadLibrary("stasm");
		System.loadLibrary("venus");
	}
}
