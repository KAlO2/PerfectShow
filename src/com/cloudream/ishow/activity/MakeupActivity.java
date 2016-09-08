package com.cloudream.ishow.activity;

import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.R;
import com.cloudream.ishow.algorithm.Effect;
import com.cloudream.ishow.algorithm.FaceDetector;
import com.cloudream.ishow.algorithm.FaceDetector.Roi;
import com.cloudream.ishow.algorithm.FaceDetector.RoiInfo;
import com.cloudream.ishow.bean.MakeupInfo;
import com.cloudream.ishow.util.BitmapUtils;
import com.cloudream.ishow.util.Compatibility;
import com.cloudream.ishow.util.MathUtils;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

public class MakeupActivity extends BaseActivity implements View.OnClickListener
{
	private static final String TAG = MakeupActivity.class.getSimpleName();
	
	private ImageView iv_image;
//	private TextView  tv_reset;

	private SeekBar   sb_weight;
	private SeekBar   sb_size;
	
	private HorizontalScrollView hsv_primary;
	private HorizontalScrollView hsv_secondary;
	private LinearLayout ll_styles;
	private LinearLayout ll_regions;
	
	private FaceDetector.Roi region;
//	private View last_region;
	
	private FaceDetector detector;
	
	private Bitmap bmp_raw;
	private Bitmap bmp_step;     // used to undo(Ctrl +　Z) modification
	
	// android.graphics.Point <- int x, y;
	// org.opencv.core.Point  <- double x, y;
	// same size
	private Rect   rect_region;
	private Point  position;
	private Bitmap bmp_region;
	private Bitmap bmp_mask;
	private Bitmap bmp_modified;
	
	
//	private static final int PINK = 0xffc84b70;
	
	private PointF region_center_l;
	private PointF region_center_r;
	
	// I haven't dug but it seems like the id is ascending with the same prefix.
	// So I can use first and last for indexing the IDs.
	// R.id.xxx R.drawable.xxx id begin with 0x7f
	// [start, end], with end inclusive.
	private int res_start, res_stop, res_selected;  // res_selected store color for lips
	private RoiInfo info;
	private MakeupInfo makeup;
	
	private String name;
	// (11, 36) (55, 12) (98, 20) (143, 46) (98, 36) (55, 27)
	// http://docs.opencv.org/3.0-beta/doc/tutorials/introduction/android_binary_package/dev_with_OCV_on_Android.html

/*	
	class ImageArrayAdapter extends ArrayAdapter<Integer> {
	    private LayoutInflater inflater;
	    private Integer[] values;
	    
	    public ImageArrayAdapter(Context context, Integer[] values) {
	        super(context, R.layout.simple_list_item_image, values);
	        inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	        this.values = values;
	    }

	    @Override
	    public View getView(int position, View convertView, ViewGroup parent) {

	        View view = (convertView != null) ? convertView
					: inflater.inflate(R.layout.simple_list_item_image, parent, false);
	        ImageView iv_thumbnail = (ImageView)view.findViewById(R.id.thumbnail);
	        int index = this.getItem(position);
	        Bitmap bitmap = BitmapFactory.decodeResource(getResources(), R.drawable.example);
	        iv_thumbnail.setImageBitmap(bitmap);
	        return view;
	    }

	}
*/	
	// Android #ARGB format
	private static String colorToString(int color)
	{
		return String.format("#%02X%02X%02X%02X", 
				(byte)(Color.alpha(color)&0xff),
				(byte)(Color.red  (color)&0xff), 
				(byte)(Color.green(color)&0xff),
				(byte)(Color.blue (color)&0xff));
	}

	private static final int BLUSHER_COLORS[] = 
	{
		0xffffddc8, 0xffffc0d5, 0xffffbca7, 0xffff9e8c, 0xffffc4db,
		0xffffacac, 0xfffd9dc1, 0xffffaeae, 0xffffc4db, 0xffffc7b5,
		0xffffc1d8, 0xffffb9cc, 0xfff999a3, 0xffffa3bc, 0xfff7846a,
		0xffffa4bd, 0xfff3899c, 0xffff9985, 0xffff9884, 0xffff84c9,
	};
	
