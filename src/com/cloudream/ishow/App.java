package com.cloudream.ishow;

import java.io.File;
import java.util.ArrayList;

import android.app.Activity;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.util.Log;

public class App extends android.app.Application
{
	private static final String TAG = App.class.getSimpleName();
	public static final String PACKAGE_NAME = "com.cloudream.makeup";

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
