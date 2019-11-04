package com.wonderful.ishow.app;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.wonderful.ishow.R;
import com.wonderful.ishow.util.FileUtils;
import com.wonderful.ishow.util.FormatUtils;
import com.wonderful.ishow.util.MediaStoreUtils;
import com.yalantis.ucrop.UCrop;

import java.io.File;


public class MainActivity extends BaseActivity implements View.OnClickListener {
	private static final String TAG = MainActivity.class.getSimpleName();
	
	static final int REQUEST_IMAGE_CROP = 1;
	static final int REQUEST_IMAGE_MAKEUP = 2;

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
			selectImage(REQUEST_IMAGE_CROP);
			break;
			
		case R.id.makeup:
			selectImage(REQUEST_IMAGE_MAKEUP);
			break;
			
		default:
			break;
		}
	}
	
	public void selectImage(int requestCode) {
		final String permission = Manifest.permission.WRITE_EXTERNAL_STORAGE;
		if(ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
			requestPermission(permission, R.string.permission_write_external_storage, requestCode);
			return;
		}
		
		Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
		intent.setType("image/*");
		if(intent.resolveActivity(getPackageManager()) != null) {
			startActivityForResult(intent, requestCode);
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
		
		Uri uri = data.getData();
		switch(requestCode) {
		case REQUEST_IMAGE_CROP:
//			startActivity(new Intent(this, com.yalantis.ucrop.UCropActivity.class));
			if(uri != null) {
				UCrop.Options options = new UCrop.Options();
				options.setCompressionFormat(Bitmap.CompressFormat.PNG);
				
				File dir = FileUtils.getPictureDirectory();
				String filename = FormatUtils.formatTime() + "cropped" + FileUtils.SUFFIX_PNG;
				Uri destinationUri = Uri.fromFile(new File(dir, filename));
				UCrop.of(uri, destinationUri)
//						.withAspectRatio(16, 9)
//						.withMaxResultSize(maxWidth, maxHeight)
						.withOptions(options)
						.start(this);
			} else {
				Toast.makeText(this, R.string.cannot_retrieve_selected_image, Toast.LENGTH_SHORT).show();
			}
			break;
			
		case UCrop.REQUEST_CROP:
			Uri resultUri = UCrop.getOutput(data);
			String destinationPath = MediaStoreUtils.getPathFromUri(this, resultUri);
			Toast.makeText(this, getString(R.string.image_saved_to, destinationPath), Toast.LENGTH_LONG).show();
			MediaStoreUtils.scanFile(this, resultUri);
			break;
			
		case REQUEST_IMAGE_MAKEUP:
			String path = MediaStoreUtils.getPathFromUri(this, uri);
			Log.d(TAG, "image path=" + path);
			MakeupActivity.start(this, path);
			break;

		default:
			super.onActivityResult(requestCode, resultCode, data);
			break;
		}
	}

}
