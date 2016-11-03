package com.cloudream.ishow.activity;

import java.io.File;

import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.R;
import com.cloudream.ishow.algorithm.Feature;
import com.cloudream.ishow.algorithm.Makeup;
import com.cloudream.ishow.algorithm.Region;
import com.cloudream.ishow.bean.MakeupParams;
import com.cloudream.ishow.util.BitmapUtils;
import com.cloudream.ishow.util.ColorUtils;
import com.cloudream.ishow.util.Compatibility;
import com.cloudream.ishow.util.MathUtils;
import com.cloudream.ishow.view.ImageViewTouch;

import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.Toast;
import android.widget.AdapterView;

public class MakeupActivity extends BaseActivity implements View.OnClickListener
{
	private static final String TAG = MakeupActivity.class.getSimpleName();
	
	private ImageViewTouch iv_image;
//	private TextView  tv_reset;

	private SeekBar   sb_weight;
	private SeekBar   sb_size;
	
	private LinearLayout ll_styles;
	private LinearLayout ll_regions;
	
	private Region region;
	private int region_id = 0;
//	private View last_region;
	
	private int indices[];
	private int colors[];
	
	private Makeup makeup;
	
	// I haven't dug but it seems like the id is ascending with the same prefix.
	// So I can use first and last for indexing the IDs.
	// R.id.xxx R.drawable.xxx id begin with 0x7f
	// [start, end], with end inclusive.
	private int id_start, id_stop, id_selected;  // res_selected store color for lips
	private int color_index = 0;
	private MakeupParams makeup_params;
	
	static final LinearLayout.LayoutParams LAYOUT_PARAMS;
	static
	{
		// 120x100 tweak size if necessary.
		LAYOUT_PARAMS = new LinearLayout.LayoutParams(120, 100);
		
		final int margin = 5;  // pixels
		LAYOUT_PARAMS.setMargins(margin, margin, margin, margin);  // (left, top, right, bottom);
	}
	
	// These arrays are stored in R.array.color_arrays.xml
	// They can be final static since it need context to load resources.
	private final int ROI_COLORS[][] = new int [Region.values().length][];
	
	private final int[] obtainRegionColorArray(Region region)
	{
		int roi = region.ordinal();
		int array[] = ROI_COLORS[roi];
		if(array != null)
			return array;
		
		switch(region)
		{
		case EYE_BROW:   array = ColorUtils.obtainColorArray(this, R.array.eye_brow_colors);   break;
		case EYE_LASH:   array = ColorUtils.obtainColorArray(this, R.array.eye_lash_colors);   break;
		case EYE_SHADOW: array = ColorUtils.obtainColorArray(this, R.array.eye_shadow_colors); break;
		case IRIS:       array = ColorUtils.obtainColorArray(this, R.array.iris_colors);       break;
		case BLUSH:      array = ColorUtils.obtainColorArray(this, R.array.blush_colors);      break;
		case LIP:        array = ColorUtils.obtainColorArray(this, R.array.lip_colors);        break;
		default:         throw new UnsupportedOperationException("not implemented yet");
		}
	
		return array;
	}
	
	// This listener controls variable indices[].
	private final View.OnClickListener lsn_texture = new View.OnClickListener()
	{
		@Override
		public void onClick(View v)
		{
			int index = (Integer)v.getTag();
			switch(region)
			{
			case IRIS:
				// iris use 2 layers for color blending
				int start = (index - R.drawable.thumb_iris_00) * 2 + R.drawable.iris_000;
				indices = new int[]{ start, start+1 };
				break;
			case EYE_SHADOW:
				// eye shadow use 3 layers for color blending
				start = (index - R.drawable.thumb_eye_shadow_00) * 3 + R.drawable.eye_shadow_001;
				indices = new int[]{ start, start+1, start+2 };
				break;
			default:
				if(indices != null && indices.length == 1)
					indices[0] = index;
				else
					indices = new int[]{ index };
				break;
			}
		}

	};
	
