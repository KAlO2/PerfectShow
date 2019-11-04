#include "platform/android/Makeup.h"

static const char* LOG_TAG = "Makeup-JNI";
#include "platform/android/jni_bridge.h"

#include "venus/compiler.h"
#include "venus/Makeup.h"

using namespace cv;
using namespace venus;


#define PROLOGUE_ENTER \
	(void)clazz;                                                           \
	                                                                       \
	AndroidBitmapInfo dst_info;                                            \
	uint32_t* dst_pixels  = lockJavaBitmap(env, _dst, dst_info);           \
	Mat dst(dst_info.height, dst_info.width, CV_8UC4, dst_pixels);         \
	                                                                       \
	AndroidBitmapInfo src_info;                                            \
	uint32_t* src_pixels  = lockJavaBitmap(env, _src, src_info);           \
	Mat src(src_info.height, src_info.width, CV_8UC4, src_pixels);         \
	                                                                       \
	const std::vector<Point2f> points = getNativePointArray(env, _points); \

#define PROLOGUE_EXIT \
	unlockJavaBitmap(env, _src);                                           \
	unlockJavaBitmap(env, _dst);                                           \


void JNICALL Java_com_wonderful_ishow_makeup_Makeup_nativeApplyBrow(JNIEnv* env,
		jclass clazz, jobject _dst, jobject _src, jobjectArray _points, jobject _mask, jint _color, jfloat amount)
{
	PROLOGUE_ENTER

	AndroidBitmapInfo mask_info;
	uint32_t* mask_pixels  = lockJavaBitmap(env, _mask, mask_info);
	assert(mask_pixels != nullptr && mask_info.format == ANDROID_BITMAP_FORMAT_A_8);
	Mat mask(mask_info.height, mask_info.width, CV_8UC1, mask_pixels);

	uint32_t color = getNativeColor(_color);
	Makeup::applyBrow(dst, src, points, mask, color, amount);

	unlockJavaBitmap(env, _mask);

	PROLOGUE_EXIT
}

void JNICALL Java_com_wonderful_ishow_makeup_Makeup_nativeApplyEye(JNIEnv* env,
		jclass clazz, jobject _dst, jobject _src, jobjectArray _points, jobject _cosmetic, jfloat amount)
{
	PROLOGUE_ENTER

	AndroidBitmapInfo cosmetic_info;
	uint32_t* cosmetic_pixels  = lockJavaBitmap(env, _cosmetic, cosmetic_info);
	Mat cosmetic(cosmetic_info.height, cosmetic_info.width, CV_8UC4, cosmetic_pixels);

	Makeup::applyEye(dst, src, points, cosmetic, amount);
	unlockJavaBitmap(env, _cosmetic);

	PROLOGUE_EXIT
}

void JNICALL Java_com_wonderful_ishow_makeup_Makeup_nativeApplyEyeLash(JNIEnv* env,
		jclass clazz, jobject _dst, jobject _src, jobjectArray _points, jobject _mask, jint _color, jfloat amount)
{
	PROLOGUE_ENTER

	AndroidBitmapInfo mask_info;
	uint32_t* mask_pixels  = lockJavaBitmap(env, _mask, mask_info);
	assert(mask_pixels != nullptr && mask_info.format == ANDROID_BITMAP_FORMAT_A_8);
	Mat mask(mask_info.height, mask_info.width, CV_8UC1, mask_pixels);

	uint32_t color = getNativeColor(_color);
	Makeup::applyEyeLash(dst, src, points, mask, color, amount);

	unlockJavaBitmap(env, _mask);

	PROLOGUE_EXIT
}

