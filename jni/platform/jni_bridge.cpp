#define LOG_TAG "jni_bridge"
#include "jni_bridge.h"

#include <assert.h>

// **************** Java to C++ **************** //
std::string getNativeString(JNIEnv *env, jstring _str)
{
	const char *str = env->GetStringUTFChars(_str, nullptr);
	std::string result(str);
	env->ReleaseStringUTFChars(_str, str);  // DON'T forget to release strings

	return result;
}

cv::Mat* getNativeMat(JNIEnv *env, jobject _mat)
{
	jclass class_Mat = env->FindClass("org/opencv/core/Mat");
	jmethodID method_getNativeObjAddr = env->GetMethodID(class_Mat, "getNativeObjAddr", "()J");
	assert(class_Mat != nullptr && method_getNativeObjAddr != nullptr);
	jlong pointer = env->CallLongMethod(_mat, method_getNativeObjAddr);

	return reinterpret_cast<cv::Mat*>(pointer);
}

// This is the Point that reside in android.graphic.Point, not org.opencv.core.Point
cv::Point2f getNativePoint(JNIEnv *env, jobject _point)
{
	jclass class_PointF = env->FindClass("android/graphics/PointF");
	jfieldID field_x = env->GetFieldID(class_PointF, "x", "F");
	jfieldID field_y = env->GetFieldID(class_PointF, "y", "F");
	assert(class_PointF != nullptr);
	assert(field_x != nullptr && field_y != nullptr);
	
	jfloat x = env->GetFloatField(_point, field_x);
	jfloat y = env->GetFloatField(_point, field_y);
	return cv::Point2f(x, y);
}

std::vector<cv::Point2f> getNativePointArray(JNIEnv *env, jobjectArray _points)
{
	jclass class_PointF = env->FindClass("android/graphics/PointF");
	jfieldID field_x = env->GetFieldID(class_PointF, "x", "F");
	jfieldID field_y = env->GetFieldID(class_PointF, "y", "F");
	assert(class_PointF != nullptr);
	assert(field_x != nullptr && field_y != nullptr);

	// Get<Primitive>ArrayRegion, no GetObjectArrayRegion method, fetch element one by one.
	jsize feature_point_count = env->GetArrayLength(_points);
	std::vector<cv::Point2f> points(feature_point_count);
	for(jsize i = 0; i < feature_point_count; ++i)
	{
		jobject element = env->GetObjectArrayElement(_points, i);
		float x = env->GetFloatField(element, field_x);
		float y = env->GetFloatField(element, field_y);
		points[i] = cv::Point2f(x, y);
		env->DeleteLocalRef(element);
	}

	return points;
}

#ifdef ANDROID
uint32_t* lockJavaBitmap(JNIEnv* env, jobject bitmap, AndroidBitmapInfo& info)
{
	// https://developer.android.com/ndk/reference/group___bitmap.html
	int ret = AndroidBitmap_getInfo(env, bitmap, &info);
	if(ret < 0)
	{
		LOGE("AndroidBitmap_getInfo() failed ! error = %d", ret);
		return nullptr;
	}

//	LOGD("width:%d height:%d stride:%d", info.width, info.height, info.stride);
	assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);

	uint32_t* pixels;
	ret = AndroidBitmap_lockPixels(env, bitmap, reinterpret_cast<void**>(&pixels));
	if(ret < 0)
	{
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		return nullptr;
	}

	return pixels;
}

void unlockJavaBitmap(JNIEnv* env, jobject bitmap)
{
	AndroidBitmap_unlockPixels(env, bitmap);
}
#endif // ANDROID

// **************** C++ to Java **************** //

jobject getJavaMat(JNIEnv *env, const cv::Mat& mat)
{
	jclass class_Mat = env->FindClass("org/opencv/core/Mat");
	jmethodID method_Mat = env->GetMethodID(class_Mat, "<init>", "()V");
	assert(class_Mat != nullptr && method_Mat != nullptr);

	jobject Mat_result = env->NewObject(class_Mat, method_Mat);
	jmethodID method_getNativeObjAddr = env->GetMethodID(class_Mat, "getNativeObjAddr", "()J");
	cv::Mat* ptr_result = reinterpret_cast<cv::Mat*>(env->CallLongMethod(Mat_result, method_getNativeObjAddr));
	assert(method_getNativeObjAddr != nullptr && ptr_result != nullptr);

	*ptr_result = mat;
	return Mat_result;
}

jobject getJavaPoint(JNIEnv *env, const cv::Point2f& point)
{
	jclass class_PointF = env->FindClass("android/graphics/PointF");
	jmethodID method_PointF = env->GetMethodID(class_PointF, "<init>", "(FF)V");
	assert(class_PointF != nullptr && method_PointF != nullptr);

	return env->NewObject(class_PointF, method_PointF, point.x, point.y);
}

void setJavaPoint(JNIEnv *env, jobject _point, const cv::Point2f& point)
{
	jclass class_PointF = env->FindClass("android/graphics/PointF");
	jfieldID field_x = env->GetFieldID(class_PointF, "x", "F");
	jfieldID field_y = env->GetFieldID(class_PointF, "y", "F");
	assert(class_PointF != nullptr);
	assert(field_x != nullptr && field_y != nullptr);

	env->SetFloatField(_point, field_x, point.x);
	env->SetFloatField(_point, field_y, point.y);
}

void setJavaPointArray(JNIEnv *env, jobjectArray _array, const std::vector<cv::Point2f>& points)
{
	jclass class_PointF = env->FindClass("android/graphics/PointF");
	jmethodID method_PointF = env->GetMethodID(class_PointF, "<init>", "(FF)V");
	assert(class_PointF != nullptr && method_PointF != nullptr);

	const size_t count = points.size();
	for(size_t i = 0; i < count; ++i)
	{
		const cv::Point2f& point = points[i];
		jobject object_point = env->NewObject(class_PointF, method_PointF, point.x, point.y);
		env->SetObjectArrayElement(_array, i, object_point);
	}
}