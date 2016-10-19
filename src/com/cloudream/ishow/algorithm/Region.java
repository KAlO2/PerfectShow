package com.cloudream.ishow.algorithm;

import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Size;

import android.graphics.PointF;

// This class is with respect to venus/Region.h in C++ side.
public class Region
{
	// Type org.opencv.core.Point is double, so use type android.graphics.PointF instead.
	public PointF pivot;  //< pin point, relative to source image.
	
	// Type org.opencv.core.Size is double, type android.util.Size (added in API level 21) is int,
	// I might as well use type android.graphics.PointF as above.
	public PointF size;  //< ROI's raw size, namely neither scaled nor rotated image size.
	
	public Mat mask;  //< mask of ROI(Region Of Interest).
	
	Region()
	{
		pivot = new PointF();
		size  = new PointF();
		mask  = new Mat();
	}
	
	// This method is used by JNI. (Don't delete it)
	@SuppressWarnings("unused")
	Region(PointF pivot, PointF size, Mat mask)
	{
		this.pivot = pivot;
		this.size = size;
		this.mask = mask;
	}
}
