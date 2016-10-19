package com.cloudream.ishow.bean;


public class MakeupParams
{
	public static enum Region
	{
		SKIN,
		EYE_BROW,
		EYE_LASH,
		EYE_SHADOW,
		BLUSHER,
		NOSE,
		LIPS;
	}
	
	public static class MixParameter
	{
		Region region;
		int id;  // image resource ID
		int color;
		String path;
		
		float weight;  // blending amount, in range [0, 1]
	}
	
	public int   lips_color_id;
	public float lips_color_weight;

}
