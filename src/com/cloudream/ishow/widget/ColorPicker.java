package com.cloudream.ishow.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.annotation.IntRange;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.FrameLayout;

import java.security.InvalidParameterException;

import com.cloudream.ishow.BuildConfig;
import com.cloudream.ishow.R;

/**
 * Provides a widget for Picking a color.
 * 
 */
public class ColorPicker extends FrameLayout
{
	private static final int MODE_CIRCLE_HS_V = 0;
	private static final int MODE_CIRCLE_HS_L = 1;
	private static final int MODE_SQUARE_SV_H = 2;
	private static final int MODE_SQUARE_HS_V = 3;
	private static final int MODE_SQUARE_HV_S = 4;

	protected int color;
	protected boolean enable_alpha;
	protected OnColorChangedListener listener;
	
	/**
	 * The callback used to indicate the user changed the color.
	 */
	public interface OnColorChangedListener
	{
		/**
		 * Called upon a color change.
		 *
		 * @param view  The view associated with this listener.
		 * @param color The color that was set.
		 */
		void onColorChanged(ColorPicker view, int color);
	}

	public ColorPicker(Context context)
	{
		this(context, null);
	}

	public ColorPicker(Context context, @Nullable AttributeSet attrs)
	{
		this(context, attrs, R.attr.colorPickerStyle);
	}

	public ColorPicker(Context context, @Nullable AttributeSet attrs, int defStyleAttr)
	{
		this(context, attrs, defStyleAttr, 0);
	}

	/**
	 * public FrameLayout(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes);
	 * method was added in Android API level 21
	 * @param context
	 * @param attrs
	 * @param defStyleAttr
	 * @param defStyleRes
	 */
	public ColorPicker(Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes)
	{
/*
		// This will not work on KitKat devices, call to super must be first statement in constructor.
		// Workaround: move constructor's parameter defStyleRes into Context#obtainStyledAttributes
		if(android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
			super(context, attrs, defStyleAttr, defStyleRes);
		else
			super(context, attrs, defStyleAttr);
*/
		super(context, attrs, defStyleAttr);
		
		final TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.ColorPicker, defStyleAttr, defStyleRes);
		int layout = a.getResourceId(R.styleable.ColorPicker_layout, R.layout.color_picker);
		final int mode = a.getInt(R.styleable.ColorPicker_mode, MODE_CIRCLE_HS_V);
		color = a.getInt(R.styleable.ColorPicker_color, 0xFFE1112B);
		enable_alpha = a.getBoolean(R.styleable.ColorPicker_enableAlpha, false);
		if(!enable_alpha)
			this.color |= 0xFF000000;
		
		final LayoutInflater inflater = LayoutInflater.from(context);
		inflater.inflate(layout, this, true);

		a.recycle();
	}

	public void setColor(int color)
	{
		if(this.color == color)
			return;
		
		this.color = color;
		if(listener != null)
			listener.onColorChanged(this, color);
	}
	
	public void setColor(int color, boolean enable_alpha)
	{
		this.enable_alpha = enable_alpha;
		if(enable_alpha)
			color |= 0xFF000000;
		
		setColor(color);
	}

	public void setAlpha(@IntRange(from=0, to=255) int alpha)
	{
		if(BuildConfig.DEBUG && (alpha < 0 || alpha > 255))
			throw new InvalidParameterException("alpha value is out of range [0, 255]");
		
		if(enable_alpha)
			color = (color & 0x00FFFFFF) | alpha << 24;
	}
	
	public int getColor()
	{
		return color;
	}

	public boolean isAlphaEnabled()
	{
		return enable_alpha;
	}

	public void setOnTimeChangedListener(OnColorChangedListener listener)
	{
		this.listener = listener;
	}

	@Override
	public CharSequence getAccessibilityClassName()
	{
		return ColorPicker.class.getName();
	}

}
