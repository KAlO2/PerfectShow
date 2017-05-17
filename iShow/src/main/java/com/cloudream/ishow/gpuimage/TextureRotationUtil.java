package com.cloudream.ishow.gpuimage;

public class TextureRotationUtil {

	public static final float TEXCOORD_ARRAY[][] =
	{
		{	// Surface.ROTATION_0
			0.0F, 1.0F,
			1.0F, 1.0F,
			0.0F, 0.0F,
			1.0F, 0.0F,
		},
		{	// Surface.ROTATION_90
			1.0F, 1.0F,
			1.0F, 0.0F,
			0.0F, 1.0F,
			0.0F, 0.0F,
		},
		{	// Surface.ROTATION_180
			1.0F, 0.0F,
			0.0F, 0.0F,
			1.0F, 1.0F,
			0.0F, 1.0F,
		},
		{	// Surface.ROTATION_270
			0.0F, 0.0F,
			0.0F, 1.0F,
			1.0F, 0.0F,
			1.0F, 1.0F,
		}
	};

	public static final float CUBE[] =
	{
		-1.0F, -1.0F,
		 1.0F, -1.0F,
		-1.0F,  1.0F,
		 1.0F,  1.0F,
	};
	
	private TextureRotationUtil()
	{
	}

	/**
	 * 
	 * @param rotation
	 * @param flipHorizontal
	 * @param flipVertical
	 * @return
	 */
	public static float[] getRotation(final int rotation, final boolean flipHorizontal, final boolean flipVertical)
	{
		float[] rotatedTex = TEXCOORD_ARRAY[rotation];

		if(flipHorizontal)
		{
			rotatedTex = new float[]
			{
				flip(rotatedTex[0]), rotatedTex[1],
				flip(rotatedTex[2]), rotatedTex[3],
				flip(rotatedTex[4]), rotatedTex[5],
				flip(rotatedTex[6]), rotatedTex[7],
			};
		}
		if(flipVertical)
		{
			rotatedTex = new float[]
			{
				rotatedTex[0], flip(rotatedTex[1]),
				rotatedTex[2], flip(rotatedTex[3]),
				rotatedTex[4], flip(rotatedTex[5]),
				rotatedTex[6], flip(rotatedTex[7]),
			};
		}
		return rotatedTex;
	}

	private static float flip(final float x)
	{
		return 1.0F - x;
	}
}
