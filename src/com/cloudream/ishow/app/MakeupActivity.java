package com.cloudream.ishow.app;

import java.io.File;

import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.R;
import com.cloudream.ishow.algorithm.Feature;
import com.cloudream.ishow.algorithm.Makeup;
import com.cloudream.ishow.algorithm.Region;
import com.cloudream.ishow.algorithm.Makeup.BlushShape;
import com.cloudream.ishow.bean.MakeupParams;
import com.cloudream.ishow.util.BitmapUtils;
import com.cloudream.ishow.util.ColorUtils;
import com.cloudream.ishow.util.Compatibility;
import com.cloudream.ishow.util.MathUtils;
import com.cloudream.ishow.view.ImageViewTouch;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.support.annotation.FloatRange;
import android.support.annotation.NonNull;
import android.util.Log;
import android.util.TimingLogger;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Toast;
import android.widget.AdapterView;

public class MakeupActivity extends BaseActivity implements View.OnClickListener
{
	private static final String TAG = MakeupActivity.class.getSimpleName();
	
	private ImageViewTouch iv_image;

	private SeekBar   sb_weight;
	
	private LinearLayout ll_styles;
	
	private Region region;
	private int region_id = 0;
	
	// Spinner options
	private static final int OPTION_TEXTURE = 0;
	private static final int OPTION_COLOR   = 1;
	
	private int option = OPTION_TEXTURE;
	private int textures[];  //< OPTION_TEXTURE
	private int colors[];    //< OPTION_COLOR
	
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
	private static final int ROI_COLORS[][] = new int [Region.values().length][];
	
	private static final void initRegionColorArray(Resources res)
	{
		if(ROI_COLORS[0] != null)
			return;  // in case initialized once
		
		ROI_COLORS[Region.EYE_BROW  .ordinal()] = ColorUtils.obtainColorArray(res, R.array.eye_brow_colors);
		ROI_COLORS[Region.EYE_LASH  .ordinal()] = ColorUtils.obtainColorArray(res, R.array.eye_lash_colors);
		ROI_COLORS[Region.EYE_SHADOW.ordinal()] = ColorUtils.obtainColorArray(res, R.array.eye_shadow_colors);
		ROI_COLORS[Region.IRIS      .ordinal()] = ColorUtils.obtainColorArray(res, R.array.iris_colors);
		ROI_COLORS[Region.BLUSH     .ordinal()] = ColorUtils.obtainColorArray(res, R.array.blush_colors);
		ROI_COLORS[Region.LIP       .ordinal()] = ColorUtils.obtainColorArray(res, R.array.lip_colors);
//		ROI_COLORS[Region.SKIN      .ordinal()] = N/A;
	}
	
	private static final int[] obtainRegionColorArray(Region region)
	{
		return ROI_COLORS[region.ordinal()];
	}
	
	private static int[] dodgeArrayAllocation(int array[], int value)
	{
		if(array != null && array.length == 1)
			array[0] = value;
		else
			array = new int[]{ value };
		
		return array;
	}
	
	private void selectTexture(Region region, int index)
	{
		switch(region)
		{
		case EYE_SHADOW:
			// eye shadow use 3 layers for color blending
			index = R.drawable.eye_shadow_001 + index * 3;
			textures = new int[]{ index, index+1, index+2 };
			break;
		case IRIS:
			// iris use 2 layers for color blending
			index = R.drawable.iris_000 + index * 2;
			textures = new int[]{ index, index+1 };
			break;
		case LIP:
//			indices = null;  // dispensable, leave it alone.
			break;
		case EYE_LASH:
			textures = dodgeArrayAllocation(textures, R.drawable.eye_lash_00 + index);
			break;
		case EYE_BROW:
			textures = dodgeArrayAllocation(textures, R.drawable.eye_brow_mask_00 + index);
			break;
		case BLUSH:
			textures = dodgeArrayAllocation(textures, index);
			break;
		default:
			throw new UnsupportedOperationException("not implemented yet");
		}
	}
	
	// This listener controls variable textures[].
	private final View.OnClickListener lsn_texture = new View.OnClickListener()
	{
		@Override
		public void onClick(View v)
		{
			int index = (Integer)v.getTag();
			selectTexture(region, index);
		}
	};
	
