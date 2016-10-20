#include "com_cloudream_ishow_algorithm_Makeup.h"

#define LOG_TAG "Makeup-JNI"
#include "platform/jni_bridge.h"

#include "venus/compiler.h"
#include "venus/Makeup.h"

using namespace cv;
using namespace venus;

void JNICALL Java_com_cloudream_ishow_algorithm_Makeup_nativeApplyLipColor(JNIEnv *env,
		jclass, jint, jfloat)
{

}

void JNICALL Java_com_cloudream_ishow_algorithm_Makeup_nativeApplyBlush(JNIEnv *env,
		jclass clazz, jobject _dst, jobject _src, jobjectArray _points, jint _shape, jint _color, jfloat amount)
{
	AndroidBitmapInfo dst_info, src_info;
	uint32_t* dst_pixels = lockJavaBitmap(env, _dst, dst_info);
	uint32_t* src_pixels = lockJavaBitmap(env, _src, src_info);
	assert(dst_pixels != nullptr && src_pixels != nullptr);
	cv::Mat dst(dst_info.height, dst_info.width, CV_8UC4, dst_pixels);
	cv::Mat src(src_info.height, src_info.width, CV_8UC4, src_pixels);

	const std::vector<Point2f> points = getNativePointArray(env, _points);
	assert(0 <= _shape && _shape < static_cast<int>(Makeup::BlushShape::SHAPE_COUNT));
	Makeup::BlushShape shape = static_cast<Makeup::BlushShape>(_shape);

	uint32_t color = *reinterpret_cast<uint32_t*>(&_color);
	Makeup::applyBlush(dst, src, points, shape, color, amount);

	unlockJavaBitmap(env, _src);
	unlockJavaBitmap(env, _dst);
}