	// This listener controls variable colors[].
	private final View.OnClickListener lsn_color = new View.OnClickListener()
	{
		@Override
		public void onClick(View v)
		{
			int index = (Integer)v.getTag();
			int COLORS[] = obtainRegionColorArray(region);
			int color = COLORS[index];
			
			boolean single = true;
			switch(region)
			{
			case LIP:
				// TODO experiment, modify alpha or store alpha value in XML file.
				color = (color & 0x00FFFFFF) | (Color.alpha(color) >> 2 << 24);
//				Log.i(TAG, "use color: " + ColorUtils.colorToString(color));
				break;
			case BLUSH:
				// TODO experiment
//				color = (color & 0x00FFFFFF) | (((int)(amount * 128)) << 24);
				color = (color & 0x00FFFFFF) | (0x80 << 24);
				break;
			case EYE_SHADOW:
				single = false;  // currently only Region#EYE_SHADOW use multiple colors.
				colors = new int[]{ color, COLORS[index+1], COLORS[index+2] };
				break;
			default:
				break;
			}
			
			if(single)
			{
				if(colors != null && colors.length == 1)
					colors[0] = color;
				else
					colors = new int[]{ color };
			}
			
			randomProgress(sb_weight);
		}
	};
	
	// This listener controls variable indices[].
	private final SeekBar.OnSeekBarChangeListener lsn_weight = new SeekBar.OnSeekBarChangeListener()
	{
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
		{
			float amount = (float)progress / seekBar.getMax();
			
			switch(region)
			{
			case LIP:
			{
				int color = obtainRegionColorArray(region)[id_selected];
//				color = Color.argb(Color.alpha(color)>>2, Color.red(lip_color), Color.green(lip_color), Color.blue(lip_color));
				color = (color & 0x00FFFFFF) | (Color.alpha(color) >> 2 << 24);
				Log.i(TAG, "use color: " + ColorUtils.colorToString(color));
				makeup.applyLipColor(color, amount);
			}
				break;
			case BLUSH:
			{
				int id = id_selected - R.drawable.blusher01;
				int color = obtainRegionColorArray(region)[id];
/*
				id = MathUtils.wrap(id, R.drawable.blush_mask_00, R.drawable.blush_mask_04);
				
				color = (color & 0x00ffffff) | (((int)(amount * 128)) << 24);
				Bitmap bmp_blush = BitmapFactory.decodeResource(getResources(), id, BitmapUtils.OPTION_RGBA_8888);
				bmp_modified = detector.blendBlusher(bmp_step, bmp_blush, color, amount);
*/
				id = MathUtils.wrap(id, R.drawable.blush_mask_00, R.drawable.blush_mask_04);
				final Makeup.BlushShape shapes[] = Makeup.BlushShape.values();
				Makeup.BlushShape shape = shapes[id % shapes.length];
				makeup.applyBlush(shape, color, amount);
			}
				break;
			case EYE_BROW:
				Bitmap bmp_eye_brow = BitmapFactory.decodeResource(getResources(), id_selected, BitmapUtils.OPTION_RGBA_8888);
				makeup.applyBrow(bmp_eye_brow, amount);
				break;
			
			case IRIS:
				// TODO
				Bitmap bmp_iris_color = BitmapFactory.decodeResource(getResources(), id_selected, BitmapUtils.OPTION_RGBA_8888);
//				bmp_modified = makeup.blendIris(bmp_step, bmp_iris_color, amount);
/*
				int iris_index = res_selected - R.drawable.thumb_iris_00;
				Bitmap bmp_iris_color = BitmapFactory.decodeResource(getResources(), R.drawable.iris_00 + iris_index, BitmapUtils.OPTION_RGBA_8888);
				Bitmap bmp_iris_mask = BitmapFactory.decodeResource(getResources(), R.drawable.iris_mask_00 + iris_index, BitmapUtils.OPTION_RGBA_8888);
				bmp_iris_color = Effect.tone(bmp_iris_color, Color.BLUE);
				SettingsActivity.saveImage(MakeupActivity.this, null, bmp_iris_color);
				bmp_modified = detector.blendIris(bmp_step, bmp_iris_color, amount);
				bmp_modified = detector.blendIris(bmp_step, bmp_iris_color, bmp_iris_mask, Color.BLUE, amount);
				bmp_modified = detector.blendIris(bmp_step, bmp_iris, amount);
*/
				break;
			case EYE_LASH:
			{
				int eye_lash_id = id_selected - R.drawable.thumb_eye_lash_00 + R.drawable.eye_lash_00;
				Bitmap bmp_eye_lash = BitmapFactory.decodeResource(getResources(), eye_lash_id, BitmapUtils.OPTION_RGBA_8888);
				makeup.applyEyeLash(bmp_eye_lash, Color.BLACK, amount);
			}
				break;

			case EYE_SHADOW:
			{
				// eye shadow use 3 layers for color blending
				int eye_shadow_id = (id_selected - R.drawable.thumb_eye_shadow_00) * 3 + R.drawable.eye_shadow_001;
				Bitmap mask[] = new Bitmap[3];
				final BitmapFactory.Options options = new BitmapFactory.Options();
				options.inPreferredConfig = Bitmap.Config.ALPHA_8;
				for(int i = 0; i < 3; ++i)
					mask[i] = BitmapFactory.decodeResource(getResources(), eye_shadow_id + i, options);
				
				int color[] = new int[] { 0x00ffb5b5, 0x00ff7575, 0x00613030};

				makeup.applyEyeShadow(mask, color, amount);
			}
				break;

			}
			
			iv_image.setImageBitmap(makeup.getIntermediateImage());
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
		setContentView(R.layout.makeup_activity);
		
		final Bitmap image;
		final PointF points[];
//		name = String.format("%s/example/example%d.jpg", App.WORKING_DIRECTORY, (int) (Math.random() * 8) + 1);
		Intent intent = getIntent();
		String name = intent.getStringExtra(GalleryActivity.EXTRA_PICTURE_PATH);
		if(name != null)
		{
			Log.i(TAG, "image name: " + name);
			image = BitmapUtils.decodeFile(name, BitmapUtils.OPTION_RGBA_8888);
			points = Feature.detectFace(this, image, name);
			
			boolean temporary = intent.getBooleanExtra(GalleryActivity.EXTRA_TEMPORARY, false);
			if(temporary)
				new File(name).delete();
		}
		else
		{
//			Object object = intent.getParcelableExtra(GalleryActivity.EXTRA_PICTURE_BITMAP);
//			bmp_raw = (object instanceof Bitmap)? (Bitmap)object : null;
//			bmp_raw = intent.getParcelableExtra(GalleryActivity.EXTRA_PICTURE_BITMAP);
			byte bytes[] = intent.getByteArrayExtra(GalleryActivity.EXTRA_PICTURE_BITMAP);
			image = BitmapFactory.decodeByteArray(bytes, 0, bytes.length);
			
			if(image == null)
			{
				Toast.makeText(this, R.string.corrupted_image, Toast.LENGTH_LONG).show();
				finish();
				return;
			}
			
			points = Feature.detectFace(this, image, name);
		}
		
		if(points.length <= 0)
		{
			Toast.makeText(this, R.string.no_face_detected, Toast.LENGTH_LONG).show();
			finish();
			return;
		}

		iv_image = (ImageViewTouch) findViewById(R.id.image);
//		iv_image.setGestureListener(iv_image.new GestureListener());
		iv_image.setScrollEnabled(true);
		
		makeup = new Makeup(image, points);
		
		sb_weight = (SeekBar) findViewById(R.id.weight);
		sb_weight.setOnSeekBarChangeListener(lsn_weight);
//		sb_weight.setVisibility(View.INVISIBLE);
		
		sb_size = (SeekBar) findViewById(R.id.size);
		
		iv_image.setImageBitmap(image);
//		tv_reset = (TextView) findViewById(R.id.reset);
		findViewById(R.id.reset).setOnClickListener(this);
		
		ll_styles = (LinearLayout) findViewById(R.id.styles);
		ll_regions = (LinearLayout) findViewById(R.id.regions);
		
		findViewById(R.id.blush)     .setOnClickListener(this);
		findViewById(R.id.eye_lash)  .setOnClickListener(this);
		findViewById(R.id.eye_shadow).setOnClickListener(this);
		findViewById(R.id.eye_brow)  .setOnClickListener(this);
		findViewById(R.id.iris)      .setOnClickListener(this);
		findViewById(R.id.lip_color) .setOnClickListener(this);
		
		if(BuildConfig.DEBUG)  // only enable this CheckBox in debug mode
		{
			CheckBox cb_mark = (CheckBox) findViewById(R.id.mark);
			cb_mark.setVisibility(View.VISIBLE);
			cb_mark.setOnCheckedChangeListener(new CheckBox.OnCheckedChangeListener()
			{
				@Override
				public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
				{
					Bitmap bmp_mark = makeup.markFeaturePoints();
					iv_image.setImageBitmap(isChecked ? bmp_mark : makeup.getIntermediateImage());
				}}
			);
		}
		
		iv_image.setOnTouchListener(new View.OnTouchListener()
		{
			@Override
			public boolean onTouch(View v, MotionEvent event)
			{
				final int action = event.getAction();
				if(action != MotionEvent.ACTION_UP)
					iv_image.setImageBitmap(makeup.getRawImage());
				else
					iv_image.setImageBitmap(makeup.getIntermediateImage());
				return true;
			}
		});

		// default makeup option
		findViewById(R.id.lip_color).performClick();
	}
	
	private static void randomProgress(final SeekBar slider)
	{
		int max = slider.getMax();
		final float low = 0.25F, high = 0.75F;
		int progress = MathUtils.random((int)(max * low), (int)(max * high));
		slider.setVisibility(View.VISIBLE);
		slider.setProgress(progress);
	}
	
	@Override
	public void onClick(View view)
	{
		switch(view.getId())
		{
		case R.id.reset:
			iv_image.setImageBitmap(makeup.getFinalImage());
			sb_weight.setProgress(0);
			return;
		case R.id.blush:
			region = Region.BLUSH;
			region_id = R.id.blush;
			id_start = R.drawable.blusher01;
			id_stop  = R.drawable.blusher20;
			break;
		case R.id.eye_lash:
			region = Region.EYE_LASH;
			region_id = R.id.eye_lash;
			id_start = R.drawable.thumb_eye_lash_00;
			id_stop  = R.drawable.thumb_eye_lash_09;
			break;
		case R.id.eye_shadow:
			region = Region.EYE_SHADOW;
			region_id = R.id.eye_shadow;
			id_start = R.drawable.thumb_eye_shadow_00;//eye_shadow01;
			id_stop  = R.drawable.thumb_eye_shadow_09;//eye_shadow12;
			break;
		case R.id.eye_brow:
			region = Region.EYE_BROW;
			region_id = R.id.eye_brow;
			id_start = R.drawable.eye_brow_00;
			id_stop  = R.drawable.eye_brow_15;
			break;
		case R.id.iris:
			region = Region.IRIS;
			region_id = R.id.iris;
			id_start = R.drawable.thumb_iris_00;
			id_stop  = R.drawable.thumb_iris_08;
			break;
		case R.id.lip_color:
			region = Region.LIP;
			region_id = R.id.lip_color;
			id_start = 0;
			id_stop  = obtainRegionColorArray(region).length - 1;
			break;
		default:
			region_id = 0;
			return;
		}

		ll_styles.removeAllViews();
		
		if(region == Region.LIP)  // Special case, lips use pure color instead of images
		{
			for(int i = id_start; i <= id_stop; ++i)
			{
				final int color = obtainRegionColorArray(Region.LIP)[i];
				GradientDrawable shape = new GradientDrawable();
				shape.setCornerRadius(12);
				shape.setColor(color);
				
				final ImageView iv_color = new ImageView(this);
				iv_color.setLayoutParams(LAYOUT_PARAMS);
				iv_color.setImageDrawable(shape);
				iv_color.setTag((Integer)(i));
				ll_styles.addView(iv_color);
				iv_color.setOnClickListener(lsn_color);
			}
		}
		else //if(pair_region != null)
		{
			final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
			
			for(int i = id_start; i <= id_stop; ++i)
			{
				final ImageView iv_style = new ImageView(this);
				
				iv_style.setImageResource(i);
				iv_style.setScaleType(ImageView.ScaleType.CENTER_CROP);
				iv_style.setTag((Integer)(i));
				ll_styles.addView(iv_style, params);
				iv_style.setOnClickListener(new View.OnClickListener()
				{
					@Override
					public void onClick(View v)
					{
						id_selected = (Integer)v.getTag();
						randomProgress(sb_weight);
						
//						Toast.makeText(MakeupActivity.this, "index: " + (res_selected - res_start), Toast.LENGTH_SHORT).show();
						long startTime = System.currentTimeMillis();
						iv_image.setImageBitmap(makeup.getFinalImage());
						long endTime = System.currentTimeMillis();
						String elapsed_time = "elapsed time: " + (endTime - startTime) + "ms";
						Log.d(TAG, elapsed_time);
					}
				});
				
			}
		}
	}
}