void JNICALL Java_com_wonderful_ishow_makeup_Makeup_nativeApplyEyeShadow(JNIEnv* env,
		jclass clazz, jobject _dst, jobject _src, jobjectArray _points, jobjectArray _masks, jintArray _colors, jfloat amount)
{
	PROLOGUE_ENTER

	jclass     class_Bitmap = env->FindClass("android/graphics/Bitmap");
	jmethodID method_Bitmap = env->GetMethodID(class_Bitmap, "<init>", "()V");
	assert(class_Bitmap != nullptr && method_Bitmap != nullptr);

	const jsize count = env->GetArrayLength(_masks);
	constexpr jsize N = 3;  // currently we use 3 layers
	assert(count >= N);

	jobject element_masks[N];
	cv::Mat masks[N];
	uint32_t colors[N];

	jint* color_array = env->GetIntArrayElements(_colors, nullptr/* isCopy */);
	for(jsize i = 0; i < N; ++i)
	{
		element_masks[i] = env->GetObjectArrayElement(_masks, i);

		AndroidBitmapInfo mask_info;
		uint8_t* mask_pixels  = reinterpret_cast<uint8_t*>(lockJavaBitmap(env, element_masks[i], mask_info));
		assert(mask_info.format == ANDROID_BITMAP_FORMAT_A_8);
		masks[i] = Mat(mask_info.height, mask_info.width, CV_8UC1, mask_pixels);

		colors[i] = getNativeColor(color_array[i]);
	}

	Makeup::applyEyeShadow(dst, src, points, masks, colors, amount);
	constexpr jint mode = 0;  // copy back the content and free the color_array buffer
	env->ReleaseIntArrayElements(_colors, color_array, mode);
	for(jsize i = 0; i < N; ++i)
	{
		unlockJavaBitmap(env, element_masks[i]);
		env->DeleteLocalRef(element_masks[i]);
	}

	PROLOGUE_EXIT
}

void JNICALL Java_com_wonderful_ishow_makeup_Makeup_nativeApplyIris(JNIEnv* env,
		jclass clazz, jobject _dst, jobject _src, jobjectArray _points, jobject _iris, jfloat amount)
{
	PROLOGUE_ENTER

	AndroidBitmapInfo iris_info;
	uint32_t* iris_pixels  = lockJavaBitmap(env, _iris, iris_info);
	assert(iris_pixels != nullptr && iris_info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);
	Mat iris(iris_info.height, iris_info.width, CV_8UC4, iris_pixels);

	Makeup::applyIris(dst, src, points, iris, amount);

	unlockJavaBitmap(env, _iris);
	PROLOGUE_EXIT
}

void JNICALL Java_com_wonderful_ishow_makeup_Makeup_nativeApplyBlush(JNIEnv* env,
		jclass clazz, jobject _dst, jobject _src, jobjectArray _points, jint _shape, jint _color, jfloat amount)
{
	PROLOGUE_ENTER

	using BlushShape = Makeup::BlushShape;
	assert(0 <= _shape && _shape < static_cast<int>(BlushShape::SHAPE_COUNT));
	BlushShape shape = static_cast<BlushShape>(_shape);
	uint32_t   color = getNativeColor(_color);

	Makeup::applyBlush(dst, src, points, shape, color, amount);

	PROLOGUE_EXIT
}

void JNICALL Java_com_wonderful_ishow_makeup_Makeup_nativeApplyLip(JNIEnv* env,
		jclass clazz, jobject _dst, jobject _src, jobjectArray _points, jint _color, jfloat amount)
{
	PROLOGUE_ENTER

/*
	use static_cast or reinterpret_cast for type conversion between signed and unsigned?
	http://stackoverflow.com/questions/1751346/interpret-signed-as-unsigned
	http://en.cppreference.com/w/cpp/language/reinterpret_cast
	
	int32_t   a = 0xCAFEBABE;
	uint32_t& b = *reinterpret_cast<uint32_t*>(&a);
	uint32_t  c = static_cast<uint32_t>(a);

	int32_t _color = 0;
024B7ADE  mov         dword ptr [_color],0h
	uint32_t* color = reinterpret_cast<uint32_t*>(&_color);
024B7AE5  lea         eax,[_color]
024B7AE8  mov         dword ptr [color],eax
	uint32_t color2 = static_cast<uint32_t>(_color);
024B7AEB  mov         ecx,dword ptr [_color]
024B7AEE  mov         dword ptr [color2],ecx

	Unlike static_cast, but like const_cast, the reinterpret_cast expression does not compile to any CPU instructions.
*/
	uint32_t color = getNativeColor(_color);

	Makeup::applyLip(dst, src, points, color, amount);

	PROLOGUE_EXIT
}
