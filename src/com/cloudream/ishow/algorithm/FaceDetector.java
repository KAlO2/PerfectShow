package com.cloudream.ishow.algorithm;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.imgcodecs.Imgcodecs;
import org.opencv.objdetect.CascadeClassifier;
import org.opencv.photo.Photo;

import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.R;
import com.cloudream.ishow.util.MathUtils;

import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Resources.NotFoundException;
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.util.Log;

// <PerfectShow>$ javah -d jni -classpath %ANDROID_SDK%/platforms/android-14/android.jar;G:/OpenCV-android-sdk/sdk/java/cls;./bin/classes com.cloudream.ishow.algorithm.FaceDetector
// http://www.cnblogs.com/Martinium/archive/2011/11/26/JNI_Hello_World.html
/**
 * Accidently found that Android itself has a FaceDetector class #android.media.FaceDetector
 * but it only gives found faces without feature points.
 * It use FFTEm library, I will dig it later.
 * FFT is Fast Fourier Transform, EM may refer to "expectation-maximization".
 * http://stackoverflow.com/questions/3353696/underlying-technique-of-androids-facedetector
 * {@link https://github.com/lqs/neven} <br>
 * {@link https://en.wikipedia.org/wiki/Expectation%E2%80%93maximization_algorithm} <br>
 * @author KAlO2
 *
 */
public class FaceDetector
{
	private static final String TAG = FaceDetector.class.getSimpleName();
	private static final boolean NATIVE = true;
	
//	private static final int MIN_SIZE = 30;

	// ROI (region Of Interest), these values must match up with the enum in jni/stasm/roi.h
	public enum Roi
	{
		FACE,  // the whole face (0, 0)
		EYE_BROW_L,
		EYE_BROW_R,
		EYE_LASH_L,
		EYE_LASH_R,
		EYE_SHADOW_L,
		EYE_SHADOW_R,
		IRIS_L,
		IRIS_R,
		BLUSH_L,
		BLUSH_R,
		NOSE,
		LIP_T,  // upper lip
		LIP_B,  // lower lip
	}
	
	public static class RoiInfo
	{
		public PointF origion;  // (left, top)
		public PointF pivot;
		public Mat    mask;
		
		RoiInfo()
		{
			origion = new PointF();
			pivot = new PointF();
			mask = new Mat();
		}
		
		// This method is used by JNI. (Don't delete it)
		@SuppressWarnings("unused")
		RoiInfo(PointF origion, PointF pivot, Mat mask)
		{
			this.origion = origion;
			this.pivot = pivot;
			this.mask = mask;
		}
	}
	
	private RoiInfo[] regions;  // cache, filled by C++
	
	private CascadeClassifier classifier_face;
	private CascadeClassifier classifier_eye_l;
	private CascadeClassifier classifier_eye_r;
	private CascadeClassifier classifier_mouth;
	
	private String data_dir;
	
//	private int image_width, image_height;
	private PointF points[];  // feature points
//	private Mat mat_model_stretched;
	
//	private float angle = 0.0f;  // skew angle, calculated once on JNI side
	private int blur_radius;  // soft edge alpha blending
	private final PointF center = new PointF();  // face center
	private final PointF up = new PointF();      // face up direction, note that Y axis is top down.
	
	private Context context;
	public FaceDetector(Context context)
	{
		this.context = context;
		
		blur_radius = 8;
/*		
		classifier_face  = loadClassifier(context, R.raw.haarcascade_frontalface_alt2);
//		classifier_eyes  = loadClassifier(context, R.raw.haarcascade_eye_tree_eyeglasses);
		classifier_eye_l = loadClassifier(context, R.raw.haarcascade_mcs_lefteye);
		classifier_eye_r = loadClassifier(context, R.raw.haarcascade_mcs_lefteye);
		classifier_mouth = loadClassifier(context, R.raw.haarcascade_mcs_lefteye);
		
		if(classifier_face.empty() || classifier_eye_l.empty() || classifier_eye_r.empty() || classifier_mouth.empty())
			Log.e(TAG, "Failed to load cascade classifier");
		else
			Log.i(TAG, "load cascade classifier successfully");
*/		
		
		if(BuildConfig.DEBUG)
		{
	        String fullname = context.getResources().getString(R.raw.haarcascade_frontalface_alt2);
	        Log.i(TAG, "full name: " + fullname);
	        String resName = fullname.substring(fullname.lastIndexOf("/") + 1);
	        Log.i(TAG, "resName: " + resName);
	        // enter "OpenCV_data", you will get "/data/data/<PACKAGE_NAME>/app_OpenCV_data", why a app_ prefix?
	        File resDir = context.getDir("OpenCV_data", Context.MODE_PRIVATE);
	        Log.i(TAG, "resDir: " + resDir.getAbsolutePath());
		}
            
		String path = Utils.exportResource(context, R.raw.haarcascade_frontalface_alt2);
		Utils.exportResource(context, R.raw.haarcascade_mcs_lefteye);
		Utils.exportResource(context, R.raw.haarcascade_mcs_mouth);
		Utils.exportResource(context, R.raw.haarcascade_mcs_righteye);
		data_dir = path.substring(0, path.lastIndexOf('/'));
		Log.d(TAG, "cascade data directory: " + data_dir);
		
		points = new PointF[81];
		int length = Roi.values().length;
		regions = new RoiInfo[length];
		for(int i = 0; i < length; ++i)
			regions[i] = new RoiInfo();
	}
	
