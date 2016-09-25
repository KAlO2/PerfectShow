package com.cloudream.ishow.util;

import android.content.Context;
import android.graphics.drawable.Drawable;

public class Compatibility
{

	@SuppressWarnings("deprecation")
	public static Drawable getDrawable(Context context, int id)
	{
		// Context.getDrawable() added in API level 21
		if(android.os.Build.VERSION.SDK_INT >= 21)
			return context.getDrawable(id/*, context.getTheme()*/);
		else
			return context.getResources().getDrawable(id);
	}
	
	@SuppressWarnings("deprecation")
	public static int getColor(Context context, int id)
	{
		// Context.getColor() added in API level 23
		if(android.os.Build.VERSION.SDK_INT >= 23)
			return context.getColor(id/*, context.getTheme()*/);
		else
			return context.getResources().getColor(id);
	}
}
