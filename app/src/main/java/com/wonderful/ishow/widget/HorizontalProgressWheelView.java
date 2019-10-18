package com.wonderful.ishow.widget;

import com.wonderful.ishow.R;
import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Build;
import android.support.annotation.ColorInt;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class HorizontalProgressWheelView extends View {
	
	public interface OnScrollListener {
		void onScrollStart();

		void onScroll(float delta, float totalDistance);

		void onScrollEnd();
	}
	
	private final Rect mCanvasClipBounds = new Rect();

	private OnScrollListener mOnScrollListener;
	private float mLastTouchedPosition;

	private Paint mProgressLinePaint;
	private float mProgressLineWidth;
	private float mProgressLineHeight;
	private float mProgressLineMargin;

	private boolean mScrollStarted;
	private float mTotalScrollDistance;

	private int mProgressLineColor;
	private int mMiddleLineColor;

	public HorizontalProgressWheelView(Context context) {
		this(context, null);
	}

	public HorizontalProgressWheelView(Context context, @Nullable AttributeSet attrs) {
		this(context, attrs, R.attr.horizontalProgressWheelViewStyle);
	}

	public HorizontalProgressWheelView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
		super(context, attrs, defStyleAttr);
		init(context, attrs, defStyleAttr, 0);
	}

	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	public HorizontalProgressWheelView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
		super(context, attrs, defStyleAttr, defStyleRes);
		init(context, attrs, defStyleAttr, defStyleRes);
	}

	private void init(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
		final Resources res = context.getResources();
		TypedArray a = context.getTheme().obtainStyledAttributes(attrs, R.styleable.HorizontalProgressWheelView, defStyleAttr, defStyleRes);

		mProgressLineColor= a.getColor(R.styleable.HorizontalProgressWheelView_progressLineColor, R.color.HorizontalProgressWheelView_progressLine);
		mMiddleLineColor  = a.getColor(R.styleable.HorizontalProgressWheelView_middleLineColor, R.color.HorizontalProgressWheelView_middleLine);
		
		final float lineWidthDefault = res.getDimension(R.dimen.HorizontalProgressWheelView_lineWidth);
		final float lineHeightDefault = res.getDimension(R.dimen.HorizontalProgressWheelView_lineHeight);
		final float lineMarginDefault = res.getDimension(R.dimen.HorizontalProgressWheelView_lineMargin);
		mProgressLineWidth  = a.getDimension(R.styleable.HorizontalProgressWheelView_lineWidth, lineWidthDefault);
		mProgressLineHeight = a.getDimension(R.styleable.HorizontalProgressWheelView_lineHeight, lineHeightDefault);
		mProgressLineMargin = a.getDimension(R.styleable.HorizontalProgressWheelView_lineMargin, lineMarginDefault);

		mProgressLinePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
		mProgressLinePaint.setStyle(Paint.Style.STROKE);
		mProgressLinePaint.setStrokeWidth(mProgressLineWidth);
	}
	
	public void setOnScrollListener(@Nullable OnScrollListener listener) {
		mOnScrollListener = listener;
	}

	public void setProgressLineColor(@ColorInt int color) {
		mMiddleLineColor = color;
		invalidate();
	}
	
	public void setMiddleLineColor(@ColorInt int color) {
		mMiddleLineColor = color;
		invalidate();
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		switch(event.getAction()) {
		case MotionEvent.ACTION_DOWN:
			mLastTouchedPosition = event.getX();
			break;
			
		case MotionEvent.ACTION_UP:
			mScrollStarted = false;
			postInvalidate();
			if(mOnScrollListener != null)
				mOnScrollListener.onScrollEnd();
			break;
			
		case MotionEvent.ACTION_MOVE:
			float currentX = event.getX();
			float distance = currentX - mLastTouchedPosition;
			if(distance != 0) {
				if(!mScrollStarted) {
					mScrollStarted = true;
					if(mOnScrollListener != null)
						mOnScrollListener.onScrollStart();
				}
				
				mTotalScrollDistance -= distance;
				postInvalidate();
				mLastTouchedPosition = currentX;
				if(mOnScrollListener != null)
					mOnScrollListener.onScroll(-distance, mTotalScrollDistance);
			}
			break;
			
		default:
			return super.onTouchEvent(event);
		}
		
		return true;
	}

	@Override
	protected void onDraw(Canvas canvas) {
		canvas.getClipBounds(mCanvasClipBounds);

		int linesCount = (int) (mCanvasClipBounds.width() / (mProgressLineWidth + mProgressLineMargin));
		float deltaX = (mTotalScrollDistance) % (mProgressLineMargin + mProgressLineWidth);
		
		final int transparentCount = linesCount / 3;  // quarter is fine. Here, I use one third.
		final float centerX = mCanvasClipBounds.centerX();
		final float centerY = mCanvasClipBounds.centerY();
		
		mProgressLinePaint.setColor(mProgressLineColor);
		for(int i = 0; i < linesCount; ++i) {
			final int alpha;
			if(i < transparentCount)
				alpha = 255 * i / transparentCount;
			else if(i > (linesCount - transparentCount))
				alpha = 255 * (linesCount - i) / transparentCount;
			else
				alpha = 255;
			
			mProgressLinePaint.setAlpha(alpha);
			float x  = -deltaX + mCanvasClipBounds.left + i * (mProgressLineWidth + mProgressLineMargin);
			
			float halfY = mScrollStarted ? (mProgressLineHeight/2):(mProgressLineHeight/4);
			float y0 = centerY - halfY;
			float y1 = centerY + halfY;
			canvas.drawLine(x, y0, x, y1, mProgressLinePaint);
		}
		
		mProgressLinePaint.setColor(mMiddleLineColor);
		float y0 = centerY - mProgressLineHeight/2;
		float y1 = centerY + mProgressLineHeight/2;
		canvas.drawLine(centerX, y0, centerX, y1, mProgressLinePaint);
	}
	
}