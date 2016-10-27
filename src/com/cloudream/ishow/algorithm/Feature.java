package com.cloudream.ishow.algorithm;

import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.util.MathUtils;

import android.graphics.Bitmap;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.Paint.Style;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

class Feature
{
	private static final String TAG = Feature.class.getSimpleName();
	private static final boolean NATIVE = true;
	
	private final Bitmap image;
	private final PointF points[];
	
	private PointF center_point;  // face center
	private PointF up_vector;     // face up direction, note that Y axis is top down.
	
	public Feature(Bitmap image, final PointF points[])
	{
		this.image = image;
		this.points = points;
		
		if(BuildConfig.DEBUG && points == null)
			throw new IllegalStateException("need to call FaceDetector.detect() method first");
		
		final PointF center_up[] = getSymmetryAxis(points);
		center_point = center_up[0];
		up_vector = center_up[1];
	}
	
	public final PointF[] getFeaturePoints()
	{
		return points;
	}
	
	/**
	 * draw closed path smooth curve through points[first] and points[last].
	 * 
	 * @param points detected feature points, currently 81.
	 * @param first First point index (included)
	 * @param last Last point index (included)
	 * @return closed path
	 */
	private static Path getClosedPath(@NonNull final PointF points[], int first, int last)
	{
		if(BuildConfig.DEBUG && (points == null || points.length != 81))
			throw new NullPointerException("missing feature point data");
		
		Path path = new Path();
		PointF point = new PointF();
		
		final int N = last - first + 1;
		path.moveTo(points[first].x, points[first].y);
		for(int i = first; i <= last; ++i)
		{
			int _0 = first + (i - first + N - 1)%N; // MathUtils.wrap(i - 1, first, last);
			int _1 = i;
			int _2 = first + (i - first + 1)%N;     // MathUtils.wrap(i + 1, first, last);
			int _3 = first + (i - first + 2)%N;     // MathUtils.wrap(i + 2, first, last);
			
//			lineTo(path, points[_0], points[_1], points[_2], points[_3]);
			
//			path.cubicTo(points[_1].x, points[_1].y, points[_2].x, points[_2].y, points[_3].x, points[_3].y);
//			path.moveTo(points[_3].x, points[_3].y);
			
			Effect.catmullRomSpline(point, 0.25f, points[_0], points[_1], points[_2], points[_3]);
			path.lineTo(point.x, point.y);
			Effect.catmullRomSpline(point, 0.50f, points[_0], points[_1], points[_2], points[_3]);
			path.lineTo(point.x, point.y);
			Effect.catmullRomSpline(point, 0.75f, points[_0], points[_1], points[_2], points[_3]);
			path.lineTo(point.x, point.y);
		}
		
		path.close();  // path.lineTo(points[first].x, points[first].y);

		return path;
	}
	
	/**
	 * Draw arc between p1 (include) and p2 (exclude) with line segment, it use Catmull-Rom spline
	 * algorithm. The more subdivision, the smoother the curve p1~p2 will be.
	 * 
	 * @param path used to add knots, or control points.
	 * @param p0
	 * @param p1
	 * @param p2
	 * @param p3
	 */
	private static void lineTo(final Path path, PointF p0, PointF p1, PointF p2, PointF p3)
	{
		PointF point = new PointF();
		
		path.lineTo(p1.x, p1.y);
		Effect.catmullRomSpline(point, 1.0f/3, p0, p1, p2, p3);
		path.lineTo(point.x, point.y);
		Effect.catmullRomSpline(point, 2.0f/3, p0, p1, p2, p3);
		path.lineTo(point.x, point.y);
	}
	