	private final SeekBar.OnSeekBarChangeListener lsn_weight = new SeekBar.OnSeekBarChangeListener()
	{
//		final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
//		final ColorMatrix matrix = new ColorMatrix();
		
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
		{
			float amount = (float)progress / seekBar.getMax();
			
			switch(region)
			{
			case LIPS:
			{
				int color = Compatibility.getColor(MakeupActivity.this, res_selected);
				color = Color.argb(Color.alpha(color)>>2, Color.red(color), Color.green(color), Color.blue(color));
				Log.i(TAG, "use color: " + colorToString(color));
				bmp_modified = detector.blendIris(bmp_step, color, amount);
			}
				break;
			case BLUSHER_R:
			{
				int id = res_selected - R.drawable.blusher01;
				int color = BLUSHER_COLORS[id];
				id = MathUtils.wrap(id, R.drawable.blusher_mask_00, R.drawable.blusher_mask_04);
				
				color = (color & 0x00ffffff) | (((int)(amount * 128/* 255 is too high */)) << 24);
				Bitmap bmp_blusher = BitmapFactory.decodeResource(getResources(), id, BitmapUtils.OPTION_RGBA_8888);
				bmp_modified = detector.blendBlusher(bmp_step, bmp_blusher, color, amount);
			}
				break;
			case IRIS_R:
				int iris_index = res_selected - R.drawable.thumb_iris_00;
				Bitmap bmp_iris_color = BitmapFactory.decodeResource(getResources(), R.drawable.iris_00 + iris_index, BitmapUtils.OPTION_RGBA_8888);
				Bitmap bmp_iris_mask = BitmapFactory.decodeResource(getResources(), R.drawable.iris_mask_00 + iris_index, BitmapUtils.OPTION_RGBA_8888);
				bmp_iris_color = Effect.tone(bmp_iris_color, Color.BLUE);
				SettingsActivity.saveImage(MakeupActivity.this, null, bmp_iris_color);
				bmp_modified = detector.blendIris(bmp_step, bmp_iris_color, amount);
				break;
			case EYE_LASH_R:
				int eye_lash_id = res_selected - R.drawable.thumb_eye_lash_00 + R.drawable.eye_lash_00;
				Bitmap bmp_eye_lash = BitmapFactory.decodeResource(getResources(), eye_lash_id, BitmapUtils.OPTION_RGBA_8888);
				bmp_modified = detector.blendEyeLash(bmp_step, bmp_eye_lash, Color.BLACK, amount);
				break;
			case EYE_BROW_R:
				Bitmap bmp_eye_brow = BitmapFactory.decodeResource(getResources(), res_selected, BitmapUtils.OPTION_RGBA_8888);
				bmp_modified = detector.blendEyeBrow(bmp_step, bmp_eye_brow, amount);
				break;
			case EYE_SHADOW_R:
				// eye shadow use 3 layers for color blending
				int eye_shadow_id = (res_selected - R.drawable.thumb_eye_shadow_00) * 3 + R.drawable.eye_shadow_001;
				Bitmap bmp_eye_shadow_1 = BitmapFactory.decodeResource(getResources(), eye_shadow_id + 0, BitmapUtils.OPTION_RGBA_8888);
				Bitmap bmp_eye_shadow_2 = BitmapFactory.decodeResource(getResources(), eye_shadow_id + 1, BitmapUtils.OPTION_RGBA_8888);
				Bitmap bmp_eye_shadow_3 = BitmapFactory.decodeResource(getResources(), eye_shadow_id + 2, BitmapUtils.OPTION_RGBA_8888);

				bmp_eye_shadow_1 = Effect.tone(bmp_eye_shadow_1, 0x00ffb5b5);
				bmp_eye_shadow_2 = Effect.tone(bmp_eye_shadow_2, 0x00ff7575);
				bmp_eye_shadow_3 = Effect.tone(bmp_eye_shadow_3, 0x00613030);
				Bitmap bmp_eye_shadow = FaceDetector.merge(bmp_eye_shadow_1, bmp_eye_shadow_2, bmp_eye_shadow_3);
//				SettingsActivity.saveImage(MakeupActivity.this, null, bmp_eye_shadow);
				bmp_modified = detector.blendEyeLash(bmp_step, bmp_eye_shadow, Color.BLACK, amount);
				break;
			}
			
			iv_image.setImageBitmap(bmp_modified);
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
	
	private final SeekBar.OnSeekBarChangeListener lsn_size = new SeekBar.OnSeekBarChangeListener()
	{
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
		{
			switch(region)
			{
			case IRIS_R:
				break;
			}
			
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
		setContentView(R.layout.activity_makeup);
		
//		name = String.format("%s/example/example%d.jpg", App.WORKING_DIRECTORY, (int) (Math.random() * 8) + 1);
		name = getIntent().getStringExtra(GalleryActivity.EXTRA_PICTURE_PATH);
		
		Log.i(TAG, "example name: " + name);
		bmp_raw = BitmapFactory.decodeFile(name, BitmapUtils.OPTION_RGBA_8888);
		if(bmp_raw == null)
		{
			Toast.makeText(this, R.string.corrupted_image, Toast.LENGTH_LONG).show();
			finish();
			return;
		}
		
		bmp_modified = bmp_raw.copy(Bitmap.Config.ARGB_8888, true);

		iv_image = (ImageView) findViewById(R.id.image);
		
		detector = new FaceDetector(this);
		final boolean has_face = detector.detect(name);
		if(!has_face)
		{
			Toast.makeText(this, R.string.no_face_detected, Toast.LENGTH_LONG).show();
			this.finish();
		}
		final PointF[] points = detector.getFeaturePoints();
		Log.i(TAG, points.length + " feature points found");
		if(points.length == 0)
		{
			Toast.makeText(this, "Face detection failed, please try another photo.", Toast.LENGTH_SHORT).show();
			this.finish();
		}
		
		sb_weight = (SeekBar) findViewById(R.id.weight);
		sb_weight.setOnSeekBarChangeListener(lsn_weight);
		sb_weight.setVisibility(View.INVISIBLE);
		
		sb_size = (SeekBar) findViewById(R.id.size);
		
		
		iv_image.setImageBitmap(bmp_raw);
//		tv_reset = (TextView) findViewById(R.id.reset);
		findViewById(R.id.reset).setOnClickListener(this);
		
		hsv_primary = (HorizontalScrollView) findViewById(R.id.primary);
		hsv_secondary = (HorizontalScrollView) findViewById(R.id.secondary);
		ll_styles = (LinearLayout) findViewById(R.id.styles);
		ll_regions = (LinearLayout) findViewById(R.id.regions);
		
		findViewById(R.id.blusher).setOnClickListener(this);
		findViewById(R.id.eye_lash).setOnClickListener(this);
		findViewById(R.id.eye_shadow).setOnClickListener(this);
		findViewById(R.id.eye_brow).setOnClickListener(this);
		findViewById(R.id.iris).setOnClickListener(this);
		findViewById(R.id.lip_color).setOnClickListener(this);
		
		if(BuildConfig.DEBUG)
		{
			CheckBox cb_mark = (CheckBox) findViewById(R.id.mark);
			cb_mark.setVisibility(View.VISIBLE);
			cb_mark.setOnCheckedChangeListener(new CheckBox.OnCheckedChangeListener()
			{
				@Override
				public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
				{
					Bitmap bmp_feature = FaceDetector.mark(bmp_raw, points);
					iv_image.setImageBitmap(isChecked? bmp_feature : bmp_modified);
				}}
			);
		}
		
		iv_image.setOnTouchListener(new View.OnTouchListener()
		{
			@Override
			public boolean onTouch(View v, MotionEvent event)
			{
				final int action = event.getAction();
				iv_image.setImageBitmap(action != MotionEvent.ACTION_UP ? bmp_raw:bmp_modified);
				return true;
			}
		});

	}
	
	private static enum Range {LOW, MEDIUM, HIGH}
	private static void randomProgress(final SeekBar slider, Range range)
	{
		int max = slider.getMax();
		final float empty, full;
		
		switch(range)
		{
		case LOW:    empty = 0.10f; full = 0.60f; break;
		case MEDIUM: empty = 0.25f; full = 0.75f; break;
		case HIGH:   empty = 0.40f; full = 0.90f; break;
		default:     empty = 0.00f; full = 1.00f; break;
		}
		
		int progress = MathUtils.random((int)(max * empty), (int)(max * full));
		slider.setVisibility(View.VISIBLE);
		slider.setProgress(progress);
	}
	
	@Override
	public void onClick(View view)
	{
		switch(view.getId())
		{
//		case R.id.primary:
//			break;
		case R.id.reset:
			bmp_modified = bmp_step.copy(Bitmap.Config.ARGB_8888, true/* mutable */);
			iv_image.setImageBitmap(bmp_modified);
			return;
		case R.id.blusher:
			region = Roi.BLUSHER_R;
			res_start = R.drawable.blusher01;
			res_stop  = R.drawable.blusher20;
			region_center_l = detector.centerOfRegion(Roi.BLUSHER_L);
			region_center_r = detector.centerOfRegion(Roi.BLUSHER_R);
			break;
		case R.id.eye_lash:
			region = Roi.EYE_LASH_R;
			res_start = R.drawable.thumb_eye_lash_00;
			res_stop  = R.drawable.thumb_eye_lash_09;
			region_center_l = detector.centerOfRegion(Roi.EYE_LASH_L);
			region_center_r = detector.centerOfRegion(Roi.EYE_LASH_R);
			break;
		case R.id.eye_shadow:
			region = Roi.EYE_SHADOW_R;
			res_start = R.drawable.thumb_eye_shadow_00;//eye_shadow01;
			res_stop  = R.drawable.thumb_eye_shadow_09;//eye_shadow12;
			break;
		case R.id.eye_brow:
			region = Roi.EYE_BROW_R;
			res_start = R.drawable.eye_brow_00;
			res_stop  = R.drawable.eye_brow_15;
			region_center_l = detector.centerOfRegion(Roi.EYE_BROW_L);
			region_center_r = detector.centerOfRegion(Roi.EYE_BROW_R);
			break;
		case R.id.iris:
			region = Roi.IRIS_R;
			res_start = R.drawable.thumb_iris_00;
			res_stop  = R.drawable.thumb_iris_08;
			break;
		case R.id.lip_color:
			region = Roi.LIPS;
			res_start = R.color.lips_color_00;
			res_stop  = R.color.lips_color_11;
			break;
		default:
			return;
		}
		
//		copy bitmap bmp_modified to bmp_step but make it immutable, namely unchangeable.
		bmp_step = bmp_modified.copy(Bitmap.Config.ARGB_8888, false);
/*		
		if(last_region == view)
		{
			boolean visible = hsv_secondary.getVisibility() == View.VISIBLE;
			hsv_secondary.setVisibility(visible ? View.INVISIBLE : View.VISIBLE);
			return;
		}
		if(last_region != null)
			last_region.setBackgroundColor(Color.WHITE);
		view.setBackgroundColor(PINK);
		last_region = view;
*/
		hsv_secondary.removeAllViews();
		ll_styles.removeAllViews();
		
//		Bitmap bitmap = BitmapFactory.decodeResource(getResources(), pair_region.first);
//		bmp_mask = bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Bitmap.Config.ARGB_8888);
//		Canvas canvas = new Canvas(bmp_mask);
//		canvas.drawColor(Color.WHITE);
		if(region == Roi.LIPS)  // lips use pure color instead of images
		{
			final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(120, 100);
			final int margin = 5;  // px
			params.setMargins(margin, margin, margin, margin);  // (left, top, right, bottom);
			
			final View.OnClickListener lsn_color = new View.OnClickListener()
			{
				@Override
				public void onClick(View v)
				{
					res_selected = (Integer)v.getTag();
					info = detector.calculateRegionInfo(Roi.LIPS);
					randomProgress(sb_weight, Range.MEDIUM);
				}
			};
//			final int[] lips_colors = MakeupActivity.this.getResources().getIntArray(R.array.lips_colors);
			for(int i = res_start; i <= res_stop; ++i)
			{
				final int color = Compatibility.getColor(MakeupActivity.this, i);
				GradientDrawable shape = new GradientDrawable();
				shape.setCornerRadius(4);
				shape.setColor(color);//(lips_colors[i]);
				
				final ImageView iv_color = new ImageView(this);
				iv_color.setLayoutParams(params);
				iv_color.setImageDrawable(shape);
				iv_color.setTag((Integer)(i));
				ll_styles.addView(iv_color);
				iv_color.setOnClickListener(lsn_color);
			}
		}
		else //if(pair_region != null)
		{
			final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
			
			for(int i = res_start; i <= res_stop; ++i)
			{
				final ImageView iv_style = new ImageView(this);
				
	//			if(bmp_style == null)
	//				Log.w(TAG, "decode resource file failed");
	//			iv_image.setImageBitmap(bmp_style);
				iv_style.setImageResource(i);
				iv_style.setScaleType(ImageView.ScaleType.CENTER_CROP);
				iv_style.setTag((Integer)(i));
				ll_styles.addView(iv_style, params);
				iv_style.setOnClickListener(new View.OnClickListener()
				{
					@Override
					public void onClick(View v)
					{
						res_selected = (int)v.getTag();
						randomProgress(sb_weight, (region == Roi.EYE_BROW_R)?Range.HIGH:Range.MEDIUM);
						
						Toast.makeText(MakeupActivity.this, "index: " + (res_selected - res_start), Toast.LENGTH_SHORT).show();
						long startTime = System.currentTimeMillis();
	/*					
						Bitmap bmp_style = BitmapFactory.decodeResource(getResources(), id, options);
						if(bmp_style.hasAlpha())
							Log.e(TAG, "this image has alpha channel");
						bmp_modified = FaceDetector.seamlessClone(bmp_style.copy(Bitmap.Config.RGB_565, false), bmp_raw.copy(Bitmap.Config.RGB_565, false), null, region_center_l, Photo.MIXED_CLONE);
						iv_image.setImageBitmap(bmp_modified);
	*/					
//						String src = String.format("/sdcard/PerfectShow/eye_lash/eye_lash_%02d.png", index);
//						bmp_modified = detector.SeamlessClone(src, name, region_center_r, org.opencv.photo.Photo.NORMAL_CLONE);
						iv_image.setImageBitmap(bmp_modified);
						long endTime = System.currentTimeMillis();
						String elapsed_time = "elapsed time: " + (endTime - startTime) + "ms";
						Log.d(TAG, elapsed_time);
					}
				});
				
			}
		}
		
		hsv_secondary.addView(ll_styles);
	}
}
