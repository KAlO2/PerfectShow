package com.cloudream.ishow.app;

import java.io.File;
import java.util.List;
import com.cloudream.ishow.R;
import com.cloudream.ishow.util.BitmapUtils;

import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.util.Pair;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.GridView;
import android.widget.Toast;

public class SettingsActivity extends PreferenceActivity
{
	private static final String TAG = SettingsActivity.class.getSimpleName();
	
	/* package */ static final String KEY_IMAGE_FORMAT         = "image_format";
	/* package */ static final String KEY_IMAGE_QUALITY        = "image_quality";

	/* package */ static final String TEMP_DIR = ".temp";
	/* package */ static final int DEFAULT_IMAGE_FORMAT = Bitmap.CompressFormat.JPEG.ordinal();
	/* package */ static final int DEFAULT_IMAGE_QUALITY = 88;
	
	private SharedPreferences prefs;

	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		prefs = PreferenceManager.getDefaultSharedPreferences(this);

		Preference pref_image_format = findPreference(KEY_IMAGE_FORMAT);
		Preference pref_image_quality = findPreference(KEY_IMAGE_QUALITY);
		
		// initial state
		boolean flag = prefs.getInt(KEY_IMAGE_FORMAT, DEFAULT_IMAGE_FORMAT) == Bitmap.CompressFormat.PNG.ordinal();
		pref_image_quality.setEnabled(!flag);  // PNG is loseless
		int quality = prefs.getInt(KEY_IMAGE_QUALITY, DEFAULT_IMAGE_QUALITY);
		final String image_quality = getString(R.string.image_quality, quality);
		pref_image_quality.setTitle(image_quality);

	}
	
	public static Pair<Bitmap.CompressFormat, Integer> getFormatAndQuality(Context context)
	{
		SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(context);
		int image_format  = preference.getInt(KEY_IMAGE_FORMAT,  DEFAULT_IMAGE_FORMAT);
		int image_quality = preference.getInt(KEY_IMAGE_QUALITY, DEFAULT_IMAGE_QUALITY);
		
		final Bitmap.CompressFormat formats[] = Bitmap.CompressFormat.values();
		Bitmap.CompressFormat compress_format;
		if(image_format < 0 || image_format >= formats.length)
			compress_format = Bitmap.CompressFormat.JPEG;
		else
			compress_format = formats[image_format];
		
		return new Pair<Bitmap.CompressFormat, Integer>(compress_format, image_quality);
	}
	
	/**
	 * Save bitmap into file.
	 * @param context
	 * @param bitmap    the bitmap to be saved.
	 * @param file      if null, generate a filename according to the current time.
	 * @param category  sub-directory
	 * @return full path of the saved image, or null if save operation failed for some reason.
	 */
	public static String saveImage(Context context, @NonNull Bitmap bitmap, @Nullable File file, String category)
	{
		Pair<Bitmap.CompressFormat, Integer> pair = SettingsActivity.getFormatAndQuality(context);
		Bitmap.CompressFormat format = pair.first;
		int quality = pair.second;
		
		// temporary directory store uncompressed intermediate image.
		if(TEMP_DIR.equals(category))
			format = Bitmap.CompressFormat.PNG;
		
		if(file == null)
		{
			String name = BitmapUtils.createUniversalFilename();
			file = BitmapUtils.createImageFile(name, category, format);
		}
		
		if(BitmapUtils.saveImage(bitmap, file, format, quality))
			return file.getAbsolutePath();
		else
			return null;
	}
	
	public static String saveImage(Context context, @NonNull Bitmap bitmap, String category)
	{
		return saveImage(context, bitmap, null, category);
	}

}