	public PointF[] getFeaturePoints()
	{
		return points;
	}
	
	public void setBlurMaskRadius(int radius)
	{
		blur_radius = radius;
	}
	
	public static CascadeClassifier loadClassifier(Context context, String filename)
	{
		AssetManager am = context.getResources().getAssets();
		try
		{
			InputStream is = am.open(filename);
			CascadeClassifier classifier = loadClassifier(context, is);
			is.close();
			
			return classifier;
		}
		catch(IOException e)
		{
			e.printStackTrace();
		}
		
		return new CascadeClassifier();  // should not be here.
	}
	
	// http://developer.android.com/tools/projects/index.html
	// I prefer to store Haar cascade file in assets/ rather than res/raw/ folder.
	// Each file in res directory is given a pre-compiled ID which can be
	// accessed easily through R.id.[res_id], but losing benefits of hierarchical directory.
	public static CascadeClassifier loadClassifier(Context context, int raw_id)
	{
		try
		{
			InputStream is = context.getResources().openRawResource(raw_id);
			CascadeClassifier classifier = loadClassifier(context, is);
			is.close();
			
			return classifier;
		} catch(NotFoundException e)
		{
			Log.e(TAG, "openRawResource(" + raw_id + ") failed");
			e.printStackTrace();
		} catch(IOException e)
		{
			Log.e(TAG, "failed to close this stream");
			e.printStackTrace();
		}
		
		return new CascadeClassifier();  // should not be here.
	}
	
	private static CascadeClassifier loadClassifier(Context context, InputStream is)
	{
		File cascadeDir = context.getDir(TAG, Context.MODE_PRIVATE);
		File file = new File(cascadeDir, "_");
		
		try
		{
			FileOutputStream os = new FileOutputStream(file);
			
			byte[] buffer = new byte[4096];
			int bytesRead;
			while((bytesRead = is.read(buffer)) != -1)
				os.write(buffer, 0, bytesRead);
			
			os.close();
		} catch(IOException e)
		{
			// Failed to load cascade. Exception thrown
			e.printStackTrace();
		}
		
		String path = file.getAbsolutePath();
		CascadeClassifier classifier = new CascadeClassifier();
		classifier.load(path);
		
		cascadeDir.delete();
		
		return classifier;
	}
	
