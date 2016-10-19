#include "com_cloudream_ishow_algorithm_Feature.h"

#include "venus/Feature.h"

#define LOG_TAG "Feature-JNI"
#include "jni_bridge.h"

using namespace cv;
using namespace venus;

jobjectArray JNICALL Java_com_cloudream_ishow_algorithm_Feature_getSymmetryAxis(JNIEnv* env,
		jclass clazz, jobjectArray _points)
{
	const std::vector<Point2f> points = getNativePointArray(env, _points);
	cv::Vec4f line = Feature::getSymmetryAxis(points);

	const jint count = 2;  // center point and up vector
	jclass class_PointF = env->FindClass("android/graphics/PointF");
	jmethodID method_PointF = env->GetMethodID(class_PointF, "<init>", "(FF)V");
	assert(class_PointF != nullptr && method_PointF != nullptr);
	assert(class_PointF != nullptr);
	jobjectArray objectArray_points = env->NewObjectArray(count, class_PointF, nullptr);

//	const Vec2f up_vector(line[0], line[1]);
//	const Point2f center_point(line[2], line[3]);
	jobject object_up_vector    = env->NewObject(class_PointF, method_PointF, line[0], line[1]);
	jobject object_center_point = env->NewObject(class_PointF, method_PointF, line[2], line[3]);
	env->SetObjectArrayElement(objectArray_points, 0, object_center_point);
	env->SetObjectArrayElement(objectArray_points, 1, object_up_vector);

	return objectArray_points;
}
