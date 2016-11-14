package com.cloudream.ishow.view;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ComposeShader;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.Shader.TileMode;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

public class ColorPickerSquare extends View
{
	private static final String TAG = ColorPickerSquare.class.getSimpleName();
	
	private final Paint paint;
	private final Rect  rect;
	
	private final float[] hsv = { 1.0F, 1.0F, 1.0F };

	public ColorPickerSquare(Context context)
	{
		this(context, null);
	}
	
	public ColorPickerSquare(Context context, AttributeSet attrs)
	{
		this(context, attrs, 0);
	}

	public ColorPickerSquare(Context context, AttributeSet attrs, int defStyle)
	{
		super(context, attrs, defStyle);
		paint = new Paint(Paint.ANTI_ALIAS_FLAG);
		rect  = new Rect();
	}

	@Override
	protected void onSizeChanged(int new_x, int new_y, int old_x, int old_y)
	{
		super.onSizeChanged(new_x, new_y, old_x, old_y);

		String text = String.format("old: %dx%d, new: %dx%d, measured: %dx%d", old_x, old_y,
				new_x, new_y, getMeasuredWidth(), getMeasuredHeight());
		Log.d(TAG, text);
		Toast.makeText(getContext(), text, Toast.LENGTH_LONG).show();

		rect.left = 0; rect.right  = getMeasuredWidth();
		rect.top  = 0; rect.bottom = getMeasuredHeight();
		
		// this avoids drawing allocations in #onDraw.
		Shader shader = createShader(hsv, rect.width(), rect.height());
		paint.setShader(shader);
	}
	
	private static Shader createShader(final float hsv[], float width, float height)
	{
		int color = Color.HSVToColor(hsv);
		final float O = 0.0F;
		Shader shader_x = new LinearGradient(O, O, width,  O, Color.WHITE, color,       TileMode.CLAMP);
		Shader shader_y = new LinearGradient(O, O, O, height, Color.WHITE, Color.BLACK, TileMode.CLAMP);
		ComposeShader shader = new ComposeShader(shader_y, shader_x, PorterDuff.Mode.MULTIPLY);
		
		return shader;
	}
	
	@Override
	protected void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);
		canvas.drawRect(rect, paint);
	}

	public void setHue(float hue)
	{
		hsv[0] = hue;
		Shader shader = createShader(hsv, rect.width(), rect.height());
		paint.setShader(shader);
		
		invalidate();
	}
}