	static Path getBrowPath(@NonNull final PointF points[], int _1, int _2, int _3, int _4, int _5, int _6)
	{
		Path path = new Path();
		final float portion = 1.0f/4;
		path.moveTo(points[_1].x, points[_1].y);
		PointF point = new PointF();
		Effect.catmullRomSpline(point, -portion, points[_1], points[_2], points[_3], points[_4]);
		path.lineTo(point.x, point.y);
		path.lineTo(points[_2].x, points[_2].y);
		Effect.catmullRomSpline(point, 0.25f, points[_1], points[_2], points[_3], points[_4]);
		path.lineTo(point.x, point.y);
		Effect.catmullRomSpline(point, 0.50f, points[_1], points[_2], points[_3], points[_4]);
		path.lineTo(point.x, point.y);
		Effect.catmullRomSpline(point, 0.75f, points[_1], points[_2], points[_3], points[_4]);
		path.lineTo(point.x, point.y);
		path.lineTo(points[_3].x, points[_3].y);
//		Effect.catmullRomSpline(point, 1.0f + portion, points[_1], points[_2], points[_3], points[_4]);
//		path.lineTo(point.x, point.y);
		float dy = (points[_4].y - points[_2].y);
		float dx = (points[_4].x - points[_2].x);
		float r = (float)Math.sqrt(dx * dx + dy * dy);
		dy /= r;
		dx /= r;
		r = (float)Math.sqrt((points[_4].x - points[_5].x) * (points[_4].x - points[_5].x) + (points[_4].y - points[_5].y) * (points[_4].y - points[_5].y));
		r *= 0.81f;
		path.rLineTo(dx * r, dy * r);
		
		path.lineTo(points[_4].x, points[_4].y);
		
		Effect.catmullRomSpline(point, -portion, points[_4], points[_5], points[_6], points[_1]);
		path.lineTo(point.x, point.y);
		path.lineTo(points[_5].x, points[_5].y);
		Effect.catmullRomSpline(point, 0.25f, points[_4], points[_5], points[_6], points[_1]);
		path.lineTo(point.x, point.y);
		Effect.catmullRomSpline(point, 0.50f, points[_4], points[_5], points[_6], points[_1]);
		path.lineTo(point.x, point.y);
		Effect.catmullRomSpline(point, 0.75f, points[_4], points[_5], points[_6], points[_1]);
		path.lineTo(point.x, point.y);
		path.lineTo(points[_6].x, points[_6].y);
		Effect.catmullRomSpline(point, 1.0f + portion, points[_4], points[_5], points[_6], points[_1]);
		path.lineTo(point.x, point.y);
		path.lineTo(points[_1].x, points[_1].y);
		
		return path;
	}
	
	private static Path getNosePath(@NonNull final PointF points[])
	{
		Path path = new Path();
		PointF point = new PointF();
		Effect.catmullRomSpline(point, 1.0f/3, points[18], points[25], points[54], points[62]);
		path.moveTo(point.x, point.y);
		Effect.catmullRomSpline(point, 2.0f/3, points[18], points[25], points[54], points[62]);
		path.lineTo(point.x, point.y);
		path.lineTo(points[54].x, points[54].y);
		lineTo(path,  points[54], points[62], points[61], points[55]);
				
		path.lineTo(points[61].x, points[61].y);
		path.lineTo(points[55].x, points[55].y);
		path.lineTo(points[60].x, points[60].y);

		path.lineTo(points[57].x, points[57].y);
		lineTo(path, points[57], points[59], points[58], points[52]);
		
		path.lineTo(points[58].x, points[58].y);
		path.lineTo(points[52].x, points[52].y);
		Effect.catmullRomSpline(point, 1.0f/3, points[58], points[52], points[26], points[14]);
		path.lineTo(point.x, point.y);
		Effect.catmullRomSpline(point, 2.0f/3, points[58], points[52], points[26], points[14]);
		path.lineTo(point.x, point.y);
		path.close();  // path.lineTo(start_x, start_y);
		
		return path;
	}
	
