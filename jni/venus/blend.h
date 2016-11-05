#ifndef VENUS_BLEND_H_
#define VENUS_BLEND_H_

#include <stdint.h>
#include <algorithm>

#include <opencv2/core.hpp>

#include "venus/compiler.h"
#include "venus/scalar.h"

/* 
	some useful links:
	http://stackoverflow.com/questions/5919663/how-does-photoshop-blend-two-images-together
	http://www.deepskycolors.com/archive/2010/04/21/formulas-for-Photoshop-blending-modes.html
	https://en.wikipedia.org/wiki/Blend_modes
	https://helpx.adobe.com/photoshop/using/blending-modes.html
	https://en.wikipedia.org/wiki/Alpha_compositing
*/

namespace venus {

template <typename T>
inline T color_round(const T& x)
{
	static_assert(std::is_floating_point<T>::value, "limited to floating-point type only");
	assert(0 <= x && x <= 1);
	return x;
}

template <>
inline uint8_t color_round<uint8_t>(const uint8_t& x)
{
	constexpr uint8_t FULL = std::numeric_limits<uint8_t>::max();
	return (x + (FULL>>1)) / FULL;
}

template <typename T, int N = 4>
cv::Vec<T, N> mix(const cv::Vec<T, N>& dst, const cv::Vec<T, N>& src, float amount)
{
	cv::Vec<T, N> result;
	for(int i = 0; i < N; ++i)
		result[i] = lerp<T>(dst[i], src[i], amount);
	return result;
}

template<> cv::Vec3f mix<float,   3>(const cv::Vec3f& dst, const cv::Vec3f& src, float amount);
template<> cv::Vec4b mix<uint8_t, 4>(const cv::Vec4b& dst, const cv::Vec4b& src, float amount);
template<> cv::Vec4f mix<float,   4>(const cv::Vec4f& dst, const cv::Vec4f& src, float amount);


/**
 * It's like lerp but use uint32_t for 4 channels.
 */
uint32_t mix(const uint32_t& dst, const uint32_t& src, float amount);

/* for other integers type
template <>
inline Integer color_divide(const Integer& x)
{
	constexpr Integer FULL = std::numeric_limits<Integer>::max();
	return (x + (FULL>>1)) / FULL;
}
*/

// gimp/app/actions/layers-commands.c
// int mode range [0, 255], float mode range [0.0, 1.0]

template<typename T> inline T normal    (const T& dst, const T& src) { return src; }
template<typename T> inline T lighten   (const T& dst, const T& src) { return std::max(src, dst); }
template<typename T> inline T darken    (const T& dst, const T& src) { return std::min(src, dst); }
template<typename T> inline T multiply	(const T& dst, const T& src) { return color_round(src * dst); }
template<typename T> inline T average	(const T& dst, const T& src) { return (src + dst + std::is_integral<T>::value)/2; }
template<typename T> inline T add		(const T& dst, const T& src) { return cv::saturate_cast<T>(src + dst); }
template<typename T> inline T subtract	(const T& dst, const T& src) { constexpr T FULL = std::is_floating_point<T>::value ? static_cast<T>(1):std::numeric_limits<T>::max(); return cv::saturate_cast<T>(src + dst - FULL); }
template<typename T> inline T difference(const T& dst, const T& src) { return std::abs(src - dst); }
template<typename T> inline T screen	(const T& dst, const T& src) { return src + dst - color_round(src * dst); }  // 1 - (1-src) * (1-dst) 
template<typename T> inline T exclusion	(const T& dst, const T& src) { return src + dst - color_round(src * dst * 2); }

template<typename T>
inline T negation(const T& dst, const T& src)
{
	auto sum = src + dst;
	constexpr T FULL = std::is_floating_point<T>::value ? static_cast<T>(1):std::numeric_limits<T>::max();
//	return (sum <= FULL) ? sum:(FULL*2 - sum);  // already full, not work for 32 bit int
	return FULL - std::abs(src + dst - FULL);
}

template<typename T>
inline T overlay(const T& dst, const T& src)
{
	constexpr T FULL = std::is_floating_point<T>::value ? static_cast<T>(1):std::numeric_limits<T>::max();
	if(src < FULL/2)
		return (2 * src * dst);
	else
		return (FULL - 2*(FULL - src)*(FULL - dst));
}

inline float softDodge(float dst, float src) 
{
	if(src + dst < 1.0f)
		return 0.5f*src / (1.0f - dst);
	else
		return 1.0f - 0.5f*(1.0f - dst)/src;
}



// specialization for cv::Vec4f color with RGBA layout
template<>
inline cv::Vec4f screen<cv::Vec4f>(const cv::Vec4f& src, const cv::Vec4f& dst)
{
	cv::Vec4f result;
	for(int i = 0; i < 3; ++i)  // keep alpha untouched
		result[i] = screen(src[i], dst[i]);
	return result;
}


// https://developer.android.com/reference/android/graphics/PorterDuffXfermode.html

inline void clear(const cv::Vec4f& src, cv::Vec4f& dst)
{
	dst = cv::Vec4f::all(0.0f);
}



// PorterDuff.Mode 	SRC 	[Sa, Sc]  
inline void src(const cv::Vec4f& src, cv::Vec4f& dst)
{
	dst = src;
}

// PorterDuff.Mode 	DST 	[Da, Dc]  
inline void dst(const cv::Vec4f& _src, cv::Vec4f& dst)
{
	
}

// PorterDuff.Mode 	SRC_IN 	[Sa * Da, Sc * Da]  
inline void src_in(const cv::Vec4f& src, cv::Vec4f& dst)
{
	float alpha = dst[3];
	dst = alpha * src;
	//dst[0] = src[0] * dst[3];
	//dst[1] = src[1] * dst[3];
	//dst[2] = src[2] * dst[3];
	//dst[3] = src[3] * dst[3];
}

// PorterDuff.Mode 	DST_IN 	[Sa * Da, Sa * Dc]  
inline void dst_in(const cv::Vec4f& src, cv::Vec4f& dst)
{
//	dst *= src[3];
	dst[0] *= src[3];
	dst[1] *= src[3];
	dst[2] *= src[3];
	dst[3] *= src[3];
}

// PorterDuff.Mode 	SRC_OUT 	[Sa * (1 - Da), Sc * (1 - Da)] 
inline void src_out(const cv::Vec4f& src, cv::Vec4f& dst)
{
	const float ida = 1.0f - dst[3];
	dst[0] = src[0] * ida;
	dst[1] = src[1] * ida;
	dst[2] = src[2] * ida;
	dst[3] = src[3] * dst[3];
}

// PorterDuff.Mode 	DST_OUT 	[Da * (1 - Sa), Dc * (1 - Sa)]  
inline void dst_out(const cv::Vec4f& src, cv::Vec4f& dst)
{
	const float ida = 1.0f - src[3];
	dst = src * ida;
}

// PorterDuff.Mode 	MULTIPLY 	[Sa * Da, Sc * Dc]  
inline void multiply(const cv::Vec4f& src, cv::Vec4f& dst)
{
//	dst *= src;
	dst[0] *= src[0];
	dst[1] *= src[1];
	dst[2] *= src[2];
	dst[3] = src[3] * dst[3];
}

} /* namespace venus */
#endif /* VENUS_BLEND_H_ */