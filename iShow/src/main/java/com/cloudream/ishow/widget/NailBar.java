package com.cloudream.ishow.widget;

import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.Nullable;
import android.graphics.Paint;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.graphics.Canvas;

import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.R;
import com.cloudream.ishow.util.Compatibility;
import com.cloudream.ishow.util.MathUtils;
import com.cloudream.ishow.util.OutOfRangeException;

/**
 * #NailBar is like #ProgressBar, but it provides floating progress, and start point need not be zero.
 * No drawables, no intermediate progress to draw. Moreover, default progress is shown on the bar.
 *
 * @see <a href="https://developer.android.com/guide/topics/ui/custom-components.html">Custom Components</a>
 */
public class NailBar extends View
{
	private static final String TAG = NailBar.class.getSimpleName();
	
	/**
	 * A callback that notifies clients when the progress level has been changed. This includes
	 * changes that were initiated by the user through a touch gesture or arrow key/trackball as
	 * well as changes that were initiated programmatically.
	 */
	public interface OnNailBarChangeListener
	{

		/**
		 * Notification that the progress level has changed. Clients can use the fromUser parameter
		 * to distinguish user-initiated changes from those that occurred programmatically.
		 *
		 * @param nailBar The NailBar whose progress has changed
		 * @param progress The current progress level. This will be in the range min..max where min
		 *        was set by {@link NailBar#setMin(float)}. (The default value for max is -100.),max
		 *        was set by {@link NailBar#setMax(float)}. (The default value for max is +100.)
		 * @param fromUser True if the progress change was initiated by the user.
		 */
		void onProgressChanged(NailBar nailBar, float progress, boolean fromUser);

		/**
		 * Notification that the user has started a touch gesture. Clients may want to use this to
		 * disable advancing the nailBar.
		 * 
		 * @param nailBar The NailBar in which the touch gesture began
		 */
		void onStartTrackingTouch(NailBar nailBar);

		/**
		 * Notification that the user has finished a touch gesture. Clients may want to use this to
		 * re-enable advancing the nailBar.
		 * 
		 * @param nailBar The NailBar in which the touch gesture began
		 */
		void onStopTrackingTouch(NailBar nailBar);
	}

	private OnNailBarChangeListener listener;
	
	private int lowSideColor;
	private int highSideColor;
	private int circleColor;
	private int nailColor;
	
	private float barThickness;
	private float circleRadius;
	private float nailRadius;
	
	private int barMinX, barMaxX;  // become final after onMeasure()'d
	private float SCALE;
	