	/**
	 * @brief mark image with feature points, along with it's order.
	 * @param image the image to be marked.
	 * @param points feature points found by {@link #detectFaceSingle}.
	 * @return marked image, mostly used for debugging.
	 */
	public Bitmap mark(Bitmap image)
	{
		if(points == null)
			return image;
		
		Bitmap mutableBitmap = image.copy(Bitmap.Config.ARGB_8888, true);
		Canvas canvas = new Canvas(mutableBitmap);
		
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
		final float radius = 1;
		for(int i = 0; i < points.length; ++i)
		{
			final PointF point = points[i];
//			canvas.drawCircle(point.x, point.y, radius, paint);
			canvas.drawPoint(point.x, point.y, paint);
			canvas.drawText(String.valueOf(i), point.x + radius, point.y, paint);
		}
		
		paint.setStyle(Style.STROKE);
		Path path_face = getClosedPath(points, 0, 19);
		Path path_eye_brow_r = getEyeBrowPath(points, 22, 21, 20 ,25, 24 ,23);
		Path path_eye_brow_l = getEyeBrowPath(points, 29, 28, 27, 26, 31, 30);
		Path path_eye_r = getClosedPath(points, 34, 41);
		Path path_eye_l = getClosedPath(points, 44, 51);
		Path path_upper_lip = getUpperLip(points);
		Path path_lower_lip = getLowerLip(points);
		
		paint.setColor(Color.BLUE);
		canvas.drawPath(path_face, paint);
		canvas.drawPath(path_eye_brow_r, paint);
		canvas.drawPath(path_eye_brow_l, paint);
		canvas.drawPath(path_eye_r, paint);
		canvas.drawPath(path_eye_l, paint);
		canvas.drawPath(path_upper_lip, paint);
		canvas.drawPath(path_lower_lip, paint);

		Path path_lip = new Path();
		path_lip.addPath(path_upper_lip);
		path_lip.addPath(path_lower_lip);
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
		if(Math.abs(up.x) < Math.abs(up.y))
		{
			float k = up.y / up.x;
			startY = k * (startX - center.x) + center.y;
			stopY = k * (stopX - center.x) + center.y;
		}
		else
		{
			float reciprocal_k = up.x / up.y;
			startX = reciprocal_k * (startY -  center.y) + center.x;
			stopX = reciprocal_k * (stopY - center.y) + center.x;
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
		
		return mutableBitmap;
	}
	
	// open interval [first, last]
	private static Path getClosedPath(final PointF points[], int first, int last)
	{
		if(BuildConfig.DEBUG && (points == null || points.length != 81))
			throw new NullPointerException("missing feature point data");
			
		assert(first + 3 < last);
		Path path = new Path();
		PointF point = new PointF();
		
		final int N = last - first + 1;
		path.moveTo(points[first].x, points[first].y);
		for(int i = first; i <= last; ++i)
		{
			int _0 = i - 1, _1 = i, _2 = i + 1, _3 = i + 2;
			if(_0 < 0 ) _0 += N;
			if(_2 > last) { _2 -= N; if(_3 > last) _3 -= N; }
//			Effect.catmullRomSpline(point, 1.0f/3, points[_0], points[_1], points[_2], points[_3]);
//			path.lineTo(point.x, point.y);
//			Effect.catmullRomSpline(point, 2.0f/3, points[_0], points[_1], points[_2], points[_3]);
//			path.lineTo(point.x, point.y);
			
			Effect.catmullRomSpline(point, 0.25f, points[_0], points[_1], points[_2], points[_3]);
			path.lineTo(point.x, point.y);
			Effect.catmullRomSpline(point, 0.50f, points[_0], points[_1], points[_2], points[_3]);
			path.lineTo(point.x, point.y);
			Effect.catmullRomSpline(point, 0.75f, points[_0], points[_1], points[_2], points[_3]);
			path.lineTo(point.x, point.y);
		}
		
		path.lineTo(points[first].x, points[first].y);
/*	
//		final int length = last - first + 1;
		path.moveTo(points[last].x, points[last].y);
		for(int i = first; i < last; ++i)
		{
			final int _1 = i;
			final int _2 = (_1 + 1 <= last) ? _1 + 1 : first;
			final int _3 = (_2 + 1 <= last) ? _2 + 1 : first;
			path.cubicTo(points[_1].x, points[_1].y, points[_2].x, points[_2].y, points[_3].x, points[_3].y);
			path.moveTo(points[_3].x, points[_3].y);

		path.moveTo(points[first].x, points[first].y);
*/
		return path;
	}

	private static Path getEyeBrowPath(final PointF points[], int _1, int _2, int _3, int _4, int _5, int _6)
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
	
	public Path getLipPath()
	{
		Path path = getUpperLip(points);
		path.addPath(getLowerLip(points));
		return path;
	}
	
	private static Path getUpperLip(final PointF points[])
	{
		Path path = new Path();
		PointF point = new PointF();
		path.moveTo(points[63].x, points[63].y);
//		path.lineTo(points[64].x, points[64].y);
		for(int i = 64; i <= 72; ++i)
		{
//			Effect.catmullRomSpline(point, 0.50f, points[i-1], points[i], points[i+1], points[i+2]);
			path.lineTo(points[i].x, points[i].y);
		}
	
		path.close();  // path.lineTo(points[63].x, points[63].y);
		return path;
	}
	
	/**
	 * draw arc between p1 and p2 with line segment
	 * @param path
	 * @param p0
	 * @param p1
	 * @param p2
	 * @param p3
	 * @param point
	 */
	private static void lineTo(Path path, PointF p0, PointF p1, PointF p2, PointF p3)
	{
		PointF point = new PointF();
		Effect.catmullRomSpline(point, 1.0f/3, p0, p1, p2, p3);
		path.lineTo(point.x, point.y);
		Effect.catmullRomSpline(point, 2.0f/3, p0, p1, p2, p3);
		path.lineTo(point.x, point.y);
		
		path.lineTo(p2.x, p2.y);
	}
	
	private static Path getLowerLip(final PointF points[])
	{
		Path path = new Path();
		PointF point = new PointF();
		path.moveTo(points[63].x, points[63].y);
		path.lineTo(points[73].x, points[73].y);
		lineTo(path, points[63], points[73], points[74], points[75]);
		lineTo(path, points[73], points[74], points[75], points[69]);

		path.lineTo(points[69].x, points[69].y);
		path.lineTo(points[76].x, points[76].y);
if(false){  // these two branches are functionally equivalent
		final int indices[] = { 69, 76, 77, 78, 79, 80, 63 };
		for(int i = 76; i <= 79; ++i)
		{
			int i0 = i == 76 ? 69 : i-1;
			int i1 = i;
			int i2 = i + 1;
			int i3 = i == 79 ? 63 : i+2;
			lineTo(path, points[i0], points[i1], points[i2], points[i3]);
		}
}else{
		lineTo(path, points[69], points[76], points[77], points[78]);
		lineTo(path, points[76], points[77], points[78], points[79]);
		lineTo(path, points[77], points[78], points[79], points[80]);
		lineTo(path, points[78], points[79], points[80], points[63]);
}
		path.close();  // path.lineTo(points[63].x, points[63].y);
		return path;
	}
	
	private static Path getNosePath(final PointF points[])
	{
		Path path = new Path();
		PointF point = new PointF();
		Effect.catmullRomSpline(point, 1.0f/3, points[21], points[25], points[54], points[62]);
		final float start_x = point.x, start_y = point.y;
		path.moveTo(point.x, point.y);
		
//		Effect.catmullRomSpline(point2, 1.0f/2, points[25], point, points[54], points[62]);
		lineTo(path,  points[25], point, points[54], points[62]);
		
		if(true){
			path.lineTo(points[62].x, points[62].y);
		PointF point2 = new PointF();
		point2.x = points[54].x + (points[62].x - points[54].x) * 0.81f;
		point2.y = points[54].y + (points[62].y - points[54].y) * 0.81f;
//		path.lineTo(point2_x, point2_y);
//		path.cubicTo(point2.x, point2.y, points[61].x, points[61].y, points[55].x, points[55].y);
		lineTo(path,  point2, points[62], points[61], points[55]);
		}else{
			
		path.cubicTo(points[62].x, points[62].y, points[61].x, points[61].y, points[55].x, points[55].y);
		}
//		path.lineTo(points[62].x, points[62].y);
//		
//		path.lineTo(points[61].x, points[61].y);
		path.lineTo(points[55].x, points[55].y);
		path.lineTo(points[60].x, points[60].y);
//		path.cubicTo(points[61].x, points[61].y, points[55].x, points[55].y, points[60].x, points[60].y);
		path.lineTo(points[57].x, points[57].y);
		path.lineTo(points[59].x, points[59].y);
		path.lineTo(points[58].x, points[58].y);
		
		path.lineTo(points[52].x, points[52].y);
		Effect.catmullRomSpline(point, 1.0f/3, points[28], points[26], points[52], points[58]);
//		Effect.catmullRomSpline(point, 1.0f/2, points[59], points[58], points[52], point2);
		lineTo(path, points[58], points[52], point, points[26]);
		
//		Effect.catmullRomSpline(point, 1.0f/3, points[28], points[26], points[52], points[59]);
//		path.lineTo(point.x, point.y);
		path.lineTo(start_x, start_y);
		
		return path;
	}
	
	public void getBlusherRegion(RectF rect)
	{
		if(rect == null)
			rect = new RectF();
		
		// TODO havn't take skew face into consideration
		rect.left = points[0].x;
		rect.right = points[54].x;
		rect.top = points[40].y;
		rect.bottom = points[71].y;
		
	}
	
	public RoiInfo getRegionInfo(Roi region)
	{
		return regions[region.ordinal()];
	}
	
	/**
	 * @brief seamlessly clone source image into destination image.
	 * @param src source image
	 * @param dst destination image
	 * @param mask the mask of source image
	 * @param center The location of the center of the src in the dst
	 * @param flags {@link org.opencv.photo.Photo#NORMAL_CLONE}, 
	 * 	{@link org.opencv.photo.Photo#MIXED_CLONE},
	 * 	{@link org.opencv.photo.Photo#MONOCHROME_TRANSFER}
	 * 	
	 * @return blended image
	 */
	public static Bitmap seamlessClone(Bitmap src, Bitmap dst, Bitmap mask, PointF center, int flags)
	{
		if(src == null || dst == null)
			throw new NullPointerException("src or dst bitmap is null");
		
		if(flags != Photo.NORMAL_CLONE && flags != Photo.MIXED_CLONE && flags != Photo.MONOCHROME_TRANSFER)
			throw new IllegalArgumentException("flags is invalid");
		
		if(NATIVE)
			return nativeSeamlessClone(src, dst, mask, center, flags);
		else
		{
			Mat _src = new Mat();
	//		Mat _src = new Mat(src.getHeight(), src.getHeight(), CvType.CV_8UC4);
			final boolean premultiplyAlpha = false;
			Utils.bitmapToMat(src, _src, premultiplyAlpha);
			
			Mat _dst = new Mat();
			Utils.bitmapToMat(dst, _dst, premultiplyAlpha);
			
			Mat _mask = new Mat();
			if(mask == null)
			{
//				_mask.create(src.getHeight(), src.getWidth(), CvType.CV_8UC1);
//				_mask.setTo(new Scalar(255));
				_mask = new Mat(src.getHeight(), src.getWidth(), CvType.CV_8UC1, new Scalar(255));
			}
			else
				Utils.bitmapToMat(mask, _mask, premultiplyAlpha);
				
			
			Mat _blend = new Mat();
			Point _center = new Point(center.x, center.y);
			Photo.seamlessClone(_src, _dst, _mask, _center, _blend, flags);
			
			Bitmap blend = dst.copy(Bitmap.Config.ARGB_8888, true);
			Utils.matToBitmap(_blend, blend, premultiplyAlpha);
			return blend;
		}
	}
	
	public RoiInfo calculateRegionInfo(Roi region)
	{
		if(BuildConfig.DEBUG && (points[0].x == 0 || points[0].y == 0))
			throw new IllegalStateException("need to call detect() method first");
		
		final int index = region.ordinal();
		if(NATIVE)
			return nativeCalculateRegionInfo(index);
		else
		{
			if(regions[index] == null)
			{
				RoiInfo roi = new RoiInfo();
			
				switch(region)
				{
				case LIP_B:
				{
					// Due to the speed factor, mouth haven't been taken skew into consideration.
					float left = points[63].x, right = points[69].x;
					float top = Math.min(points[65].y, points[67].y), bottom = points[78].y;
					
					roi.origion = new PointF(left, top);
					roi.pivot = new PointF(points[74].x, points[74].y);
					
//					Imgproc.fillPoly(img, pts, color, lineType, shift, offset);
				}
					break;
				default:
					throw new NullPointerException("region unimplemented yet");
				}
				regions[index] = roi;
			}
			return regions[index];
		}
	}
	
	public PointF centerOfRegion(Roi region)
	{
		if(points == null)
			throw new IllegalStateException("need to call detect() method first");
		
		if(NATIVE)
			return nativeCenterOfRegion(region.ordinal());
		else
		{
			PointF center = new PointF();
			switch(region)
			{
			case FACE:
				break;
			case EYE_BROW_L:
				center.x = (points[27].x + points[28].x + points[30].x + points[31].x)/4;
				center.y = (points[27].y + points[28].y + points[30].y + points[31].y)/4;
				break;
			case EYE_BROW_R:
				center.x = (points[20].x + points[21].x + points[23].x + points[24].x)/4;
				center.y = (points[20].y + points[21].y + points[23].y + points[24].y)/4;
				break;
			case EYE_SHADOW_L:
			case EYE_LASH_L:
				center.x = (points[47].x + points[49].x)/2;
				center.y = (points[47].y + points[49].y)/2;
				break;
			case EYE_SHADOW_R:
			case EYE_LASH_R:
				center.x = (points[37].x + points[39].x)/2;
				center.y = (points[37].y + points[39].y)/2;
				break;
			case BLUSH_L:
				center.x = (points[10].x + points[58].x)/2;
				center.y = (points[10].y + points[58].y)/2;
				break;
			case BLUSH_R:
				center.x = (points[2].x + points[62].x)/2;
				center.y = (points[2].y + points[62].y)/2;
				break;
			case NOSE:
				center.x = points[53].x;
				center.y = points[53].y;
				break;
			case LIP_B:
				center.x = points[74].x;
				center.y = points[74].y;
				break;
			default:
				throw new IllegalArgumentException("region is invalid");
			}
			
			return center;
		}
	}
	
	public void SeamlessClone(Bitmap src, Bitmap dst, PointF center, int flags)
	{
		if(src == null || dst == null )
			throw new NullPointerException("src or dst bitmap is null");
		if(!dst.isMutable())
			throw new NullPointerException("cannot write to dst Bitmap");
		
		if(flags != Photo.NORMAL_CLONE && flags != Photo.MIXED_CLONE && flags != Photo.MONOCHROME_TRANSFER)
			throw new IllegalArgumentException("flags is invalid");

		
	}
	
	public Bitmap SeamlessClone(String src, String dst, PointF center, int flags)
	{
		if(src == null || src.isEmpty() || dst == null || dst.isEmpty())
			throw new NullPointerException("src or dst bitmap is null");
		
		if(flags != Photo.NORMAL_CLONE && flags != Photo.MIXED_CLONE && flags != Photo.MONOCHROME_TRANSFER)
			throw new IllegalArgumentException("flags is invalid");
		
		Mat _src = Imgcodecs.imread(src);
		Mat _dst = Imgcodecs.imread(dst);
		Mat src_mask = new Mat(_src.height(), _src.width(), CvType.CV_8UC3, new Scalar(255));
		
		Mat _blend = new Mat();
		Photo.seamlessClone(_src, _dst, src_mask, new Point(center.x, center.y), _blend, flags);
		
		Bitmap blend = Bitmap.createBitmap(_dst.width(), _dst.height(), Bitmap.Config.ARGB_8888);
		Utils.matToBitmap(_blend, blend, true/* premultiplyAlpha */);
		return blend;
//		return nativeSeamlessClone2(src, dst, center, flags);
	}
	
	public Bitmap blendColor(Bitmap src, Bitmap mask, int color, float alpha)
	{
		if(alpha < 0 || alpha > 1)
			throw new IllegalArgumentException("alpha is out of range [0, 1]");
		
		return null;
	}
	
	public boolean detect(String image_path)
	{
		points = nativeDetectFaceSingle(image_path, data_dir);
		if(points.length > 0)
			getSymmetryAxis(points, center, up);
		return points.length > 0;
	}
	
	public boolean detect(Bitmap image)
	{
		points = nativeDetectFaceSingle2(image, data_dir);
		if(points.length > 0)
			getSymmetryAxis(points, center, up);
		
		return points.length > 0;
	}
	
	public static PointF[] detectFaceSingle(String image_path, String data_dir)
	{
		return nativeDetectFaceSingle(image_path, data_dir);
	}
	
	public static Mat stretchImage(Mat src_image, Mat dst_image, PointF[] src_points, PointF[] dst_points)
	{
		return nativeStretchImage(src_image, dst_image, src_points, dst_points);
	}
	
	public static Bitmap stretchImage(Bitmap src_image, int dst_width, int dst_height, PointF[] src_points, PointF[] dst_points)
	{
		final boolean unPremultiplyAlpha = true;
		Mat src_mat = new Mat();
//		LOGD("nBitmapToMat: RGBA_8888 -> CV_8UC4");
		Utils.bitmapToMat(src_image, src_mat, unPremultiplyAlpha);
		
		Mat dst_mat = nativeStretchImage2(src_mat, dst_width, dst_height, src_points, dst_points);
		
		Bitmap dst_image = Bitmap.createBitmap(dst_width, dst_height, Bitmap.Config.ARGB_8888);
		Utils.matToBitmap(dst_mat, dst_image, unPremultiplyAlpha);
		return dst_image;
	}
	
	public static Bitmap stretchImage(Bitmap src_image, Bitmap dst_image, PointF[] src_points, PointF[] dst_points)
	{
		final boolean unPremultiplyAlpha = true;
		Mat src_mat = new Mat();
//		LOGD("nBitmapToMat: RGBA_8888 -> CV_8UC4");
		Utils.bitmapToMat(src_image, src_mat, unPremultiplyAlpha);
//		src_mat.convertTo(src_mat, CvType.CV_32FC3);
//		Imgproc.cvtColor(src_mat, src_mat, Imgproc.COLOR_BGRA2GRAY);
		Mat dst_mat = new Mat();  // dst_image.getHeight(), dst_image.getWidth(), CvType.CV_32FC3);
		Utils.bitmapToMat(dst_image, dst_mat, unPremultiplyAlpha);

/*		// this is a test. Mat get return RGBA, not BGRA, or ARGB.
		//(200, 337) RGBA 205 157 131 182 0xcd9d83
		byte data[] = new byte[4];  // 
		src_mat.get(337, 200, data);
		Log.i(TAG, "expect RGBA 205 157 131 182, and we get " + (data[2]&0xff) + ", " + (data[1]&0xff) + ", " + (data[0]&0xff) + ", " + (data[3]&0xff));
*/
		// bitmapToMat use type CV_8UC4 wherever RGBA_8888 or RGB_565
		nativeStretchImage(src_mat, dst_mat, src_points, dst_points);
		
		// LOGD("nMatToBitmap: CV_8UC4 -> RGBA_8888");
		Utils.matToBitmap(dst_mat, dst_image);
		return dst_image;
	}
/*	
	public Mat stretchImage(Bitmap model_image, String data_dir)
	{
		if(feature_points == null)
			throw new IllegalStateException("need to call detect() method first");
		
//		if(!model_image.isMutable())
//			throw new NullPointerException("cannot write to dst Bitmap");
		
		Mat src = new Mat();
		Utils.bitmapToMat(model_image, src);
		return nativeStretchImage(src, data_dir);
//		Mat stretched = nativeStretchImage(src, data_dir);
//		Utils.matToBitmap(stretched, model_image);
//		
//		return model_image;
	}
*/
	
	public static Bitmap cloneFace(String user_image_path, String model_image_path, String data_dir)
	{
		Mat mat = nativeCloneFace(user_image_path, model_image_path, data_dir);
		Bitmap result = Bitmap.createBitmap(mat.cols(), mat.rows(), Bitmap.Config.ARGB_8888);
		Utils.matToBitmap(mat, result);
		return result;
	}

	public static Bitmap overlay(Bitmap src, Bitmap dst, float left, float top, int alpha)
	{
		alpha = Math.min(Math.max(0, alpha), 255);
		if(!dst.isMutable())
			throw new NullPointerException("cannot write to dst Bitmap");
		
		// operations on Canvas, a preferable way!
		Paint paint = new Paint();
		paint.setAntiAlias(true);
		paint.setAlpha(255 - alpha);
		
		Canvas canvas = new Canvas(dst);
		canvas.drawBitmap(dst, 0, 0, paint);
		
		paint.setAlpha(alpha);
		canvas.drawBitmap(src, left, top, paint);

		return dst;
	}
	
	public static Bitmap overlay(Bitmap src, Bitmap dst, Matrix matrix, int alpha)
	{
		alpha = Math.min(Math.max(0, alpha), 255);
		if(!dst.isMutable())
			throw new NullPointerException("cannot write to dst Bitmap");
		
		Paint paint = new Paint();
		paint.setAntiAlias(true);
		paint.setAlpha(255 - alpha);
		Canvas canvas = new Canvas(dst);
	    canvas.drawBitmap(dst, matrix, paint);
		return dst;
	}
	
	public static Bitmap overlay2(Bitmap src_image, Bitmap dst_image, int origin_x, int origin_y, int alpha)
	{
		if(NATIVE)
		{
			final boolean unPremultiplyAlpha = true;
			Mat src_mat = new Mat();
			Utils.bitmapToMat(src_image, src_mat, unPremultiplyAlpha);
			Mat dst_mat = new Mat();
			Utils.bitmapToMat(dst_image, dst_mat, unPremultiplyAlpha);
			
			nativeOverlay(src_mat, dst_mat, origin_x, origin_y, alpha);
			
//			Bitmap result = Bitmap.createBitmap(dst_image.getWidth(), dst_image.getHeight(), Bitmap.Config.ARGB_8888);
			Utils.matToBitmap(dst_mat, dst_image, unPremultiplyAlpha);
		}
		else
		{
			final int left = Math.max(origin_x, 0);
			final int right = Math.min(origin_x + src_image.getWidth(), dst_image.getWidth());
			final int top = Math.max(origin_y, 0);
			final int bottom = Math.min(origin_y + src_image.getHeight(), dst_image.getHeight());
			
			// rect1 intersects with rect2
			if(left < right && top < bottom)
			{
				for(int j = top; j < bottom; ++j)
				for(int i = left; i < right; ++i)
				{
					int src_argb = src_image.getPixel(i, j);
					int dst_argb = dst_image.getPixel(i, j);
					
					final int src_a = Color.alpha(src_argb);
					int red = ((255 - src_a) * Color.red(dst_argb) + src_a * Color.red(src_argb)) / 255;
					int green = ((255 - src_a) * Color.green(dst_argb) + src_a * Color.green(src_argb)) / 255;
					int blue = ((255 - src_a) * Color.blue(dst_argb) + src_a * Color.blue(src_argb)) / 255;
					red = Math.min(Math.max(0, red), 255);
					green = Math.min(Math.max(0, green), 255);
					blue = Math.min(Math.max(0, blue), 255);
					dst_image.setPixel(i, j, Color.argb(Color.alpha(dst_argb), red, green, blue));		        
				}
			}
		}
		return dst_image;
	}
	
	public Bitmap blendLip(final Bitmap image, int color, float amount)
	{
		if(BuildConfig.DEBUG && points == null)
			throw new IllegalStateException("need to call detect() method first");
		
		Bitmap result = image.copy(Bitmap.Config.ARGB_8888, true);
		final PointF mask_position = new PointF();

		Path path = getLipPath();
		Bitmap mask = Effect.createMask(path, color, 8, mask_position);
		mask = Effect.tint(mask, color, amount);
//		SettingsActivity.saveImage(MakeupActivity.this, null, mask);
//		bmp_modified = Effect.tint(bmp_step, color, t);
//		Bitmap mask, PointF mask_poisition,
		Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC));
//		bmp_modified = bmp_step.copy(Bitmap.Config.ARGB_8888, true);
		final Rect rect = new Rect(Math.round(mask_position.x), Math.round(mask_position.y), mask.getWidth(), mask.getHeight());
//		canvas.drawColor(Color.TRANSPARENT);
		Canvas canvas = new Canvas(result);
//		canvas.drawBitmap(bmp_step, 0, 0, paint);
//		canvas.drawBitmap(bmp_step, rect, rect, paint);
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));
		canvas.drawBitmap(mask, mask_position.x, mask_position.y, paint);
		return result;
	}

	// use resource R.drawable.iris[01~16].jpg, maybe delete later.
	public Bitmap blendIris(final Bitmap image, final Bitmap iris, float amount)
	{
		if(BuildConfig.DEBUG && points == null)
			throw new IllegalStateException("need to call detect() method first");
		
		Bitmap result = image.copy(Bitmap.Config.ARGB_8888, true);
		nativeBlendIris(result, iris, points, amount);
		return result;
	}
	
	public Bitmap blendIris(Bitmap image, final Bitmap iris, final Bitmap iris_mask, int color, float amount)
	{
		if(BuildConfig.DEBUG && points == null)
			throw new IllegalStateException("need to call detect() method first");
		
		Bitmap result = image.copy(Bitmap.Config.ARGB_8888, true);
		nativeBlendIris2(result, iris, iris_mask, points, color, amount);
		return result;
	}
	
	/**
	 * with a blusher mask, color with alpha, in ARGB format from #Color class. RGB is the 
	 * primary color of blusher, and alpha is the blending amount.
	 * 
	 * @param image
	 * @param blusher
	 * @param color
	 * @return
	 */
	public Bitmap blendBlusher(final Bitmap image, final Bitmap blusher, int color, float amount)
	{
		if(BuildConfig.DEBUG && points == null)
			throw new IllegalStateException("need to call detect() method first");
		
		Bitmap result = image.copy(Bitmap.Config.ARGB_8888, true);
		nativeBlendBlusher(result, blusher, points, color, amount);
		return result;
	}
	
	public Bitmap blendEyeLash(Bitmap image, final Bitmap eye_lash, int color, float amount)
	{
		if(BuildConfig.DEBUG && points == null)
			throw new IllegalStateException("need to call detect() method first");
		
		Bitmap result = image.copy(Bitmap.Config.ARGB_8888, true);
		nativeBlendEyeLash(result, eye_lash, points, color, amount);
		return result;
	}
	
	/**
	 * composite a bitmap from alpha channel and specified color.
	 * @param image alpha mask, with RGB being 0x000000, anyway, it'll be overridden by new color.
	 * @param color ARGB format constructed form #Color
	 * @param amount color blending amount
	 * @return image
	 */
	public static Bitmap createBitmap(Bitmap image, int color)
	{
		if(BuildConfig.DEBUG && image.getConfig() != Bitmap.Config.ARGB_8888)
			throw new IllegalStateException("image must be RGBA format");
		
		if(NATIVE)
			nativeCreateBitmap(image, color);
		else
		{
//			Canvas canvas = new Canvas(image);
			
		}
		return image;
	}
	
	// tried to use #LayerDrawable 
	public static Bitmap merge(final Bitmap layer1, final Bitmap layer2, final Bitmap layer3)
	{
		Bitmap bitmap = layer1.copy(Bitmap.Config.ARGB_8888, true);
		Canvas canvas = new Canvas(bitmap);
		canvas.drawBitmap(layer2, 0, 0, null);
		canvas.drawBitmap(layer3, 0, 0, null);
		
		return bitmap;
	}
	
	public static Bitmap merge(final Bitmap layers[])
	{
		if(layers == null || layers.length <= 0)
			return null;
		
		Bitmap bitmap = layers[0].copy(Bitmap.Config.ARGB_8888, true);
		Canvas canvas = new Canvas(bitmap);
		
		for(int i = 1; i < layers.length; ++i)
			canvas.drawBitmap(layers[i], 0, 0, null);
		
		return bitmap;
	}
	
	public Bitmap blendEyeBrow(Bitmap image, final Bitmap eye_brow, float amount)
	{
		if(BuildConfig.DEBUG && points == null)
			throw new IllegalStateException("need to call detect() method first");
		
		Bitmap result = image.copy(Bitmap.Config.ARGB_8888, true);
		int tile_width = Math.round(points[25].x - points[20].x);
		int tile_height = 8;
		int tile_x = Math.round(points[20].x);
		int tile_y = Math.round(points[20].y - tile_height);
		Bitmap pattern = Bitmap.createBitmap(image, tile_x, tile_y, tile_width, tile_height);
		BitmapShader shader = new BitmapShader(pattern, Shader.TileMode.MIRROR, Shader.TileMode.MIRROR);

		Path path_eye_brow_r = getEyeBrowPath(points, 22, 21, 20 ,25, 24 ,23);
		Path path_eye_brow_l = getEyeBrowPath(points, 29, 28, 27, 26, 31, 30);
		
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

		nativeBlendEyeBrow(result, eye_brow, points, null/*eye_brow_points*/, amount);
		return result;
	}
	
	/**
	 * morph oval face into rectangle face for fun.
	 * @param bitmap
	 * @param time_ms
	 * @return morphing bitmap
	 */
	public Bitmap square(final Bitmap image, long time_ms)
	{
		// points are the feature points detected from this bitmap
		Bitmap result = image.copy(Bitmap.Config.ARGB_8888, true);
		nativeSquare(result, points, time_ms);
		return result;
	}
	
	private static native PointF[] nativeDetectFaceSingle(String image_path, String data_path);
	private static native PointF[] nativeDetectFaceSingle2(final Bitmap image, String data_path);
	
	private static native Bitmap nativeSeamlessClone(Bitmap src, Bitmap dst, Bitmap mask, PointF center, int flags);
	private static native Bitmap nativeSeamlessClone2(String src, String dst, PointF center, int flags);
	
	private native RoiInfo[] nativeDetect(String image_path, String data_dir);
	private native PointF nativeCenterOfRegion(int region);
	private native RoiInfo nativeCalculateRegionInfo(int region);

	private native static Mat nativeStretchImage(Mat src_image, Mat dst_image, PointF[] src_points, PointF[] dst_points);
	private native static Mat nativeStretchImage2(Mat src_image, int dst_width, int dst_height, PointF[] src_points, PointF[] dst_points);
	
	private native static void nativeOverlay(Mat src_image, Mat dst_image, int origin_x, int origion_y, int alpha);
	
	private static native Mat nativeCloneFace(String user_image_path, String model_image_path, String data_dir);
	
	private static native void nativeCreateBitmap(Bitmap mask, int color);
	
	private static native void getSymmetryAxis(final PointF[] points, PointF center, PointF up);
	
	// use bitmap instead of filename to reduce I/O, result = image + iris * amount x 2
	private static native void nativeBlendIris(Bitmap image, final Bitmap iris, final PointF points[], float amount);
	private static native void nativeBlendIris2(Bitmap image, final Bitmap iris, final Bitmap iris_mask, final PointF points[], int color, float amount);
	private static native void nativeBlendBlusher(Bitmap image, final Bitmap blusher, final PointF points[], int color, float amount);
	private static native void nativeBlendEyeBrow(Bitmap image, final Bitmap eye_brow, final PointF points[], final PointF[] eye_brow_points, float amount);
	private static native void nativeBlendEyeLash(Bitmap image, final Bitmap eye_lash, final PointF points[], int color, float amount);
	
//	private static native void nativeBlendEyeShadow(Bitmap image, final Bitmap eye_shadow[], final PointF points[], int color, float amout[]);
	// Android's Size class was brought in API level 21, too late, here use int instead.
//	private static native Bitmap nativeStretch(Bitmap image, int src_width, int src_height, int dst_width, int dst_height, PointF[] src_points, PointF[] dst_points);
	
	private static native void nativeSquare(final Bitmap bitmap, final PointF points[], long time_ms);
	
	static
	{
//		System.loadLibrary("c++_shared");
		System.loadLibrary("opencv_java3");
		System.loadLibrary("venus");
	}
	
}
