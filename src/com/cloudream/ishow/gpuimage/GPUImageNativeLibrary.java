package com.cloudream.ishow.gpuimage;

/**
 * Note Android has android.graphics.YuvImage, but conversion to android.graphics.Bitmap is not
 * provided.
 * @see http://stackoverflow.com/questions/32276522/convert-nv21-byte-array-into-bitmap-readable-format
 */
public class GPUImageNativeLibrary
{
	static
	{
		System.loadLibrary("gpuimage");
	}

	public static native void YUVtoRBGA(byte[] yuv, int width, int height, int[] out);
	public static native void YUVtoARBG(byte[] yuv, int width, int height, int[] out);
}
