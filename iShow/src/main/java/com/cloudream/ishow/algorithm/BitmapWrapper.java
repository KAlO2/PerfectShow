package com.cloudream.ishow.algorithm;

import android.graphics.Bitmap;

// TODO I haven't come up with a good class name.
class BitmapWrapper
{
	protected final Bitmap bmp_start;  //< raw image
	protected Bitmap bmp_mask;  //< do operation on mask instead of entire image
	protected Bitmap bmp_stop;  //< image after every single operation
	protected Bitmap bmp_step;  //< intermediate state image, usually generates when sliding bar moves.
	
	
	protected BitmapWrapper(Bitmap image)
	{
		bmp_start = image;
		bmp_stop  = image.copy(Bitmap.Config.ARGB_8888, true/* mutable */);
		bmp_step  = image.copy(Bitmap.Config.ARGB_8888, true/* mutable */);
		
		bmp_mask  = Bitmap.createBitmap(image.getWidth(), image.getHeight(), Bitmap.Config.ALPHA_8);
	}
	
	public void apply()
	{
		bmp_stop = bmp_step.copy(Bitmap.Config.ARGB_8888, true/* mutable */);;
	}
	
	public Bitmap getRawImage()          { return bmp_start; }
	public Bitmap getIntermediateImage() { return bmp_step;  }
	public Bitmap getFinalImage()        { return bmp_stop;  }
}
