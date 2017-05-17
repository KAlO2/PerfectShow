package com.cloudream.ishow.app;

import android.content.Intent;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.drawable.Animatable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.ColorInt;
import android.support.annotation.IdRes;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.v4.util.Pair;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;

import java.io.File;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import com.cloudream.ishow.App;
import com.cloudream.ishow.R;
import com.cloudream.ishow.bean.AspectRatio;
import com.cloudream.ishow.bean.CropOptionBundle;
import com.cloudream.ishow.util.ColorUtils;
import com.cloudream.ishow.util.Compatibility;
import com.cloudream.ishow.view.CropView;
import com.cloudream.ishow.view.OverlayView;
import com.cloudream.ishow.widget.AspectRatioTextView;
import com.cloudream.ishow.widget.CropImageView;
import com.cloudream.ishow.widget.GestureCropImageView;
import com.cloudream.ishow.widget.HorizontalProgressWheelView;
import com.cloudream.ishow.widget.TransformImageView;

public class ImageCropActivity extends BaseActivity implements View.OnClickListener
{
	private static final String TAG = ImageCropActivity.class.getSimpleName();

	public static final int NONE = 0;
	public static final int SCALE = 1;
	public static final int ROTATE = 2;
	public static final int ALL = 3;

	@IntDef({ NONE, SCALE, ROTATE, ALL })
	@Retention(RetentionPolicy.SOURCE)
	public @interface GestureTypes
	{

	}
	
	// keep in order with ratios specified in XML file
	private static final float RATIOS[] = { 0, 1, 2.F/3, 3.F/4, 9.F/16, 3.F/2, 4.F/3, 16.F/9 };
	
	private static final int TABS_COUNT = 3;
	private static final int SCALE_WIDGET_SENSITIVITY_COEFFICIENT = 15000;
	private static final int ROTATE_WIDGET_SENSITIVITY_COEFFICIENT = 42;

	private String mToolbarTitle;

	// Enables dynamic coloring
	private int mToolbarColor;
	private int mStatusBarColor;
	private int mActiveWidgetColor;
	private int mToolbarWidgetColor;

	private boolean mShowBottomControls;
	private boolean mShowLoader = true;

	private CropView mCropView;
	private GestureCropImageView mGestureCropImageView;
	private OverlayView mOverlayView;
	private ViewGroup mWrapperStateAspectRatio, mWrapperStateRotate, mWrapperStateScale;
	private ViewGroup mLayoutAspectRatio, mLayoutRotate, mLayoutScale;
	private List<ViewGroup> mCropAspectRatioViews = new ArrayList<ViewGroup>();
	private TextView mTextViewRotateAngle, mTextViewScalePercent;
	private View mBlockingView;

	private Bitmap.CompressFormat mCompressFormat;
	private int mCompressQuality;
	private int[] mAllowedGestures = new int[]{ SCALE, ROTATE, ALL };

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_image_crop);

		final Intent intent = getIntent();
		String path = intent.getStringExtra(GalleryActivity.EXTRA_PICTURE_PATH);
		
		CropOptionBundle options = new CropOptionBundle();
		options.useSourceImageAspectRatio();
		
