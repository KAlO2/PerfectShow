package com.cloudream.ishow.bean;

import android.graphics.Bitmap;


public class CropParameters
{

	private int mMaxResultImageSizeX, mMaxResultImageSizeY;

	private Bitmap.CompressFormat mCompressFormat;
	private int mCompressQuality;
	private String mImageInputPath, mImageOutputPath;
	private ExifInfo mExifInfo;

	public CropParameters(int maxResultImageSizeX, int maxResultImageSizeY, Bitmap.CompressFormat compressFormat,
			int compressQuality, String imageInputPath, String imageOutputPath, ExifInfo exifInfo)
	{
		mMaxResultImageSizeX = maxResultImageSizeX;
		mMaxResultImageSizeY = maxResultImageSizeY;
		mCompressFormat = compressFormat;
		mCompressQuality = compressQuality;
		mImageInputPath = imageInputPath;
		mImageOutputPath = imageOutputPath;
		mExifInfo = exifInfo;
	}

	public int getMaxResultImageSizeX()
	{
		return mMaxResultImageSizeX;
	}

	public int getMaxResultImageSizeY()
	{
		return mMaxResultImageSizeY;
	}

	public Bitmap.CompressFormat getCompressFormat()
	{
		return mCompressFormat;
	}

	public int getCompressQuality()
	{
		return mCompressQuality;
	}

	public String getImageInputPath()
	{
		return mImageInputPath;
	}

	public String getImageOutputPath()
	{
		return mImageOutputPath;
	}

	public ExifInfo getExifInfo()
	{
		return mExifInfo;
	}

}
