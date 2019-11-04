package com.wonderful.ishow.makeup;

import java.io.File;

import com.wonderful.ishow.BuildConfig;
import com.wonderful.ishow.R;
import com.wonderful.ishow.util.BitmapUtils;
import com.wonderful.ishow.util.FileUtils;
import com.wonderful.ishow.util.MathUtils;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.Paint.Style;
import android.support.annotation.ColorInt;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;

/*
 * Accidentally found that Android itself has a FaceDetector class {@link android.media.FaceDetector}
 * but it only gives found faces without feature points.
 * It use FFTEm library, I will dig it later.
 * FFT is Fast Fourier Transform, EM may refer to "expectation-maximization".
 * see <a href="http://stackoverflow.com/questions/3353696/underlying-technique-of-androids-facedetector">
 * FaceDetector</a>
 * <a href="https://github.com/lqs/neven">https://github.com/lqs/neven</a><br>
 * <a href="https://en.wikipedia.org/wiki/Expectation%E2%80%93maximization_algorithm">
 * Expectation-maximization_algorithm</a><br>
 */
public class Feature {
	private static final String TAG = Feature.class.getSimpleName();
	private static final boolean NATIVE = true;
	
	private final Bitmap image;
	private final PointF points[];
	
	private PointF centerPoint;  // face center
	private PointF downVector;   // face up direction, note that Y axis is top down.
	
	static {
		System.loadLibrary("c++_shared");
		System.loadLibrary("opencv_java4");
		System.loadLibrary("venus");
	}
	
	public Feature(@NonNull Bitmap image, @NonNull PointF[] points) {
		this.image = image;
		this.points = points;
		
		if(points == null)
			throw new IllegalStateException("need to call FaceDetector.detect() method first");
		
		PointF[] downCenter = nativeGetSymmetryAxis(points);
		downVector = downCenter[0];
		centerPoint = downCenter[1];
	}
	
	private static String loadClassifier(Context context) {
		String OPENCV_DIR = "OpenCV";
		if(BuildConfig.DEBUG) {
			String fullName = context.getResources().getResourceName(R.raw.haarcascade_frontalface_alt2);
			Log.i(TAG, "fullName: " + fullName);
			String resName = fullName.substring(fullName.lastIndexOf("/") + 1);
			Log.i(TAG, "resName: " + resName);
			
			// Enter "OpenCV", you will get "/data/data/<PACKAGE_NAME>/app_OpenCV", why a "app_" prefix?
			File resDir = context.getDir(OPENCV_DIR, Context.MODE_PRIVATE);
			Log.i(TAG, "resDir: " + resDir.getAbsolutePath());
		}
		
		String path = FileUtils.exportResource(context, R.raw.haarcascade_frontalface_alt2, OPENCV_DIR);
		FileUtils.exportResource(context, R.raw.haarcascade_mcs_lefteye, OPENCV_DIR);
		FileUtils.exportResource(context, R.raw.haarcascade_mcs_mouth, OPENCV_DIR);
		FileUtils.exportResource(context, R.raw.haarcascade_mcs_righteye, OPENCV_DIR);
		String classifierDir = path.substring(0, path.lastIndexOf('/'));
		Log.d(TAG, "cascade data directory: " + classifierDir);
		
		return classifierDir;
	}
	
	public static PointF[] detectFace(Context context, Bitmap image, @Nullable String image_name) {
		String CLASSIFIER_DIR = loadClassifier(context);
		return nativeDetectFace(image, image_name, CLASSIFIER_DIR);
	}
	
	public static PointF[][] detectFaces(Context context, Bitmap image, @Nullable String image_name) {
		String CLASSIFIER_DIR = loadClassifier(context);
		return nativeDetectFaces(image, image_name, CLASSIFIER_DIR);
	}
	