//		Pair<Bitmap.CompressFormat, Integer> pair = SettingsActivity.getFormatAndQuality(this);
//		options.setCompressionFormat(pair.first);
//		options.setCompressionQuality(pair.second);
		
		setupViews(intent);
		mGestureCropImageView.setImageUri(Uri.fromFile(new File(path)), Uri.fromFile(new File(App.getWorkingDirectory())));
		processOptions(options);
		setInitialState();
		addBlockingView();
	}

	private static void tintMenuItem(@NonNull MenuItem item, int color)
	{
		Drawable drawable = item.getIcon();
		if(drawable == null)
			return;
		
		drawable.mutate();
		drawable.setColorFilter(color, PorterDuff.Mode.SRC_ATOP);
		item.setIcon(drawable);
	}
	
	@Override
	public boolean onCreateOptionsMenu(final Menu menu)
	{
		getMenuInflater().inflate(R.menu.image_crop_activity, menu);

		// Change crop & loader menu icons color to match the rest of the UI colors
		MenuItem menuItemLoader = menu.findItem(R.id.loader);
		tintMenuItem(menuItemLoader, mToolbarWidgetColor);
		Drawable menuItemLoaderIcon = menuItemLoader.getIcon();
		if(menuItemLoaderIcon != null)
		{
			// override color resource (ucrop_color_toolbar_widget) in the app to make it work on pre-Lollipop devices
			((Animatable)menuItemLoaderIcon).start();
		}

		MenuItem menuItemCrop = menu.findItem(R.id.crop);
		tintMenuItem(menuItemCrop, mToolbarWidgetColor);

		return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu)
	{
		menu.findItem(R.id.crop).setVisible(!mShowLoader);
		menu.findItem(R.id.loader).setVisible(mShowLoader);
		return super.onPrepareOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch(item.getItemId())
		{
		case R.id.crop:
			cropAndSaveImage();
			break;
		case android.R.id.home:
			onBackPressed();
			break;
		default:
			return super.onOptionsItemSelected(item);
		}
		
		return true;
	}

	@Override
	public void onClick(View v)
	{
		int id = v.getId();
		switch(v.getId())
		{
		case R.id.state_aspect_ratio:
		case R.id.state_rotate:
		case R.id.state_scale:
			if(!v.isSelected())
				setWidgetState(id);
			break;
		case R.id.wrapper_reset_rotate:
			resetRotation();
			break;
		case R.id.wrapper_rotate_by_angle:
			rotateByAngle(90);
			break;
		default:
			break;
		}
		
	}
	
	@Override
	protected void onStop()
	{
		super.onStop();
		if(mGestureCropImageView != null)
			mGestureCropImageView.cancelAllAnimations();
	}

	private void processOptions(final @NonNull CropOptionBundle options)
	{
		final Bundle bundle = options.getBundle();
		
		// Bitmap compression options
		String compressionFormatName = bundle.getString(CropOptionBundle.EXTRA_COMPRESSION_FORMAT_NAME);
		Bitmap.CompressFormat compressFormat = null;
		if(!TextUtils.isEmpty(compressionFormatName))
		{
			compressFormat = Bitmap.CompressFormat.valueOf(compressionFormatName);
		}
		
		Pair<Bitmap.CompressFormat, Integer> format_quality = SettingsActivity.getFormatAndQuality(this);
		mCompressFormat = (compressFormat == null) ? format_quality.first : compressFormat;
		mCompressQuality = bundle.getInt(CropOptionBundle.EXTRA_COMPRESSION_QUALITY, format_quality.second);

		// Gestures options
		int[] allowedGestures = bundle.getIntArray(CropOptionBundle.EXTRA_ALLOWED_GESTURES);
		if(allowedGestures != null && allowedGestures.length == TABS_COUNT)
		{
			mAllowedGestures = allowedGestures;
		}

		// Crop image view options
		mGestureCropImageView.setMaxBitmapSize(
				bundle.getInt(CropOptionBundle.EXTRA_MAX_BITMAP_SIZE, CropImageView.DEFAULT_MAX_BITMAP_SIZE));
		mGestureCropImageView.setMaxScaleMultiplier(bundle.getFloat(CropOptionBundle.EXTRA_MAX_SCALE_MULTIPLIER,
				CropImageView.DEFAULT_MAX_SCALE_MULTIPLIER));
		mGestureCropImageView.setImageToWrapCropBoundsAnimDuration(
				bundle.getInt(CropOptionBundle.EXTRA_IMAGE_TO_CROP_BOUNDS_ANIM_DURATION,
						CropImageView.DEFAULT_IMAGE_TO_CROP_BOUNDS_ANIM_DURATION));

		// Overlay view options
		mOverlayView.setFreestyleCropEnabled(
				bundle.getBoolean(CropOptionBundle.EXTRA_FREE_STYLE_CROP, OverlayView.DEFAULT_FREESTYLE_CROP_ENABLED));

		mOverlayView.setDimmedColor(bundle.getInt(CropOptionBundle.EXTRA_DIMMED_LAYER_COLOR,
				Compatibility.getColor(this, R.color.CropImageView_default_dimmed)));
		mOverlayView.setOvalDimmedLayer(
				bundle.getBoolean(CropOptionBundle.EXTRA_OVAL_DIMMED_LAYER, OverlayView.DEFAULT_OVAL_DIMMED_LAYER));

		mOverlayView.setShowCropFrame(
				bundle.getBoolean(CropOptionBundle.EXTRA_SHOW_CROP_FRAME, OverlayView.DEFAULT_SHOW_CROP_FRAME));
		mOverlayView.setCropFrameColor(bundle.getInt(CropOptionBundle.EXTRA_CROP_FRAME_COLOR,
				Compatibility.getColor(this, R.color.CropImageView_default_crop_frame)));
		mOverlayView.setCropFrameStrokeWidth(bundle.getInt(CropOptionBundle.EXTRA_CROP_FRAME_STROKE_WIDTH,
				getResources().getDimensionPixelSize(R.dimen.CropImageView_default_crop_frame_stoke_width)));

		mOverlayView.setShowCropGrid(
				bundle.getBoolean(CropOptionBundle.EXTRA_SHOW_CROP_GRID, OverlayView.DEFAULT_SHOW_CROP_GRID));
		mOverlayView.setCropGridRowCount(
				bundle.getInt(CropOptionBundle.EXTRA_CROP_GRID_ROW_COUNT, OverlayView.DEFAULT_CROP_GRID_ROW_COUNT));
		mOverlayView.setCropGridColumnCount(bundle.getInt(CropOptionBundle.EXTRA_CROP_GRID_COLUMN_COUNT,
				OverlayView.DEFAULT_CROP_GRID_COLUMN_COUNT));
		mOverlayView.setCropGridColor(bundle.getInt(CropOptionBundle.EXTRA_CROP_GRID_COLOR,
				Compatibility.getColor(this, R.color.CropImageView_default_crop_grid)));
		mOverlayView.setCropGridStrokeWidth(bundle.getInt(CropOptionBundle.EXTRA_CROP_GRID_STROKE_WIDTH,
				getResources().getDimensionPixelSize(R.dimen.CropImageView_default_crop_grid_stoke_width)));

		// Aspect ratio options
		float aspectRatioX = bundle.getInt(CropOptionBundle.EXTRA_ASPECT_RATIO_X, AspectRatio.DEFAULT_ASPECT_RATIO);
		float aspectRatioY = bundle.getInt(CropOptionBundle.EXTRA_ASPECT_RATIO_Y, AspectRatio.DEFAULT_ASPECT_RATIO);

		int aspectRationSelectedByDefault = bundle.getInt(CropOptionBundle.EXTRA_ASPECT_RATIO_SELECTED_BY_DEFAULT, 0);
		ArrayList<AspectRatio> aspectRatioList = bundle
				.getParcelableArrayList(CropOptionBundle.EXTRA_ASPECT_RATIO_OPTIONS);

		if(aspectRatioX > 0 && aspectRatioY > 0)
		{
			if(mWrapperStateAspectRatio != null)
			{
				mWrapperStateAspectRatio.setVisibility(View.GONE);
			}
			mGestureCropImageView.setTargetAspectRatio(aspectRatioX / aspectRatioY);
		}
		else if(aspectRatioList != null && aspectRationSelectedByDefault < aspectRatioList.size())
		{
			mGestureCropImageView
					.setTargetAspectRatio(aspectRatioList.get(aspectRationSelectedByDefault).getAspectRatioX()
							/ aspectRatioList.get(aspectRationSelectedByDefault).getAspectRatioY());
		}
		else
		{
			mGestureCropImageView.setTargetAspectRatio(AspectRatio.DEFAULT_ASPECT_RATIO);
		}

		// Result bitmap max size options
		int maxSizeX = bundle.getInt(CropOptionBundle.EXTRA_MAX_SIZE_X, 0);
		int maxSizeY = bundle.getInt(CropOptionBundle.EXTRA_MAX_SIZE_Y, 0);

		if(maxSizeX > 0 && maxSizeY > 0)
		{
			mGestureCropImageView.setMaxResultImageSizeX(maxSizeX);
			mGestureCropImageView.setMaxResultImageSizeY(maxSizeY);
		}
	}

	private void setupViews(final @NonNull Intent intent)
	{
		mStatusBarColor = intent.getIntExtra(CropOptionBundle.EXTRA_STATUS_BAR_COLOR,
				Compatibility.getColor(this, R.color.crop_statusbar));
		mToolbarColor = intent.getIntExtra(CropOptionBundle.EXTRA_TOOL_BAR_COLOR,
				Compatibility.getColor(this, R.color.crop_toolbar));
		mActiveWidgetColor = intent.getIntExtra(CropOptionBundle.EXTRA_UCROP_COLOR_WIDGET_ACTIVE,
				Compatibility.getColor(this, R.color.crop_widget_active));
		mToolbarWidgetColor = intent.getIntExtra(CropOptionBundle.EXTRA_UCROP_WIDGET_COLOR_TOOLBAR,
				Compatibility.getColor(this, R.color.crop_toolbar_widget));
		mToolbarTitle = intent.getStringExtra(CropOptionBundle.EXTRA_UCROP_TITLE_TEXT_TOOLBAR);
		mToolbarTitle = !TextUtils.isEmpty(mToolbarTitle) ? mToolbarTitle
				: getResources().getString(R.string.crop);
		
		mShowBottomControls = !intent.getBooleanExtra(CropOptionBundle.EXTRA_HIDE_BOTTOM_CONTROLS, false);

		initiateRootViews();

		if(mShowBottomControls)
		{
			ViewGroup photoBox = (ViewGroup) findViewById(R.id.ucrop_photobox);
			View.inflate(this, R.layout.crop_controls, photoBox);

			mWrapperStateAspectRatio = (ViewGroup) findViewById(R.id.state_aspect_ratio);
			mWrapperStateAspectRatio.setOnClickListener(this);
			mWrapperStateRotate = (ViewGroup) findViewById(R.id.state_rotate);
			mWrapperStateRotate.setOnClickListener(this);
			mWrapperStateScale = (ViewGroup) findViewById(R.id.state_scale);
			mWrapperStateScale.setOnClickListener(this);

			mLayoutAspectRatio = (ViewGroup) findViewById(R.id.layout_aspect_ratio);
			mLayoutRotate = (ViewGroup) findViewById(R.id.layout_rotate_wheel);
			mLayoutScale = (ViewGroup) findViewById(R.id.layout_scale_wheel);

			setupAspectRatioWidget();
			setupRotateWidget();
			setupScaleWidget();
			setupStatesWrapper();
		}
	}

	private void initiateRootViews()
	{
		mCropView = (CropView) findViewById(R.id.ucrop);
		mGestureCropImageView = mCropView.getCropImageView();
		mOverlayView = mCropView.getOverlayView();

		mGestureCropImageView.setTransformImageListener(mImageListener);

	}
	
	private final TransformImageView.TransformImageListener mImageListener = new TransformImageView.TransformImageListener()
	{
		@Override
		public void onRotate(float currentAngle)
		{
			if(mTextViewRotateAngle != null)
				mTextViewRotateAngle.setText(String.format(Locale.getDefault(), "%.2f°", currentAngle));
		}

		@Override
		public void onScale(float currentScale)
		{
			if(mTextViewScalePercent != null)
				mTextViewScalePercent.setText(String.format(Locale.getDefault(), "%d%%", (int) (currentScale * 100)));
		}

		@Override
		public void onLoadComplete()
		{
			mCropView.animate().alpha(1).setDuration(300).setInterpolator(new AccelerateInterpolator());
			mBlockingView.setClickable(false);
			mShowLoader = false;
			invalidateOptionsMenu();
		}

		@Override
		public void onLoadFailure(@NonNull Exception e)
		{
			setResultError(e);
			finish();
		}

	};

	private void setupStatesWrapper()
	{
		ImageView stateScaleImageView = (ImageView) findViewById(R.id.image_view_state_scale);
		ImageView stateRotateImageView = (ImageView) findViewById(R.id.image_view_state_rotate);
		ImageView stateAspectRatioImageView = (ImageView) findViewById(R.id.image_view_state_aspect_ratio);

//		stateScaleImageView
//				.setImageDrawable(new SelectedStateListDrawable(stateScaleImageView.getDrawable(), mActiveWidgetColor));
//		stateRotateImageView.setImageDrawable(
//				new SelectedStateListDrawable(stateRotateImageView.getDrawable(), mActiveWidgetColor));
//		stateAspectRatioImageView.setImageDrawable(
//				new SelectedStateListDrawable(stateAspectRatioImageView.getDrawable(), mActiveWidgetColor));
//		
	}

	private void setupAspectRatioWidget()
	{
		final RadioGroup.OnCheckedChangeListener lsn_ratio = new RadioGroup.OnCheckedChangeListener()
		{
			@Override
			public void onCheckedChanged(RadioGroup group, int checkedId)
			{
				int index = group.indexOfChild(group.findViewById(checkedId));
				if(index < 0)  // Selection is cleared, which checkedId is -1.
					return;
				mGestureCropImageView.setTargetAspectRatio(RATIOS[index]);
				mGestureCropImageView.setImageToWrapCropBounds();
			}
		};
		
//		LinearLayout wrapperAspectRatioList = (LinearLayout) findViewById(R.id.layout_aspect_ratio);
		RadioGroup rg_aspect_ratios = (RadioGroup)findViewById(R.id.aspect_ratios);
		
		rg_aspect_ratios.setOnCheckedChangeListener(lsn_ratio);
		rg_aspect_ratios.check(0);
	}

	private void setupRotateWidget()
	{
		mTextViewRotateAngle = ((TextView) findViewById(R.id.text_view_rotate));
		((HorizontalProgressWheelView) findViewById(R.id.rotate_scroll_wheel))
				.setOnScrollListener(new HorizontalProgressWheelView.OnScrollListener()
				{
					@Override
					public void onScroll(float delta, float totalDistance)
					{
						mGestureCropImageView.postRotate(delta / ROTATE_WIDGET_SENSITIVITY_COEFFICIENT);
					}

					@Override
					public void onScrollEnd()
					{
						mGestureCropImageView.setImageToWrapCropBounds();
					}

					@Override
					public void onScrollStart()
					{
						mGestureCropImageView.cancelAllAnimations();
					}
				});

		((HorizontalProgressWheelView) findViewById(R.id.rotate_scroll_wheel)).setMiddleLineColor(mActiveWidgetColor);

		findViewById(R.id.wrapper_reset_rotate).setOnClickListener(this);
		findViewById(R.id.wrapper_rotate_by_angle).setOnClickListener(this);
	}

	private void setupScaleWidget()
	{
		mTextViewScalePercent = ((TextView) findViewById(R.id.text_view_scale));
		((HorizontalProgressWheelView) findViewById(R.id.scale_scroll_wheel))
				.setOnScrollListener(new HorizontalProgressWheelView.OnScrollListener()
				{
					@Override
					public void onScroll(float delta, float totalDistance)
					{
						if(delta > 0)
						{
							mGestureCropImageView.zoomInImage(mGestureCropImageView.getCurrentScale() + delta
									* ((mGestureCropImageView.getMaxScale() - mGestureCropImageView.getMinScale())
											/ SCALE_WIDGET_SENSITIVITY_COEFFICIENT));
						}
						else
						{
							mGestureCropImageView.zoomOutImage(mGestureCropImageView.getCurrentScale() + delta
									* ((mGestureCropImageView.getMaxScale() - mGestureCropImageView.getMinScale())
											/ SCALE_WIDGET_SENSITIVITY_COEFFICIENT));
						}
					}

					@Override
					public void onScrollEnd()
					{
						mGestureCropImageView.setImageToWrapCropBounds();
					}

					@Override
					public void onScrollStart()
					{
						mGestureCropImageView.cancelAllAnimations();
					}
				});
		((HorizontalProgressWheelView) findViewById(R.id.scale_scroll_wheel)).setMiddleLineColor(mActiveWidgetColor);
	}

	private void resetRotation()
	{
		mGestureCropImageView.postRotate(-mGestureCropImageView.getCurrentAngle());
		mGestureCropImageView.setImageToWrapCropBounds();
	}

	private void rotateByAngle(int angle)
	{
		mGestureCropImageView.postRotate(angle);
		mGestureCropImageView.setImageToWrapCropBounds();
	}

	private void setInitialState()
	{
		if(mShowBottomControls)
		{
			if(mWrapperStateAspectRatio.getVisibility() == View.VISIBLE)
				setWidgetState(R.id.state_aspect_ratio);
			else
				setWidgetState(R.id.state_scale);
		}
		else
		{
			mGestureCropImageView.setScaleEnabled(true);
			mGestureCropImageView.setRotateEnabled(true);
		}
	}

	private void setWidgetState(@IdRes int stateViewId)
	{
		if(!mShowBottomControls)
			return;

		mWrapperStateAspectRatio.setSelected(stateViewId == R.id.state_aspect_ratio);
		mWrapperStateRotate.setSelected(stateViewId == R.id.state_rotate);
		mWrapperStateScale.setSelected(stateViewId == R.id.state_scale);

		mLayoutAspectRatio.setVisibility(stateViewId == R.id.state_aspect_ratio ? View.VISIBLE : View.GONE);
		mLayoutRotate.setVisibility(stateViewId == R.id.state_rotate ? View.VISIBLE : View.GONE);
		mLayoutScale.setVisibility(stateViewId == R.id.state_scale ? View.VISIBLE : View.GONE);

		switch(stateViewId)
		{
		case R.id.state_scale:  setAllowedGestures(0); break;
		case R.id.state_rotate: setAllowedGestures(1); break;
		default:                setAllowedGestures(2); break;
		}
	}

	private void setAllowedGestures(int tab)
	{
		mGestureCropImageView.setScaleEnabled(mAllowedGestures[tab] == ALL || mAllowedGestures[tab] == SCALE);
		mGestureCropImageView.setRotateEnabled(mAllowedGestures[tab] == ALL || mAllowedGestures[tab] == ROTATE);
	}

	/**
	 * Adds view that covers everything below the Toolbar. When it's clickable - user won't be able
	 * to click/touch anything below the Toolbar. Need to block user input while loading and
	 * cropping an image.
	 */
	private void addBlockingView()
	{
		if(mBlockingView == null)
		{
			mBlockingView = new View(this);
			RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
					ViewGroup.LayoutParams.MATCH_PARENT);
//			lp.addRule(RelativeLayout.BELOW, R.id.toolbar);
			mBlockingView.setLayoutParams(lp);
			mBlockingView.setClickable(true);
		}

		((RelativeLayout) findViewById(R.id.ucrop_photobox)).addView(mBlockingView);
	}

	protected void cropAndSaveImage()
	{
		mBlockingView.setClickable(true);
		mShowLoader = true;
		invalidateOptionsMenu();

		mGestureCropImageView.cropAndSaveImage(mCompressFormat, mCompressQuality, new CropImageView.BitmapCropCallback()
		{
			@Override
			public void onBitmapCropped(@NonNull Uri resultUri)
			{
				setResultUri(resultUri, mGestureCropImageView.getTargetAspectRatio());
				finish();
			}

			@Override
			public void onCropFailure(@NonNull Throwable t)
			{
				setResultError(t);
				finish();
			}
		});
	}

	protected void setResultUri(Uri uri, float resultAspectRatio)
	{
		Intent intent = new Intent();
		intent.putExtra(CropOptionBundle.EXTRA_OUTPUT_URI, uri);
		intent.putExtra(CropOptionBundle.EXTRA_OUTPUT_CROP_ASPECT_RATIO, resultAspectRatio);
		setResult(RESULT_OK, intent);
	}

	protected void setResultError(Throwable throwable)
	{
		setResult(CropOptionBundle.RESULT_ERROR, new Intent().putExtra(CropOptionBundle.EXTRA_ERROR, throwable));
	}

}
