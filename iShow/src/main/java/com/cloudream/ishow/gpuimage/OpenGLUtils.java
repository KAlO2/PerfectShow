package com.cloudream.ishow.gpuimage;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.IntBuffer;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLException;
import android.opengl.GLU;
import android.opengl.GLUtils;
import android.support.annotation.NonNull;
import android.util.Log;

public class OpenGLUtils
{
	private static final String TAG = OpenGLUtils.class.getSimpleName();
	
	public static final int NO_TEXTURE = -1;
	public static final int NOT_INIT = -1;
	public static final int ON_DRAWN = 1;

	/**
	 * static-methods-only (utility) class
	 */
	private OpenGLUtils(){}
	
	/**
	 * As of version 4.2 Android offers an option called "Enable OpenGL traces" in the phone's
	 * developer options. If you set this to "Call stack on glGetError" you'll see error logs if
	 * OpenGL error happens.
	 * 
	 * @return true if found, otherwise false.
	 */
	public static boolean checkError()
	{
		boolean found = false;
		int error_code = GLES20.GL_NO_ERROR;
		do
		{
/*
			If more than one flag has recorded an error, glGetError returns and clears an
			arbitrary error flag value. Thus, glGetError should always be called in a loop, until
			it returns GL_NO_ERROR, if all error flags are to be reset.
*/
			error_code = GLES20.glGetError();
			if(error_code != GLES20.GL_NO_ERROR)
			{
				found = true;
				Log.e(TAG, "Error: " + GLU.gluErrorString(error_code));
			}
		} while(error_code != GLES20.GL_NO_ERROR);

		return found;
	}

	/**
	 * Checks if OpenGL ES 2.0 is supported on the current device.
	 *
	 * @param context the context
	 * @return true if supports OpenGL ES 2.0, otherwise false.
	 */
	public static boolean isOpenGLES2Supported(Context context)
	{
		ActivityManager activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
		ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
		return configurationInfo.reqGlEsVersion >= 0x20000;
	}
	
	/**
	 * Get the max size of bitmap allowed to be rendered on the device.<br>
	 * http://stackoverflow.com/questions/7428996/hw-accelerated-activity-how-to-get-opengl-texture-size-limit.
	 */
	public static int getMaxTextureSize()
	{
		// Safe minimum default size
		final int IMAGE_MAX_BITMAP_DIMENSION = 2048;

		try
		{
			// Get EGL Display
			EGL10 egl = (EGL10) EGLContext.getEGL();
			EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

			// Initialize
			int[] version = new int[2];
			egl.eglInitialize(display, version);

			// Query total number of configurations
			int[] totalConfigurations = new int[1];
			egl.eglGetConfigs(display, null, 0, totalConfigurations);

			// Query actual list configurations
			EGLConfig[] configurationsList = new EGLConfig[totalConfigurations[0]];
			egl.eglGetConfigs(display, configurationsList, totalConfigurations[0], totalConfigurations);

			int[] textureSize = new int[1];
			int maximumTextureSize = 0;

			// Iterate through all the configurations to located the maximum texture size
			for(int i = 0; i < totalConfigurations[0]; i++)
			{
				// Only need to check for width since OpenGL textures are always squared
				egl.eglGetConfigAttrib(display, configurationsList[i], EGL10.EGL_MAX_PBUFFER_WIDTH, textureSize);

				// Keep track of the maximum texture size
				if(maximumTextureSize < textureSize[0])
					maximumTextureSize = textureSize[0];
			}

			// Release
			egl.eglTerminate(display);

			// Return largest texture size found, or default
			return Math.max(maximumTextureSize, IMAGE_MAX_BITMAP_DIMENSION);
		}
		catch(Exception e)
		{
			return IMAGE_MAX_BITMAP_DIMENSION;
		}
	}

	public static Bitmap createBitmapFromGLSurface(int x, int y, int w, int h, GL10 gl)
	{
		int bitmapBuffer[] = new int[w * h];
		int bitmapSource[] = new int[w * h];
		IntBuffer intBuffer = IntBuffer.wrap(bitmapBuffer);
		intBuffer.position(0);

		try
		{
			gl.glPixelStorei(GL10.GL_PACK_ALIGNMENT, 4);
			gl.glReadPixels(x, y, w, h, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, intBuffer);
			
			for(int i = 0; i < h; i++)
			{
				int offset1 = i * w;
				int offset2 = (h - i - 1) * w;
				for(int j = 0; j < w; j++)
				{
					int texturePixel = bitmapBuffer[offset1 + j];
					int blue = (texturePixel >> 16) & 0xff;
					int red = (texturePixel << 16) & 0x00ff0000;
					int pixel = (texturePixel & 0xff00ff00) | red | blue;
					bitmapSource[offset2 + j] = pixel;
				}
			}
		}
		catch(GLException e)
		{
			return null;
		}

		return Bitmap.createBitmap(bitmapSource, w, h, Bitmap.Config.ARGB_8888);
	}

	public static int loadTexture(final Bitmap img, final int usedTexId)
	{
		return loadTexture(img, usedTexId, false);
	}

