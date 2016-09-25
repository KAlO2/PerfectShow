package com.cloudream.ishow.activity;

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
	
	private static final int REQUEST_COLLAGE_IMAGE_ACTIVITY = 1;
	private static final int REQUEST_CROP_IMAGE_ACTIVITY = 2;
	private static final int REQUEST_COLOR_BALANCE_ACTIVITY = 3;
	private static final int REQUEST_MAKEUP_ACTIVITY = 4;
	private static final int REQUEST_EFFECT_ACTIVITY = 5;
	
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
			startActivityForResult(gotoGallery(1), REQUEST_MAKEUP_ACTIVITY);
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
		
		switch(requestCode)
		{
		case REQUEST_MAKEUP_ACTIVITY:
			intent.setClass(this, MakeupActivity.class);
			startActivity(intent);
			break;
		default:
			super.onActivityResult(requestCode, resultCode, data);
			break;
		}
	}
	

}
