package com.cloudream.ishow.app;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;

import com.cloudream.ishow.App;
import com.cloudream.ishow.R;
import com.cloudream.ishow.adapter.GalleryAdapter;
import com.cloudream.ishow.adapter.GalleryAdapter.GalleryItem;
import com.nostra13.universalimageloader.cache.disc.impl.UnlimitedDiscCache;
import com.nostra13.universalimageloader.cache.memory.impl.WeakMemoryCache;
import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.ImageLoaderConfiguration;
import com.nostra13.universalimageloader.core.assist.ImageScaleType;
import com.nostra13.universalimageloader.core.assist.PauseOnScrollListener;
import com.nostra13.universalimageloader.utils.StorageUtils;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class GalleryActivity extends Activity
{
	private static final String TAG = GalleryActivity.class.getSimpleName();
	
	// An advice from @{link https://developer.android.com/guide/components/intents-filters.html}
	// If you define your own actions, be sure to include your app's package name as a prefix.
	private static final String EXTRA_PREFIX = GalleryActivity.class.getName() + ".";
	/* package */ static final String ACTION_PICK_SINGLE = EXTRA_PREFIX + "PICK_SINGLE";
	/* package */ static final String ACTION_PICK_MULTIPLE = EXTRA_PREFIX + "PICK_MULTIPLE";
	
	/* package */ static final String EXTRA_PICTURE_PATH = EXTRA_PREFIX + "PICTURE_PATH";
	/* package */ static final String EXTRA_PICTURE_BITMAP = EXTRA_PREFIX + "PICTURE_BITMAP";
	/* package */ static final String EXTRA_TEMPORARY = EXTRA_PREFIX + "TEMPORARY";
	/* package */ static final String EXTRA_CLASS = EXTRA_PREFIX + "CLASS";
	/* package */ static final String EXTRA_PICTURE_MAX = EXTRA_PREFIX + "PICTURE_MAX";
	
	private static final String TYPE_IMAGE = "image/*";
	
	private int picture_max = 9;
	
	GridView gv_gallery;
	GalleryAdapter adapter;

	private ImageView iv_picture_unavailable;

	private LinearLayout ll_thumbnails;
	private TextView tv_picture_selection_info;
	
	private ImageLoader imageLoader;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.gallery_activity);

		((TextView)findViewById(R.id.title)).setText(R.string.select_picture);
		ll_thumbnails = (LinearLayout)findViewById(R.id.thumbnails);

		Intent intent = getIntent();
		picture_max = intent.getIntExtra(EXTRA_PICTURE_MAX, 9);
		
		final String action = intent.getAction();
		if(action == null)
			finish();

		initImageLoader();
		init(action);
	}

	private void initImageLoader()
	{
		final String CACHE_DIR = App.getWorkingDirectory() + File.separator + ".temp";
		File file = new File(CACHE_DIR);
		if(!file.exists())
			file.mkdirs();
		
		try
		{
			File cacheDir = StorageUtils.getOwnCacheDirectory(getBaseContext(), CACHE_DIR);

			DisplayImageOptions defaultOptions = new DisplayImageOptions.Builder().cacheOnDisc(true)
					.imageScaleType(ImageScaleType.EXACTLY).bitmapConfig(Bitmap.Config.RGB_565).build();
			ImageLoaderConfiguration.Builder builder = new ImageLoaderConfiguration.Builder(getBaseContext())
					.defaultDisplayImageOptions(defaultOptions).discCache(new UnlimitedDiscCache(cacheDir))
					.memoryCache(new WeakMemoryCache());

			ImageLoaderConfiguration config = builder.build();
			imageLoader = ImageLoader.getInstance();
			imageLoader.init(config);

		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	}
	
	private void init(String action)
	{
		gv_gallery = (GridView) findViewById(R.id.gallery);
		gv_gallery.setFastScrollEnabled(true);
		adapter = new GalleryAdapter(this, imageLoader);
		PauseOnScrollListener listener = new PauseOnScrollListener(imageLoader, true, true);
		gv_gallery.setOnScrollListener(listener);

		if(action.equals(ACTION_PICK_SINGLE))
		{
			findViewById(R.id.container).setVisibility(View.GONE);
			gv_gallery.setOnItemClickListener(mItemSingleClickListener);
			adapter.setSelectMode(false);
			if(picture_max > 1)
			{
				picture_max = 1;
				Log.w(TAG, "ACTION_PICK_SINGLE mismatch with picture number " + picture_max);
			}
		}

		gv_gallery.setAdapter(adapter);
		iv_picture_unavailable = (ImageView) findViewById(R.id.picture_unavailable);
		
		new Runnable()
		{
			@Override
			public void run()
			{
				adapter.add(getGalleryPhotos());
				iv_picture_unavailable.setVisibility(adapter.isEmpty() ? View.VISIBLE:View.GONE);
			}
		}.run();

	}

	private final String[] getSelectedPath()
	{
		int count = 0;
		final ArrayList<GalleryAdapter.GalleryItem> items = adapter.getSelectedItems();
		for(GalleryAdapter.GalleryItem item: items)
			count += item.count;
		
		final String[] paths = new String[count];
		int index = 0;
		for(GalleryAdapter.GalleryItem item: items)
			for(int j = 0; j < item.count; ++index, ++j)
				paths[index] = item.path;
		
		return paths;
	}

	private final AdapterView.OnItemClickListener mItemSingleClickListener = new AdapterView.OnItemClickListener()
	{

		@Override
		public void onItemClick(AdapterView<?> l, View v, int position, long id)
		{
			GalleryAdapter.GalleryItem item = adapter.getItem(position);
			Intent intent = new Intent();
			intent.putExtra(EXTRA_PICTURE_PATH, item.path);
			setResult(RESULT_OK, intent);
			finish();
		}
	};
	
	private ArrayList<GalleryAdapter.GalleryItem> getGalleryPhotos()
	{
		ArrayList<GalleryAdapter.GalleryItem> galleryList = new ArrayList<GalleryAdapter.GalleryItem>();

		try
		{
			final String[] columns =
			{ MediaStore.Images.Media.DATA, MediaStore.Images.Media._ID };
			final String orderBy = MediaStore.Images.Media._ID;

			Cursor imagecursor = managedQuery(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, columns, null, null, orderBy);
			if(imagecursor != null && imagecursor.getCount() > 0)
			{

				while(imagecursor.moveToNext())
				{
					GalleryAdapter.GalleryItem item = new GalleryAdapter.GalleryItem();
					int dataColumnIndex = imagecursor.getColumnIndex(MediaStore.Images.Media.DATA);

					item.path = imagecursor.getString(dataColumnIndex);
					galleryList.add(item);
				}
			}
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}

		// show newest photo at beginning of the list
		Collections.reverse(galleryList);
		return galleryList;
	}

	public static Intent startGalleryActivity(Context context, int picture_count)
	{
		if(picture_count <= 0)
			throw new IllegalArgumentException("picture_count must be positive");
		
		Intent intent = new Intent(context, GalleryActivity.class);
		intent.setType(GalleryActivity.TYPE_IMAGE);
		
		if(picture_count < 2)
		{
			intent.setAction(GalleryActivity.ACTION_PICK_SINGLE);
			intent.putExtra(EXTRA_PICTURE_MAX, 1);
		}
		else
		{
			intent.setAction(GalleryActivity.ACTION_PICK_MULTIPLE);
			intent.putExtra(EXTRA_PICTURE_MAX, picture_count);
		}
		
		return intent;
	}
}