	private void selectColor(Region region, int index)
	{
		int COLORS[] = obtainRegionColorArray(region);
		int color = COLORS[index];
		
		boolean single = true;
		switch(region)
		{
		case LIP:
			// TODO experiment, modify alpha or store alpha value in XML file.
			color = (color & 0x00FFFFFF) | (Color.alpha(color) >> 2 << 24);
//			Log.i(TAG, "use color: " + ColorUtils.colorToString(color));
			break;
		case BLUSH:
			// TODO experiment
//			color = (color & 0x00FFFFFF) | (((int)(amount * 128)) << 24);
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
			colors = dodgeArrayAllocation(colors, color);
	}
	
	// This listener controls variable colors[].
	private final View.OnClickListener lsn_color = new View.OnClickListener()
	{
		@Override
		public void onClick(View v)
		{
			int index = (Integer)v.getTag();
			selectColor(region, index);
			
			randomProgress(sb_weight);
		}
	};
	
	private final SeekBar.OnSeekBarChangeListener lsn_weight = new SeekBar.OnSeekBarChangeListener()
	{
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
		{
			float amount = (float)progress / seekBar.getMax();
			
			TimingLogger timings = new TimingLogger(TAG, "makeup");
			
			applyCosmestic(MakeupActivity.this, makeup, region, textures, colors, amount);
			
			timings.addSplit("applyCosmestic");
			timings.dumpToLog();
			
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
	
	private final AdapterView.OnItemSelectedListener lsn_options = new AdapterView.OnItemSelectedListener()
	{
		@Override
		public void onItemSelected(AdapterView<?> parent, View view, int position, long id)
		{
			// Lip has only one parameter--color, so do nothing for lip.
			if(option == position || region == Region.LIP)
				return;
			
			option = position;
			updateCosmeticContent(region, option);
		}

		@Override
		public void onNothingSelected(AdapterView<?> parent)
		{
			// do nothing
		}
	};

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.header, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch(item.getItemId())
		{
		case R.id.back:
			onBackPressed();
			return true;
		case R.id.save:
			SettingsActivity.saveImage(this, makeup.getIntermediateImage(), null);
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}
	
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
			image = BitmapUtils.decodeFile(name, BitmapUtils.OPTION_RGBA8888);
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
		
		iv_image.setImageBitmap(image);
//		tv_reset = (TextView) findViewById(R.id.reset);
		findViewById(R.id.reset).setOnClickListener(this);
		
		ll_styles = (LinearLayout) findViewById(R.id.styles);
		
		findViewById(R.id.blush)     .setOnClickListener(this);
		findViewById(R.id.eye_lash)  .setOnClickListener(this);
		findViewById(R.id.eye_shadow).setOnClickListener(this);
		findViewById(R.id.eye_brow)  .setOnClickListener(this);
		findViewById(R.id.iris)      .setOnClickListener(this);
		findViewById(R.id.lip)       .setOnClickListener(this);
		
		initRegionColorArray(this.getResources());
		
		Spinner spinner = (Spinner) findViewById(R.id.spinner);
		ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this,
		        R.array.cosmetic_option, android.R.layout.simple_spinner_item);
		adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		spinner.setAdapter(adapter);
		spinner.setOnItemSelectedListener(lsn_options);
		
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
			private float x, y;
			
			@Override
			public boolean onTouch(View v, MotionEvent event)
			{
				final int action = event.getAction();
				switch(action)
				{
				case MotionEvent.ACTION_DOWN:
					x = event.getX();
					y = event.getY();
					return false;
				default:
					if(x == event.getX() && y == event.getY())
					{
						if(action != MotionEvent.ACTION_UP)
							iv_image.setImageBitmap(makeup.getRawImage());
						else
							iv_image.setImageBitmap(makeup.getIntermediateImage());
						
						return true;
					}
					else
						return iv_image.onTouchEvent(event);
				}
			}
		});

