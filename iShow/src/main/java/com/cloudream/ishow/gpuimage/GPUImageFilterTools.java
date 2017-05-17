package com.cloudream.ishow.gpuimage;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.BitmapFactory;
import android.graphics.PointF;
import java.util.LinkedList;
import java.util.List;

import com.cloudream.ishow.R;
import com.cloudream.ishow.gpuimage.filter.GPUImage3x3ConvolutionFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImage3x3TextureSamplingFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageAddBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageAlphaBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageBilateralFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageBoxBlurFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageBrightnessFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageBulgeDistortionFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageCGAColorspaceFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageChromaKeyBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageColorBalanceFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageColorBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageColorBurnBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageColorDodgeBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageColorInvertFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageContrastFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageCrosshatchFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageDarkenBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageDifferenceBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageDilationFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageDirectionalSobelEdgeDetectionFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageDissolveBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageDivideBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageEmbossFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageExclusionBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageExposureFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageFalseColorFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageGammaFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageGaussianBlurFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageGlassSphereFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageGrayscaleFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageHalftoneFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageHardLightBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageHazeFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageHighlightShadowFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageHueBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageHueFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageKuwaharaFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageLaplacianFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageLevelsFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageLightenBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageLinearBurnBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageLookupFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageLuminosityBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageMonochromeFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageMultiplyBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageNonMaximumSuppressionFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageNormalBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageOpacityFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageOverlayBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImagePixelationFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImagePosterizeFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageRGBDilationFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageRGBFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSaturationBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSaturationFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageScreenBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSepiaFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSharpenFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSketchFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSmoothToonFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSobelEdgeDetection;
import com.cloudream.ishow.gpuimage.filter.GPUImageSoftLightBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSourceOverBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSphereRefractionFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSubtractBlendFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageSwirlFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageToneCurveFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageToonFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageTransformFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageVignetteFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageWeakPixelInclusionFilter;
import com.cloudream.ishow.gpuimage.filter.GPUImageWhiteBalanceFilter;

