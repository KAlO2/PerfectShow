package com.cloudream.ishow.gpuimage.filter;

import com.cloudream.ishow.gpuimage.GPUImageFilterType;

/**
 * Applies a simple sepia effect.
 */
public class GPUImageSepiaFilter extends GPUImageColorMatrixFilter
{

	public GPUImageSepiaFilter()
	{
		this(1.0F);
	}

	public GPUImageSepiaFilter(final float intensity)
	{
		super(intensity, new float[]
		{
			0.3588F, 0.7044F, 0.1368F, 0.0F,
			0.2990F, 0.5870F, 0.1140F, 0.0F,
			0.2392F, 0.4696F, 0.0912F, 0.0F,
			0F, 0F, 0F, 1.0F
		});
		setFilterType(GPUImageFilterType.SEPIA);
	}
}
