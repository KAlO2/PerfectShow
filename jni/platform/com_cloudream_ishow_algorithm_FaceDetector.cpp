#include "com_cloudream_ishow_algorithm_FaceDetector.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/imgproc.hpp>

#include <omp.h>

#define LOG_TAG "FaceDetector"
#include "platform/jni_bridge.h"

#include "stasm/stasm_lib.h"

#include "venus/colorspace.h"
#include "venus/Effect.h"
#include "venus/Feature.h"
#include "venus/Makeup.h"
#include "venus/opencv_utility.h"
#include "venus/region_operation.h"


using namespace cv;
using namespace venus;

jobject getJavaRoiInfo(JNIEnv* env, const RoiInfo& roi)
{
	jobject _origion = getJavaPoint(env, roi.origion);
	jobject _pivot   = getJavaPoint(env, roi.pivot);
	jobject _mask    = getJavaMat(env, roi.mask);

	jclass class_RoiInfo = env->FindClass("com/cloudream/ishow/algorithm/FaceDetector$RoiInfo");
	jmethodID method_RoiInfo = env->GetMethodID(class_RoiInfo, "<init>", "(Landroid/graphics/PointF;Landroid/graphics/PointF;Lorg/opencv/core/Mat;)V");
	assert(class_RoiInfo != nullptr && method_RoiInfo != nullptr);

	return env->NewObject(class_RoiInfo, method_RoiInfo, _origion, _pivot, _mask);
}

// used for debugging intermediate output image
static bool dump(std::string filename, const cv::Mat& image)
{
	static std::string DIR = "/sdcard/Pictures/PerfectShow/";
	cv::Mat debug_image;
	cv::cvtColor(image, debug_image, CV_RGBA2BGRA);  // turn Android' RGBA to OpenCV's BGRA format
	return cv::imwrite(DIR + filename, debug_image);
}

static std::vector<Point2f> getFaceFeaturePoints(JNIEnv* env, jobject thiz)
{
	jclass class_FaceDetector = env->GetObjectClass(thiz);
	jfieldID field_points = env->GetFieldID(class_FaceDetector, "points", "[Landroid/graphics/PointF;");
	assert(class_FaceDetector != nullptr && field_points != nullptr);
	jobjectArray PointF_array_points = reinterpret_cast<jobjectArray>(env->GetObjectField(thiz, field_points));

	return getNativePointArray(env, PointF_array_points);
}

jobjectArray JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeDetectFaceSingle(JNIEnv* env,
		jclass clazz, jstring _image_path, jstring _data_dir)
{
	std::string image_path = getNativeString(env, _image_path);
	std::string data_dir = getNativeString(env, _data_dir);

	const std::vector<Point2f> points = getFaceFeaturePoints(image_path, data_dir);

	// http://stackoverflow.com/questions/1036666/use-of-array-of-zero-length
    // if count == 0, return zero length array, so you don't have to check for null situation.

	const size_t count = points.size();
	jclass class_PointF = env->FindClass("android/graphics/PointF");
	assert(class_PointF != nullptr);
	jobjectArray objectArray_points = env->NewObjectArray(count, class_PointF, 0);  // 4 bonus points
	setJavaPointArray(env, objectArray_points, points);

	return objectArray_points;
}

#if 0
void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeDetect77(JNIEnv* env,
		jobject thiz, jstring _image_path, jstring _data_dir)
{
	LOGI("enter %s", __FUNCTION__);
	std::string image_path = getNativeString(env, _image_path);
	std::string data_dir = getNativeString(env, _data_dir);

	const std::vector<Point2f> points = getFaceFeaturePoints(image_path, data_dir, false);

	jclass class_FaceDetector = env->GetObjectClass(thiz);
	jfieldID field_points = env->GetFieldID(class_FaceDetector, "points", "[Landroid/graphics/PointF;");
	assert(class_FaceDetector != nullptr && field_points != nullptr);
	jobjectArray PointF_array_points = reinterpret_cast<jobjectArray>(env->GetObjectField(thiz, field_points));

	setJavaPointArray(env, PointF_array_points, points);
}
#endif

jobjectArray JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeDetectFaceSingle2(JNIEnv* env,
		jclass clazz, jobject _image, jstring _data_dir)
{
	const std::string data_dir = getNativeString(env, _data_dir);

	AndroidBitmapInfo image_info;
	uint32_t* image_pixels = lockJavaBitmap(env, _image, image_info);
	assert(image_pixels != nullptr);
	cv::Mat image_rgba(image_info.height, image_info.width, CV_8UC4, image_pixels);
	cv::Mat image_gray;
	cvtColor(image_rgba, image_gray, CV_RGBA2GRAY);
	unlockJavaBitmap(env, _image);

	const std::vector<Point2f> points = getFaceFeaturePoints(image_gray, "android/graphics/Bitmap", data_dir);

	const size_t count = points.size();
	jclass class_PointF = env->FindClass("android/graphics/PointF");
	jobjectArray objectArray_points = env->NewObjectArray(count, class_PointF, 0);  // 4 bonus points
	setJavaPointArray(env, objectArray_points, points);

	return objectArray_points;
}

