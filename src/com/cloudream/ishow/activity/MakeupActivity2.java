package com.cloudream.ishow.activity;

import com.cloudream.ishow.App;
import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.R;
import com.cloudream.ishow.algorithm.FaceDetector;
import com.cloudream.ishow.util.MakeupDatabaseHelper;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PointF;
import android.os.Bundle;
import android.support.annotation.IdRes;
import android.util.Log;
import android.util.Pair;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.Toast;

public class MakeupActivity2 extends Activity implements View.OnClickListener, View.OnTouchListener
{
	private static final String TAG = MakeupActivity2.class.getSimpleName();
	

	private ImageView iv_image;
//	private ImageView iv_model;
	
	private Bitmap bmp_raw;
	private Bitmap bmp_modified;
	private Canvas canvas;
	
	private Bitmap bmp_example_before;
	private Bitmap bmp_example_after;
	private Bitmap bmp_example_stretched;  
	private Bitmap bmp_temp;
	private Bitmap bmp_makeup;
	
	private Button btn_makeup;
	private String name;
	
	private FaceDetector detector;
	private SeekBar sb_weight;
	
	private int makeup_region_image;
		
	private static final BitmapFactory.Options options = new BitmapFactory.Options();
	static
	{
		options.inPreferredConfig = Bitmap.Config.ARGB_8888;
		options.inSampleSize = 1;
		options.inDither = false;
		
		// DON'T scale drawable/ images, OpenCV need the real size to process images.
		options.inScaled = false;
	}
	
	enum MakeupRegion
	{
		BLUSH,
		EYE_BROW,
		EYE_LASH,
		EYE_SHADOW,
		LIPS,
		NOSE,
	}
    
	private final Pair<MakeupRegion, Integer> params[] = new Pair[MakeupRegion.values().length];


	
	private final SeekBar.OnSeekBarChangeListener lsn_weight = new SeekBar.OnSeekBarChangeListener()
	{
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
		{
			if(makeup_region_image == 0)
				return;
			
//			Bitmap bmp_region = BitmapFactory.decodeResource(getResources(), makeup_region_image);
//			FaceDetector.overlay(bmp_region, bmp_modified, 0, 0, progress);
//			bmp_modified = FaceDetector.overlay(src, dst, left, top, alpha);
			bmp_temp = Bitmap.createBitmap(bmp_modified);
			FaceDetector.overlay2(bmp_example_stretched, bmp_temp, 0, 0, progress);
//			/*bmp_modified =*/ FaceDetector.overlay(bmp_example_stretched, bmp_modified, 0, 0);
			iv_image.setImageBitmap(bmp_temp);
		}

		@Override
		public void onStartTrackingTouch(SeekBar seekBar)
		{
			// TODO Auto-generated method stub
			
		}

		@Override
		public void onStopTrackingTouch(SeekBar seekBar)
		{
			// TODO Auto-generated method stub
			
		}
	};
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		this.setContentView(R.layout.makeup2_activity);
		
		iv_image = (ImageView) findViewById(R.id.image);
//		iv_model = (ImageView) findViewById(R.id.model);
		btn_makeup = (Button) findViewById(R.id.makeup);
		
		sb_weight = (SeekBar) findViewById(R.id.weight);
		sb_weight.setOnSeekBarChangeListener(lsn_weight);
		
//		name = String.format("%s/example/example%d.jpg", App.WORKING_DIRECTORY,  (int) (Math.random() * 8) + 1);
		name = getIntent().getStringExtra(GalleryActivity.EXTRA_PICTURE_PATH);
		Log.i(TAG, "example name: " + name);
		
		bmp_raw = BitmapFactory.decodeFile(name, options);
		bmp_modified = bmp_raw.copy(Bitmap.Config.ARGB_8888, true);
//		bmp_makeup = bmp_raw.copy(Bitmap.Config.ARGB_8888, true);
		bmp_makeup = Bitmap.createBitmap(bmp_raw.getWidth(), bmp_raw.getHeight(), Bitmap.Config.ARGB_8888);
		canvas = new Canvas(bmp_makeup);
		Log.i(TAG, "JPG image: " + bmp_raw.getConfig().toString());
		bmp_example_before = BitmapFactory.decodeResource(getResources(), R.drawable.model_before, options);
		bmp_example_after = BitmapFactory.decodeResource(getResources(), R.drawable.model_after, options);

