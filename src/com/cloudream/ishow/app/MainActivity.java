package com.cloudream.ishow.app;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.content.PermissionChecker;
import android.util.Log;
import android.view.View;
import android.widget.Toast;
import android.widget.Toolbar.LayoutParams;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;

import com.cloudream.ishow.R;


public class MainActivity extends Activity implements View.OnClickListener
{
	private static final String TAG = MainActivity.class.getSimpleName();
	
	private enum RequestFrom
	{
		COLLAGE_IMAGE_ACTIVITY,
		CROP_IMAGE_ACTIVITY,
		COLOR_BALANCE_ACTIVITY,
		BEAUTIFY_ACTIVITY,
		MAKEUP_ACTIVITY,
		EFFECT_ACTIVITY,
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main_activity);


		findViewById(R.id.makeup).setOnClickListener(this);
	}

	private Intent gotoGallery(int picture_count)
	{
		return GalleryActivity.startGalleryActivity(this, picture_count);
	}
	
	@Override
	public void onClick(View view)
	{
		switch(view.getId())
		{
		case R.id.makeup:
			startActivityForResult(gotoGallery(1), RequestFrom.MAKEUP_ACTIVITY.ordinal());
			break;
		}
	}

	@Override
	protected void onActivityResult(final int requestCode, final int resultCode, final Intent data)
	{
		if(resultCode != RESULT_OK)
		{
			Log.w(TAG, "What's happening here?");
			return;
		}
		
		Intent intent = new Intent();
		intent.putExtras(data);
		
		final RequestFrom values[] = RequestFrom.values();
		if(requestCode >= values.length)
			return;
		RequestFrom request = values[requestCode];
		switch(request)
		{
		case MAKEUP_ACTIVITY:
			intent.setClass(this, MakeupActivity.class);
			startActivity(intent);
			break;
		default:
			super.onActivityResult(requestCode, resultCode, data);
			break;
		}
	}
	

}
