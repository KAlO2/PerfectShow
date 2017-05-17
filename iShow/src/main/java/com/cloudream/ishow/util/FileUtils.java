package com.cloudream.ishow.util;

import android.annotation.SuppressLint;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.support.annotation.NonNull;
import android.util.Log;
import android.webkit.MimeTypeMap;

import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.Comparator;
import java.util.Date;
import java.util.Locale;

public class FileUtils
{
	private static final String TAG = "FileUtils";
	private static final boolean DBG = false; // Set to true to enable logging

	public static final String MIME_TYPE_AUDIO = "audio/*";
	public static final String MIME_TYPE_TEXT  = "text/*";
	public static final String MIME_TYPE_IMAGE = "image/*";
	public static final String MIME_TYPE_VIDEO = "video/*";
	public static final String MIME_TYPE_APP   = "application/*";

	public static final String HIDDEN_PREFIX = ".";

	/**
	 * private constructor to enforce Singleton pattern
	 */
	private FileUtils()
	{
	}
	
	/**
	 * Gets the extension of a file name, like ".png" or ".jpg".
	 *
	 * @param uri
	 * @return Extension including the dot("."); "" if there is no extension; null if uri was null.
	 */
	public static String getExtension(String uri)
	{
		if(uri == null)
			return null;

		int dot = uri.lastIndexOf(".");
		if(dot >= 0)
			return uri.substring(dot);
		else
			return "";  // No extension.
	}

	/**
	 * @return Whether the URI is a local one.
	 */
	public static boolean isLocal(@NonNull String url)
	{
		return !url.startsWith("http://") && !url.startsWith("https://");
	}

	/**
	 * @return True if Uri is a MediaStore Uri.
	 */
	public static boolean isMediaUri(Uri uri)
	{
		return "media".equalsIgnoreCase(uri.getAuthority());
	}

	/**
	 * Returns the path only (without file name).
	 *
	 * @param file
	 * @return
	 */
	public static File getPathWithoutFilename(@NonNull File file)
	{
		if(file.isDirectory())
			// no file to be split off. Return everything
			return file;
		else
			return file.getParentFile();
	}

	/**
	 * @return The MIME type for the given file.
	 */
	public static String getMimeType(File file)
	{
		String extension = getExtension(file.getName());

		if(extension.length() > 0)
			return MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension.substring(1));