jobjectArray JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeDetect(JNIEnv* env,
		jobject thiz, jstring _image_path, jstring _data_dir)
{
	const std::string image_path = getNativeString(env, _image_path);
	const std::string data_dir   = getNativeString(env, _data_dir);

	Point size;
	const std::vector<Point2f> points = getFaceFeaturePoints(image_path, data_dir, size);
	LOGI("detected image size: %dx%d", size.x, size.y);

	jclass class_FaceDetector = env->GetObjectClass(thiz);
	jfieldID field_points = env->GetFieldID(class_FaceDetector, "points", "[Landroid/graphics/PointF;");
	assert(class_FaceDetector != nullptr && field_points != nullptr);
	jobjectArray PointF_array_points = reinterpret_cast<jobjectArray>(env->GetObjectField(thiz, field_points));

	jfieldID field_image_width = env->GetFieldID(class_FaceDetector, "image_width", "I");
	jfieldID field_image_height = env->GetFieldID(class_FaceDetector, "image_height", "I");
	assert(field_image_width != nullptr && field_image_height != nullptr);
	env->SetIntField(thiz, field_image_width, size.x);
	env->SetIntField(thiz, field_image_height, size.y);

	// C++ float[]  =>  Java PointF[]
	jclass class_PointF = env->FindClass("android/graphics/PointF");
	jmethodID method_PointF = env->GetMethodID(class_PointF, "<init>", "(FF)V");
	assert(class_PointF != nullptr && method_PointF != nullptr);

	const size_t count = points.size();
//	PointF_array_points = env->NewObjectArray(count, class_PointF, 0);  // 4 bonus points
	for(size_t i = 0; i < count; ++i)
	{
		const Point2f& point = points[i];
		jobject object_point = env->NewObject(class_PointF, method_PointF, point.x, point.y);
		env->SetObjectArrayElement(PointF_array_points, i, object_point);
	}
	
	// Mat mask = new Mat();  // java code to C++
	jclass class_Mat = env->FindClass("org/opencv/core/Mat");
	jmethodID method_Mat = env->GetMethodID(class_Mat, "<init>", "()V");
	jmethodID method_getNativeObjAddr = env->GetMethodID(class_Mat, "getNativeObjAddr", "()J");
	assert(class_Mat != nullptr);
	assert(method_Mat != nullptr && method_getNativeObjAddr != nullptr);
	
	jclass class_RoiInfo = env->FindClass("com/cloudream/ishow/algorithm/FaceDetector$RoiInfo");
	jmethodID method_RoiInfo = env->GetMethodID(class_RoiInfo, "<init>", "(Landroid/graphics/PointF;Landroid/graphics/PointF;Lorg/opencv/core/Mat;)V");
	jfieldID field_regions = env->GetFieldID(class_FaceDetector, "regions", "[Lcom/cloudream/ishow/algorithm/FaceDetector$RoiInfo;");
	assert(class_RoiInfo != nullptr && field_regions != nullptr);

	// access RoiInfo regions[];
	jobject object_field_regions = env->GetObjectField(thiz, field_regions);
	jobjectArray RoiInfoArray_regions = reinterpret_cast<jobjectArray>(object_field_regions);
	// try to allocate memory from native side without hope, don't know why, currently changed to new object on Java side.
	jmethodID method_RoiInfo2 = env->GetMethodID(class_RoiInfo, "<init>", "()V");
	jobject default_RoiInfo = env->NewObject(class_RoiInfo, method_RoiInfo2);
	RoiInfoArray_regions = env->NewObjectArray(REGION_COUNT, class_RoiInfo, default_RoiInfo);
	assert(RoiInfoArray_regions != nullptr);

	auto assignRegionInfo = [&env, &RoiInfoArray_regions,
					 &class_PointF, &method_PointF,
					 &class_Mat, &method_Mat, &method_getNativeObjAddr,
					 &class_RoiInfo, &method_RoiInfo]
					 (const RoiInfo& info, int region)
		{
			jobject PointF_origion = env->NewObject(class_PointF, method_PointF, info.origion.x, info.origion.y);
			jobject PointF_pivot = env->NewObject(class_PointF, method_PointF, info.pivot.x, info.pivot.y);

			jobject Mat_mask = env->NewObject(class_Mat, method_Mat, &info.mask);
			jlong pointer_mask = env->CallLongMethod(Mat_mask, method_getNativeObjAddr);
			*reinterpret_cast<Mat*>(pointer_mask) = info.mask;

			jobject RoiInfo_region = env->NewObject(class_RoiInfo, method_RoiInfo, PointF_origion, PointF_pivot, Mat_mask);
			env->SetObjectArrayElement(RoiInfoArray_regions, region, RoiInfo_region);
		};

/*
		auto assignRegionInfo = [&env, &RoiInfoArray_regions,
					 &class_PointF, &method_PointF,
					 &class_Mat, &method_Mat, &method_getNativeObjAddr,
					 &class_RoiInfo, &method_RoiInfo]
					 (const RoiInfo& info, int region)
		{
			jfieldID field_x = env->GetFieldID(class_PointF, "x", "F");
			jfieldID field_y = env->GetFieldID(class_PointF, "y", "F");
			assert(field_x != nullptr && field_y != nullptr);
			env->SetIntField(thiz, field_image_width, size.x);
			jobject PointF_origion = env->NewObject(class_PointF, method_PointF, info.origion.x, info.origion.y);
			jobject PointF_pivot = env->NewObject(class_PointF, method_PointF, info.pivot.x, info.pivot.y);

			jobject Mat_mask = env->NewObject(class_Mat, method_Mat, &info.mask);
			jlong pointer_mask = env->CallLongMethod(Mat_mask, method_getNativeObjAddr);
			*reinterpret_cast<Mat*>(pointer_mask) = info.mask;

			jobject RoiInfo_region = env->NewObject(class_RoiInfo, method_RoiInfo, PointF_origion, PointF_pivot, Mat_mask);
			env->SetObjectArrayElement(RoiInfoArray_regions, region, RoiInfo_region);
		};
*/
	// fill RoiInfo data
	RoiInfo face = calcuateFaceRegionInfo(points);
	assignRegionInfo(face, FACE);

	RoiInfo lips = calcuateLipsRegionInfo(points);
	assignRegionInfo(lips, LIP_B);

	return RoiInfoArray_regions;
/*
	RoiInfo lips = calcuateLipsRoiInfo(points);
	jobject PointF_origion = env->NewObject(class_PointF, method_PointF, lips.origion.x, lips.origion.y);
	jobject PointF_pivot = env->NewObject(class_PointF, method_PointF, lips.pivot.x, lips.pivot.y);

	jobject Mat_mask = env->NewObject(class_Mat, method_Mat, &lips.mask);
	jlong pointer_mask = env->CallLongMethod(Mat_mask, method_getNativeObjAddr);
	*reinterpret_cast<Mat*>(pointer_mask) = lips.mask;

	jobject RoiInfo_lips = env->NewObject(class_RoiInfo, method_RoiInfo, PointF_origion, PointF_pivot, Mat_mask);
	env->SetObjectArrayElement(RoiInfoArray_regions, LIPS, RoiInfo_lips);
*/
}

