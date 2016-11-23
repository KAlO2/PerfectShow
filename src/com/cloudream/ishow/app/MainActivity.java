package com.cloudream.ishow.app;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
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
