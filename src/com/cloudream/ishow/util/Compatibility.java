package com.cloudream.ishow.util;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Build;

public class Compatibility
{

	@SuppressWarnings("deprecation")
	public static Drawable getDrawable(Context context, int id)
	{
		// Context#getDrawable(int) added in API level 21.
		if(Build.VERSION.SDK_INT >= 21/* Build.VERSION_CODES.LOLLIPOP */)
			return context.getDrawable(id/*, context.getTheme()*/);
		else
			return context.getResources().getDrawable(id);
	}
	
	@SuppressWarnings("deprecation")
	public static int getColor(Context context, int id)
	{
		// Context#getColor(int, Theme) is added in API level 23.
		if(Build.VERSION.SDK_INT >= 23/* Build.VERSION_CODES.MARSHMALLOW */)
			return context.getColor(id/*, context.getTheme()*/);
		else
			return context.getResources().getColor(id);
	}
}
