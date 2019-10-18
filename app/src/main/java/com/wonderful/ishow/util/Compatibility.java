package com.wonderful.ishow.util;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.view.View;

public class Compatibility
{

	@SuppressWarnings("deprecation")
	public static Drawable getDrawable(Context context, int id) {
		// Context#getDrawable(int) is added in API level 21
		if(Build.VERSION.SDK_INT >= 21/* Build.VERSION_CODES.LOLLIPOP */)
			return context.getDrawable(id/*, context.getTheme()*/);
		else
			return context.getResources().getDrawable(id);
	}
	
	@SuppressWarnings("deprecation")
	public static int getColor(Context context, int id) {
		// Context#getColor(int, Theme) is added in API level 23
		if(Build.VERSION.SDK_INT >= 23/* Build.VERSION_CODES.MARSHMALLOW */)
			return context.getColor(id/*, context.getTheme()*/);
		else
			return context.getResources().getColor(id);
	}
	
	@SuppressWarnings("deprecation")
	public static ColorStateList getColorStateList(Context context, int id) {
		// Context#getColor(int, Theme) is added in API level 23
		if(Build.VERSION.SDK_INT >= 23/* Build.VERSION_CODES.MARSHMALLOW */)
			return context.getColorStateList(id/*, context.getTheme()*/);
		else
			return context.getResources().getColorStateList(id);
	}
	
	@SuppressWarnings("deprecation")
	public static void setBackground(View view, Drawable background) {
		if(Build.VERSION.SDK_INT >= 17)
			view.setBackground(background);
		else
			view.setBackgroundDrawable(background);  // deprecated in API level 16.
	}
}
