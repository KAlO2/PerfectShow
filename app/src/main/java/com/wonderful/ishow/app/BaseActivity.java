package com.wonderful.ishow.app;

import com.wonderful.ishow.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v4.content.PermissionChecker;

abstract class BaseActivity extends Activity {
	protected static final int PERMISSION_READ_EXTERNAL_STORAGE = 1;
	protected static final int PERMISSION_WRITE_EXTERNAL_STORAGE = 2;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
	}

	/**
	 * Requests given permission. If the permission has been denied previously, a Dialog will prompt
	 * the user to grant the permission, otherwise it is requested directly.
	 */
	protected void requestPermission(final String permission, String message, final int requestCode) {
		if(ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
			if(ActivityCompat.shouldShowRequestPermissionRationale(this, permission)) {
				// Show an explanation to the user *asynchronously* -- don't block
				// this thread waiting for the user's response! After the user
				// sees the explanation, try again to request the permission.
				new AlertDialog.Builder(this)
					.setTitle(R.string.permission_request)
					.setMessage(message)
					.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
						@Override
						public void onClick(DialogInterface dialog, int which) {
							requestPermissions(new String[]{ permission }, requestCode);
						}
					})
					.setNegativeButton(android.R.string.cancel, null)
					.show();
			}
		    else {  // No explanation needed, we can request the permission.
				ActivityCompat.requestPermissions(this, new String[]{permission}, requestCode);
			}
		}
	}
	
	/*
	 * checkSelfPermission returning PERMISSION_GRANTED for revoked permission with targetSdkVersion <= 22
	 * http://stackoverflow.com/questions/33407250/checkselfpermission-method-is-not-working-in-targetsdkversion-22
	 */
	public boolean checkSelfPermission(Context context, String permission) {
		// For Android < Android Marshmallow(23), self permissions are always granted.
		boolean result = true;

		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			// https://developer.android.com/guide/topics/manifest/uses-sdk-element.html
			int targetSdkVersion = Build.VERSION_CODES.BASE;
			try {
				final PackageInfo info = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
				targetSdkVersion = info.applicationInfo.targetSdkVersion;
			} catch(PackageManager.NameNotFoundException e) {
				e.printStackTrace();
			}

			if(targetSdkVersion >= Build.VERSION_CODES.M) {
				// targetSdkVersion >= Android M, we can use Context#checkSelfPermission
				result = context.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
			} else {
				// targetSdkVersion < Android M, we have to use PermissionChecker
				result = PermissionChecker.checkSelfPermission(context,
						permission) == PermissionChecker.PERMISSION_GRANTED;
			}
		}

		return result;
	}

}