public class GPUImageFilterTools
{
	public static void showDialog(final Context context, final OnGpuImageFilterChosenListener listener)
	{
		final FilterList filters = new FilterList();
		filters.addFilter("Contrast", GPUImageFilterType.CONTRAST);
		filters.addFilter("Invert", GPUImageFilterType.COLOR_INVERT);
		filters.addFilter("Pixelation", GPUImageFilterType.PIXELATION);
		filters.addFilter("Hue", GPUImageFilterType.HUE);
		filters.addFilter("Gamma", GPUImageFilterType.GAMMA);
		filters.addFilter("Brightness", GPUImageFilterType.BRIGHTNESS);
		filters.addFilter("Sepia", GPUImageFilterType.SEPIA);
		filters.addFilter("Grayscale", GPUImageFilterType.GRAYSCALE);
		filters.addFilter("Sharpness", GPUImageFilterType.SHARPEN);
		filters.addFilter("Sobel Edge Detection", GPUImageFilterType.SOBEL_EDGE_DETECTION);
		filters.addFilter("3x3 Convolution", GPUImageFilterType.THREE_X_THREE_CONVOLUTION);
		filters.addFilter("Emboss", GPUImageFilterType.EMBOSS);
		filters.addFilter("Posterize", GPUImageFilterType.POSTERIZE);
		filters.addFilter("Grouped filters", GPUImageFilterType.FILTER_GROUP);
		filters.addFilter("Saturation", GPUImageFilterType.SATURATION);
		filters.addFilter("Exposure", GPUImageFilterType.EXPOSURE);
		filters.addFilter("Highlight Shadow", GPUImageFilterType.HIGHLIGHT_SHADOW);
		filters.addFilter("Monochrome", GPUImageFilterType.MONOCHROME);
		filters.addFilter("Opacity", GPUImageFilterType.OPACITY);
		filters.addFilter("RGB", GPUImageFilterType.RGB);
		filters.addFilter("White Balance", GPUImageFilterType.WHITE_BALANCE);
		filters.addFilter("Vignette", GPUImageFilterType.VIGNETTE);
		filters.addFilter("ToneCurve", GPUImageFilterType.TONE_CURVE);

		filters.addFilter("Blend (Difference)", GPUImageFilterType.BLEND_DIFFERENCE);
		filters.addFilter("Blend (Source Over)", GPUImageFilterType.BLEND_SOURCE_OVER);
		filters.addFilter("Blend (Color Burn)", GPUImageFilterType.BLEND_COLOR_BURN);
		filters.addFilter("Blend (Color Dodge)", GPUImageFilterType.BLEND_COLOR_DODGE);
		filters.addFilter("Blend (Darken)", GPUImageFilterType.BLEND_DARKEN);
		filters.addFilter("Blend (Dissolve)", GPUImageFilterType.BLEND_DISSOLVE);
		filters.addFilter("Blend (Exclusion)", GPUImageFilterType.BLEND_EXCLUSION);
		filters.addFilter("Blend (Hard Light)", GPUImageFilterType.BLEND_HARD_LIGHT);
		filters.addFilter("Blend (Lighten)", GPUImageFilterType.BLEND_LIGHTEN);
		filters.addFilter("Blend (Add)", GPUImageFilterType.BLEND_ADD);
		filters.addFilter("Blend (Divide)", GPUImageFilterType.BLEND_DIVIDE);
		filters.addFilter("Blend (Multiply)", GPUImageFilterType.BLEND_MULTIPLY);
		filters.addFilter("Blend (Overlay)", GPUImageFilterType.BLEND_OVERLAY);
		filters.addFilter("Blend (Screen)", GPUImageFilterType.BLEND_SCREEN);
		filters.addFilter("Blend (Alpha)", GPUImageFilterType.BLEND_ALPHA);
		filters.addFilter("Blend (Color)", GPUImageFilterType.BLEND_COLOR);
		filters.addFilter("Blend (Hue)", GPUImageFilterType.BLEND_HUE);
		filters.addFilter("Blend (Saturation)", GPUImageFilterType.BLEND_SATURATION);
		filters.addFilter("Blend (Luminosity)", GPUImageFilterType.BLEND_LUMINOSITY);
		filters.addFilter("Blend (Linear Burn)", GPUImageFilterType.BLEND_LINEAR_BURN);
		filters.addFilter("Blend (Soft Light)", GPUImageFilterType.BLEND_SOFT_LIGHT);
		filters.addFilter("Blend (Subtract)", GPUImageFilterType.BLEND_SUBTRACT);
		filters.addFilter("Blend (Chroma Key)", GPUImageFilterType.BLEND_CHROMA_KEY);
		filters.addFilter("Blend (Normal)", GPUImageFilterType.BLEND_NORMAL);

		filters.addFilter("Lookup (Amatorka)", GPUImageFilterType.LOOKUP_AMATORKA);
		filters.addFilter("Gaussian Blur", GPUImageFilterType.GAUSSIAN_BLUR);
		filters.addFilter("Crosshatch", GPUImageFilterType.CROSSHATCH);

		filters.addFilter("Box Blur", GPUImageFilterType.BOX_BLUR);
		filters.addFilter("CGA Color Space", GPUImageFilterType.CGA_COLORSPACE);
		filters.addFilter("Dilation", GPUImageFilterType.DILATION);
		filters.addFilter("Kuwahara", GPUImageFilterType.KUWAHARA);
		filters.addFilter("RGB Dilation", GPUImageFilterType.RGB_DILATION);
		filters.addFilter("Sketch", GPUImageFilterType.SKETCH);
		filters.addFilter("Toon", GPUImageFilterType.TOON);
		filters.addFilter("Smooth Toon", GPUImageFilterType.SMOOTH_TOON);
		filters.addFilter("Halftone", GPUImageFilterType.HALFTONE);

		filters.addFilter("Bulge Distortion", GPUImageFilterType.BULGE_DISTORTION);
		filters.addFilter("Glass Sphere", GPUImageFilterType.GLASS_SPHERE);
		filters.addFilter("Haze", GPUImageFilterType.HAZE);
		filters.addFilter("Laplacian", GPUImageFilterType.LAPLACIAN);
		filters.addFilter("Non Maximum Suppression", GPUImageFilterType.NON_MAXIMUM_SUPPRESSION);
		filters.addFilter("Sphere Refraction", GPUImageFilterType.SPHERE_REFRACTION);
		filters.addFilter("Swirl", GPUImageFilterType.SWIRL);
		filters.addFilter("Weak Pixel Inclusion", GPUImageFilterType.WEAK_PIXEL_INCLUSION);
		filters.addFilter("False Color", GPUImageFilterType.FALSE_COLOR);

		filters.addFilter("Color Balance", GPUImageFilterType.COLOR_BALANCE);

		filters.addFilter("Levels Min (Mid Adjust)", GPUImageFilterType.LEVELS_FILTER_MIN);

		filters.addFilter("Bilateral Blur", GPUImageFilterType.BILATERAL);

		filters.addFilter("Transform (2-D)", GPUImageFilterType.TRANSFORM2D);

		AlertDialog.Builder builder = new AlertDialog.Builder(context);
		builder.setTitle("Choose a filter");
		builder.setItems(filters.names.toArray(new String[filters.names.size()]), new DialogInterface.OnClickListener()
		{
			@Override
			public void onClick(final DialogInterface dialog, final int item)
			{
				listener.onGpuImageFilterChosenListener(createFilterForType(context, filters.filters.get(item)));
			}
		});
		builder.create().show();
	}