	private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);;
	
	private static final float EPSILON = 0.02F;
	
	private float start, stop, step;  // [start, stop, step] maps to [0, max, 1].
	private float default_value;      // in range [start, stop].
	private float current_value;
	
	private float nailX, nailY;
	
	private float downX;
	private float downProgress;

	/**
	 * Create a new progress bar with range -100...100 and initial progress of 0.
	 * @param context the application environment
	 */
	public NailBar(Context context)
	{
		this(context, null);
	}

	public NailBar(Context context, @Nullable AttributeSet attrs)
	{
		this(context, attrs, R.style.Theme);
	}

	public NailBar(Context context, @Nullable AttributeSet attrs, int defStyleAttr)
	{
		super(context, attrs, defStyleAttr);
		int defStyleRes = 0;  // R.style.Widget_NailBar
		init(context, attrs, defStyleAttr, defStyleRes);
	}

	public NailBar(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
	{
		super(context, attrs, defStyleAttr, defStyleRes);
		init(context, attrs, defStyleAttr, defStyleRes);
	}
	
	private void init(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
	{
		final Resources res = context.getResources();
		TypedArray a = context.getTheme().obtainStyledAttributes(attrs, R.styleable.NailBar, defStyleAttr, defStyleRes);

		try
		{
			start = a.getFloat(R.styleable.NailBar_min, -100.0F);
			stop  = a.getFloat(R.styleable.NailBar_max, +100.0F);
			int steps = a.getInt(R.styleable.NailBar_steps, 200);
			
			if(steps < 2)
			{
				if(BuildConfig.DEBUG)
					throw new IllegalArgumentException("app:steps should have at least 2 steps");
				else
					steps = 2;
			}
			step = (stop - start) / steps;
			
			final int lowSideColorDefault  = Compatibility.getColor(context, R.color.NailBar_lowSideColor);
			final int highSideColorDefault = Compatibility.getColor(context, R.color.NailBar_highSideColor);
			final int circleColorDefault   = Compatibility.getColor(context, R.color.NailBar_circleColor);
			final int nailColorDefault     = Compatibility.getColor(context, R.color.NailBar_nailColor);
			lowSideColor  = a.getColor(R.styleable.NailBar_lowSideColor, lowSideColorDefault);
			highSideColor = a.getColor(R.styleable.NailBar_highSideColor, highSideColorDefault);
			circleColor   = a.getColor(R.styleable.NailBar_circleColor, circleColorDefault);
			nailColor     = a.getColor(R.styleable.NailBar_nailColor, nailColorDefault);
			
			final float barThicknessDefault = res.getDimension(R.dimen.NailBar_barThickness);
			final float circleRadiusDefault = res.getDimension(R.dimen.NailBar_circleRadius);
			final float nailRadiusDefault   = res.getDimension(R.dimen.NailBar_nailRadius);
			barThickness = a.getDimension(R.styleable.NailBar_barThickness, barThicknessDefault);
			circleRadius = a.getDimension(R.styleable.NailBar_circleRadius, circleRadiusDefault);
			nailRadius   = a.getDimension(R.styleable.NailBar_nailRadius, nailRadiusDefault);
		}
		finally
		{
			a.recycle();  // TypedArray objects are shared and must be recycled.
		}
	}
	
	@Override
	protected synchronized void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
	{
		int desiredWidth = MeasureSpec.getSize(widthMeasureSpec);
		int desiredHeight = MeasureSpec.getSize(widthMeasureSpec);
		
		int min = Math.round(Math.max(circleRadius, nailRadius) * 2);
		if(desiredWidth < min)  desiredWidth = min;
		if(desiredHeight < min)  desiredHeight = min;
		
//		nailY = desiredHeight / 2.0F;  // nail moves horizontally, Y axis is fixed.
//		desiredWidth  += getPaddingLeft() + getPaddingRight();
//		desiredHeight += getPaddingTop() + getPaddingBottom();
		
		final int measuredWidth  = resolveSizeAndState(desiredWidth, widthMeasureSpec, 0);
		final int measuredHeight = resolveSizeAndState(desiredHeight, heightMeasureSpec, 0);
//		Log.i(TAG, "desiredHeight: " + desiredHeight + " desiredHeight: " + desiredHeight);
		setMeasuredDimension(measuredWidth, measuredHeight);
	}
	
	@Override
	protected void onLayout(boolean changed, int left, int top, int right, int bottom)
	{
		super.onLayout(changed, left, top, right, bottom);
		
		nailY = (top + bottom) / 2.0F;
		barMinX = left + getPaddingLeft();
		barMaxX = right - getPaddingRight();
		
		SCALE = (stop - start) / (barMaxX - barMinX);
	}
	
	@Override
	protected void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);
		int width = getWidth();

		int paddingLeft = getPaddingLeft();
		int paddingRight = getPaddingRight();
		float barStart = paddingLeft + nailRadius;
		float barLength = (width - paddingLeft - paddingRight) - nailRadius * 2.0F;
		float barEnd = barStart + barLength;
		
		float circleCenterX = barStart + barLength * getAmount(default_value);
		float centerY = nailY;
		float bar_top    = centerY - barThickness/2;
		float bar_bottom = centerY + barThickness/2;
		
		if(circleCenterX > circleRadius)
		{
			// draw left bar
			paint.setColor(lowSideColor);
			canvas.drawRect(barStart, bar_top, circleCenterX - circleRadius, bar_bottom, paint);
		}
		
		if(circleCenterX < barEnd - circleRadius)
		{
			// draw right bar
			paint.setColor(highSideColor);
			canvas.drawRect(circleCenterX + circleRadius, bar_top, barEnd, bar_bottom, paint);
		}
		
		// draw circle after left and right bar, to overwrite overlapping area.
		paint.setColor(circleColor);
		paint.setStyle(Paint.Style.STROKE);
		paint.setStrokeWidth(barThickness);
		canvas.drawCircle(circleCenterX, centerY, circleRadius, paint);
		
		// draw nail last.
		nailX = barStart + barLength * getAmount(current_value);
		paint.setColor(nailColor);
		paint.setStyle(Paint.Style.FILL);
		canvas.drawCircle(nailX, nailY, nailRadius, paint);
	}

	@Override
	public boolean onTouchEvent(MotionEvent event)
	{
		final float x = event.getX(), y = event.getY();
		float dx = x - nailX, dy = y - nailY;

		switch(event.getActionMasked())
		{
		case MotionEvent.ACTION_DOWN:
			downX = x;
			downProgress = current_value;

			if(listener != null)
				listener.onStartTrackingTouch(this);
			break;
		case MotionEvent.ACTION_MOVE:
		{
			float old_value = current_value;
			current_value = downProgress + (x - downX) * SCALE;
			current_value = MathUtils.clamp(current_value, start, stop);
			
			final float SNAP_TO_DEFAULT = 0.25F;
			if(Math.abs(current_value - default_value) < SNAP_TO_DEFAULT)
				reset();
			else
				if(listener != null)
					listener.onProgressChanged(this, old_value, true);
			
			invalidate();
		}
			break;
		case MotionEvent.ACTION_CANCEL:
		case MotionEvent.ACTION_UP:
			if(listener != null)
				listener.onStopTrackingTouch(this);
			break;
		}
		
		return true;
	}

	public void reset()
	{
		float old_value = current_value;
		current_value = default_value;
		if(listener != null)
			listener.onProgressChanged(this, old_value, false);
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event)
	{
		if(isEnabled())
		{
			float increment = step;
			switch(keyCode)
			{
			case KeyEvent.KEYCODE_DPAD_LEFT:
				increment = -increment;
				// [[fallthrough]]
			case KeyEvent.KEYCODE_DPAD_RIGHT:
				increment = getLayoutDirection() == LAYOUT_DIRECTION_RTL ? -increment : increment;
				
				// Let progress bar handle clamping values.
				if(setProgress(getProgress() + increment, true))
					return true;
				break;
			}
		}
		
		return super.onKeyDown(keyCode, event);
	}
	
	@Override
	public CharSequence getAccessibilityClassName()
	{
		return NailBar.class.getName();
	}

	 /**
	 * <p>Set the range of the progress bar to <tt>min</tt>...<tt>max</tt>.</p>
	 *
	 * @param start the lower range of this progress bar
	 * @param stop  the upper range of this progress bar
	 * @param step  the increment of this progress bar
	 *
	 * @see #getMin()
	 * @see #getMax()
	 * @see #setDefaultValue(float)
	 */
	public synchronized void setRange(float start, float stop, float step)
	{
		this.start = start;
		this.stop  = stop;
		this.step  = step;
	}
	
	/**
	 * Be careful not to be confused with {@link #setRange(float, float, float)}.
	 * 
	 * @see setRange(float, float, float)
	 */
	public synchronized void setRange(float start, float stop, int steps)
	{
		this.start = start;
		this.stop  = stop;
		this.step  = (stop - step) / steps;
	}
	
	public synchronized void setRange(float start, float stop)
	{
		setRange(start, stop, 1.0F);
	}
	
	public synchronized void setProgress(float progress)
	{
		setProgress(progress, false);
	}
	
	private synchronized boolean setProgress(float progress, boolean fromUser)
	{
		progress = MathUtils.clamp(progress, start, stop);
		if(Math.abs(current_value - progress) <= EPSILON)
			return false;
		
		if(listener != null)
			listener.onProgressChanged(this, current_value, fromUser);
		current_value = progress;
		invalidate();
		return true;
	}
	
	public synchronized float getProgress()
	{
		return current_value;
	}
	
	private float getAmount(float progress)
	{
		if(BuildConfig.DEBUG)
		{
			if(progress < start || progress > stop)
				throw new OutOfRangeException("progress is out of range");
		}
		else
		{
			if(progress < start)
				return 0.0F;
			else if(progress > stop)
				return 1.0F;
		}
		
		return (progress - start) / (stop - start);
	}
	
	public float getMin()
	{
		return start;
	}
	
	public float getMax()
	{
		return stop;
	}

	public void setOnNailBarChangeListener(@Nullable OnNailBarChangeListener listener)
	{
		this.listener = listener;
	}
	
	static class SavedState extends BaseSavedState
	{
		private float progress;

		/**
		 * Constructor called from {@link NailBar#onSaveInstanceState()}
		 */
		SavedState(Parcelable superState)
		{
			super(superState);
		}

		/**
		 * Constructor called from {@link #CREATOR}
		 */
		private SavedState(Parcel in)
		{
			super(in);
			progress = in.readFloat();
		}

		@Override
		public void writeToParcel(Parcel out, int flags)
		{
			super.writeToParcel(out, flags);
			out.writeFloat(progress);
		}

		public static final Parcelable.Creator<SavedState> CREATOR = new Parcelable.Creator<SavedState>()
		{
			public SavedState createFromParcel(Parcel in)
			{
				return new SavedState(in);
			}

			public SavedState[] newArray(int size)
			{
				return new SavedState[size];
			}
		};
	}

	@Override
	public Parcelable onSaveInstanceState()
	{
		// Force our ancestor class to save its state
		Parcelable superState = super.onSaveInstanceState();
		SavedState ss = new SavedState(superState);

		ss.progress = current_value;

		return ss;
	}

	@Override
	public void onRestoreInstanceState(Parcelable state)
	{
		SavedState ss = (SavedState) state;
		super.onRestoreInstanceState(ss.getSuperState());

		setProgress(ss.progress);
	}
}
