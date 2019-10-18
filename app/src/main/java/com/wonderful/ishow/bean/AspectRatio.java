package com.wonderful.ishow.bean;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.Nullable;


public class AspectRatio implements Parcelable {
	/**
	 * Use source image aspect ratio as default.
	 */
	public static final int DEFAULT_ASPECT_RATIO = 0;
	
	@Nullable
	private String mAspectRatioTitle;
	private int mAspectRatioX;
	private int mAspectRatioY;

	public AspectRatio(@Nullable String aspectRatioTitle, int aspectRatioX, int aspectRatioY) {
		mAspectRatioTitle = aspectRatioTitle;
		mAspectRatioX = aspectRatioX;
		mAspectRatioY = aspectRatioY;
	}

	protected AspectRatio(Parcel in) {
		mAspectRatioTitle = in.readString();
		mAspectRatioX = in.readInt();
		mAspectRatioY = in.readInt();
	}

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeString(mAspectRatioTitle);
		dest.writeInt(mAspectRatioX);
		dest.writeInt(mAspectRatioY);
	}

	@Override
	public int describeContents() {
		return 0;
	}

	public static final Creator<AspectRatio> CREATOR = new Creator<AspectRatio>() {
		@Override
		public AspectRatio createFromParcel(Parcel in) {
			return new AspectRatio(in);
		}

		@Override
		public AspectRatio[] newArray(int size) {
			return new AspectRatio[size];
		}
	};

	public void setmAspectRatioTitle(String title) {
		mAspectRatioTitle = title;
	}
	
	@Nullable
	public String getAspectRatioTitle() {
		return mAspectRatioTitle;
	}

	public int getAspectRatioX() {
		return mAspectRatioX;
	}

	public int getAspectRatioY() {
		return mAspectRatioY;
	}

	public float getAspectRatio() {
		if(mAspectRatioX == DEFAULT_ASPECT_RATIO || mAspectRatioY == DEFAULT_ASPECT_RATIO)
			return DEFAULT_ASPECT_RATIO;
		else
			return mAspectRatioX / mAspectRatioY;
	}
	
	public synchronized void switchOrientation() {
		if(mAspectRatioX == DEFAULT_ASPECT_RATIO || mAspectRatioY == DEFAULT_ASPECT_RATIO)
			return;
		
		int tmp = mAspectRatioX;
		mAspectRatioX = mAspectRatioY;
		mAspectRatioY = tmp;
	}
}
