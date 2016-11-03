#include "com_cloudream_ishow_algorithm_Feature.h"

#include "venus/Feature.h"

#define LOG_TAG "Feature-JNI"
#include "jni_bridge.h"

using namespace cv;
using namespace venus;

jobjectArray JNICALL Java_com_cloudream_ishow_algorithm_Feature_nativeDetectFace(JNIEnv* env,
		jclass clazz, jobject _image, jstring _image_name, jstring _classifier_dir)
{
	AndroidBitmapInfo image_info;
	uint32_t* pixels = lockJavaBitmap(env, _image, image_info);
	assert(pixels != nullptr);

	cv::Mat image(image_info.height, image_info.width, CV_8UC4, pixels);
	std::string image_name = getNativeString(env, _image_name);
	std::string classifier_dir = getNativeString(env, _classifier_dir);

	cv::Mat gray;
	cv::cvtColor(image, gray, CV_RGBA2GRAY);
	unlockJavaBitmap(env, _image);

	const std::vector<Point2f> points = Feature::detectFace(gray, image_name, classifier_dir);

	jclass class_PointF = env->FindClass("android/graphics/PointF");
	assert(class_PointF != nullptr);

	// http://stackoverflow.com/questions/1036666/use-of-array-of-zero-length
    // if count == 0, return zero length array, so you don't have to check for null situation.
	const size_t count = points.size();
	jobjectArray objectArray_points = env->NewObjectArray(count, class_PointF, 0);
	setJavaPointArray(env, objectArray_points, points);

	return objectArray_points;
}

jobjectArray JNICALL Java_com_cloudream_ishow_algorithm_Feature_nativeGetSymmetryAxis(JNIEnv* env,
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

//	const Vec2f    down_vector(line[0], line[1]);
//	const Point2f center_point(line[2], line[3]);
	jobject object_down_vector  = env->NewObject(class_PointF, method_PointF, line[0], line[1]);
	jobject object_center_point = env->NewObject(class_PointF, method_PointF, line[2], line[3]);
	env->SetObjectArrayElement(objectArray_points, 0, object_down_vector);
	env->SetObjectArrayElement(objectArray_points, 1, object_center_point);

	return objectArray_points;
}