		// the state of first time in: Region.LIP & color index 0
		findViewById(R.id.lip).performClick();
	}
	
	private static void randomProgress(final SeekBar slider)
	{
		int max = slider.getMax();
		final float low = 0.25F, high = 0.75F;
		int progress = MathUtils.random((int)(max * low), (int)(max * high));
		slider.setProgress(progress);
	}
	
	private void updateCosmeticContent(Region region, int option)
	{
		ll_styles.removeAllViews();
		
		if(region == Region.LIP)  // Special case, Region.LIP has only one option.
		{
			int COLORS[] = obtainRegionColorArray(Region.LIP);
//			for(int i = 0; i < COLORS.length; ++i)
			for(int i = id_start; i <= id_stop; ++i)
			{
				final int color = COLORS[i];
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
		else if(option == OPTION_TEXTURE)
		{
			final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
			
			for(int i = id_start; i <= id_stop; ++i)
			{
				final ImageView iv_style = new ImageView(this);
				
				iv_style.setImageResource(i);
				iv_style.setScaleType(ImageView.ScaleType.CENTER_CROP);
				iv_style.setTag((Integer)(i - id_start));
				ll_styles.addView(iv_style, params);
				iv_style.setOnClickListener(lsn_texture);
			}
			
			// default color parameter
			if(colors == null)
				selectColor(region, 0);
		}
		else if(option == OPTION_COLOR)
		{
			final Drawable circle = Compatibility.getDrawable(this, R.drawable.circle);
			final int COLORS[] = obtainRegionColorArray(region);
			
			for(int i = 0; i < COLORS.length; ++i)
			{
				Drawable drawable = circle.mutate().getConstantState().newDrawable();
				drawable.setColorFilter(COLORS[i], PorterDuff.Mode.SRC_ATOP);
				
				final ImageView iv_color = new ImageView(MakeupActivity.this);
				iv_color.setLayoutParams(LAYOUT_PARAMS);
				iv_color.setImageDrawable(drawable);
				iv_color.setTag((Integer)(i));
				ll_styles.addView(iv_color);
				iv_color.setOnClickListener(lsn_color);
			}
			
			// default texture parameter
			if(textures == null);
				selectTexture(region, 0);
		}
	}
	
	/**
	 * All the makeup stuff in one go, with corresponding parameters.
	 * 
	 * @param context
	 * @param makeup  #Makeup
	 * @param region  #Region
	 * @param textures <ul>
	 *                	<li>For {@link Region#EYE_BROW} single drawable resource of the cosmetics.</li>
	 *                	<li>For {@link Region#EYE_LASH} ditto.</li>
	 *                	<li>For {@link Region#EYE_SHADOW} multiple drawable resources of the cosmetics.
	 *                  Note that they are masks loaded with Bitmap.Config.ALPHA_8 parameter.
	 *                  </li>
	 *                	<li>For {@link Region#IRIS} multiple drawable resources of the cosmetics.</li>
	 *                	<li>For {@link Region#BLUSH}, it's {@link Makeup.BlushShape} ordinal.</li>
	 *                	<li>For {@link Region#NOSE} </li>
	 *                	<li>For {@link Region#LIP}, unused, passing <code>null</code> value is OK.</li>
	 *                <ul>
	 * @param colors  Color of the cosmetics. {@link Region#EYE_SHADOW} use multiple colors probably,
	 *                since they enhance the face's beauty.
	 * @param amount  Blending amount in range [0, 1], 0 being no effect, 1 being fully applied.
	 * 
	 * @see {@link android.graphics.Bitmap.Config#ALPHA_8}
	 */
	public void applyCosmestic(Context context, @NonNull Makeup makeup, Region region,
			int textures[], @NonNull int colors[], @FloatRange(from=0.0D, to=1.0D) float amount)
	{
		switch(region)
		{
		case LIP:
			makeup.applyLip(colors[0], amount);
			break;
		case BLUSH:
			makeup.applyBlush(Makeup.BlushShape.values()[textures[0]], colors[0], amount);
			break;
		case EYE_BROW:
		{
			Bitmap eye_brow = BitmapFactory.decodeResource(context.getResources(), textures[0], BitmapUtils.OPTION_A8);
			makeup.applyBrow(eye_brow, colors[0], amount);
		}
			break;
		case IRIS:
			break;
		case EYE_LASH:
		{
			Bitmap eye_lash = BitmapFactory.decodeResource(context.getResources(), textures[0], BitmapUtils.OPTION_A8);
			makeup.applyEyeLash(eye_lash, colors[0], amount);
		}
			break;
		case EYE_SHADOW:
		{
			final int length = textures.length;
			Bitmap mask[] = new Bitmap[length];
			for(int i = 0; i < length; ++i)
				mask[i] = BitmapFactory.decodeResource(context.getResources(), textures[i], BitmapUtils.OPTION_A8);
			
			makeup.applyEyeShadow(mask, colors, amount);
		}
			break;
		default:
			throw new UnsupportedOperationException("not implemented yet");
		}
	}
	
	@Override
	public void onClick(View view)
	{
		Region last_region = region;
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
			id_stop  = R.drawable.blusher01 + BlushShape.values().length - 1;  // TODO resource
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
			id_start = R.drawable.thumb_eye_shadow_00;
			id_stop  = R.drawable.thumb_eye_shadow_09;
			break;
		case R.id.eye_brow:
			region = Region.EYE_BROW;
			region_id = R.id.eye_brow;
			id_start = R.drawable.thumb_eye_brow_00;
			id_stop  = R.drawable.thumb_eye_brow_27;
			break;
		case R.id.iris:
			region = Region.IRIS;
			region_id = R.id.iris;
			id_start = R.drawable.thumb_iris_00;
			id_stop  = R.drawable.thumb_iris_08;
			break;
		case R.id.lip:
			region = Region.LIP;
			region_id = R.id.lip;
			id_start = 0;
			id_stop  = obtainRegionColorArray(region).length - 1;
			break;
		default:
			region_id = 0;
			return;
		}

		if(last_region != region)
			updateCosmeticContent(region, option);
	}
	
}
