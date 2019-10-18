package com.wonderful.ishow.widget;

import android.content.Context;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.ScaleGestureDetector.OnScaleGestureListener;
import android.view.ViewConfiguration;

public class ImageViewTouch extends ImageViewTouchBase {
	private static final String TAG = ImageViewTouch.class.getSimpleName();
	
	private static final float SCROLL_DELTA_THRESHOLD = 1.0f;
	protected ScaleGestureDetector mScaleDetector;
	protected GestureDetector mGestureDetector;
	protected int mTouchSlop;
	protected float mScaleFactor;
	protected int mDoubleTapDirection;
	protected OnGestureListener mGestureListener;
	protected OnScaleGestureListener mScaleListener;
	protected boolean mDoubleTapEnabled = true;
	protected boolean mScaleEnabled = true;
	protected boolean mScrollEnabled = true;
	private OnImageViewTouchDoubleTapListener mDoubleTapListener;
	private OnImageViewTouchSingleTapListener mSingleTapListener;

	private static final int MIN_DISTANCE = 100;
	private float downX, upX;
	private SwipeListener mListener;

	public ImageViewTouch(Context context) {
		super( context);
	}

	public ImageViewTouch(Context context, AttributeSet attrs) {
		this( context, attrs, 0);
	}

	public ImageViewTouch(Context context, AttributeSet attrs, int defStyle) {
		super( context, attrs, defStyle);
	}

	public void setListener(SwipeListener listener) {
		mListener = listener;
	}

	public interface SwipeListener {
		public void onSwipeRightToLeft();
		public void onSwipeLeftToRight();
	}
 
	@Override
	protected void init(Context context, AttributeSet attrs, int defStyle) {
		super.init(context, attrs, defStyle);
		mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();

		mScaleListener = new ScaleListener();
		mScaleDetector = new ScaleGestureDetector(context, mScaleListener);
		setGestureListener(new GestureListener());
		mDoubleTapDirection = 1;
	}

	public void setDoubleTapListener(OnImageViewTouchDoubleTapListener listener) {
		mDoubleTapListener = listener;
	}

	public void setSingleTapListener( OnImageViewTouchSingleTapListener listener) {
		mSingleTapListener = listener;
	}
	
	public void setGestureListener(GestureListener listener) {
		mGestureListener = listener;
		
		if(mGestureListener != null)
			mGestureDetector = new GestureDetector(getContext(), mGestureListener, null, true);
	}
	
	public void setScaleListener(ScaleListener listener) {
		mScaleListener = listener;
	}
	
	public void setDoubleTapEnabled(boolean value) {
		mDoubleTapEnabled = value;
	}

	public void setScaleEnabled(boolean value) {
		mScaleEnabled = value;
	}

	public void setScrollEnabled(boolean value) {
		mScrollEnabled = value;
	}

	public boolean isDoubleTapEnabled() {
		return mDoubleTapEnabled;
	}

	@Override
	protected void _setImageDrawable(final Drawable drawable, final Matrix initial_matrix, float min_zoom, float max_zoom) {
		super._setImageDrawable( drawable, initial_matrix, min_zoom, max_zoom);
		mScaleFactor = getMaxScale() / 3;
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		mScaleDetector.onTouchEvent(event);

		if(!mScaleDetector.isInProgress() && mGestureDetector != null)
			mGestureDetector.onTouchEvent(event);

		int action = event.getAction();
		switch(action & MotionEvent.ACTION_MASK) {
		case MotionEvent.ACTION_DOWN:
			downX = event.getX();
			break;
		case MotionEvent.ACTION_UP:
			upX = event.getX();

			float deltaX = downX - upX;

			// horizontal swipe detection
			if (Math.abs(deltaX) > MIN_DISTANCE) {
				// left to right
				if (deltaX < 0) {
					if(getScale() == getMinScale() && mListener != null)
						mListener.onSwipeLeftToRight();
				}
				//right to left
				if (deltaX > 0) {
					if(getScale() == getMinScale() && mListener != null)
						mListener.onSwipeRightToLeft();
				}
			}
			return onUp(event);
		}
		return true;
	}
	
	@Override
	protected void onZoomAnimationCompleted(float scale) {
		Log.d(TAG, "onZoomAnimationCompleted. scale: " + scale + ", minZoom: " + getMinScale());
		
		if(scale < getMinScale()) {
			zoomTo( getMinScale(), 50);
		}
	}

	protected float onDoubleTapPost(float scale, float maxZoom) {
		if(mDoubleTapDirection == 1) {
			if(( scale + ( mScaleFactor * 2)) <= maxZoom) {
				return scale + mScaleFactor;
			} else {
				mDoubleTapDirection = -1;
				return maxZoom;
			}
		} else {
			mDoubleTapDirection = 1;
			return 1f;
		}
	}

	public boolean onSingleTapConfirmed( MotionEvent e) {
		return true;
	}

