package com.cloudream.ishow.adapter;

import java.util.ArrayList;

import com.cloudream.ishow.R;
import com.cloudream.ishow.app.GalleryActivity;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.listener.SimpleImageLoadingListener;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.BaseAdapter;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

public class GalleryAdapter extends BaseAdapter
{
	private static final String TAG = GalleryAdapter.class.getSimpleName();
	
	public static class GalleryItem
	{
		public String path;
		public int count;
	}
	
	private ArrayList<GalleryItem> data = new ArrayList<GalleryItem>();
	private ImageLoader imageLoader;
	
	private boolean isActionPickMultiple;
	private final int width;
	
	private Context context;
	public GalleryAdapter(Context context, ImageLoader imageLoader)
	{
		this.context = context;
		this.imageLoader = imageLoader;
		
		WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
		Display display = wm.getDefaultDisplay();
		Point size = new Point();
		display.getSize(size);
		Log.i(TAG, "size: " + size.x + "x" + size.y);

		// make horizontal line 4 items
		width = Math.min(size.x, size.y) / 5;
//		params = new LinearLayout.LayoutParams(width, width);
		// clearCache();
	}

	@Override
	public int getCount()
	{
		return data.size();
	}

	@Override
	public GalleryItem getItem(int position)
	{
		return data.get(position);
	}

	@Override
	public long getItemId(int position)
	{
		return position;
	}

	public void setSelectMode(boolean multiple)
	{
		this.isActionPickMultiple = multiple;
	}

	private void selectAll(boolean selected)
	{
		final int count = selected?1:0;
		for(int i = 0, size = data.size(); i < size; ++i)
			data.get(i).count = count;

		notifyDataSetChanged();
	}

	public boolean isAllSelected()
	{
		boolean isAllSelected = true;

		for(int i = 0, size = data.size(); i < size; ++i)
		{
			if(data.get(i).count <= 0)
			{
				isAllSelected = false;
				break;
			}
		}

		return isAllSelected;
	}

	public boolean isAnySelected()
	{
		boolean isAnySelected = false;

		for(int i = 0, size = data.size(); i < size; ++i)
		{
			if(data.get(i).count > 0)
			{
				isAnySelected = true;
				break;
			}
		}

		return isAnySelected;
	}

	public int getSelectedItemsSum()
	{
		int count = 0;
		for(int i = 0, size = data.size(); i < size; ++i)
			count += data.get(i).count;
		
		return count;
	}
	
	public ArrayList<GalleryItem> getSelectedItems()
	{
		ArrayList<GalleryItem> result = new ArrayList<GalleryItem>();

		for(int i = 0, size = data.size(); i < size; ++i)
			if(data.get(i).count > 0)
				result.add(data.get(i));

		return result;
	}

	public void add(ArrayList<GalleryItem> files)
	{
		try
		{
			data.clear();
			data.addAll(files);

		}
		catch(Exception e)
		{
			e.printStackTrace();
		}

		notifyDataSetChanged();
	}

	public void changeSelection(View v, int position)
	{
		final GalleryItem item = data.get(position);
		final boolean previous_state = item.count > 0;
		item.count = previous_state ? 0:1;
	}

	public void add(View v, int position)
	{
		final GalleryItem item = data.get(position);
		++item.count;
	}
	
	public void remove(View v, int position)
	{
		final GalleryItem item = data.get(position);
		if(item.count > 1)
			--item.count;
		else
			data.remove(position);
	}
	
	@Override
	public View getView(int position, View convertView, ViewGroup parent)
	{
		if(convertView == null)
		{
			convertView = LayoutInflater.from(context).inflate(R.layout.gallery_item, parent, false);
			ViewGroup.LayoutParams params = convertView.findViewById(R.id.image).getLayoutParams();
			params.width = params.height = width;
		}
		
		final ImageView iv_image = (ImageView) convertView.findViewById(R.id.image);
		final String uri = "file://" + data.get(position).path;
		imageLoader.displayImage(uri, iv_image, new SimpleImageLoadingListener()
				{
					@Override
					public void onLoadingStarted(String imageUri, View view)
					{
						iv_image.setImageResource(R.drawable.picture_unavailable);
						super.onLoadingStarted(imageUri, view);
					}
				});

		return convertView;
	}

	public void clearCache()
	{
		imageLoader.clearDiscCache();
		imageLoader.clearMemoryCache();
	}

	public void clear()
	{
		data.clear();
		notifyDataSetChanged();
	}
}
