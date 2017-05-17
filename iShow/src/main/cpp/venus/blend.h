#ifndef VENUS_BLEND_H_
#define VENUS_BLEND_H_

#include <stdint.h>
#include <algorithm>

#include <opencv2/core.hpp>

#include "venus/compiler.h"
#include "venus/scalar.h"

/* 
	some useful links:
	https://en.wikipedia.org/wiki/Blend_modes and other valuable references at the bottom page.
	https://en.wikipedia.org/wiki/Alpha_compositing

	http://stackoverflow.com/questions/5919663/how-does-photoshop-blend-two-images-together
	http://www.deepskycolors.com/archive/2010/04/21/formulas-for-Photoshop-blending-modes.html
	https://helpx.adobe.com/photoshop/using/blending-modes.html
*/

namespace venus {
/*
template <typename T, int N = 3>
cv::Vec<T, N> mix(const cv::Vec<T, N>& from, const cv::Vec<T, N>& to, float amount)
{
	cv::Vec<T, N> result;
	for(int i = 0; i < N; ++i)
		result[i] = lerp<T>(from[i], to[i], amount);
	return result;
}

template <typename T>
cv::Vec<T, 4> mix(const cv::Vec<T, 4>& from, const cv::Vec<T, 4>& to, float amount)
{
	cv::Vec<T, 4> result;
	if(std::is_floating_point<T>::value)
		amount *= to[3];
	else
		amount *= static_cast<float>(to[3]) / std::numeric_limits<T>::max();

	for(int i = 0; i < 3; ++i)
		result[i] = lerp<T>(from[i], to[i], amount);
	result[3] = from[3];  // keep alpha untouched
	return result;
}
*/

cv::Vec3b mix(const cv::Vec3b& from, const cv::Vec3b& to, float amount);
cv::Vec3f mix(const cv::Vec3f& from, const cv::Vec3f& to, float amount);
cv::Vec4b mix(const cv::Vec4b& from, const cv::Vec4b& to, float amount);
cv::Vec4f mix(const cv::Vec4f& from, const cv::Vec4f& to, float amount);


/**
 * It's like lerp but use uint32_t for 4 channels.
 */
uint32_t mix(const uint32_t& from, const uint32_t& to, float amount);



// gimp/app/actions/layers-commands.c
// int mode range [0, 255], float mode range [0.0, 1.0]



// blend mode for uint8_t version
inline uint8_t blendNormal     (uint8_t A, uint8_t B)  { return A; }
inline uint8_t blendLighten    (uint8_t A, uint8_t B)  { return std::max(A, B); }
inline uint8_t blendDarken     (uint8_t A, uint8_t B)  { return std::min(A, B); }
inline uint8_t blendMultiply   (uint8_t A, uint8_t B)  { return (A * B + 127) / 255; }
inline uint8_t blendAverage    (uint8_t A, uint8_t B)  { return (A + B + 1) / 2; }
inline uint8_t blendAdd        (uint8_t A, uint8_t B)  { return std::min(255, A + B); }
inline uint8_t blendSubtract   (uint8_t A, uint8_t B)  { return std::max(0, A + B - 255); }
inline uint8_t blendDifference (uint8_t A, uint8_t B)  { return std::abs(A - B); }
inline uint8_t blendNegation   (uint8_t A, uint8_t B)  { return 255 - std::abs(255 - A - B); }
inline uint8_t blendScreen     (uint8_t A, uint8_t B)  { return 255 - ((255 - A) * (255 - B) + 127) / 255; }
inline uint8_t blendExclusion  (uint8_t A, uint8_t B)  { return A + B - (2 * A * B + 127)/ 255; }
inline uint8_t blendOverlay    (uint8_t A, uint8_t B)  { return (uint8_t)((B < 128 ? 2*A*B : (255 - 2*(255 - A)*(255 - B)) + 127)/ 255); }
inline uint8_t blendSoftLight  (uint8_t A, uint8_t B)  { return (B < 128) ? ((2*((A>>1)+64)) * B + 127)/255 : (255 - (2*(255-((A>>1)+64))*(255-B) + 127)/255); }
inline uint8_t blendHardLight  (uint8_t A, uint8_t B)  { return blendOverlay(B, A); }
inline uint8_t blendColorDodge (uint8_t A, uint8_t B)  { return (B == 255) ? 255 : std::min(255, A * 255 / (255 - B)); }
inline uint8_t blendColorBurn  (uint8_t A, uint8_t B)  { return (B ==   0) ?   0 : std::max(  0, 255 - (255 - A) * 255 / B); }
inline uint8_t blendLinearDodge(uint8_t A, uint8_t B)  { return blendAdd     (A, B); }
inline uint8_t blendLinearBurn (uint8_t A, uint8_t B)  { return blendSubtract(A, B); }
inline uint8_t blendLinearLight(uint8_t A, uint8_t B)  { return (B < 128) ? blendLinearBurn(A, 2 * B) : blendLinearDodge(A, 2 * (B - 128)); }
inline uint8_t blendVividLight (uint8_t A, uint8_t B)  { return (B < 128) ? blendColorBurn (A, 2 * B) : blendColorDodge (A, 2 * (B - 128)); }
inline uint8_t blendPinLight   (uint8_t A, uint8_t B)  { return (B < 128) ? blendDarken    (A, 2 * B) : blendLighten    (A, 2 * (B - 128)); }
inline uint8_t blendHardMix    (uint8_t A, uint8_t B)  { return blendVividLight(A, B) < 128 ? 0:255; }
inline uint8_t blendReflect    (uint8_t A, uint8_t B)  { return (B == 255) ? 255 : std::min(255, A*A / (255 - B)); }
inline uint8_t blendGlow       (uint8_t A, uint8_t B)  { return blendReflect(B, A); }
inline uint8_t blendPhoenix    (uint8_t A, uint8_t B)  { return std::min(A, B) - std::max(A, B) + 255; }
inline uint8_t blendAlpha      (uint8_t A, uint8_t B, uint8_t O)  { return lerp(B, A, O);/*O * A + (1 - O) * B*/ }
inline uint8_t blendAlphaF     (uint8_t A, uint8_t B, uint8_t (*F)(uint8_t, uint8_t), uint8_t O)  { return blendAlpha(F(A, B), A, O); }

// blend mode for float version
inline float blendNormal     (float A, float B)  { return A; }
inline float blendLighten    (float A, float B)  { return std::max(A, B); }
inline float blendDarken     (float A, float B)  { return std::min(A, B); }
inline float blendMultiply   (float A, float B)  { return A * B; }
inline float blendAverage    (float A, float B)  { return (A + B) / 2; }
inline float blendAdd        (float A, float B)  { return std::min(1.0F, A + B); }
inline float blendSubtract   (float A, float B)  { return std::max(0.0F, A + B - 1.0F); }
inline float blendDifference (float A, float B)  { return std::abs(A - B); }
inline float blendNegation   (float A, float B)  { return 1.0F - std::abs(1.0F - A - B); }
inline float blendScreen     (float A, float B)  { return 1.0F - (1.0F - A) * (1.0F - B); }
inline float blendExclusion  (float A, float B)  { return A + B - 2*A*B; }
inline float blendOverlay    (float A, float B)  { return (B <= 0.5F) ? (2*A*B) : (1.0F - 2*(1.0F - A)*(1.0F - B)); }
inline float blendSoftLight  (float A, float B)  { return (B <= 0.5F) ? (A + 0.5F)*B : (1.0F - (1.5F - A)*(1.0F - B)); }
inline float blendHardLight  (float A, float B)  { return blendOverlay(B, A); }
inline float blendColorDodge (float A, float B)  { return fuzzyEqual(B, 1.0F) ? B : std::min(1.0F, A * 1.0F / (1.0F - B)); }
inline float blendColorBurn  (float A, float B)  { return fuzzyEqual(B, 0.0F) ? B : std::max(0.0F, 1.0F - (1.0F - A) * 1.0F / B); }
inline float blendLinearDodge(float A, float B)  { return blendAdd     (A, B); }
inline float blendLinearBurn (float A, float B)  { return blendSubtract(A, B); }
inline float blendLinearLight(float A, float B)  { return (B <= 0.5F) ? blendLinearBurn(A, 2 * B) : blendLinearDodge(A, 2 * (B - 0.5F)); }
inline float blendVividLight (float A, float B)  { return (B <= 0.5F) ? blendColorBurn (A, 2 * B) : blendColorDodge (A, 2 * (B - 0.5F)); }
inline float blendPinLight   (float A, float B)  { return (B <= 0.5F) ? blendDarken    (A, 2 * B) : blendLighten    (A, 2 * (B - 0.5F)); }
inline float blendHardMix    (float A, float B)  { return blendVividLight(A, B) <= 0.5F ? 0:1.0F; }
inline float blendReflect    (float A, float B)  { return fuzzyEqual(B, 1.0F) ? B : std::min(1.0F, A*A / (1.0F - B)); }
inline float blendGlow       (float A, float B)  { return blendReflect(B, A); }
inline float blendPhoenix    (float A, float B)  { return std::min(A, B) - std::max(A, B) + 1.0F; }
inline float blendAlpha      (float A, float B, float O)  { return lerp(B, A, O);/*O * A + (1 - O) * B*/ }
inline float blendAlphaF     (float A, float B, float (*F)(float, float), float O)  { return blendAlpha(F(A, B), A, O); }

/*
 * @p target can be written directly to @p rgbA or @p rgbB. So you can write code like this:
 * <code>
 * blendColor(dst, src, dst);
 * </code>
 */
void blendHue       (float* target, const float* rgbA, const float* rgbB);
void blendSaturation(float* target, const float* rgbA, const float* rgbB);
void blendLuminosity(float* target, const float* rgbA, const float* rgbB);
void blendColor     (float* target, const float* rgbA, const float* rgbB);




} /* namespace venus */
#endif /* VENUS_BLEND_H_ */