	public static PointF[][] detectFace(Context context, String image_name) {
		Bitmap image = BitmapFactory.decodeFile(image_name, BitmapUtils.OPTION_RGBA8888);
		return detectFaces(context, image, image_name);
	}
	
	public final PointF[] getFeaturePoints() {
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
	private static Path getClosedPath(@NonNull PointF points[], int first, int last) {
		if(points == null || points.length != 81)
			throw new NullPointerException("missing feature point data");
		
		Path path = new Path();
		path.moveTo(points[first].x, points[first].y);
		
		final int N = last - first + 1;
		for(int i = first; i <= last; ++i) {
			int _0 = first + (i - first + N - 1)%N; // MathUtils.wrap(i - 1, first, last);
			int _1 = i;
			int _2 = first + (i - first + 1)%N;     // MathUtils.wrap(i + 1, first, last);
			int _3 = first + (i - first + 2)%N;     // MathUtils.wrap(i + 2, first, last);
			
			lineTo(path, points[_0], points[_1], points[_2], points[_3]);
			
//			path.cubicTo(points[_1].x, points[_1].y, points[_2].x, points[_2].y, points[_3].x, points[_3].y);
//			path.moveTo(points[_3].x, points[_3].y);
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
	private static void lineTo(final Path path, PointF p0, PointF p1, PointF p2, PointF p3) {
		PointF point = new PointF();
		
		path.lineTo(p1.x, p1.y);
		Effect.catmullRomSpline(point, 1.0f/3, p0, p1, p2, p3);
		path.lineTo(point.x, point.y);
		Effect.catmullRomSpline(point, 2.0f/3, p0, p1, p2, p3);
		path.lineTo(point.x, point.y);
	}
	
	static Path getBrowPath(@NonNull final PointF points[], int _1, int _2, int _3, int _4, int _5, int _6) {
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
	
	private static Path getNosePath(@NonNull final PointF points[]) {
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
	
	public static Path getLipsPath(@NonNull final PointF points[]) {
		Path path = new Path();
		PointF point = new PointF();
		
		// upper lip
		path.moveTo(points[63].x, points[63].y);
		for(int i = 64; i <= 72; ++i) {
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
	
	public Path getLipsPath() {
		return getLipsPath(points);
	}
	
	/**
	 * @brief mark image with feature points, along with it's order.
	 * @param image the image to be marked.
	 * @param points feature points found by {@link #detectFaceSingle}.
	 * @return marked image, mostly used for debugging.
	 */
	public Bitmap mark() {
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
		for(int i = 0; i < points.length; ++i) {
			final float radius = 1;
			final PointF point = points[i];
			canvas.drawCircle(point.x, point.y, radius, paint);
			canvas.drawPoint(point.x, point.y, paint);
			canvas.drawText(String.valueOf(i), point.x + radius, point.y, paint);
		}
		
		paint.setStyle(Style.STROKE);
		Path facePath = getClosedPath(points, 0, 19);
		Path eyeBrowPath_R = getBrowPath(points, 22, 21, 20 ,25, 24 ,23);
		Path eyeBrowPath_L = getBrowPath(points, 29, 28, 27, 26, 31, 30);
		Path eyePath_R = getClosedPath(points, 34, 41);
		Path eyePath_L = getClosedPath(points, 44, 51);
//		Path upperLipPath = getUpperLip(points);
//		Path lowerLipPath = getLowerLip(points);
		Path lipsPath = getLipsPath(points);
		
		paint.setColor(Color.BLUE);
		canvas.drawPath(facePath, paint);
		canvas.drawPath(eyeBrowPath_R, paint);
		canvas.drawPath(eyeBrowPath_L, paint);
		canvas.drawPath(eyePath_R, paint);
		canvas.drawPath(eyePath_L, paint);
//		canvas.drawPath(upperLipPath, paint);
//		canvas.drawPath(lowerLipPath, paint);
		canvas.drawPath(lipsPath, paint);
		
//		Path path_lip = new Path();
//		path_lip.addPath(path_upper_lip);
//		path_lip.addPath(path_lower_lip);
//		Region region = new Region();
//		region.setPath(path_lip, null);
		
		// iris
		float irisRadius_R = 0, irisRadius_L = 0;
		final int[] irisIndices_R = {35, 37, 39, 41};
		final int[] irisIndices_L = {45, 47, 49, 51};
		if(false) {
			// average value, not so good for stasm face detector.
			for(int i = 0; i < irisIndices_R.length; ++i)
				irisRadius_R += MathUtils.distance(points[42], points[irisIndices_R[i]]);
			irisRadius_R /= 4;

			for(int i = 0; i < irisIndices_L.length; ++i)
				irisRadius_L += MathUtils.distance(points[43], points[irisIndices_L[i]]);
			irisRadius_R /= 4;
		} else {
			irisRadius_R = MathUtils.distance(points[42], points[irisIndices_R[0]]);
			for(int i = 1; i < irisIndices_R.length; ++i) {
				float dist = MathUtils.distance(points[42], points[irisIndices_R[i]]);
				if(irisRadius_R > dist)
					irisRadius_R = dist;
			}
			
			irisRadius_L = MathUtils.distance(points[43], points[irisIndices_L[0]]);
			for(int i = 1; i < irisIndices_L.length; ++i) {
				float dist = MathUtils.distance(points[43], points[irisIndices_L[i]]);
				if(irisRadius_L > dist)
					irisRadius_L = dist;
			}
		}
		canvas.drawCircle(points[42].x, points[42].y, irisRadius_R, paint);
		canvas.drawCircle(points[43].x, points[43].y, irisRadius_L, paint);
		
		// nose
		Path path_nose = getNosePath(points);
		canvas.drawPath(path_nose, paint);
		
		// draw perpendicular bisector
		float startX = 0, startY = 0, stopX = image.getWidth() - 1, stopY = image.getHeight() - 1;
		if(Math.abs(downVector.x) < Math.abs(downVector.y)) {
			float k = downVector.y / downVector.x;
			startY = k * (startX - centerPoint.x) + centerPoint.y;
			stopY = k * (stopX - centerPoint.x) + centerPoint.y;
		} else {
			float reciprocal_k = downVector.x / downVector.y;
			startX = reciprocal_k * (startY -  centerPoint.y) + centerPoint.x;
			stopX = reciprocal_k * (stopY - centerPoint.y) + centerPoint.x;
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
	 * @param blurRadius Blur radius, use it to get smooth mask.
	 * @param position stores the mask top-left position if <code>position</code> is not null.
	 * @return mask
	 *
	 */
	public static Bitmap createMask(Path path, @ColorInt int color, float blurRadius, @Nullable PointF position) {
		if(path == null || path.isEmpty())
			return null;
		
		RectF bounds = new RectF();
		path.computeBounds(bounds, true);
		bounds.inset(-blurRadius, -blurRadius);
		
		int width  = (int)bounds.width();
		int height = (int)bounds.height();
		Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);  // mutable
		Canvas canvas = new Canvas(bitmap);
		canvas.drawColor(Color.TRANSPARENT);
		
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		paint.setMaskFilter(new BlurMaskFilter(blurRadius, BlurMaskFilter.Blur.NORMAL));
		paint.setColor(color);
		paint.setStyle(Style.FILL);
		path.offset(-bounds.left, -bounds.top);
		canvas.drawPath(path, paint);
		
		if(position != null) {
			position.x = bounds.left;
			position.y = bounds.top;
		}
		
		return bitmap;
	}
	
	private static native PointF[]   nativeDetectFace(Bitmap image, String imageName, String classifierDir);
	private static native PointF[][] nativeDetectFaces(Bitmap image, String imageName, String classifierDir);
	private static native PointF[]   nativeGetSymmetryAxis(PointF[] points);
	
}
