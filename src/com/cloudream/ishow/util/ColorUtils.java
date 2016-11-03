package com.cloudream.ishow.util;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.support.annotation.ArrayRes;

public class ColorUtils
{
	/**
	 * #Color use #AARRGGBB, namely BGRA in memory layout, while native layer use #AABBGGRR, namely
	 * RGBA memory layout, need a swap(R, B) here.
	 * 
	 * @param  color in BGRA layout, as expressed in #Color
	 * @return color in RGBA layout
	 */
	public static int bgra2rgba(int color)
	{
		int alpha = color & 0xff000000;  // alplha >>> 24
		int red   = Color.red(color);
		int green = color & 0x0000ff00;  // (color >> 8) & 0xFF
		int blue  = Color.blue(color);
		return alpha | (blue << 16) | green | red;
	}
	
	/**
	 * Output human readable string for color.
	 * 
	 * @param  Android #AARRGGBB format, namely 0xBBGGRRAA
	 * @return string "#AARRGGBB"
	 */
	public static String colorToString(int color)
	{
		return String.format("#%02X%02X%02X%02X",
				(byte)(Color.alpha(color)&0xff),
				(byte)(Color.red  (color)&0xff), 
				(byte)(Color.green(color)&0xff),
				(byte)(Color.blue (color)&0xff));
	}
	
	public static int[] obtainColorArray(Context context, @ArrayRes int resId)
	{
		TypedArray array = context.getResources().obtainTypedArray(resId);
		final int length = array.length();
		
		int[] resIds = new int[length];
		
		for(int i = 0; i < length; ++i)
		    resIds[i] = array.getColor(i, Color.BLACK/* default color*/);
		
		array.recycle();
		return resIds;
	}
}
