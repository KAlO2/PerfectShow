package com.cloudream.ishow.util;

import java.security.InvalidParameterException;
import java.util.Random;

import com.cloudream.ishow.BuildConfig;

import android.graphics.PointF;
import android.renderscript.Sampler.Value;

/**
 * A class that contains utility methods related to numbers.
 * supplement utilities for #java.lang.Math
 */
public final class MathUtils
{
	private static final Random random = new Random();
	
	private static final float DEG_TO_RAD = (float)(Math.PI / 180);
	private static final float RAD_TO_DEG = (float)(180 / Math.PI);

	private MathUtils()
	{
	}

	public static int clamp(int amount, int low, int high)
	{
		if(BuildConfig.DEBUG && low > high)
			throw new InvalidParameterException("low > high");
		return amount < low ? low : (amount > high ? high : amount);
	}

	public static long clamp(long amount, long low, long high)
	{
		if(BuildConfig.DEBUG && low > high)
			throw new InvalidParameterException("low > high");
		return amount < low ? low : (amount > high ? high : amount);
	}

	public static float clamp(float amount, float low, float high)
	{
		if(BuildConfig.DEBUG && low > high)
			throw new InvalidParameterException("low > high");
		return amount < low ? low : (amount > high ? high : amount);
	}

	public static float clamp(float amount)
	{
		if(amount < 0)
			return 0;
		else if(amount > 1)
			return 1;
		else return amount;
	}
	
	// with modulation fall in range [low, high]
	public static int wrap(int amount, int low, int high)
	{
		if(BuildConfig.DEBUG && low >= high)
			throw new InvalidParameterException("low >= high");
		
		int range = high - low + 1;
		if(amount < low)
			amount += range *((low - amount)/range + 1);
		
		return low + (amount - low)%range;
	}
	
	public static float max(float a, float b, float c)
	{
//		return Math.max(Math.max(a, b), c); // take consider NAN condition
		return a > b ? (a > c ? a : c) : (b > c ? b : c);
	}

	public static int max(int a, int b, int c)
	{
		return Math.max(Math.max(a, b), c);
	}

	public static float min(float a, float b, float c)
	{
//		return Math.min(Math.min(a, b), c); // take consider NAN condition
		return a < b ? (a < c ? a : c) : (b < c ? b : c);
	}

	public static int min(int a, int b, int c)
	{
		return Math.min(Math.min(a, b), c);
	}

	/**
	 *  Normalizes a value from [min, max] to [0, 1]
	 */
	public static float normalize(float val, float min, float max)
	{
		if(min >= max || Float.isNaN(min) || Float.isNaN(max))
			throw new InvalidParameterException("invalid parameter min and max");
		
		if(Math.abs(max - min) < 0.01f)
			return 0.5f;

		if(val <= min)
			return 0;
		else if(val >= max)
			return 1;
		
		float scale = 1 / (max - min);
		return (val - min) * scale;
	}
    
	public static float distance(float x1, float y1, float x2, float y2)
	{
		final float dx = (x2 - x1);
		final float dy = (y2 - y1);
//		return (float) Math.hypot(x, y);  // number too big consideration
		return (float) Math.sqrt(dx * dx + dy * dy);
	}

	public static float distance(PointF p1, PointF p2)
	{
		float dx = p1.x - p2.x;
		float dy = p1.y - p2.y;
		return (float)Math.sqrt(dx * dx + dy * dy);
	}
	
	public static float distance(float x1, float y1, float z1, float x2, float y2, float z2)
	{
		final float x = (x2 - x1);
		final float y = (y2 - y1);
		final float z = (z2 - z1);
		return (float) Math.sqrt(x * x + y * y + z * z);
	}

	public static float mag(float a, float b)
	{
		return (float) Math.hypot(a, b);
	}

	public static float mag(float a, float b, float c)
	{
		return (float) Math.sqrt(a * a + b * b + c * c);
	}

	public static float dot(float v1x, float v1y, float v2x, float v2y)
	{
		return v1x * v2x + v1y * v2y;
	}

	public static float dot(PointF v1, PointF v2)
	{
		return v1.x * v2.x + v1.y * v2.y;
	}
	
	public static float cross(float v1x, float v1y, float v2x, float v2y)
	{
		return v1x * v2y - v1y * v2x;
	}

	public static float cross(PointF v1, PointF v2)
	{
		return v1.x * v2.y - v1.y * v2.x;
	}
	
	public static float radians(float degrees)
	{
		return degrees * DEG_TO_RAD;
	}

	public static float degrees(float radians)
	{
		return radians * RAD_TO_DEG;
	}

	public static float lerp(float start, float stop, float amount)
	{
		return start + (stop - start) * amount;
	}

	public static float norm(float start, float stop, float value)
	{
		return (value - start) / (stop - start);
	}

	public static float map(float minStart, float minStop, float maxStart, float maxStop, float value)
	{
		return maxStart + (maxStart - maxStop) * ((value - minStart) / (minStop - minStart));
	}

	public static int random(int howbig)
	{
		return (int) (random.nextFloat() * howbig);
	}

	public static int random(int low, int high)
	{
		if(BuildConfig.DEBUG && low > high)
			throw new InvalidParameterException("low > high");

//		return random.nextInt() % (high - low + 1) + low;  // not so good
		return (int) (random.nextFloat() * (high - low + 1) + low);
	}

	public static float random(float high)
	{
		return random.nextFloat() * high;
	}

	public static float random(float low, float high)
	{
		if(BuildConfig.DEBUG && low > high)
			throw new InvalidParameterException("low > high");
		return random.nextFloat() * (high - low) + low;
	}

	public static void randomSeed(long seed)
	{
		random.setSeed(seed);
	}
}