	public static Path getLipPath(@NonNull final PointF points[])
	{
		Path path = new Path();
		PointF point = new PointF();
		
		// upper lip
		path.moveTo(points[63].x, points[63].y);
		for(int i = 64; i <= 72; ++i)
		{
//			Effect.catmullRomSpline(point, 0.50f, points[i-1], points[i], points[i+1], points[i+2]);
			path.lineTo(points[i].x, points[i].y);
		}
	
		path.close();  // path.lineTo(points[63].x, points[63].y);
		
		// lower lip
//		path.moveTo(points[63].x, points[63].y);  // upper end point is lower start point
		lineTo(path, points[63], points[73], points[74], points[75]);
		lineTo(path,             points[73], points[74], points[75], points[69]);
		path.lineTo(points[75].x, points[75].y);
		path.lineTo(points[69].x, points[69].y);
		
		lineTo(path, points[69], points[76], points[77], points[78]);
		lineTo(path,             points[76], points[77], points[78], points[79]);
		lineTo(path,                         points[77], points[78], points[79], points[80]);
		lineTo(path,                                     points[78], points[79], points[80], points[63]);

		path.lineTo(points[80].x, points[80].y);
		path.close();  // path.lineTo(points[63].x, points[63].y);

		return path;
	}
	
	public Path getLipPath()
	{
		return getLipPath(points);
	}
	