	private static GPUImageFilter createFilterForType(final Context context, final GPUImageFilterType type)
	{
		switch(type)
		{
		case CONTRAST:
			return new GPUImageContrastFilter(2.0F);
		case GAMMA:
			return new GPUImageGammaFilter(2.0F);
		case COLOR_INVERT:
			return new GPUImageColorInvertFilter();
		case PIXELATION:
			return new GPUImagePixelationFilter();
		case HUE:
			return new GPUImageHueFilter(90.0F);
		case BRIGHTNESS:
			return new GPUImageBrightnessFilter();
		case GRAYSCALE:
			return new GPUImageGrayscaleFilter();
		case SEPIA:
			return new GPUImageSepiaFilter();
		case SHARPEN:
			GPUImageSharpenFilter sharpness = new GPUImageSharpenFilter();
			sharpness.setSharpness(2.0F);
			return sharpness;
		case SOBEL_EDGE_DETECTION:
			return new GPUImageSobelEdgeDetection();
		case THREE_X_THREE_CONVOLUTION:
			GPUImage3x3ConvolutionFilter convolution = new GPUImage3x3ConvolutionFilter();
			convolution.setConvolutionKernel(new float[]
			{ -1.0F, 0.0F, 1.0F, -2.0F, 0.0F, 2.0F, -1.0F, 0.0F, 1.0F });
			/*
			 * [-1.0F, 0.0F, 1.0F] [1.0F] [-2.0F, 0.0F, 2.0F] = [2.0F] * [-1.0F, 0.0F, 1.0F] [-1.0F,
			 * 0.0F, 1.0F] [1.0F]
			 */
			return convolution;
		case EMBOSS:
			return new GPUImageEmbossFilter();
		case POSTERIZE:
			return new GPUImagePosterizeFilter();
		case FILTER_GROUP:
			List<GPUImageFilter> filters = new LinkedList<GPUImageFilter>();
			filters.add(new GPUImageContrastFilter());
			filters.add(new GPUImageDirectionalSobelEdgeDetectionFilter());
			filters.add(new GPUImageGrayscaleFilter());
			return new GPUImageFilterGroup(filters);
		case SATURATION:
			return new GPUImageSaturationFilter(1.0F);
		case EXPOSURE:
			return new GPUImageExposureFilter(0.0F);
		case HIGHLIGHT_SHADOW:
			return new GPUImageHighlightShadowFilter(0.0F, 1.0F);
		case MONOCHROME:
			return new GPUImageMonochromeFilter(1.0f, new float[]{ 0.6F, 0.45F, 0.3F, 1.0F });
		case OPACITY:
			return new GPUImageOpacityFilter(1.0F);
		case RGB:
			return new GPUImageRGBFilter(1.0F, 1.0F, 1.0F);
		case WHITE_BALANCE:
			return new GPUImageWhiteBalanceFilter(5000.0F, 0.0F);
		case VIGNETTE:
			PointF centerPoint = new PointF();
			centerPoint.x = 0.5F;
			centerPoint.y = 0.5F;
			return new GPUImageVignetteFilter(centerPoint, new float[]{ 0.0F, 0.0F, 0.0F }, 0.3F, 0.75F);
		case TONE_CURVE:
			GPUImageToneCurveFilter toneCurveFilter = new GPUImageToneCurveFilter();
			toneCurveFilter
					.setFromCurveFileInputStream(context.getResources().openRawResource(R.raw.tone_cuver_sample));
			return toneCurveFilter;
		case BLEND_DIFFERENCE:
			return createBlendFilter(context, GPUImageDifferenceBlendFilter.class);
		case BLEND_SOURCE_OVER:
			return createBlendFilter(context, GPUImageSourceOverBlendFilter.class);
		case BLEND_COLOR_BURN:
			return createBlendFilter(context, GPUImageColorBurnBlendFilter.class);
		case BLEND_COLOR_DODGE:
			return createBlendFilter(context, GPUImageColorDodgeBlendFilter.class);
		case BLEND_DARKEN:
			return createBlendFilter(context, GPUImageDarkenBlendFilter.class);
		case BLEND_DISSOLVE:
			return createBlendFilter(context, GPUImageDissolveBlendFilter.class);
		case BLEND_EXCLUSION:
			return createBlendFilter(context, GPUImageExclusionBlendFilter.class);

		case BLEND_HARD_LIGHT:
			return createBlendFilter(context, GPUImageHardLightBlendFilter.class);
		case BLEND_LIGHTEN:
			return createBlendFilter(context, GPUImageLightenBlendFilter.class);
		case BLEND_ADD:
			return createBlendFilter(context, GPUImageAddBlendFilter.class);
		case BLEND_DIVIDE:
			return createBlendFilter(context, GPUImageDivideBlendFilter.class);
		case BLEND_MULTIPLY:
			return createBlendFilter(context, GPUImageMultiplyBlendFilter.class);
		case BLEND_OVERLAY:
			return createBlendFilter(context, GPUImageOverlayBlendFilter.class);
		case BLEND_SCREEN:
			return createBlendFilter(context, GPUImageScreenBlendFilter.class);
		case BLEND_ALPHA:
			return createBlendFilter(context, GPUImageAlphaBlendFilter.class);
		case BLEND_COLOR:
			return createBlendFilter(context, GPUImageColorBlendFilter.class);
		case BLEND_HUE:
			return createBlendFilter(context, GPUImageHueBlendFilter.class);
		case BLEND_SATURATION:
			return createBlendFilter(context, GPUImageSaturationBlendFilter.class);
		case BLEND_LUMINOSITY:
			return createBlendFilter(context, GPUImageLuminosityBlendFilter.class);
		case BLEND_LINEAR_BURN:
			return createBlendFilter(context, GPUImageLinearBurnBlendFilter.class);
		case BLEND_SOFT_LIGHT:
			return createBlendFilter(context, GPUImageSoftLightBlendFilter.class);
		case BLEND_SUBTRACT:
			return createBlendFilter(context, GPUImageSubtractBlendFilter.class);
		case BLEND_CHROMA_KEY:
			return createBlendFilter(context, GPUImageChromaKeyBlendFilter.class);
		case BLEND_NORMAL:
			return createBlendFilter(context, GPUImageNormalBlendFilter.class);

		case LOOKUP_AMATORKA:
			GPUImageLookupFilter amatorka = new GPUImageLookupFilter();
			amatorka.setBitmap(BitmapFactory.decodeResource(context.getResources(), R.drawable.lookup_amatorka));
			return amatorka;
		case GAUSSIAN_BLUR:
			return new GPUImageGaussianBlurFilter();
		case CROSSHATCH:
			return new GPUImageCrosshatchFilter();

		case BOX_BLUR:
			return new GPUImageBoxBlurFilter();
		case CGA_COLORSPACE:
			return new GPUImageCGAColorspaceFilter();
		case DILATION:
			return new GPUImageDilationFilter();
		case KUWAHARA:
			return new GPUImageKuwaharaFilter();
		case RGB_DILATION:
			return new GPUImageRGBDilationFilter();
		case SKETCH:
			return new GPUImageSketchFilter();
		case TOON:
			return new GPUImageToonFilter();
		case SMOOTH_TOON:
			return new GPUImageSmoothToonFilter();

		case BULGE_DISTORTION:
			return new GPUImageBulgeDistortionFilter();
		case GLASS_SPHERE:
			return new GPUImageGlassSphereFilter();
		case HAZE:
			return new GPUImageHazeFilter();
		case LAPLACIAN:
			return new GPUImageLaplacianFilter();
		case NON_MAXIMUM_SUPPRESSION:
			return new GPUImageNonMaximumSuppressionFilter();
		case SPHERE_REFRACTION:
			return new GPUImageSphereRefractionFilter();
		case SWIRL:
			return new GPUImageSwirlFilter();
		case WEAK_PIXEL_INCLUSION:
			return new GPUImageWeakPixelInclusionFilter();
		case FALSE_COLOR:
			return new GPUImageFalseColorFilter();
		case COLOR_BALANCE:
			return new GPUImageColorBalanceFilter();
		case LEVELS_FILTER_MIN:
			GPUImageLevelsFilter levelsFilter = new GPUImageLevelsFilter();
			levelsFilter.setMin(0.0f, 3.0f, 1.0f);
			return levelsFilter;
		case HALFTONE:
			return new GPUImageHalftoneFilter();

		case BILATERAL:
			return new GPUImageBilateralFilter();

		case TRANSFORM2D:
			return new GPUImageTransformFilter();

		default:
			throw new IllegalStateException("No filter of that type!");
		}

	}

	private static GPUImageFilter createBlendFilter(Context context,
			Class<? extends GPUImageTwoInputFilter> filterClass)
	{
		try
		{
			GPUImageTwoInputFilter filter = filterClass.newInstance();
			filter.setBitmap(BitmapFactory.decodeResource(context.getResources(), R.drawable.ic_launcher));
			return filter;
		}
		catch(Exception e)
		{
			e.printStackTrace();
			return null;
		}
	}

	public interface OnGpuImageFilterChosenListener
	{
		void onGpuImageFilterChosenListener(GPUImageFilter filter);
	}

	private static class FilterList
	{
		public List<String> names = new LinkedList<String>();
		public List<GPUImageFilterType> filters = new LinkedList<GPUImageFilterType>();

		public void addFilter(final String name, final GPUImageFilterType filter)
		{
			names.add(name);
			filters.add(filter);
		}
	}

	}
