package com.cloudream.ishow.gpuimage;


import com.cloudream.ishow.gpuimage.filter.GPUImage3x3TextureSamplingFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageBilateralFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageBrightnessFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageBulgeDistortionFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageColorBalanceFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageContrastFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageCrosshatchFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageDissolveBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageEmbossFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageExposureFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageGammaFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageGaussianBlurFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageGlassSphereFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageHazeFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageHighlightShadowFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageHueFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageLevelsFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageMonochromeFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageOpacityFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImagePixelationFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImagePosterizeFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageRGBFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSaturationFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSepiaFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSharpenFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSobelEdgeDetection;
import com.cloudream.ishow.gpuimage.filter.GPUImageSphereRefractionFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSwirlFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageTransformFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageVignetteFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageWhiteBalanceFilter;

import android.opengl.Matrix;

public class GPUImageFilterAdjuster
{
	private final Adjuster<? extends GPUImageFilter> adjuster;

	public GPUImageFilterAdjuster(final GPUImageFilter filter)
	{
		GPUImageFilterType type = filter.getFilterType();
		switch(type)
		{
		case SHARPEN:        adjuster = new SharpnessAdjuster().filter(filter);    break;
		case SEPIA:          adjuster = new SepiaAdjuster().filter(filter);        break;
		case CONTRAST:       adjuster = new ContrastAdjuster().filter(filter);     break;
		case GAMMA:          adjuster = new GammaAdjuster().filter(filter);        break;
		case BRIGHTNESS:     adjuster = new BrightnessAdjuster().filter(filter);   break;
		case SOBEL_EDGE_DETECTION: adjuster = new SobelAdjuster().filter(filter);  break;
		case EMBOSS:         adjuster = new EmbossAdjuster().filter(filter);       break;
		case THREE_X_THREE_CONVOLUTION: adjuster = new GPU3x3TextureAdjuster().filter(filter); break;
		case HUE:            adjuster = new HueAdjuster().filter(filter);          break;
		case POSTERIZE:      adjuster = new PosterizeAdjuster().filter(filter);    break;
		case PIXELATION:     adjuster = new PixelationAdjuster().filter(filter);   break;
		case SATURATION:     adjuster = new SaturationAdjuster().filter(filter);   break;
		case EXPOSURE:       adjuster = new ExposureAdjuster().filter(filter);     break;
		case HIGHLIGHT_SHADOW: adjuster = new HighlightShadowAdjuster().filter(filter); break;
		case MONOCHROME:     adjuster = new MonochromeAdjuster().filter(filter);   break;
		case OPACITY:        adjuster = new OpacityAdjuster().filter(filter);      break;
		case RGB:            adjuster = new RGBAdjuster().filter(filter);          break;
		case WHITE_BALANCE:  adjuster = new WhiteBalanceAdjuster().filter(filter); break;
		case VIGNETTE:       adjuster = new VignetteAdjuster().filter(filter);     break;
		case TONE_CURVE:     adjuster = new DissolveBlendAdjuster().filter(filter);break;
		case GAUSSIAN_BLUR:  adjuster = new GaussianBlurAdjuster().filter(filter); break;
		case CROSSHATCH:     adjuster = new CrosshatchBlurAdjuster().filter(filter); break;
		case BULGE_DISTORTION: adjuster = new BulgeDistortionAdjuster().filter(filter); break;
		case GLASS_SPHERE:   adjuster = new GlassSphereAdjuster().filter(filter);  break;
		case HAZE:           adjuster = new HazeAdjuster().filter(filter);         break;
		case LAPLACIAN:      adjuster = new SphereRefractionAdjuster().filter(filter); break;
		case SWIRL:          adjuster = new SwirlAdjuster().filter(filter);        break;
		case COLOR_BALANCE:  adjuster = new ColorBalanceAdjuster().filter(filter); break;
		case LEVELS_FILTER_MIN: adjuster = new LevelsMinMidAdjuster().filter(filter); break;
		case BILATERAL:      adjuster = new BilateralAdjuster().filter(filter);    break;
		case TRANSFORM2D:    adjuster = new RotateAdjuster().filter(filter);       break;
		default:             adjuster = null;
		}
	}

	public boolean canAdjust()
	{
		return adjuster != null;
	}

	public void adjust(final int percentage)
	{
		if(adjuster != null)
		{
			adjuster.adjust(percentage);
		}
	}

	private abstract class Adjuster<T extends GPUImageFilter>
	{
		private T filter;

		@SuppressWarnings("unchecked")
		public Adjuster<T> filter(final GPUImageFilter filter)
		{
			this.filter = (T) filter;
			return this;
		}

		public T getFilter()
		{
			return filter;
		}

		public abstract void adjust(int percentage);

		protected float range(final int percentage, final float start, final float end)
		{
			return (end - start) * percentage / 100.0F + start;
		}

		protected int range(final int percentage, final int start, final int end)
		{
			return (end - start) * percentage / 100 + start;
		}
	}

