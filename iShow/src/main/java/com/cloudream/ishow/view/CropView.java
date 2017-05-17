package com.cloudream.ishow.view;

import com.cloudream.ishow.R;
import com.cloudream.ishow.widget.CropImageView;
import com.cloudream.ishow.widget.GestureCropImageView;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.RectF;
import android.support.annotation.NonNull;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.FrameLayout;

public class CropView extends FrameLayout
{

	private final GestureCropImageView mGestureCropImageView;
	private final OverlayView mViewOverlay;

	public CropView(Context context, AttributeSet attrs)
	{
		this(context, attrs, 0);
	}

	public CropView(Context context, AttributeSet attrs, int defStyleAttr)
	{
		super(context, attrs, defStyleAttr);

		LayoutInflater.from(context).inflate(R.layout.crop_view, this, true);
		mGestureCropImageView = (GestureCropImageView) findViewById(R.id.image_view_crop);
		mViewOverlay = (OverlayView) findViewById(R.id.view_overlay);

		TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.CropView);
		mViewOverlay.processStyledAttributes(a);
		mGestureCropImageView.processStyledAttributes(a);
		a.recycle();

		mGestureCropImageView.setCropBoundsChangeListener(new CropImageView.OnCropBoundsChangeListener()
		{
			@Override
			public void onCropAspectRatioChanged(float cropRatio)
			{
				mViewOverlay.setTargetAspectRatio(cropRatio);
			}
		});
		mViewOverlay.setOverlayViewChangeListener(new OverlayView.OnOverlayViewChangeListener()
		{
			@Override
			public void onCropRectUpdated(RectF cropRect)
			{
				mGestureCropImageView.setCropRect(cropRect);
			}
		});
	}

	@Override
	public boolean shouldDelayChildPressedState()
	{
		return false;
	}

	@NonNull
	public GestureCropImageView getCropImageView()
	{
		return mGestureCropImageView;
	}

	@NonNull
	public OverlayView getOverlayView()
	{
		return mViewOverlay;
	}

}