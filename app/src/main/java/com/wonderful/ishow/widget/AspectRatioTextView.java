package com.wonderful.ishow.widget;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Build;
import android.support.annotation.NonNull;
import android.util.AttributeSet;
import android.view.Gravity;
import android.widget.TextView;

import com.wonderful.ishow.R;
import com.wonderful.ishow.bean.AspectRatio;

public class AspectRatioTextView extends TextView
{
	private final Rect  mBounds = new Rect();
	private final Paint mDotPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
	private float mDotSize;

	private AspectRatio mAspectRatio;

	public AspectRatioTextView(Context context) {
		this(context, null);
	}
	
	public AspectRatioTextView(Context context, AttributeSet attrs) {
		this(context, attrs, R.style.Widget_AspectRatioTextView);
	}

	public AspectRatioTextView(Context context, AttributeSet attrs, int defStyleAttr) {
		super(context, attrs, defStyleAttr);
		init(context, attrs);
	}

	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	public AspectRatioTextView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
		super(context, attrs, defStyleAttr, defStyleRes);
		init(context, attrs);
	}

	public void setAspectRatio(@NonNull AspectRatio aspectRatio) {
		mAspectRatio = aspectRatio;
		setTitle();
	}

	public float getAspectRatio(boolean toggleRatio) {
		if(toggleRatio) {
			mAspectRatio.switchOrientation();
			setTitle();
		}
		return mAspectRatio.getAspectRatio();
	}

	@Override
	protected void onDraw(Canvas canvas) {
		if(isSelected()) {
			canvas.getClipBounds(mBounds);
			canvas.drawCircle((mBounds.right - mBounds.left) / 2.0F,
					mBounds.bottom - mDotSize, mDotSize / 2, mDotPaint);
		}
	}

	private void init(Context context, AttributeSet attrs) {
		setGravity(Gravity.CENTER_HORIZONTAL);
		
		TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.AspectRatioTextView);
		String title = a.getString(R.styleable.AspectRatioTextView_android_text);
		int aspectRatioX = a.getInt(R.styleable.AspectRatioTextView_aspectRatioX, AspectRatio.DEFAULT_ASPECT_RATIO);
		int aspectRatioY = a.getInt(R.styleable.AspectRatioTextView_aspectRatioY, AspectRatio.DEFAULT_ASPECT_RATIO);
		a.recycle();
		
		mAspectRatio = new AspectRatio(title, aspectRatioX, aspectRatioY);
		
		Resources res = context.getResources();
		mDotSize = res.getDimension(R.dimen.AspectRatioTextView_dotSize);
		mDotPaint.setStyle(Paint.Style.FILL);

		setTitle();
//		setTextColor(Compatibility.getColorStateList(context, R.color.selector_image_edit));
	}

	private void setTitle() {
		String title = mAspectRatio.getAspectRatioTitle();
		if(title == null || title.isEmpty()) {  // TextUtils.isEmpty(title)
			title = String.format("%d:%d", mAspectRatio.getAspectRatioX(), mAspectRatio.getAspectRatioY());
			mAspectRatio.setmAspectRatioTitle(title);
		}
		
		setText(title);
	}
}