	public static int loadTexture(final @NonNull Bitmap image, final int usedTexId, final boolean recycle)
	{
		if(image == null)
			throw new NullPointerException("loadTexture can't be used with a null Bitmap");
		if(image.isRecycled())
			throw new IllegalArgumentException("bitmap is recycled");

		int textures[] = new int[1];
		if(usedTexId == NO_TEXTURE)
		{
			GLES20.glGenTextures(1, textures, 0);
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
			GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
			GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
			GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
			GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);

			GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, image, 0);
		}
		else
		{
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, usedTexId);
			GLUtils.texSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, image);
			textures[0] = usedTexId;
		}
		if(recycle)
			image.recycle();
		
		return textures[0];
	}

	public static int loadTexture(final IntBuffer data, final int width, final int height, final int usedTexId)
	{
		if(data == null)
			return NO_TEXTURE;
		int textures[] = new int[1];
		if(usedTexId == NO_TEXTURE)
		{
			GLES20.glGenTextures(1, textures, 0);
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
			GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
			GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
			GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
			GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
			GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, width, height, 0, GLES20.GL_RGBA,
					GLES20.GL_UNSIGNED_BYTE, data);
		}
		else
		{
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, usedTexId);
			GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, width, height, GLES20.GL_RGBA,
					GLES20.GL_UNSIGNED_BYTE, data);
			textures[0] = usedTexId;
		}
		return textures[0];
	}

	public static int loadTextureAsBitmap(final IntBuffer data, int width, int height, final int usedTexId)
	{
		Bitmap bitmap = Bitmap.createBitmap(data.array(), width, height, Config.ARGB_8888);
		return loadTexture(bitmap, usedTexId);
	}

	public static int loadTexture(final Context context, final String name)
	{
		final int[] textureHandle = new int[1];

		GLES20.glGenTextures(1, textureHandle, 0);

		if(textureHandle[0] != 0)
		{

			// Read in the resource
			final Bitmap bitmap = getImageFromAssetsFile(context, name);

			// Bind to the texture in OpenGL
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureHandle[0]);

			// Set filtering
			GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
			GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
			GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
			GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
			// Load the bitmap into the bound texture.
			GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);

			// Recycle the bitmap, since its data has been loaded into OpenGL.
			bitmap.recycle();
		}

		if(textureHandle[0] == 0)
		{
			throw new RuntimeException("Error loading texture.");
		}

		return textureHandle[0];
	}

	private static Bitmap getImageFromAssetsFile(Context context, String fileName)
	{
		Bitmap image = null;
		AssetManager am = context.getResources().getAssets();
		try
		{
			InputStream is = am.open(fileName);
			image = BitmapFactory.decodeStream(is);
			is.close();
		}
		catch(IOException e)
		{
			e.printStackTrace();
		}
		return image;
	}

	public static int loadProgram(final String vertSource, final String fragSource)
	{
		int[] link = new int[1];
		int iVShader = loadShader(vertSource, GLES20.GL_VERTEX_SHADER);
		if(iVShader == 0)
		{
			Log.e(TAG, "Load Program, Vertex Shader Failed");
			return 0;
		}
		int iFShader = loadShader(fragSource, GLES20.GL_FRAGMENT_SHADER);
		if(iFShader == 0)
		{
			Log.e(TAG, "Load Program, Fragment Shader Failed");
			return 0;
		}
		
		int program = GLES20.glCreateProgram();

		GLES20.glAttachShader(program, iVShader);
		GLES20.glAttachShader(program, iFShader);

		GLES20.glLinkProgram(program);

		GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, link, 0);
		if(link[0] <= 0)
		{
			Log.e(TAG, "Load Program, Linking Failed");
			return 0;
		}
		GLES20.glDeleteShader(iVShader);
		GLES20.glDeleteShader(iFShader);
		return program;
	}

	private static int loadShader(final String strSource, final int iType)
	{
		int[] compiled = new int[1];
		int iShader = GLES20.glCreateShader(iType);
		GLES20.glShaderSource(iShader, strSource);
		GLES20.glCompileShader(iShader);
		GLES20.glGetShaderiv(iShader, GLES20.GL_COMPILE_STATUS, compiled, 0);
		if(compiled[0] == 0)
		{
			Log.e(TAG, "LoadShader failed, compilation\n" + GLES20.glGetShaderInfoLog(iShader));
			return 0;
		}
		return iShader;
	}

	public static int getExternalOESTextureID()
	{
		int[] texture = new int[1];

		GLES20.glGenTextures(1, texture, 0);
		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, texture[0]);
		GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
		GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);

		return texture[0];
	}

	public static String readShaderFromRawResource(final Context context, final int resourceId)
	{
		final InputStream inputStream = context.getResources().openRawResource(resourceId);
		final InputStreamReader inputStreamReader = new InputStreamReader(inputStream);
		final BufferedReader bufferedReader = new BufferedReader(inputStreamReader);

		String nextLine;
		final StringBuilder body = new StringBuilder();

		try
		{
			while((nextLine = bufferedReader.readLine()) != null)
			{
				body.append(nextLine);
				body.append('\n');
			}
		}
		catch(IOException e)
		{
			return null;
		}
		return body.toString();
	}
}
