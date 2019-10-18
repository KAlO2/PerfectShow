package com.wonderful.ishow.app;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import com.wonderful.ishow.R;
import com.wonderful.ishow.util.MediaStoreUtils;


public class MainActivity extends BaseActivity implements View.OnClickListener {
	private static final String TAG = MainActivity.class.getSimpleName();
	
	static final int REQUEST_IMAGE_GET = 1;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main_activity);
		
		initView();
		initData();
		initListener();
	}
	
	private void initView() {
	
	}

	private void initData() {
		Intent appLinkIntent = getIntent();
		String appLinkAction = appLinkIntent.getAction();
		Uri appLinkData = appLinkIntent.getData();
	}
	
	private void initListener() {
		findViewById(R.id.crop).setOnClickListener(this);
		findViewById(R.id.makeup).setOnClickListener(this);
	}
	
	@Override
	public void onClick(View view) {
		switch(view.getId()) {
		case R.id.crop:
			startActivity(new Intent(this, com.yalantis.ucrop.UCropActivity.class));
			break;
			
		case R.id.makeup:
			selectImage();
			break;
			
		default:
			break;
		}
	}
	
	public void selectImage() {
		Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
		intent.setType("image/*");
		if(intent.resolveActivity(getPackageManager()) != null) {
			startActivityForResult(intent, REQUEST_IMAGE_GET);
		}
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		Log.d(TAG, "onActivityResult requestCode=" + requestCode + ", data=" + data);
		if(resultCode != RESULT_OK) {
			Log.w(TAG, "result canceled");
			return;
		}

		Intent intent = new Intent();
		intent.putExtras(data);
		
		switch(requestCode) {
		case REQUEST_IMAGE_GET:
			Uri fullPhotoUri = data.getData();
			String path = MediaStoreUtils.getPathFromUri(this, fullPhotoUri);
			Log.d(TAG, "image path=" + path);
			MakeupActivity.start(this, path);
			break;

		default:
			super.onActivityResult(requestCode, resultCode, data);
			break;
		}
	}

}