jobject JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeStretchImage(JNIEnv* env,
		jobject thiz, jobject _src, jstring _data_dir)
{
	LOGI("enter %s", __FUNCTION__);
//	const std::string image_path   = getNativeString(env, _image_path);
	const std::string data_dir     = getNativeString(env, _data_dir);

	Mat* ptr_src = getNativeMat(env, _src);

	Mat src_gray, image;
	cv::cvtColor(*ptr_src, src_gray, CV_BGR2GRAY);
	cvtColor(*ptr_src, image, CV_BGRA2BGR);
	image.convertTo(image, CV_32FC3, 1/255.0);
	LOGI("model image size: %dx%d", image.cols, image.rows);

	jclass class_FaceDetector = env->GetObjectClass(thiz);
	jfieldID field_image_width = env->GetFieldID(class_FaceDetector, "image_width", "I");
	jfieldID field_image_height = env->GetFieldID(class_FaceDetector, "image_height", "I");
	assert(field_image_width != nullptr && field_image_height != nullptr);
	jint image_width = env->GetIntField(thiz, field_image_width);
	jint image_height = env->GetIntField(thiz, field_image_height);
	LOGI("user image size: %dx%d", image_width, image_height);

	std::ostringstream os;
	os << "memory addr@" << static_cast<void*>(ptr_src);
	const std::string image_tag = os.str();
	Mat dst(image_height, image_width, CV_32FC3, CV_RGB(0, 0, 0));
	const std::vector<Point2f> dst_points = getFaceFeaturePoints(env, thiz);
	const std::vector<Point2f> src_points = getFaceFeaturePoints(src_gray, image_tag, data_dir);

	// model image stretched onto the image that used to made up.
	stretchImage(image, dst, src_points, dst_points, Feature::triangle_indices);

	jfieldID field_mat_model_stretched = env->GetFieldID(class_FaceDetector, "mat_model_stretched", "org/opencv/core/Mat");
	jobject _model_stretched = env->GetObjectField(thiz, field_mat_model_stretched);
	Mat& model_stretched = *getNativeMat(env, _model_stretched);
	model_stretched = dst;
	LOGI("stretched image size: %dx%d, type: %d", model_stretched.cols, model_stretched.rows, model_stretched.type());
	return _model_stretched;
}

