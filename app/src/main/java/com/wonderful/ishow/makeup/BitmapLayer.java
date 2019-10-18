package com.wonderful.ishow.makeup;

import android.graphics.Bitmap;
import android.support.annotation.NonNull;


class BitmapLayer {
	protected final Bitmap inputImage;
	
	/**
	 * do operations on masked image instead of the entire image.
	 */
	protected Bitmap mask;
	
	/**
	 * mutable image after every single operation.
	 */
	protected Bitmap outputImage;
	
	/**
	 * intermediate state image, usually generates when sliding bar moves.
	 */
	protected Bitmap intermediateImage;
	
	
	protected BitmapLayer(@NonNull Bitmap image) {
		inputImage = image;
		outputImage  = image.copy(Bitmap.Config.ARGB_8888, true/* mutable */);
		intermediateImage  = image.copy(Bitmap.Config.ARGB_8888, true/* mutable */);
		
		mask = Bitmap.createBitmap(image.getWidth(), image.getHeight(), Bitmap.Config.ALPHA_8);
	}
	
	/**
	 * apply modification to output image.
	 */
	public void apply() {
		outputImage = intermediateImage.copy(Bitmap.Config.ARGB_8888, true/* mutable */);
	}
	
	public Bitmap getInputImage() {
		return inputImage;
	}
	
	public Bitmap getIntermediateImage() {
		return intermediateImage;
	}
	
	public Bitmap getOutputImage() {
		return outputImage;
	}
}