	private class SharpnessAdjuster extends Adjuster<GPUImageSharpenFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setSharpness(range(percentage, -4.0F, 4.0F));
		}
	}

	private class PixelationAdjuster extends Adjuster<GPUImagePixelationFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setPixel(range(percentage, 1.0F, 100.0F));
		}
	}

	private class HueAdjuster extends Adjuster<GPUImageHueFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setHue(range(percentage, 0.0F, 360.0F));
		}
	}

	private class ContrastAdjuster extends Adjuster<GPUImageContrastFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setContrast(range(percentage, 0.0F, 2.0F));
		}
	}

	private class GammaAdjuster extends Adjuster<GPUImageGammaFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setGamma(range(percentage, 0.0F, 3.0F));
		}
	}

	private class BrightnessAdjuster extends Adjuster<GPUImageBrightnessFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setBrightness(range(percentage, -1.0F, 1.0F));
		}
	}

	private class SepiaAdjuster extends Adjuster<GPUImageSepiaFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setIntensity(range(percentage, 0.0F, 2.0F));
		}
	}

	private class SobelAdjuster extends Adjuster<GPUImageSobelEdgeDetection>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setLineSize(range(percentage, 0.0F, 5.0F));
		}
	}

	private class EmbossAdjuster extends Adjuster<GPUImageEmbossFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setIntensity(range(percentage, 0.0F, 4.0F));
		}
	}

	private class PosterizeAdjuster extends Adjuster<GPUImagePosterizeFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			// In theories to 256, but only first 50 are interesting
			getFilter().setColorLevels(range(percentage, 1, 50));
		}
	}

	private class GPU3x3TextureAdjuster extends Adjuster<GPUImage3x3TextureSamplingFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setLineSize(range(percentage, 0.0F, 5.0F));
		}
	}

	private class SaturationAdjuster extends Adjuster<GPUImageSaturationFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setSaturation(range(percentage, 0.0F, 2.0F));
		}
	}

	private class ExposureAdjuster extends Adjuster<GPUImageExposureFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
//			getFilter().setExposure(range(percentage, -10.0F, 10.0F));
		}
	}

	private class HighlightShadowAdjuster extends Adjuster<GPUImageHighlightShadowFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setShadows(range(percentage, 0.0F, 1.0F));
			getFilter().setHighlights(range(percentage, 0.0F, 1.0F));
		}
	}

	private class MonochromeAdjuster extends Adjuster<GPUImageMonochromeFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setIntensity(range(percentage, 0.0F, 1.0F));
//			getFilter().setColor(new float[]{0.6F, 0.45F, 0.3F, 1.0F});
		}
	}

	private class OpacityAdjuster extends Adjuster<GPUImageOpacityFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setOpacity(range(percentage, 0.0F, 1.0F));
		}
	}

	private class RGBAdjuster extends Adjuster<GPUImageRGBFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setRed(range(percentage, 0.0F, 1.0F));
//			getFilter().setGreen(range(percentage, 0.0F, 1.0F));
//			getFilter().setBlue(range(percentage, 0.0F, 1.0F));
		}
	}

	private class WhiteBalanceAdjuster extends Adjuster<GPUImageWhiteBalanceFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setTemperature(range(percentage, 2000.0F, 8000.0F));
			// getFilter().setTint(range(percentage, -100.0f, 100.0f));
		}
	}

	private class VignetteAdjuster extends Adjuster<GPUImageVignetteFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setVignetteStart(range(percentage, 0.0F, 1.0F));
		}
	}

	private class DissolveBlendAdjuster extends Adjuster<GPUImageDissolveBlendFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setMix(range(percentage, 0.0F, 1.0F));
		}
	}

	private class GaussianBlurAdjuster extends Adjuster<GPUImageGaussianBlurFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setBlurSize(range(percentage, 0.0F, 1.0F));
		}
	}

	private class CrosshatchBlurAdjuster extends Adjuster<GPUImageCrosshatchFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setCrossHatchSpacing(range(percentage, 0.0F, 0.06F));
			getFilter().setLineWidth(range(percentage, 0.0F, 0.006F));
		}
	}

	private class BulgeDistortionAdjuster extends Adjuster<GPUImageBulgeDistortionFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setRadius(range(percentage, 0.0F, 1.0F));
			getFilter().setScale(range(percentage, -1.0F, 1.0F));
		}
	}

	private class GlassSphereAdjuster extends Adjuster<GPUImageGlassSphereFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setRadius(range(percentage, 0.0F, 1.0F));
		}
	}

	private class HazeAdjuster extends Adjuster<GPUImageHazeFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setDistance(range(percentage, -0.3F, 0.3F));
			getFilter().setSlope(range(percentage, -0.3F, 0.3F));
		}
	}

	private class SphereRefractionAdjuster extends Adjuster<GPUImageSphereRefractionFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setRadius(range(percentage, 0.0F, 1.0F));
		}
	}

	private class SwirlAdjuster extends Adjuster<GPUImageSwirlFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setAngle(range(percentage, 0.0F, 2.0F));
		}
	}

	private class ColorBalanceAdjuster extends Adjuster<GPUImageColorBalanceFilter>
	{

		@Override
		public void adjust(int percentage)
		{
			getFilter().setMidtones(new float[]
			{
				range(percentage,     0.0F, 1.0F),
				range(percentage / 2, 0.0F, 1.0F),
				range(percentage / 3, 0.0F, 1.0F)
			});
		}
	}

	private class LevelsMinMidAdjuster extends Adjuster<GPUImageLevelsFilter>
	{
		@Override
		public void adjust(int percentage)
		{
			getFilter().setMin(0.0F, range(percentage, 0.0F, 1.0F), 1.0F);
		}
	}

	private class BilateralAdjuster extends Adjuster<GPUImageBilateralFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			getFilter().setDistanceNormalizationFactor(range(percentage, 0.0F, 15.0F));
		}
	}

	private class RotateAdjuster extends Adjuster<GPUImageTransformFilter>
	{
		@Override
		public void adjust(final int percentage)
		{
			float[] transform = new float[16];
			Matrix.setRotateM(transform, 0, 360 * percentage / 100, 0, 0, 1.0F);
			getFilter().setTransform3D(transform);
		}
	}
	

}
