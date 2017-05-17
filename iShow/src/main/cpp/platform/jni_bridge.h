#ifndef JNI_BRIDGE_H_
#define JNI_BRIDGE_H_

#include <jni.h>
#include <stdint.h>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#ifdef ANDROID
#include <android/bitmap.h>
#endif

#ifndef NDEBUG
#  ifdef ANDROID
#    ifndef LOG_TAG
#      define LOG_TAG "ANDROID"
#    endif
#    include <android/log.h>
#    define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#    define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG,   LOG_TAG, __VA_ARGS__))
#    define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,    LOG_TAG, __VA_ARGS__))
#    define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN,    LOG_TAG, __VA_ARGS__))
#    define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,   LOG_TAG, __VA_ARGS__))
#  else
#    include <stdio.h>
#    define LOGV(...) printf(__VA_ARGS__)
#    define LOGD(...) printf(__VA_ARGS__)
#    define LOGI(...) printf(__VA_ARGS__)
#    define LOGW(...) printf(__VA_ARGS__)
#    define LOGE(...) printf(__VA_ARGS__)
#  endif /* ANDROID */
#else
#  define TIME_START
#  define TIME_STOP(TAG)

#  define LOGV(...)
#  define LOGD(...)
#  define LOGI(...)
#  define LOGW(...)
#  define LOGE(...)
#endif

/**
 * timing macro, use it like this:
 *
 * TIME_START;
 * TIME_STOP("procedure 1");
 * TIME_STOP("procedure 2");
 */
#ifndef NDEBUG
#  include <ctime>
#  define TIME_START std::time_t start = std::clock(), stop
#  define TIME_STOP(TAG) stop = std::clock(); \
		LOGI(TAG " uses %gs\n", static_cast<double>(stop - start)/CLOCKS_PER_SEC); \
		start = stop
#else
#  define TIME_START
#  define TIME_STOP(TAG)
#endif


// **************** Java to C++ **************** //
uint32_t                 getNativeColor(jint _color);
std::string              getNativeString(JNIEnv *env, jstring _str);
cv::Mat*                 getNativeMat(JNIEnv *env, jobject _mat);

// Android doesn't come up with a native Point type, currently use OpenCV's cv::Point2f.
cv::Point2f              getNativePoint(JNIEnv *env, jobject _point);
std::vector<cv::Point2f> getNativePointArray(JNIEnv *env, jobjectArray _points);

/**
 * we use Bitmap RGBA8888 or A8 format
 */
#ifdef ANDROID
uint32_t* lockJavaBitmap(JNIEnv* env, jobject bitmap, AndroidBitmapInfo& info);
void unlockJavaBitmap(JNIEnv* env, jobject bitmap);
#endif

// **************** C++ to Java **************** //

jobject getJavaMat(JNIEnv *env, const cv::Mat& mat);
jobject getJavaPoint(JNIEnv *env, const cv::Point2f& point);

void setJavaPoint(JNIEnv *env, jobject _point, const cv::Point2f& point);
void setJavaPointArray(JNIEnv *env, jobjectArray _array, const std::vector<cv::Point2f>& points);


#endif /* JNI_BRIDGE_H_ */