		iv_image.setImageBitmap(bmp_raw);
//		iv_model.setImageBitmap(bmp_example_after);
		btn_makeup.setOnClickListener(this);
		
		iv_image.setOnTouchListener(this);
//		iv_model.setOnTouchListener(this);
		
        findViewById(R.id.region_blush).setOnClickListener(this);
        findViewById(R.id.region_eye_brow).setOnClickListener(this);
        findViewById(R.id.region_eye_lash).setOnClickListener(this);
        findViewById(R.id.region_eye_shadow).setOnClickListener(this);
        findViewById(R.id.region_lips).setOnClickListener(this);
        findViewById(R.id.region_nose).setOnClickListener(this);
        
		long startTime = System.currentTimeMillis();
		detector = new FaceDetector(this);
		if(!detector.detect(name))
		{
			Toast.makeText(this, "Face detection failed, please try another photo.", Toast.LENGTH_SHORT).show();
			this.finish();
		}
		long endTime = System.currentTimeMillis();
		String elapsed_time = "elapsed time: " + (endTime - startTime) + "ms";
		Log.d(TAG, elapsed_time);

	}

	@Override
	public boolean onTouch(View view, MotionEvent event)
	{
		int action = event.getAction();
		switch(view.getId())
		{
		case R.id.image:
			iv_image.setImageBitmap(action != MotionEvent.ACTION_UP ? bmp_raw:bmp_temp/*bmp_modified*/);
			return true;
//		case R.id.model:
//			iv_model.setImageResource(action != MotionEvent.ACTION_UP ? R.drawable.model_before : R.drawable.model_after);
//			iv_model.setImageBitmap(action != MotionEvent.ACTION_UP ? bmp_example_before : bmp_example_after);
//			return true;
		default:
			return false;
		}
	}
	
	private void drawBitmap(@IdRes int resId, float center_x, float center_y)
	{
		Bitmap bitmap = BitmapFactory.decodeResource(getResources(), resId, options);
		canvas.drawBitmap(bitmap, center_x - bitmap.getWidth()/2.0f, center_y - bitmap.getHeight()/2.0f, null);
	}
	
	@Override
	public void onClick(View v)
	{
		final int id = v.getId();
		if(id == R.id.makeup)
		{
			makeup();
			return;
		}
		
		final int drawable;
if(true){	
		
		switch(id)
		{
        case R.id.region_blush:      drawable = R.drawable.makeup2_cheek;      break;
        case R.id.region_eye_brow:   drawable = R.drawable.makeup2_eye_brow;   break;
        case R.id.region_eye_lash:   drawable = R.drawable.makeup2_eye_lash;   break;
        case R.id.region_eye_shadow: drawable = R.drawable.makeup2_eye_shadow; break;
        case R.id.region_lips:       drawable = R.drawable.makeup_lips;       break;
        case R.id.region_nose:       drawable = R.drawable.makeup_nose;       break;
        default:                     drawable = 0;                            break;
		}
		
		// stretch image if necessary
		if(drawable != 0)
		{
			Bitmap bmp_region = BitmapFactory.decodeResource(getResources(), drawable, options);
			
			final PointF[] src_points = MakeupDatabaseHelper.getFeaturePoints(0);
			final PointF[] dst_points = detector.getFeaturePoints();
		
			bmp_example_stretched = FaceDetector.stretchImage(bmp_region, bmp_modified.getWidth(), bmp_modified.getHeight(), src_points, dst_points);
		}
}else{
		canvas.drawColor(Color.TRANSPARENT);
		switch(id)
		{
        case R.id.region_blush:
        	drawBitmap(R.drawable.region2_blush_r, 192.0f, 467.0f);
        	drawBitmap(R.drawable.region2_blush_l, 447.5f, 475.0f);
        	drawable = R.drawable.makeup_blush;
        	break;
        case R.id.region_eye_brow:
        	drawBitmap(R.drawable.region2_eye_brow_r, 218.5f, 314.0f);
        	drawBitmap(R.drawable.region2_eye_brow_l, 426.5f, 315.0f);
        	drawable = R.drawable.makeup_eye_brow;
        	break;
        case R.id.region_eye_lash:
        	drawBitmap(R.drawable.region2_eye_lash_r, 218.5f, 314.0f);
        	drawBitmap(R.drawable.region2_eye_lash_l, 426.0f, 315.0f);
        	drawable = R.drawable.makeup_eye_lash;
        	break;
        case R.id.region_eye_shadow:
        	drawBitmap(R.drawable.region2_eye_shadow_r, 213.5f, 377.5f);
        	drawBitmap(R.drawable.region2_eye_shadow_l, 436.0f, 377.5f);
        	drawable = R.drawable.makeup_eye_shadow;
        	break;
        case R.id.region_lips:
         	drawBitmap(R.drawable.region2_lips, 319.0f, 560.0f);
         	drawable = R.drawable.makeup_lips;
        	break;
        case R.id.region_nose:
        	drawBitmap(R.drawable.makeup_nose, 0.0f, 0.0f);
        	drawable = R.drawable.makeup_nose;
        	break;
        default:
        	drawable = 0;
        	break;
		}
		
		Log.i(TAG, "canvas width: " + bmp_makeup.getWidth() + ", height: " + bmp_makeup.getHeight());
		Log.i(TAG, "bmp_modified width: " + bmp_modified.getWidth() + ", height: " + bmp_modified.getHeight());
		
		// stretch image if necessary
		if(drawable != 0)
		{
			final PointF[] src_points = MakeupDatabaseHelper.getFeaturePoints(0);
			final PointF[] dst_points = detector.getFeaturePoints();
		
			bmp_example_stretched = FaceDetector.stretchImage(bmp_makeup, bmp_modified.getWidth(), bmp_modified.getHeight(), src_points, dst_points);
		}
}		
		if(makeup_region_image != drawable)
		{
			// apply previous modification
			int alpha = sb_weight.getProgress();
//			assert(0 <= alpha && alpha < 256);
			if(BuildConfig.DEBUG && !(0 <= alpha && alpha < 256))
				throw new AssertionError("alpha is out of range [0, 256)");
			FaceDetector.overlay2(bmp_example_stretched, bmp_modified, 0, 0, alpha);
			
			// show current modification
			alpha = 128;
			bmp_temp = Bitmap.createBitmap(bmp_modified);
			FaceDetector.overlay2(bmp_example_stretched, bmp_temp, 0, 0, alpha);
			sb_weight.setProgress(alpha);
			iv_image.setImageBitmap(bmp_temp);
//			private TimingLogger log = new TimingLogger(TAG);


//			/*bmp_modified =*/ FaceDetector.overlay(bmp_example_stretched, bmp_modified, 0, 0);
			
			makeup_region_image = drawable;
		}
	}
	
	private void makeup()
	{
		long startTime = System.currentTimeMillis();
		
		final String makeup_before = App.getWorkingDirectory() + "/model_before.png";
		final String makeup_after = App.getWorkingDirectory() + "/model_after.png";
//		bmp_modified = FaceDetector.fusion(name, App.WORKING_DIRECTORY, makeup_before, makeup_after);
		Log.i(TAG, "clone " + name + " to " + makeup_after);
		bmp_modified = FaceDetector.cloneFace(name, makeup_after, App.getWorkingDirectory());
		
		iv_image.setImageBitmap(bmp_modified);
		
		long endTime = System.currentTimeMillis();
		String elapsed_time = "elapsed time: " + (endTime - startTime) + "ms";
		Toast.makeText(this, elapsed_time, Toast.LENGTH_SHORT).show();
		Log.d(TAG, elapsed_time);
	}
}