jobject JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeSeamlessClone(JNIEnv* env,
		jclass clazz, jobject src, jobject dst, jobject mask, jobject center, jint flags)
{
	LOGI("enter %s", __FUNCTION__);
	const jboolean premultiplyAlpha = true;

/*
	<OpenCV-android-sdk>/sdk/java/bin/classes>javap -p -s org.opencv.android.Utils
	Compiled from "Utils.java"
	public class org.opencv.android.Utils {

	public static void bitmapToMat(android.graphics.Bitmap, org.opencv.core.Mat, boolean);
	  descriptor: (Landroid/graphics/Bitmap;Lorg/opencv/core/Mat;Z)V

	public static void matToBitmap(org.opencv.core.Mat, android.graphics.Bitmap, boolean);
	  descriptor: (Lorg/opencv/core/Mat;Landroid/graphics/Bitmap;Z)V

	}
*/
	LOGI("Utils.bitmapToMat(bitmap, mat, premultiplyAlpha);");
	jclass class_Utils = env->FindClass("org/opencv/android/Utils");
	jmethodID method_bitmapToMat = env->GetStaticMethodID(class_Utils, "bitmapToMat", "(Landroid/graphics/Bitmap;Lorg/opencv/core/Mat;Z)V");
	assert(class_Utils != nullptr && method_bitmapToMat != nullptr);

	jclass class_Mat = env->FindClass("org/opencv/core/Mat");
	jmethodID method_Mat = env->GetMethodID(class_Mat, "<init>", "()V");
	assert(class_Mat != nullptr && method_Mat != nullptr);
	jobject mat_src = env->NewObject(class_Mat, method_Mat);
	jobject mat_dst = env->NewObject(class_Mat, method_Mat);
	env->CallStaticVoidMethod(class_Utils, method_bitmapToMat, src, mat_src, premultiplyAlpha);
	env->CallStaticVoidMethod(class_Utils, method_bitmapToMat, dst, mat_dst, premultiplyAlpha);

	jmethodID method_getNativeObjAddr = env->GetMethodID(class_Mat, "getNativeObjAddr", "()J");
	assert(method_getNativeObjAddr != nullptr);
	jlong ptr_src = env->CallLongMethod(mat_src, method_getNativeObjAddr);
	jlong ptr_dst = env->CallLongMethod(mat_dst, method_getNativeObjAddr);
	const Mat& _src = *reinterpret_cast<const Mat*>(ptr_src);
	const Mat& _dst = *reinterpret_cast<const Mat*>(ptr_dst);

	// remove alpha channel
//	cv::cvtColor(_src_without_alpha, _src, Imgproc.COLOR_RGBA2RGB, 0);

	jclass class_PointF = env->FindClass("android/graphics/PointF");
	jfieldID field_x = env->GetFieldID(class_PointF, "x", "F");
	jfieldID field_y = env->GetFieldID(class_PointF, "y", "F");
	assert(class_PointF != nullptr);
	assert(field_x != nullptr && field_y != nullptr);
	jfloat x = env->GetFloatField(center, field_x);
	jfloat y = env->GetFloatField(center, field_y);
	Point _center(x, y);
	LOGI("center point: (%f, %f)", x, y);

	Mat _blend;
	if(mask == nullptr)
	{
		Mat _mask(_src.rows, _src.cols, CV_8UC1, Scalar(255));
		cv::seamlessClone(_src, _dst, _mask, _center, _blend, flags);
	}
	else
	{
		jobject mat_mask = env->NewObject(class_Mat, method_Mat);
		env->CallStaticVoidMethod(class_Utils, method_bitmapToMat, mask, mat_mask, premultiplyAlpha);
		jlong ptr_mask = env->CallLongMethod(mat_mask, method_getNativeObjAddr);
		const Mat& _mask = *reinterpret_cast<const Mat*>(ptr_mask);
		cv::seamlessClone(_src, _dst, _mask, _center, _blend, flags);
	}

	LOGI("Bitmap createBitmap(int width, int height, Config config);");
//	Bitmap blend = dst.copy(Bitmap.Config.ARGB_8888, true);
	jclass class_Bitmap = env->FindClass("android/graphics/Bitmap");
	jclass class_Bitmap$Config = env->FindClass("android/graphics/Bitmap$Config");
	jmethodID method_createBitmap = env->GetMethodID(class_Utils, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
	assert(class_Bitmap != nullptr && method_createBitmap != 0 && class_Bitmap$Config != 0);
	jfieldID field_ARGB_8888 = env->GetStaticFieldID(class_Bitmap$Config, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
	jobject Bitmap_Config_ARGB_8888 = env->GetStaticObjectField(class_Bitmap$Config, field_ARGB_8888);
	jobject blend = env->CallStaticObjectMethod(class_Bitmap, method_createBitmap, _dst.cols, _dst.rows, Bitmap_Config_ARGB_8888);

	LOGI("Utils.matToBitmap(_blend, blend, premultiplyAlpha);");
	jmethodID method_matToBitmap = env->GetMethodID(class_Utils, "matToBitmap", "(Lorg/opencv/core/Mat;Landroid/graphics/Bitmap;Z)V");
	env->CallStaticVoidMethod(class_Utils, method_matToBitmap, reinterpret_cast<jobject>(&_blend), blend, premultiplyAlpha);

	return blend;
}

jobject JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeSeamlessClone2(JNIEnv* env,
		jclass clazz, jstring src, jstring dst, jobject center, jint flags)
{
	return nullptr;
}

jobject JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeCalculateRegionInfo(JNIEnv* env,
		jobject thiz, jint region)
{
	const std::vector<cv::Point2f> points = getFaceFeaturePoints(env, thiz);

	jclass class_FaceDetector = env->GetObjectClass(thiz);
	jfieldID field_blur_radius = env->GetFieldID(class_FaceDetector, "blur_radius", "I");
	jint blur_radius = env->GetIntField(thiz, field_blur_radius);

	RoiInfo roi;
	switch(region)
	{
	case LIP_B:
		roi = calcuateLipsRegionInfo(points, blur_radius);
		break;
	default:
		assert(false);
	}

	return getJavaRoiInfo(env, roi);
}

jobject JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeCenterOfRegion(JNIEnv* env,
		jobject thiz, jint region)
{
	LOGI("enter %s", __FUNCTION__);
	jclass class_FaceDetector = env->GetObjectClass(thiz);
	jfieldID field_points = env->GetFieldID(class_FaceDetector, "points", "[Landroid/graphics/PointF;");
	assert(class_FaceDetector != nullptr && field_points != nullptr);
	jobjectArray PointF_array_points = reinterpret_cast<jobjectArray>(env->GetObjectField(thiz, field_points));

	std::vector<cv::Point2f> points = getNativePointArray(env, PointF_array_points);

	cv::Point2f point;
	switch(region)
	{
	case FACE:
		break;
	case EYE_BROW_L:
		point.x = (points[27].x + points[28].x + points[30].x + points[31].x)/4;
		point.y = (points[27].y + points[28].y + points[30].y + points[31].y)/4;
		break;
	case EYE_BROW_R:
		point.x = (points[20].x + points[21].x + points[23].x + points[24].x)/4;
		point.y = (points[20].y + points[21].y + points[23].y + points[24].y)/4;
		break;
	case EYE_SHADOW_L:
	case EYE_LASH_L:
		point.x = (points[47].x + points[49].x)/2;
		point.y = (points[47].y + points[49].y)/2;
		break;
	case EYE_SHADOW_R:
	case EYE_LASH_R:
		point.x = (points[37].x + points[39].x)/2;
		point.y = (points[37].y + points[39].y)/2;
		break;
	case BLUSH_L:
		point.x = (points[10].x + points[58].x)/2;
		point.y = (points[10].y + points[58].y)/2;
		break;
	case BLUSH_R:
		point.x = (points[2].x + points[62].x)/2;
		point.y = (points[2].y + points[62].y)/2;
		break;
	case NOSE:
		point = points[53];
		break;
	case LIP_B:
		point = points[74];
		break;
	default:
		point.x = point.y = 0.0f;
		assert(false);  // region is invalid
	}

	return getJavaPoint(env, point);
}

jobject JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeCloneFace(JNIEnv* env,
		jclass clazz, jstring _user_image_path, jstring _model_image_path, jstring _data_dir)
{
	LOGI("enter %s", __FUNCTION__);
	const std::string user_image_path  = getNativeString(env, _user_image_path);
	const std::string model_image_path = getNativeString(env, _model_image_path);
	const std::string data_dir         = getNativeString(env, _data_dir);

	cv::Mat native_result = cloneFace(user_image_path, model_image_path, data_dir);

	// post processing, Android's Bitmap use ARGB format, while OpenCV's Mat use BGRA format,
	// why doesn't the world use the common RGBA order like it does in HLSL/GLSL shader, sighs!
	cvtColor(native_result, native_result, CV_BGR2RGB);

	jobject result = getJavaMat(env, native_result);
	return result;
}

jobject JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeStretchImage(JNIEnv* env,
		jclass clazz, jobject _src_image, jobject _dst_image, jobjectArray _src_points, jobjectArray _dst_points)
{
	// src_image and dst_image are CV_8UC4 type, RGBA
	cv::Mat& src_image = *getNativeMat(env, _src_image);
	cv::Mat& dst_image = *getNativeMat(env, _dst_image);

	const std::vector<Point2f> src_points = getNativePointArray(env, _src_points);
	const std::vector<Point2f> dst_points = getNativePointArray(env, _dst_points);
	stretchImageWithAlpha(src_image, dst_image, src_points, dst_points, Feature::triangle_indices);

//	dump("stretched_image.png", dst_image);
	return getJavaMat(env, dst_image);
}

jobject JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeStretchImage2(JNIEnv* env,
		jclass clazz, jobject _src_image, jint _dst_width, jint _dst_height, jobjectArray _src_points, jobjectArray _dst_points)
{
	// src_image and dst_image are CV_8UC4 type, RGBA
	cv::Mat& src_image = *getNativeMat(env, _src_image);
	cv::Mat dst_image(_dst_height, _dst_width, CV_8UC4, Scalar::all(0.0));

	const std::vector<Point2f> src_points = getNativePointArray(env, _src_points);
	const std::vector<Point2f> dst_points = getNativePointArray(env, _dst_points);
	stretchImageWithAlpha(src_image, dst_image, src_points, dst_points, Feature::triangle_indices);

//	dump("stretched_image.png", dst_image);
	return getJavaMat(env, dst_image);
}

void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeOverlay(JNIEnv* env,
		jclass clazz, jobject _src_image, jobject _dst_image, jint _origin_x, jint _origin_y, jint _alpha)
{
	cv::Mat& src_image = *getNativeMat(env, _src_image);
	cv::Mat& dst_image = *getNativeMat(env, _dst_image);

	cv::Point origin(_origin_x, _origin_y);
	overlay(src_image, dst_image, origin, _alpha);

//	dump("overlay_image.png", dst_image);
}

void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeCreateBitmap(JNIEnv* env,
		jclass clazz, jobject _image, jint _color)
{
	AndroidBitmapInfo info;
	uint32_t* pixels = lockJavaBitmap(env, _image, info);
	assert(pixels != nullptr);
//	Mat image(info.height, info.width, CV_8UC4, pixels);

	const int length = info.height * info.width;
	uint8_t red   = ((_color >> 16) &0xFF);
	uint8_t green = ((_color >> 8 ) &0xFF);
	uint8_t blue  = ((_color      ) &0xFF);
	const uint32_t color = red | (green << 8) | (blue << 16);

#ifndef NDEBUG
	bool trigged = false;
#endif

	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
	{
#ifndef NDEBUG
		uint8_t* p = reinterpret_cast<uint8_t*>(pixels + i);
		if(!trigged && (p[0] != p[1] || p[1] != p[2] || p[3] != 0xFF))
		{
			LOGW("#RGBA 0x%02X%02X%02X%02X, not a gray image!", p[0], p[1], p[2], p[3]);
			trigged = true;
		}
#endif
		pixels[i] = (pixels[i] & 0xff000000) | color;
	}
}

void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_getSymmetryAxis(JNIEnv* env,
		jclass clazz, jobjectArray _points, jobject _center, jobject _up)
{
	const std::vector<Point2f> points = getNativePointArray(env, _points);
	Vec4f line = Feature::getSymmetryAxis(points);

	Vec2f center(line[2], line[3]), up(line[0], line[1]);
	setJavaPoint(env, _center, center);
	setJavaPoint(env, _up, up);
}

void screen(cv::Mat& dst, const cv::Mat& src, const cv::Mat& mask, const cv::Point2i& position, float amount/* = 1.0f */)
{
	assert(src.channels() == 4 && dst.channels() == 4 && src.depth() == dst.depth());
	assert(mask.type() == CV_8UC1);

	Rect2i rect1(position.x, position.y, src.cols, src.rows);
	Rect2i rect2(0, 0, dst.cols, dst.rows);
	rect1 &= rect2;

	Rect2i rect_mask(0, 0, mask.cols, mask.rows);
	int offset_x = (src.cols - mask.cols)/2;
	int offset_y = (src.rows - mask.rows)/2;

	for(int r = rect1.y; r < (rect1.y + rect1.height); ++r)
	for(int c = rect1.x; c < (rect1.x + rect1.width); ++c)
	{
		int src_r = r - position.y, src_c = c - position.x;
		Point2i mask_position(src_c - offset_x, src_r - offset_y);
		if(!rect_mask.contains(mask_position) || mask.at<uchar>(mask_position) != 0)
			continue;

		const cv::Vec4b& src_color = src.at<cv::Vec4b>(src_r, src_c);

		cv::Vec4b& dst_color = dst.at<cv::Vec4b>(r, c);
		const int src_alpha = cvRound(src_color[3] * amount), isc_alpha = 255 - src_alpha;

		for(int i = 0; i < 3; ++i)
		{
			int sc = (src_color[i] * src_alpha + 127)/255;
			int dc = (dst_color[i] * isc_alpha + 127)/255;
			dst_color[i] = sc + dc - (2*sc*dc + 127)/255;
		}
	}
}

void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeBlendIris(JNIEnv* env,
		jclass clazz, jobject _image, jobject _iris, jobjectArray _points, jfloat amount)
{
	const std::vector<Point2f> points = getNativePointArray(env, _points);

	// iris right index 42， left index 43
	const int iris_indices_r[] = {35, 37, 39, 41};
	const int iris_indices_l[] = {45, 47, 49, 51};
	float iris_radius_r = distance(points[42], points[iris_indices_r[0]]);
	for(size_t i = 1; i < NELEM(iris_indices_r); ++i)
	{
		float dist = distance(points[42], points[iris_indices_r[i]]);
		if(iris_radius_r > dist)
			iris_radius_r = dist;
	}

	float iris_radius_l = distance(points[43], points[iris_indices_l[0]]);
	for(size_t i = 1; i < NELEM(iris_indices_l); ++i)
	{
		float dist = distance(points[43], points[iris_indices_l[i]]);
		if(iris_radius_l > dist)
			iris_radius_l = dist;
	}

	AndroidBitmapInfo iris_info;
	uint32_t* iris_pixels = lockJavaBitmap(env, _iris, iris_info);
	assert(iris_pixels != nullptr);
	Mat iris, iris_(iris_info.height, iris_info.width, CV_8UC4, iris_pixels);
	iris_.convertTo(iris, CV_32FC4, 1/255.0);
	unlockJavaBitmap(env, _iris);

	const Vec3f FROM_COLOR(1.0f, 1.0f, 1.0f);
	cvtColor(iris, iris, CV_BGR2BGRA);
	for(int r = 0; r < iris.rows; ++r)
	for(int c = 0; c < iris.cols; ++c)
	{
		Vec4f& color = iris.at<Vec4f>(r, c);
		color = venus::color2alpha(color, FROM_COLOR);
	}
	iris.convertTo(iris, CV_8UC4, 255.0);

	AndroidBitmapInfo image_info;
	uint32_t* image_pixels  = lockJavaBitmap(env, _image, image_info);
	assert(image_pixels != nullptr);
	Mat image(image_info.height, image_info.width, CV_8UC4, image_pixels);

	// TODO it seems this value fit every iris image in res/drawable/
	const float iris_image_radius = (157.5f - 4.5f)/2;//100;  // (84.0f - 12.0f)/2;

	Mat iris_r, iris_l;
	float iris_ratio_r = iris_radius_r / iris_image_radius;
	float iris_ratio_l = iris_radius_l / iris_image_radius;
	cv::resize(iris, iris_r, Size(), iris_ratio_r, iris_ratio_r);
	cv::resize(iris, iris_l, Size(), iris_ratio_l, iris_ratio_l);

	const Point2f& iris_center_r = points[42];
	const Point2f& iris_center_l = points[43];
	Point2i iris_position_r(iris_center_r.x - (iris_r.cols >> 1), iris_center_r.y - (iris_r.rows >> 1));
	Point2i iris_position_l(iris_center_l.x - (iris_l.cols >> 1), iris_center_l.y - (iris_l.rows >> 1));

	RoiInfo eye_info_r = calcuateEyeRegionInfo_r(points);
	RoiInfo eye_info_l = calcuateEyeRegionInfo_l(points);

	// used to use blend, now screen
	screen(image, iris_r, eye_info_r.mask, iris_position_r, amount);
	screen(image, iris_l, eye_info_l.mask, iris_position_l, amount);

//	dump("iris.png", result);
	unlockJavaBitmap(env, _image);
}

void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeBlendIris2(JNIEnv* env,
		jclass clazz, jobject _image, jobject _iris, jobject _iris_mask, jobjectArray _points, jint _color, jfloat amount)
{
	const std::vector<Point2f> points = getNativePointArray(env, _points);

	AndroidBitmapInfo image_info;
	uint32_t* image_pixels  = lockJavaBitmap(env, _image, image_info);
	assert(image_pixels != nullptr);
	Mat image(image_info.height, image_info.width, CV_8UC4, image_pixels);

	AndroidBitmapInfo iris_info;
	uint32_t* iris_pixels = lockJavaBitmap(env, _iris, iris_info);
	assert(iris_pixels != nullptr);
	Mat iris(iris_info.height, iris_info.width, CV_8UC4, iris_pixels);

	AndroidBitmapInfo iris_mask_info;
	uint32_t* iris_mask_pixels = lockJavaBitmap(env, _iris_mask, iris_mask_info);
	assert(iris_mask_pixels != nullptr);
	assert(iris_info.height == iris_mask_info.height && iris_info.width == iris_mask_info.width);
//	Mat iris_mask(iris_mask_info.height, iris_mask_info.width, CV_8UC4, iris_mask_pixels);

//	uint8_t alpha = ((_color >> 24) &0xFF);
	uint8_t red   = ((_color >> 16) &0xFF);
	uint8_t green = ((_color >> 8 ) &0xFF);
	uint8_t blue  = ((_color      ) &0xFF);
/*
	const int length = iris_info.height * iris_info.width;
	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
	{
		uint8_t alpha = iris_mask_pixels[i] >> 24;
		uint8_t r = ((iris_pixels[i] & 0xFF) * alpha + 127) / 255;
		uint8_t g = (((iris_pixels[i] >> 8)& 0xFF) * alpha + 127) / 255;
		uint8_t b = (((iris_pixels[i] >> 16)& 0xFF) * alpha + 127) / 255;
		uint8_t a = iris_pixels[i] & 0xFF000000;
		iris_pixels[i] = r | (g << 8) | (b << 16) | a;
	}
	dump("iris.png", iris);
*/
	// iris right index 42， left index 43
	const int iris_indices_r[] = {35, 37, 39, 41};
	const int iris_indices_l[] = {45, 47, 49, 51};
	float iris_radius_r = distance(points[42], points[iris_indices_r[0]]);
	for(size_t i = 1; i < NELEM(iris_indices_r); ++i)
	{
		float dist = distance(points[42], points[iris_indices_r[i]]);
		if(iris_radius_r > dist)
			iris_radius_r = dist;
	}

	float iris_radius_l = distance(points[43], points[iris_indices_l[0]]);
	for(size_t i = 1; i < NELEM(iris_indices_l); ++i)
	{
		float dist = distance(points[43], points[iris_indices_l[i]]);
		if(iris_radius_l > dist)
			iris_radius_l = dist;
	}

	const float iris_image_radius = 100;  // (84.0f - 12.0f)/2;

	Mat iris_r, iris_l;
	float iris_ratio_r = iris_radius_r / iris_image_radius;
	float iris_ratio_l = iris_radius_l / iris_image_radius;
	cv::resize(iris, iris_r, Size(), iris_ratio_r, iris_ratio_r);
	cv::resize(iris, iris_l, Size(), iris_ratio_l, iris_ratio_l);

	const Point2f& iris_center_r = points[42];
	const Point2f& iris_center_l = points[43];
	Point2i iris_position_r(iris_center_r.x - (iris_r.cols >> 1), iris_center_r.y - (iris_r.rows >> 1));
	Point2i iris_position_l(iris_center_l.x - (iris_l.cols >> 1), iris_center_l.y - (iris_l.rows >> 1));

	RoiInfo eye_info_r = calcuateEyeRegionInfo_r(points);
	RoiInfo eye_info_l = calcuateEyeRegionInfo_l(points);

	Makeup::blend(image, iris_r, eye_info_r.mask, iris_position_r, amount);
	Makeup::blend(image, iris_l, eye_info_l.mask, iris_position_l, amount);

	unlockJavaBitmap(env, _iris_mask);
	unlockJavaBitmap(env, _iris);
	unlockJavaBitmap(env, _image);
}

void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeBlendBlusher(JNIEnv* env,
		jclass clazz, jobject _image, jobject _blush, jobjectArray _points, jint color, jfloat amount)
{
	const std::vector<Point2f> points = getNativePointArray(env, _points);

	AndroidBitmapInfo blush_info;
	uint32_t* blush_pixels = lockJavaBitmap(env, _blush, blush_info);
	assert(blush_pixels != nullptr);
	Mat blush(blush_info.height, blush_info.width, CV_8UC4, blush_pixels);

	// Java doesn't have unsigned integer types, here we need reinterpret_cast.
//	const uint32_t color = *reinterpret_cast<const uint32_t*>(&_color);
//	const float amount = (color >> 24) / 255.0f;
//	const uint8_t  alpha = static_cast<uint8_t>(color >> 24);  // #ARGB

	const int length = blush_info.height * blush_info.width;
	#pragma omp parallel for
	for(int i = 0; i < length; ++i)
	{
#ifndef NDEBUG
		static bool trigged = false;
		uint8_t* p = reinterpret_cast<uint8_t*>(blush_pixels + i);
		if(!trigged && (p[0] != p[1] || p[1] != p[2] || p[3] != 0xff))
		{
			LOGW("#RGBA 0x%02X%02X%02X%02X, not a gray image!", p[0], p[1], p[2], p[3]);
			trigged = true;
		}
#endif

		// Android's Color use AARRGGBB layout, while JNI use RRGGBBAA layout.
		// JPG gray image (single channel), pixel value 0xaa turns 0xffaaaaaa.
		// RGBA with little endian, 0xAABBGGRR: 0xffaaaaaa -> 0xaaffffff
		// combined with primary color rrggbb, we get 0xaarrggbb
		blush_pixels[i] = ((color >> 16) & 0xff) | (color &0x00ff00) | ((color & 0xff) << 16) | ((blush_pixels[i] & 0xff) << 24);
	}

//	dump("blush.png", blush);

	// TODO haven't take screw face into consideration
	const float& blush_r_l = points[ 0].x, &blush_l_l = points[52].x;
	const float& blush_r_r = points[54].x, &blush_l_r = points[12].x;
	const float& blush_r_t = points[40].y, &blush_l_t = points[50].y;
	const float& blush_r_b = points[71].y, &blush_l_b = points[71].y;

	Mat blush_r, blush_l;
	float blush_x_scale = (blush_r_r - blush_r_l)/blush.cols;
	float blush_y_scale = (blush_r_b - blush_r_t)/blush.rows;
	cv::resize(blush, blush_r, Size(0, 0), blush_x_scale, blush_y_scale, INTER_CUBIC);

	Mat tmp;
	blush_x_scale = (blush_l_r - blush_l_l)/blush.cols;
	blush_y_scale = (blush_l_b - blush_l_t)/blush.rows;
	cv::resize(blush, tmp, Size(0, 0), blush_x_scale, blush_y_scale, INTER_CUBIC);
	cv::flip(tmp, blush_l, 1/* horizontally */);

	AndroidBitmapInfo image_info;
	uint32_t* image_pixels  = lockJavaBitmap(env, _image, image_info);
	assert(image_pixels != nullptr);
	Mat image(image_info.height, image_info.width, CV_8UC4, image_pixels);

	Makeup::blend(image, blush_r, Point2f(blush_r_l, blush_r_t), amount);
	Makeup::blend(image, blush_l, Point2f(blush_l_l, blush_l_t), amount);

//	dump("result.png", image);
	unlockJavaBitmap(env, _blush);
	unlockJavaBitmap(env, _image);
}

void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeBlendEyeBrow(JNIEnv* env,
		jclass clazz, jobject _image, jobject _eye_brow, jobjectArray _points, jobjectArray _eye_brow_points, jfloat amount)
{
	const std::vector<Point2f> points = getNativePointArray(env, _points);
/*
	// Note that user_points and reference_points are eye brow feature points
	std::vector<Point2f> reference_points = getNativePointArray(env, _eye_brow_points);
	assert(reference_points.size() == 6);

	std::vector<Point2f> user_points(reference_points.size());
	for(int i = 20; i <= 25; ++i)
		user_points[i - 20] = points[i];
*/
	const float WIDTH = 148 - 6;  // fixed image width
	float width = points[25].x - points[22].x;
	float scale = width / WIDTH;

	// right side eye brow in the image, that's left for you.
	AndroidBitmapInfo eye_brow_info;
	uint32_t* eye_brow_pixels = lockJavaBitmap(env, _eye_brow, eye_brow_info);
	assert(eye_brow_pixels != nullptr);
	cv::Mat eye_brow(eye_brow_info.height, eye_brow_info.width, CV_8UC4, eye_brow_pixels);

	cv::Mat eye_brow_r, eye_brow_l;
	cv::resize(eye_brow, eye_brow_r, Size(0, 0), scale, scale, INTER_LANCZOS4/*INTER_LINEAR*/);
	cv::flip(eye_brow_r, eye_brow_l, 1/* horizontally */);
	unlockJavaBitmap(env, _eye_brow);

	Point2f pivot_r, pivot_l;
	pivot_r.x = (points[20].x + points[21].x + points[23].x + points[24].x)/4;
	pivot_r.y = (points[20].y + points[21].y + points[23].y + points[24].y)/4;
	pivot_l.x = (points[27].x + points[28].x + points[30].x + points[31].x)/4;
	pivot_l.y = (points[27].y + points[28].y + points[30].y + points[31].y)/4;

	// 0.50, 0.42 these two value fit for all the eye brow images, @see doc/eye_brow.xcf
	// if you add more eye brow images, please follow the same values, otherwise
	// add a new field for each image.
	pivot_r.x -= eye_brow_r.cols * 0.50f; pivot_r.y -= eye_brow_r.rows * 0.42f;
	pivot_l.x -= eye_brow_l.cols * 0.50f; pivot_l.y -= eye_brow_l.rows * 0.42f;

	AndroidBitmapInfo image_info;
	uint32_t* image_pixels  = lockJavaBitmap(env, _image, image_info);
	assert(image_pixels != nullptr);
	Mat image(image_info.height, image_info.width, CV_8UC4, image_pixels);

	Makeup::blend(image, eye_brow_r, pivot_r, amount);
	Makeup::blend(image, eye_brow_l, pivot_l, amount);

	unlockJavaBitmap(env, _image);
}

/*
	Point2f pivot((top.x + bottom.x)/2, (left.y + right.y)/2);
//	float eye_lash_x_scale = (eye_lash_r.x - eye_lash_l.x) / (RIGHT.x - LEFT.x);
//	float eye_lash_y_scale = (eye_lash_b.y - eye_lash_t.y) / (BOTTOM.y - TOP.y);
//	cv::resize(eye_lash, pivot, Size(0, 0), eye_lash_x_scale, eye_lash_y_scale, INTER_LANCZOS4);
	float left_scale =
			pivot.x - left.x, right.x - pivot.x, pivot.y - top.y, bottom.y - pivot.y
	eye_lash_l = points[44]; eye_lash_r = points[48]; eye_lash_t = points[46]; eye_lash_b = points[50];
	pivot = Point2f((eye_lash_t.x + eye_lash_b.x)/2, (eye_lash_l.y + eye_lash_r.y)/2);
	eye_lash_x_scale = (eye_lash_r.x - eye_lash_l.x) / (RIGHT.x - LEFT.x);
	eye_lash_y_scale = (eye_lash_b.y - eye_lash_t.y) / (BOTTOM.y - TOP.y);
	cv::resize(eye_lash, eye_lash_l, Size(0, 0), eye_lash_x_scale, eye_lash_y_scale, INTER_LANCZOS4/ *INTER_LINEAR* /);
	cv::flip(eye_lash_l, eye_lash_l, 1/ * horizontally * /);


	cv::Point2f eye_lash_pivot_r((points[36].x + points[40])/2, (points[38].y + points[34].y)/2);
	cv::Point2f eye_lash_pivot_l((points[46].x + points[50])/2, (points[48].y + points[44].y)/2);
	venus::resize(eye_lash, eye_lash_r, (eye_lash_pivot_r.x - eye_lash_r_l)/);
*/

void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeBlendEyeLash(JNIEnv* env,
		jclass clazz, jobject _image, jobject _eye_lash, jobjectArray _points, jint color, jfloat amount)
{
	const std::vector<Point2f> points = getNativePointArray(env, _points);

	// right side eye lash in the image, that's left for you.
	AndroidBitmapInfo eye_lash_info;
	uint32_t* eye_lash_pixels = lockJavaBitmap(env, _eye_lash, eye_lash_info);
	assert(eye_lash_pixels != nullptr);
	Mat eye_lash(eye_lash_info.height, eye_lash_info.width, CV_8UC4, eye_lash_pixels);

	// tint eye lash with specified color
	color &= 0x00ffffff;  // RRGGBBAA
	if(color != 0)  // color BLACK does nothing, so skip this loop
	{
		const int length = eye_lash_info.height * eye_lash_info.width;
		#pragma omp parallel for
		for(int i = 0; i < length; ++i)
			eye_lash_pixels[i] = (eye_lash_pixels[i] & 0xff000000) | color;
	}

	// I've rearrange eye lashes into file doc/eye_lash.xcf
	const Point2f LEFT(284, 287), RIGHT(633, 287);
	const Point2f TOP(458, 213), BOTTOM(458, 362);

/*
				36                    46
			 37    35              45    47
	right  38   42   34 -------- 44   43   48   left
			 39    41              51    49
				40                    50
*/
	LOGI("H %f | %f", points[36].x - points[38].x, points[34].x - points[36].x);
	LOGI("V %f | %f", points[38].y - points[36].y, points[40].y - points[38].y);
	const Point2f& left_r(points[38]), &right_r(points[34]), &top_r(points[36]), &bottom_r(points[40]);
	const Point2f& left_l(points[44]), &right_l(points[48]), &top_l(points[46]), &bottom_l(points[50]);

	Point2f PIVOT, pivot_r, pivot_l;
	Vec4f DISTANCE = Feature::calcuateDistance(PIVOT, LEFT, RIGHT, TOP, BOTTOM);
	Vec4f distance_r = Feature::calcuateDistance(pivot_r, left_r, right_r, top_r, bottom_r);
	Vec4f distance_l = Feature::calcuateDistance(pivot_l, left_l, right_l, top_l, bottom_l);
	LOGI("pivot_r = (%f, %f), pivot_l = (%f, %f)", pivot_r.x, pivot_r.y, pivot_l.x, pivot_l.y);
	Vec4f scale_r, scale_l;
	for(int i = 0; i < 4; ++i)  // no operator / overloaded for Vec4f.
		scale_r[i] = distance_r[i] / DISTANCE[i];
	LOGI("right lash scale left: %f, right: %f, top: %f, bottom: %f", scale_r[0], scale_r[1], scale_r[2], scale_r[3]);
	// use INTER_LANCZOS4 instead of INTER_LINEAR for best anti-aliasing result
	Mat eye_lash_r = Region::resize(eye_lash, PIVOT, scale_r, INTER_LANCZOS4);

	// mirror left and right side for left eye lash
	std::swap(DISTANCE[0], DISTANCE[1]);
	PIVOT.x = static_cast<float>(eye_lash_info.width - 1) - PIVOT.x;  // left + right == width - 1
	cv::flip(eye_lash, eye_lash, 1/* horizontally */);

	for(int i = 0; i < 4; ++i)
		scale_l[i] = distance_l[i] / DISTANCE[i];
	LOGI("left lash scale left: %f, right: %f, top: %f, bottom: %f", scale_l[0], scale_l[1], scale_l[2], scale_l[3]);
	Mat eye_lash_l = Region::resize(eye_lash, PIVOT, scale_l, INTER_LANCZOS4);

	AndroidBitmapInfo image_info;
	uint32_t* image_pixels  = lockJavaBitmap(env, _image, image_info);
	assert(image_pixels != nullptr);
	Mat image(image_info.height, image_info.width, CV_8UC4, image_pixels);

	// make pivot coincides
	Point2f position_r(left_r.x - (eye_lash_r.cols - (right_r.x - left_r.x))/2,
			top_r.y - (eye_lash_r.rows - (bottom_r.y - top_r.y))/2);
	Point2f position_l(left_l.x - (eye_lash_l.cols - (right_l.x - left_l.x))/2,
			top_l.y - (eye_lash_l.rows - (bottom_l.y - top_l.y))/2);
	LOGI("(%f, %f)", position_r.x, position_r.y);
	LOGI("(%f, %f)", position_l.x, position_l.y);

	// rotate if skew too much

	Makeup::blend(image, eye_lash_r, position_r, amount);
	Makeup::blend(image, eye_lash_l, position_l, amount);

//	dump("result.png", image);
	unlockJavaBitmap(env, _eye_lash);
	unlockJavaBitmap(env, _image);
}

void JNICALL Java_com_cloudream_ishow_algorithm_FaceDetector_nativeSquare(JNIEnv* env,
		jclass clazz, jobject _bitmap, jobjectArray _points, jlong time_ms)
{
	// can cache feature points, no need to fetch them every time.
	const std::vector<Point2f> points = getNativePointArray(env, _points);
	const Rect2i region = cv::boundingRect(points);

	AndroidBitmapInfo bitmap_info;
	uint32_t* pixels = lockJavaBitmap(env, _bitmap, bitmap_info);
	assert(pixels != nullptr);
#if 0
	for(uint32_t j = 0; j < info.height; ++j)
	for(uint32_t i = 0; i < info.width; ++i)
	{
		uint32_t& pixel = pixels[j * info.width + i];
		uint8_t r = static_cast<uint8_t>((pixel & 0x000000FF)>>0);
		uint8_t g = static_cast<uint8_t>((pixel & 0x0000FF00)>>8);
		uint8_t b = static_cast<uint8_t>((pixel & 0x00FF0000)>>16);
		uint8_t a = static_cast<uint8_t>((pixel & 0xFF000000)>>24);
		r += 7;
		g += 11;
		b += 13;
		pixel = r | (g<<8) | (b<<16) | (a<<24);
	}
#endif

	cv::Mat image(bitmap_info.height, bitmap_info.width, CV_8UC4, pixels);

	unlockJavaBitmap(env, _bitmap);
}



