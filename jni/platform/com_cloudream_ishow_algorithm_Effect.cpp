#include "com_cloudream_ishow_algorithm_Effect.h"
#include "venus/region_operation.h"
#include "venus/scalar.h"

#define LOG_TAG "Effect"
#include "jni_bridge.h"

#include <assert.h>

#include <android/Bitmap.h>

using namespace venus;

void JNICALL Java_com_cloudream_ishow_algorithm_Effect_nativeApplyWhirlPinch
	(JNIEnv* env, jclass clazz, jobject _bitmap, jobject _pivot, jfloat whirl, jfloat pinch, jfloat radius)
{
	AndroidBitmapInfo info;
	uint32_t* pixels = lockJavaBitmap(env, _bitmap, info);
	assert(pixels != nullptr);

	cv::Mat image(info.height, info.width, CV_8UC4, pixels);
	cv::Point2f pivot = getNativePoint(env, _pivot);
//	applyWhirlPinch(image, pivot, whirl, pinch, radius);

	unlockJavaBitmap(env, _bitmap);
}

void JNICALL Java_com_cloudream_ishow_algorithm_Effect_nativeApplyWhirlPinch2
	(JNIEnv* env, jclass clazz, jobject _bitmap, jfloat whirl, jfloat pinch, jfloat radius)
{
	AndroidBitmapInfo info;
	uint32_t* pixels = lockJavaBitmap(env, _bitmap, info);
	assert(pixels != nullptr);

	cv::Mat image(info.height, info.width, CV_8UC4, pixels);
//	applyWhirlPinch(image, whirl, pinch, radius);

	unlockJavaBitmap(env, _bitmap);
}

void JNICALL Java_com_cloudream_ishow_algorithm_Effect_nativeDistort
	(JNIEnv* env, jclass clazz, jobject _bitmap, jobject _point0, jobject _point1, jfloat strength, jint type)
{
	AndroidBitmapInfo info;
	uint32_t* pixels = lockJavaBitmap(env, _bitmap, info);
	assert(pixels != nullptr);

	cv::Mat image(info.height, info.width, CV_8UC4, pixels);
	cv::Point2f point0 = getNativePoint(env, _point0);
	cv::Point2f point1 = getNativePoint(env, _point1);
//	distort(image, point0, point1, strength, static_cast<DistortionType>(type));

	unlockJavaBitmap(env, _bitmap);
}

void JNICALL Java_com_cloudream_ishow_algorithm_Effect_nativeCatmullRomSpline
	(JNIEnv * env, jclass clazz, jobject _result, jfloat t, jobject _p0, jobject _p1, jobject _p2, jobject _p3)
{
	cv::Point2f p0 = getNativePoint(env, _p0);
	cv::Point2f p1 = getNativePoint(env, _p1);
	cv::Point2f p2 = getNativePoint(env, _p2);
	cv::Point2f p3 = getNativePoint(env, _p3);

	cv::Point2f result = catmullRomSpline(t, p0, p1, p2, p3);

	setJavaPoint(env, _result, result);
}

void JNICALL Java_com_cloudream_ishow_algorithm_Effect_nativeTone(JNIEnv *env,
		jclass clazz, jobject _bitmap, jint color, jfloat amount)
{
	assert(0.0f <= amount && amount <= 1.0f);

	AndroidBitmapInfo info;
	uint32_t* pixels = lockJavaBitmap(env, _bitmap, info);
	assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);

//	uint8_t src_a = color >> 24;
	uint8_t src_r = color >> 16;
	uint8_t src_g = color >> 8;
	uint8_t src_b = color;

	uint8_t* bytes = reinterpret_cast<uint8_t*>(pixels);
	const int length = info.height * info.width * 4;
	#pragma omp parallel for
	for(int i = 0; i < length; i += 4)
	{
		bytes[i+0] = bytes[i+0] + (int32_t)(src_r - bytes[i+0]) * amount;
		bytes[i+1] = bytes[i+1] + (int32_t)(src_g - bytes[i+1]) * amount;
		bytes[i+2] = bytes[i+2] + (int32_t)(src_b - bytes[i+2]) * amount;
//		bytes[i+0] = clamp(venus::lerp<int32_t>(bytes[i+0], src_r, amount), 0, 255);
//		bytes[i+1] = clamp(venus::lerp<int32_t>(bytes[i+1], src_g, amount), 0, 255);
//		bytes[i+2] = clamp(venus::lerp<int32_t>(bytes[i+2], src_b, amount), 0, 255);
//		uint32_t dst_r = pixels[i]       & 0xff;
//		uint32_t dst_g = (pixels[i]>>8)  & 0xff;
//		uint32_t dst_b = (pixels[i]>>16) & 0xff;
//		uint32_t dst_a = pixels[i] & 0xff000000;  // (pixels[i]>>24) & 0xff;
//
//		dst_r = venus::lerp(dst_r, src_r, amount);
//		dst_g = venus::lerp(dst_g, src_g, amount);
//		dst_b = venus::lerp(dst_b, src_b, amount);
//
//		pixels[i] = dst_r | (dst_g << 8) | (dst_b << 16) | dst_a;
	}

	unlockJavaBitmap(env, _bitmap);
}

