#include "venus/blend.h"
#include "venus/colorspace.h"
#include "venus/Scalar.h"

namespace venus {

uint32_t mix(const uint32_t& from, const uint32_t& to, float amount)
{
	assert(0 <= amount && amount <= 1.0F);
	uint8_t _0 = lerp<uint8_t>((from      ) & 0xFF, (to      ) & 0xFF, amount);
	uint8_t _1 = lerp<uint8_t>((from >>  8) & 0xFF, (to >>  8) & 0xFF, amount);
	uint8_t _2 = lerp<uint8_t>((from >> 16) & 0xFF, (to >> 16) & 0xFF, amount);
//	uint8_t _3 = lerp<uint8_t>((from >> 24) & 0xFF, (to >> 24) & 0xFF, amount);
//	return _0 | (_1 << 8) | (_2 << 16) || (_3  << 24);
	return _0 | (_1 << 8) | (_2 << 16) || (from & 0xFF000000);
}

template <>
cv::Vec3f mix<float, 3>(const cv::Vec3f& dst, const cv::Vec3f& src, float amount)
{
	cv::Vec3f result;
	const float l_amount = 1 - amount;
	for(int i = 0; i < 3; ++i)
		result[i] = dst[i] * l_amount + src[i] * amount;
	return result;
}

template <>
cv::Vec4b mix<uint8_t, 4>(const cv::Vec4b& dst, const cv::Vec4b& src, float amount)
{
	cv::Vec4b result;
	int src_a = cvRound(src[3] * amount);
	int l_src_a = 255 - src_a;

	for(int i = 0; i < 3; ++i)
		result[i] = (dst[i] * l_src_a + src[i] * src_a + 127) / 255;
	result[3] = dst[3];
	
	return result;
}

template <>
cv::Vec4f mix<float, 4>(const cv::Vec4f& dst, const cv::Vec4f& src, float amount)
{
	cv::Vec4f result;
	float src_a = src[3] * amount;
	float l_src_a = 1.0f - src_a;

	for(int i = 0; i < 3; ++i)
		result[i] = (dst[i] * l_src_a + src[i] * src_a);
	result[3] = dst[3];
	return result;
}

#define blendHSL(i)          \
	float hslA[3], hslB[3];	 \
	rgb2hsl(rgbA, hslA);     \
	rgb2hsl(rgbB, hslB);     \
	hslA[i] = hslB[i];       \
	hsl2rgb(hslA, T);        \

void blendHue       (float* T, const float* rgbA, const float* rgbB)  { blendHSL(0) }
void blendSaturation(float* T, const float* rgbA, const float* rgbB)  { blendHSL(1) }
void blendLuminosity(float* T, const float* rgbA, const float* rgbB)  { blendHSL(2) }
void blendColor     (float* T, const float* rgbA, const float* rgbB)
{
	float hslA[3], hslB[3];
	rgb2hsl(rgbA, hslA);
	rgb2hsl(rgbB, hslB);
	hslB[0] = hslA[0];
	hslB[1] = hslA[1];
	hsl2rgb(hslB, T);
}

} /* namespace venus */