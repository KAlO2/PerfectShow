#include "venus/blend.h"
#include "venus/colorspace.h"
#include "venus/scalar.h"

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

cv::Vec3b mix(const cv::Vec3b& from, const cv::Vec3b& to, float amount)
{
	cv::Vec3b result;
	int a = cvRound(255 * amount);
	int l_a = 255 - a;

	for(int i = 0; i < 3; ++i)
		result[i] = (from[i] * l_a + to[i] * a + 127) / 255;
	return result;
}

cv::Vec3f mix(const cv::Vec3f& from, const cv::Vec3f& to, float amount)
{
	cv::Vec3f result;
	float l_amount = 1.0F - amount;
	for(int i = 0; i < 3; ++i)
		result[i] = from[i] * l_amount + to[i] * amount;
	return result;
}

cv::Vec4b mix(const cv::Vec4b& from, const cv::Vec4b& to, float amount)
{
	cv::Vec4b result;
	int a = cvRound(to[3] * amount);
	int l_a = 255 - a;

	for(int i = 0; i < 3; ++i)
		result[i] = (from[i] * l_a + to[i] * a + 127) / 255;
	result[3] = from[3];
	return result;
}

cv::Vec4f mix(const cv::Vec4f& from, const cv::Vec4f& to, float amount)
{
	cv::Vec4f result;
	float a = to[3] * amount;
	float l_a = 1.0F - a;

	for(int i = 0; i < 3; ++i)
		result[i] = (from[i] * l_a + to[i] * a);
	result[3] = from[3];
	return result;
}

#define blendHSL(i)          \
	float hslA[3], hslB[3];	 \
	rgb2hsl(rgbA, hslA);     \
	rgb2hsl(rgbB, hslB);     \
	hslA[i] = hslB[i];       \
	hsl2rgb(hslA, target);   \


void blendHue       (float* target, const float* rgbA, const float* rgbB)  { blendHSL(0) }
void blendSaturation(float* target, const float* rgbA, const float* rgbB)  { blendHSL(1) }
void blendLuminosity(float* target, const float* rgbA, const float* rgbB)  { blendHSL(2) }
void blendColor     (float* target, const float* rgbA, const float* rgbB)
{
	float hslA[3], hslB[3];
	rgb2hsl(rgbA, hslA);
	rgb2hsl(rgbB, hslB);
	hslB[0] = hslA[0];
	hslB[1] = hslA[1];
	hsl2rgb(hslB, target);
}

} /* namespace venus */
