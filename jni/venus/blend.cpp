#include "venus/blend.h"
#include "venus/colorspace.h"
#include "venus/Scalar.h"

namespace venus {

uint32_t mix(const uint32_t& dst, const uint32_t& src, float amount)
{
	assert(0 <= amount && amount <= 1.0f);
	uint32_t r = lerp(src & 0xff,         dst & 0xff, amount);
	uint32_t g = lerp((src >>  8) & 0xff, (dst >>  8) & 0xff, amount);
	uint32_t b = lerp((src >> 16) & 0xff, (dst >> 16) & 0xff, amount);
//	uint32_t a = lerp((src >> 24) & 0xff, (dst >> 24) & 0xff, amount);
//	return (r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16) || ((a & 0xff) << 24);
	return (r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16) || (src & 0xff000000);
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
	int src_a = static_cast<int>(src[3] * amount);
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