		return "application/octet-stream";
	}

	/**
	 * @return The MIME type for the give Uri.
	 */
	public static String getMimeType(Context context, Uri uri)
	{
		File file = new File(getPath(context, uri));
		return getMimeType(file);
	}

	/**
	 * @param uri The Uri to check.
	 * @return Whether the Uri authority is ExternalStorageProvider.
	 */
	public static boolean isExternalStorageDocument(Uri uri)
	{
		return "com.android.externalstorage.documents".equals(uri.getAuthority());
	}

	/**
	 * @param uri The Uri to check.
	 * @return Whether the Uri authority is DownloadsProvider.
	 */
	public static boolean isDownloadsDocument(Uri uri)
	{
		return "com.android.providers.downloads.documents".equals(uri.getAuthority());
	}

	/**
	 * @param uri The Uri to check.
	 * @return Whether the Uri authority is MediaProvider.
	 */
	public static boolean isMediaDocument(Uri uri)
	{
		return "com.android.providers.media.documents".equals(uri.getAuthority());
	}

	/**
	 * @param uri The Uri to check.
	 * @return Whether the Uri authority is Google Photos.
	 */
	public static boolean isGooglePhotosUri(Uri uri)
	{
		return "com.google.android.apps.photos.content".equals(uri.getAuthority());
	}

	/**
	 * Get the value of the data column for this Uri. This is useful for MediaStore Uris, and other
	 * file-based ContentProviders.
	 *
	 * @param context The context.
	 * @param uri The Uri to query.
	 * @param selection (Optional) Filter used in the query.
	 * @param selectionArgs (Optional) Selection arguments used in the query.
	 * @return The value of the _data column, which is typically a file path.
	 */
	public static String getDataColumn(Context context, Uri uri, String selection, String[] selectionArgs)
	{

		Cursor cursor = null;
		final String column = "_data";
		final String[] projection = { column };

		try
		{
			cursor = context.getContentResolver().query(uri, projection, selection, selectionArgs, null);
			if(cursor != null && cursor.moveToFirst())
			{
				if(DBG)
					DatabaseUtils.dumpCursor(cursor);

				final int column_index = cursor.getColumnIndexOrThrow(column);
				return cursor.getString(column_index);
			}
		}
		catch(IllegalArgumentException ex)
		{
			Log.e(TAG, "getDataColumn: _data", ex);
		}
		finally
		{
			if(cursor != null)
				cursor.close();
		}
		return null;
	}

	/**
	 * Get a file path from a Uri. This will get the the path for Storage Access Framework
	 * Documents, as well as the _data field for the MediaStore and other file-based
	 * ContentProviders.<br>
	 * <br>
	 * Callers should check whether the path is local before assuming it represents a local file.
	 *
	 * @param context The context.
	 * @param uri The Uri to query.
	 * @see #isLocal(String)
	 * @see #getFile(Context, Uri)
	 */
	@SuppressLint("NewApi")
	public static String getPath(final Context context, final Uri uri)
	{

		if(DBG)
			Log.d(TAG + " File -",
					"Authority: " + uri.getAuthority() +
					", Fragment: " + uri.getFragment() +
					", Port: " + uri.getPort() +
					", Query: " + uri.getQuery() +
					", Scheme: " + uri.getScheme() +
					", Host: " + uri.getHost() +
					", Segments: " + uri.getPathSegments().toString());

		final boolean isKitKat = Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;

		// DocumentProvider
		if(isKitKat && DocumentsContract.isDocumentUri(context, uri))
		{
			if(isExternalStorageDocument(uri))
			{
				final String docId = DocumentsContract.getDocumentId(uri);
				final String[] split = docId.split(":");
				final String type = split[0];

				if("primary".equalsIgnoreCase(type))
				{
					return Environment.getExternalStorageDirectory() + "/" + split[1];
				}

				// TODO handle non-primary volumes
			}
			// DownloadsProvider
			else if(isDownloadsDocument(uri))
			{

				final String id = DocumentsContract.getDocumentId(uri);
				final Uri contentUri = ContentUris.withAppendedId(Uri.parse("content://downloads/public_downloads"),
						Long.valueOf(id));

				return getDataColumn(context, contentUri, null, null);
			}
			// MediaProvider
			else if(isMediaDocument(uri))
			{
				final String docId = DocumentsContract.getDocumentId(uri);
				final String[] split = docId.split(":");
				final String type = split[0];

				Uri contentUri = null;
				if("image".equals(type))
				{
					contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
				}
				else if("video".equals(type))
				{
					contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
				}
				else if("audio".equals(type))
				{
					contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
				}

				final String selection = "_id=?";
				final String[] selectionArgs = new String[]
				{ split[1] };

				return getDataColumn(context, contentUri, selection, selectionArgs);
			}
		}
		// MediaStore (and general)
		else if("content".equalsIgnoreCase(uri.getScheme()))
		{

			// Return the remote address
			if(isGooglePhotosUri(uri))
				return uri.getLastPathSegment();

			return getDataColumn(context, uri, null, null);
		}
		// File
		else if("file".equalsIgnoreCase(uri.getScheme()))
		{
			return uri.getPath();
		}

		return null;
	}

	/**
	 * Convert Uri into File, if possible.
	 *
	 * @return file A local file that the Uri was pointing to, or null if the Uri is unsupported or
	 *         pointed to a remote resource.
	 * @see #getPath(Context, Uri)
	 */
	public static File getFile(Context context, Uri uri)
	{
		if(uri != null)
		{
			String path = getPath(context, uri);
			if(path != null && isLocal(path))
			{
				return new File(path);
			}
		}
		return null;
	}

	/**
	 * Get the file size in a human-readable string.
	 *
	 * @param size
	 * @return
	 */
	public static String getReadableFileSize(int size)
	{
		final int BYTES_IN_KILOBYTES = 1024;
		final DecimalFormat dec = new DecimalFormat("###.#");
		final String KILOBYTES = " KB";
		final String MEGABYTES = " MB";
		final String GIGABYTES = " GB";
		float fileSize = 0;
		String suffix = KILOBYTES;

		if(size > BYTES_IN_KILOBYTES)
		{
			fileSize = size / BYTES_IN_KILOBYTES;
			if(fileSize > BYTES_IN_KILOBYTES)
			{
				fileSize = fileSize / BYTES_IN_KILOBYTES;
				if(fileSize > BYTES_IN_KILOBYTES)
				{
					fileSize = fileSize / BYTES_IN_KILOBYTES;
					suffix = GIGABYTES;
				}
				else
				{
					suffix = MEGABYTES;
				}
			}
		}
		return String.valueOf(dec.format(fileSize) + suffix);
	}

	/**
	 * Attempt to retrieve the thumbnail of given File from the MediaStore. This should not be
	 * called on the UI thread.
	 *
	 * @param context
	 * @param file
	 * @return
	 */
	public static Bitmap getThumbnail(Context context, @NonNull File file)
	{
		Uri uri = Uri.fromFile(file);
		return getThumbnail(context, uri, getMimeType(file));
	}

	/**
	 * Attempt to retrieve the thumbnail of given Uri from the MediaStore. This should not be called
	 * on the UI thread.
	 *
	 * @param context
	 * @param uri
	 * @return
	 */
	public static Bitmap getThumbnail(Context context, Uri uri)
	{
		return getThumbnail(context, uri, getMimeType(context, uri));
	}

	/**
	 * Attempt to retrieve the thumbnail of given Uri from the MediaStore. This should not be called
	 * on the UI thread.
	 *
	 * @param context
	 * @param uri
	 * @param mimeType
	 * @return
	 */
	public static Bitmap getThumbnail(Context context, Uri uri, String mimeType)
	{
		if(DBG)
			Log.d(TAG, "Attempting to get thumbnail");

		if(!isMediaUri(uri))
		{
			Log.e(TAG, "You can only retrieve thumbnails for images and videos.");
			return null;
		}

		Bitmap bitmap = null;
		if(uri != null)
		{
			final ContentResolver resolver = context.getContentResolver();
			Cursor cursor = null;
			try
			{
				cursor = resolver.query(uri, null, null, null, null);
				if(cursor.moveToFirst())
				{
					final int id = cursor.getInt(0);
					if(DBG)
						Log.d(TAG, "Got thumb ID: " + id);

					if(mimeType.contains("video"))
					{
						bitmap = MediaStore.Video.Thumbnails.getThumbnail(resolver, id,
								MediaStore.Video.Thumbnails.MINI_KIND, null);
					}
					else if(mimeType.contains(FileUtils.MIME_TYPE_IMAGE))
					{
						bitmap = MediaStore.Images.Thumbnails.getThumbnail(resolver, id,
								MediaStore.Images.Thumbnails.MINI_KIND, null);
					}
				}
			}
			catch(Exception e)
			{
				if(DBG)
					Log.e(TAG, "getThumbnail", e);
			}
			finally
			{
				if(cursor != null)
					cursor.close();
			}
		}
		return bitmap;
	}

	/**
	 * File and folder comparator. TODO Expose sorting option method
	 *
	 */
	public static final Comparator<File> sComparator = new Comparator<File>()
	{
		@Override
		public int compare(@NonNull File f1, @NonNull File f2)
		{
			// Sort alphabetically by lower case, which is much cleaner.
			String str1 = f1.getName().toLowerCase();
			String str2 = f2.getName().toLowerCase();
			return str1.compareTo(str2);
		}
	};

	/**
	 * File (not directories) filter.
	 *
	 */
	public static final FileFilter sFileFilter = new FileFilter()
	{
		@Override
		public boolean accept(@NonNull File file)
		{
			final String fileName = file.getName();
			// Return files only (not directories) and skip hidden files
			return file.isFile() && !fileName.startsWith(HIDDEN_PREFIX);
		}
	};

	/**
	 * Folder (directories) filter.
	 *
	 */
	public static final FileFilter sDirFilter = new FileFilter()
	{
		@Override
		public boolean accept(@NonNull File file)
		{
			final String fileName = file.getName();
			// Return directories only and skip hidden directories
			return file.isDirectory() && !fileName.startsWith(HIDDEN_PREFIX);
		}
	};

	public static boolean copyFile(@NonNull String pathFrom, @NonNull String pathTo)
	{
		FileChannel outputChannel = null;
		FileChannel inputChannel = null;
		try
		{
			FileInputStream is = new FileInputStream(new File(pathFrom));
			FileOutputStream os = new FileOutputStream(new File(pathTo));
			inputChannel = is.getChannel();
			outputChannel = os.getChannel();
			inputChannel.transferTo(0, inputChannel.size(), outputChannel);
			inputChannel.close();
			outputChannel.close();
		}
		catch(IOException e)
		{
			e.printStackTrace();
			return false;
		}
		
		return true;
	}

}