	public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
		if(getScale() == 1f) return false;
		mUserScaled = true;
		scrollBy( -distanceX, -distanceY);
		invalidate();
		return true;
	}

	public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
		float diffX = e2.getX() - e1.getX();
		float diffY = e2.getY() - e1.getY();

		if(Math.abs(velocityX) > 800 || Math.abs(velocityY) > 800) {
			mUserScaled = true;
			scrollBy( diffX / 2, diffY / 2, 300);
			invalidate();
			return true;
		}
		return false;
	}

	public boolean onDown(MotionEvent e) {
		return true;
	}

	public boolean onUp(MotionEvent e) {
		if(getScale() < getMinScale()) {
			zoomTo(getMinScale(), 50);
		}
		return true;
	}

	public boolean onSingleTapUp(MotionEvent e) {
		return true;
	}

	/**
	 * Determines whether this ImageViewTouch can be scrolled.
	 * 
	 * @param direction
	 *            - positive direction value means scroll from right to left,
	 *            negative value means scroll from left to right
	 * 
	 * @return true if there is some more place to scroll, false - otherwise.
	 */
	public boolean canScroll(int direction) {
		RectF bitmapRect = getBitmapRect();
		updateRect( bitmapRect, mScrollRect);
		Rect imageViewRect = new Rect();
		getGlobalVisibleRect( imageViewRect);

		if(bitmapRect == null) {
			return false;
		}

		if(bitmapRect.right >= imageViewRect.right) {
			if(direction < 0) {
				return Math.abs( bitmapRect.right - imageViewRect.right) > SCROLL_DELTA_THRESHOLD;
			}
		}

		double bitmapScrollRectDelta = Math.abs( bitmapRect.left - mScrollRect.left);
		return bitmapScrollRectDelta > SCROLL_DELTA_THRESHOLD;
	}
	
	@Override
	public boolean performClick() {
		return super.performClick();
	}
	
	public class GestureListener extends GestureDetector.SimpleOnGestureListener {

		@Override
		public boolean onSingleTapConfirmed(MotionEvent e) {

			if(mSingleTapListener != null) {
				mSingleTapListener.onSingleTapConfirmed();
			}

			return ImageViewTouch.this.onSingleTapConfirmed(e);
		}

		@Override
		public boolean onDoubleTap(MotionEvent e) {
			Log.i( TAG, "onDoubleTap. double tap enabled? " + mDoubleTapEnabled);
			if(mDoubleTapEnabled) {
				mUserScaled = true;
				float scale = getScale();
				float targetScale = scale;
				targetScale = onDoubleTapPost( scale, getMaxScale());
				targetScale = Math.min( getMaxScale(), Math.max( targetScale, getMinScale()));
				zoomTo( targetScale, e.getX(), e.getY(), DEFAULT_ANIMATION_DURATION);
				invalidate();
			}

			if(null != mDoubleTapListener) {
				mDoubleTapListener.onDoubleTap();
			}

			return super.onDoubleTap( e);
		}

		@Override
		public void onLongPress(MotionEvent e) {
			if(isLongClickable()) {
				if(!mScaleDetector.isInProgress()) {
					setPressed( true);
					performLongClick();
				}
			}
		}

		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {

			if(!mScrollEnabled) return false;
			if(e1 == null || e2 == null) return false;
			if(e1.getPointerCount() > 1 || e2.getPointerCount() > 1) return false;
			if(mScaleDetector.isInProgress()) return false;
			return ImageViewTouch.this.onScroll( e1, e2, distanceX, distanceY);
		}

		@Override
		public boolean onFling( MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
			if(!mScrollEnabled)
				return false;

			if(e1.getPointerCount() > 1 || e2.getPointerCount() > 1) return false;
			if(mScaleDetector.isInProgress()) return false;
			if(getScale() == 1f) return false;

			return ImageViewTouch.this.onFling( e1, e2, velocityX, velocityY);
		}

		@Override
		public boolean onSingleTapUp(MotionEvent e) {
			return ImageViewTouch.this.onSingleTapUp( e);
		}

		@Override
		public boolean onDown(MotionEvent e) {
			return ImageViewTouch.this.onDown( e);
		}
	}

	public class ScaleListener extends ScaleGestureDetector.SimpleOnScaleGestureListener {

		protected boolean mScaled = false;

		@Override
		public boolean onScale( ScaleGestureDetector detector) {
			float span = detector.getCurrentSpan() - detector.getPreviousSpan();
			float targetScale = getScale() * detector.getScaleFactor();

			if(mScaleEnabled) {
				if(mScaled && span != 0) {
					mUserScaled = true;
					targetScale = Math.min( getMaxScale(), Math.max( targetScale, getMinScale() - 0.1f));
					zoomTo( targetScale, detector.getFocusX(), detector.getFocusY());
					mDoubleTapDirection = 1;
					invalidate();
					return true;
				}

				// This is to prevent a glitch the first time image is scaled.
				if(!mScaled)
					mScaled = true;
			}
			return true;
		}

	}

	public interface OnImageViewTouchDoubleTapListener {

		void onDoubleTap();
	}

	public interface OnImageViewTouchSingleTapListener {

		void onSingleTapConfirmed();
	}
}