	/**
	 * @brief mark image with feature points, along with it's order.
	 * @param image the image to be marked.
	 * @param points feature points found by {@link #detectFaceSingle}.
	 * @return marked image, mostly used for debugging.
	 */
	public Bitmap mark()
	{
		if(points == null)
			return image;
		
		Bitmap bitmap = image.copy(Bitmap.Config.ARGB_8888, true);
		Canvas canvas = new Canvas(bitmap);
		
		Paint paint = new Paint();
		paint.setAntiAlias(true);
		paint.setDither(true);
		paint.setStyle(Paint.Style.STROKE);
		paint.setStrokeJoin(Paint.Join.ROUND);
		paint.setStrokeCap(Paint.Cap.ROUND);
//		paint.setStrokeWidth(1);

//		canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);  // clear
//		paint.setColor(Color.TRANSPARENT);
//		canvas.drawRect(0, 0, bmp_raw.getWidth(), bmp_raw.getHeight(), paint);
		paint.setColor(Color.GREEN);
		for(int i = 0; i < points.length; ++i)
		{
			final float radius = 1;
			final PointF point = points[i];
			canvas.drawCircle(point.x, point.y, radius, paint);
//			canvas.drawPoint(point.x, point.y, paint);
			canvas.drawText(String.valueOf(i), point.x + radius, point.y, paint);
		}
		
		paint.setStyle(Style.STROKE);
		Path path_face = getClosedPath(points, 0, 19);
		Path path_eye_brow_r = getBrowPath(points, 22, 21, 20 ,25, 24 ,23);
		Path path_eye_brow_l = getBrowPath(points, 29, 28, 27, 26, 31, 30);
		Path path_eye_r = getClosedPath(points, 34, 41);
		Path path_eye_l = getClosedPath(points, 44, 51);
//		Path path_upper_lip = getUpperLip(points);
//		Path path_lower_lip = getLowerLip(points);
		Path path_lips = getLipPath(points);
		
		paint.setColor(Color.BLUE);
		canvas.drawPath(path_face, paint);
		canvas.drawPath(path_eye_brow_r, paint);
		canvas.drawPath(path_eye_brow_l, paint);
		canvas.drawPath(path_eye_r, paint);
		canvas.drawPath(path_eye_l, paint);
//		canvas.drawPath(path_upper_lip, paint);
//		canvas.drawPath(path_lower_lip, paint);
		canvas.drawPath(path_lips, paint);
		
//		Path path_lip = new Path();
//		path_lip.addPath(path_upper_lip);
//		path_lip.addPath(path_lower_lip);
//		Region region = new Region();
//		region.setPath(path_lip, null);
		
		// iris
		float iris_r_radius = 0, iris_l_radius = 0;
		final int iris_indices_r[] = {35, 37, 39, 41};
		final int iris_indices_l[] = {45, 47, 49, 51};
		if(false)
		{
			// average value, not so good for stasm face detector.
			for(int i = 0; i < iris_indices_r.length; ++i)
				iris_r_radius += MathUtils.distance(points[42], points[iris_indices_r[i]]);
			iris_r_radius /= 4;

			for(int i = 0; i < iris_indices_l.length; ++i)
				iris_l_radius += MathUtils.distance(points[43], points[iris_indices_l[i]]);
			iris_r_radius /= 4;
		}
		else
		{
			iris_r_radius = MathUtils.distance(points[42], points[iris_indices_r[0]]);
			for(int i = 1; i < iris_indices_r.length; ++i)
			{
				float dist = MathUtils.distance(points[42], points[iris_indices_r[i]]);
				if(iris_r_radius > dist)
					iris_r_radius = dist;
			}
			
			iris_l_radius = MathUtils.distance(points[43], points[iris_indices_l[0]]);
			for(int i = 1; i < iris_indices_l.length; ++i)
			{
				float dist = MathUtils.distance(points[43], points[iris_indices_l[i]]);
				if(iris_l_radius > dist)
					iris_l_radius = dist;
			}
		}
		canvas.drawCircle(points[42].x, points[42].y, iris_r_radius, paint);
		canvas.drawCircle(points[43].x, points[43].y, iris_l_radius, paint);
		
		// nose
		Path path_nose = getNosePath(points);
		canvas.drawPath(path_nose, paint);
		
		// draw perpendicular bisector
		float startX = 0, startY = 0, stopX = image.getWidth() - 1, stopY = image.getHeight() - 1;
		if(Math.abs(up_vector.x) < Math.abs(up_vector.y))
		{
			float k = up_vector.y / up_vector.x;
			startY = k * (startX - center_point.x) + center_point.y;
			stopY = k * (stopX - center_point.x) + center_point.y;
		}
		else
		{
			float reciprocal_k = up_vector.x / up_vector.y;
			startX = reciprocal_k * (startY -  center_point.y) + center_point.x;
			stopX = reciprocal_k * (stopY - center_point.y) + center_point.x;
		}
//		paint.setColor(Color.GREEN);
		paint.setPathEffect(new DashPathEffect(new float[] {10, 20}, 0));
		canvas.drawLine(startX, startY, stopX, stopY, paint);
		
		// from inner eye corner to lip point where it is close to center top point
		canvas.drawLine(points[34].x, points[34].y, points[65].x, points[65].y, paint);
		canvas.drawLine(points[44].x, points[44].y, points[67].x, points[67].y, paint);
		
		// from outer eye corner to lip corner
		canvas.drawLine(points[38].x, points[38].y, points[63].x, points[63].y, paint);
		canvas.drawLine(points[48].x, points[48].y, points[69].x, points[69].y, paint);
		
		// from wing of nose to lip corner
		float x1 = points[63].x * 2 - points[62].x;
		float y1 = points[63].y * 2 - points[62].y;
		canvas.drawLine(points[62].x, points[62].y, x1, y1, paint);
		x1 = points[69].x * 2 - points[58].x;
		y1 = points[69].y * 2 - points[58].y;
		canvas.drawLine(points[58].x, points[58].y, x1, y1, paint);
		
		return bitmap;
	}
	
	/**
	 * create closed path and filled with specified color.
	 * 
	 * @param path 
	 * @param color the color of filled region.
	 * @param blur_radius Blur radius, use it to get smooth mask.
	 * @param position stores the mask top-left position if <code>position</code> is not null.
	 * @return mask
	 * 
	 * @see #Color
	 */
	public static Bitmap createMask(final Path path, int color, float blur_radius, @Nullable PointF position)
	{
		if(path == null || path.isEmpty())
			return null;
		
		RectF bounds = new RectF();
		path.computeBounds(bounds, true);
		bounds.inset(-blur_radius, -blur_radius);
		
		int width  = (int)bounds.width();
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
	
	private static native PointF[] getSymmetryAxis(PointF points[]);
}
