package com.wonderful.ishow.view;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ComposeShader;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.RadialGradient;
import android.graphics.Shader;
import android.graphics.Shader.TileMode;
import android.util.AttributeSet;
import android.view.View;

public class ColorPickerCircle extends View {
	private Paint  paint;
	private Shader shader_y;
	
	private final float[] hsv = { 1.0F, 1.0F, 1.0F };
	private int color = Color.HSVToColor(hsv);

	public ColorPickerCircle(Context context)
	{
		super(context);
	}
	
	public ColorPickerCircle(Context context, AttributeSet attrs)
	{
		super(context, attrs);
	}

	public ColorPickerCircle(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		paint = new Paint();
	}

	@Override
	protected void onSizeChanged(int new_x, int new_y, int old_x, int old_y) {
		super.onSizeChanged(new_x, new_y, old_x, old_y);
		
		float center_x = getMeasuredWidth() / 2.0F;
		float center_y = getMeasuredHeight()/ 2.0F;
		float radius   = Math.min(center_x, center_y);
		shader_y = new RadialGradient(center_x, center_y, radius, Color.TRANSPARENT, Color.BLACK, TileMode.MIRROR);
	}
	
	@SuppressLint("DrawAllocation")
	@Override
	protected void onDraw(Canvas canvas) {
		super.onDraw(canvas);
		int width  = getMeasuredWidth();
		int height = getMeasuredHeight();

		int new_color = Color.HSVToColor(hsv);
		if(color != new_color) {
			Shader shader_x = new LinearGradient(0.0F, 0.0F, width, 0.0F, Color.WHITE, new_color, TileMode.CLAMP);
			
			ComposeShader shader = new ComposeShader(shader_y, shader_x, PorterDuff.Mode.MULTIPLY);
			paint.setShader(shader);
			
			color = new_color;
		}
		canvas.drawRect(0.0F, 0.0F, width, height, paint);
	}

	public void setHue(float hue) {
		hsv[0] = hue;
		invalidate();
	}
}