void JNICALL Java_com_cloudream_ishow_algorithm_Effect_nativeGrayToAlpha
	(JNIEnv *env, jclass clazz, jobject _src_bitmap, jobject _dst_bitmap)
{
	AndroidBitmapInfo src_info, dst_info;
	uint32_t* src_pixels = lockJavaBitmap(env, _src_bitmap, src_info);
	uint32_t* dst_pixels = lockJavaBitmap(env, _dst_bitmap, dst_info);
	assert(src_pixels != nullptr && dst_pixels != nullptr);

	// RGBA little endian 0xffababab -> 0xabffffff
	LOGW("color: 0x%08X", src_pixels[0]);
	for(int i = 0, length =  src_info.height * src_info.width; i < length; ++i)
	{
#ifndef NDEBUG
		static bool trigged = false;
		uint8_t* p = reinterpret_cast<uint8_t*>(src_pixels);
		if(!trigged && (p[0] != p[1] || p[1] != p[2] || p[3] != 0xff))
		{
			LOGW("not a gray image!");
			trigged = true;
		}
#endif

		dst_pixels[i] = 0x00ffffff | ((src_pixels[i]&0xff) << 24);
//		dst_pixels[i] = (i%2==0)?0x10304050:0x20304050;  // 16 fbfcfd  32 73fe7f
//		dst_pixels[i] = gray * 0x01010100 + alpha;
	}

	unlockJavaBitmap(env, _dst_bitmap);
	unlockJavaBitmap(env, _src_bitmap);
}

void JNICALL Java_com_cloudream_ishow_algorithm_Effect_nativeColorToAlpha
	(JNIEnv *env, jclass clazz, jint _color, jobject _src_bitmap, jobject _dst_bitmap)
{
	AndroidBitmapInfo src_info, dst_info;
	uint32_t* src_pixels = lockJavaBitmap(env, _src_bitmap, src_info);
	uint32_t* dst_pixels = lockJavaBitmap(env, _dst_bitmap, dst_info);
	assert(src_pixels != nullptr && dst_pixels != nullptr);

	float color[4];  // RGBA format
	color[0] = ((_color >> 16) & 0xff) / 255.0f;
	color[1] = ((_color >> 8) & 0xff) / 255.0f;
	color[2] = ((_color >> 0) & 0xff) / 255.0f;
	color[3] = ((_color >> 24) & 0xff) / 255.0f;

	assert(false);  // TODO float type uint8_t modification
//	for(int i = 0; i < height * stride; ++i)
//	{
//		colorToAlpha (color, src_pixels, dst_pixels);
//		src_pixels += 4;
//		dst_pixels += 4;
//	}

	unlockJavaBitmap(env, _dst_bitmap);
	unlockJavaBitmap(env, _src_bitmap);
}

cv::Vec4f getNativeColor(jint color)
{
	// @see http://stackoverflow.com/questions/1751346/interpret-signed-as-unsigned
	// Should one use static_cast or reinterpret_cast between signed and unsigned integers?
	int _color = static_cast<uint32_t>(color);

	cv::Vec4f rgba;  // RGBA format
	rgba[0] = ((_color >> 16) & 0xff) / 255.0f;
	rgba[1] = ((_color >> 8) & 0xff) / 255.0f;
	rgba[2] = ((_color >> 0) & 0xff) / 255.0f;
	rgba[3] = ((_color >> 24) & 0xff) / 255.0f;

	return rgba;
}

void JNICALL Java_com_cloudream_ishow_algorithm_Effect_nativeSelectContiguousRegionByColor(JNIEnv *env,
		jclass clazz, jobject _mask, jobject _image, jint _color, jint _select_criterion, jfloat threshold, jboolean antialias, jboolean select_transparent)
{
	AndroidBitmapInfo image_info;
	uint32_t* image_pixels = lockJavaBitmap(env, _image, image_info);
	assert(image_pixels != nullptr);

	unlockJavaBitmap(env, _image);
}
