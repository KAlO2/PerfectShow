package com.cloudream.ishow;

import java.io.File;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.Signature;
import android.os.Environment;
import android.util.Base64;
import android.util.Log;

public class App extends android.app.Application
{
	private static final String TAG = App.class.getSimpleName();
	public static final String PACKAGE_NAME = "com.cloudream.ishow";
	
	// Please change SIGNATURE according to your release .keystore file.
	private static final String SIGNATURE = "WwINm6WD6tN1Ao7juOkmWt3plow=";
	
	private ArrayList<Activity> runningActivities;

	@Override
	public void onCreate()
	{
		super.onCreate();

		if(BuildConfig.DEBUG)
		{
			final String package_name = getApplicationContext().getPackageName();
			if(!PACKAGE_NAME.equals(package_name))
				throw new SecurityException("Cached package name doesn't match with App's package name");
		}
		else
		{
			if(!checkAppSignature(this))
				throw new RuntimeException("wrong signature");
		}
	}

	private String getVersion()
	{
		try
		{
			PackageManager pm = this.getPackageManager();
			PackageInfo pi = pm.getPackageInfo(this.getPackageName(), PackageManager.GET_ACTIVITIES);
			return pi.versionName;
		}
		catch(PackageManager.NameNotFoundException e)
		{
			e.printStackTrace();
		}
		return "1.0.0";
	}

	public static boolean checkAppSignature(Context context)
	{
		MessageDigest md;
		try
		{
			md = MessageDigest.getInstance("SHA");
		}
		catch(NoSuchAlgorithmException e)
		{
			e.printStackTrace();
			return false;
		}
		
		try
		{
			PackageManager pm = context.getPackageManager();
			PackageInfo packageInfo = pm.getPackageInfo(context.getPackageName(), PackageManager.GET_SIGNATURES);
			
			for(Signature signature : packageInfo.signatures)
			{
				md.update(signature.toByteArray());
				byte[] signatureBytes = md.digest();
				
				final String currentSignature = Base64.encodeToString(signatureBytes, Base64.DEFAULT);
//				Log.d(TAG, "signature: " + currentSignature);  // show signature
				if(SIGNATURE.equals(currentSignature))
					return true;
			}
		}
		catch(NameNotFoundException e)
		{
			e.printStackTrace();
		}
		
		return false;
	}
	
	public static String getWorkingDirectory()
	{
		final File pictures = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
		File file = new File(pictures, "PerfectShow");
		if(!file.exists())
			file.mkdir();
		else if(file.isFile())
		{
			Log.w(TAG, file.getPath() + " is occupied");
			return pictures.getPath();
		}
		
		return file.getPath();
	}
	
	public boolean isApplicationInstalled(String packageName)
	{
		try
		{
			PackageManager pm = this.getPackageManager();
			pm.getApplicationInfo(packageName, PackageManager.GET_UNINSTALLED_PACKAGES);
			return true;
		}
		catch(PackageManager.NameNotFoundException e)
		{
			return false;
		}
	}

	public void exit()
	{
		// stopService(intent);
		// this.finishAffinity();
		android.os.Process.killProcess(android.os.Process.myPid());
//		System.exit(0);
	}
}
