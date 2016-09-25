#include "color_space.h"

#include <algorithm>

#define HSV_UNDEFINED -1.0f
#define HSL_UNDEFINED -1.0f

namespace venus {

template<typename T>
void minmax(T& min, T& max, const T& _1, const T& _2, const T& _3)
{
	if(_1 < _2)
	{
		min = _1;
		max = _2;
	}
	else
	{
		min = _2;
		max = _1;
	}

	if(min > _3)
		min = _3;
	else if(max < _3)
		max = _3;
}

cv::Vec3f rgb2hsv(const cv::Vec3f& rgb)
{
	float min, max;
	minmax(min, max, rgb[0], rgb[1], rgb[2]);

	float h, s, v = max;
	float delta = max - min;
	if(delta > 0.0001f)
    {
		s = delta / max;

		if(rgb[0] == max)
        {
			h = (rgb[1] - rgb[2]) / delta;
			if(h < 0.0f)
				h += 6.0f;
		}
		else if(rgb[1] == max)
        {
			h = 2.0f + (rgb[2] - rgb[0]) / delta;
        }
		else
		{
			h = 4.0f + (rgb[0] - rgb[1]) / delta;
		}
		
		h /= 6;
	}
	else
	{
		s = 0.0f;
		h = 0.0f;
    }

	return cv::Vec3f(h, s, v);
}

cv::Vec3f hsv2rgb(const cv::Vec3f& hsv)
{
	const float& h = hsv[0], &s = hsv[1], &v = hsv[2];
	cv::Vec3f color;
	if(s == 0.0)
		color[2] = color[1] = color[0] = v;
	else
    {
		float hue = h;
		if(hue == 1.0f)
			hue = 0.0;
		hue *= 6.0f;

		int i = static_cast<int>(hue);
		float f = hue - i;
		float w = v * (1.0f - s);
		float q = v * (1.0f - (s * f));
		float t = v * (1.0f - (s * (1.0f - f)));

		switch(i)
		{
		case 0:
			color[0] = v;
			color[1] = t;
			color[2] = w;
			break;
		case 1:
			color[0] = q;
			color[1] = v;
			color[2] = w;
			break;
		case 2:
			color[0] = w;
			color[1] = v;
			color[2] = t;
			break;
		case 3:
			color[0] = w;
			color[1] = q;
			color[2] = v;
			break;
		case 4:
			color[0] = t;
			color[1] = w;
			color[2] = v;
			break;
		case 5:
			color[0] = v;
			color[1] = w;
			color[2] = q;
			break;
		}
	}

	return color;
}

cv::Vec3f rgb2hsl(const cv::Vec3f& rgb)
{
	float min, max;
	minmax(min, max, rgb[0], rgb[1], rgb[2]);

	float h, s, l = (max + min) / 2;

	if(max == min)
	{
		s = 0.0f;
		h = HSL_UNDEFINED;
	}
	else
	{
		if(l <= 0.5f)
			s = (max - min) / (max + min);
		else
			s = (max - min) / (2.0f - max - min);

		float delta = max - min;
		if(delta == 0.0f)
			delta = 1.0f;

		if(rgb[0] == max)
			h = (rgb[1] - rgb[2]) / delta;
		else if(rgb[1] == max)
			h = 2.0f + (rgb[2] - rgb[0]) / delta;
		else
			h = 4.0f + (rgb[0] - rgb[1]) / delta;

		h /= 6;
		if(h < 0.0f)
			h += 1.0f;
	}

	return cv::Vec3f(h, s, l);
}

static inline float hsl_value(float n1, float n2, float hue)
{
	float val;

	if (hue > 6.0f)
		hue -= 6.0f;
	else if (hue < 0.0f)
		hue += 6.0f;

	if (hue < 1.0f)
		val = n1 + (n2 - n1) * hue;
	else if (hue < 3.0f)
		val = n2;
	else if (hue < 4.0f)
		val = n1 + (n2 - n1) * (4.0f - hue);
	else
		val = n1;

	return val;
}

cv::Vec3f hsl2rgb(const cv::Vec3f& hsl)
{
	const float& h = hsl[0], &s = hsl[1], &l = hsl[2];
	cv::Vec3f rgb;
	if(s == 0.0f)  // achromatic case
		rgb[2] = rgb[1] = rgb[0] = l;
	else
	{
		float m2;
		if (l <= 0.5f)
			m2 = l * (1.0f + s);
		else
			m2 = l + s - l * s;
		
		float m1 = 2.0f * l - m2;

		rgb[0] = hsl_value (m1, m2, h * 6.0f + 2.0f);
		rgb[1] = hsl_value (m1, m2, h * 6.0f);
		rgb[2] = hsl_value (m1, m2, h * 6.0f - 2.0f);
    }

	return rgb;
}

cv::Vec4f rgb2cmyk(const cv::Vec3f& rgb, float pullout)
{
	float c = 1.0f - rgb[0];
	float m = 1.0f - rgb[1];
	float y = 1.0f - rgb[2];
	float k = 1.0f;

	if(c < k)  k = c;
	if(m < k)  k = m;
	if(y < k)  k = y;

	k *= pullout;
	if(k < 1.0f)
	{
		c = (c - k) / (1.0f - k);
		m = (m - k) / (1.0f - k);
		y = (y - k) / (1.0f - k);
	}
	else
	{
		c = 0.0f;
		m = 0.0f;
		y = 0.0f;
	}

	return cv::Vec4f(c, m, y, k);
}

cv::Vec3f cmyk2rgb(const cv::Vec4f& cmyk)
{
	const float &c = cmyk[0], &m = cmyk[1], &y = cmyk[2], &k = cmyk[3];
	cv::Vec3f color;

	if(k < 1.0f)
	{
		color[0] = 1.0f - c * (1.0f - k) - k;
		color[1] = 1.0f - m * (1.0f - k) - k;
		color[2] = 1.0f - y * (1.0f - k) - k;
	}
	else
	{
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
	}

	return color;
}

cv::Vec4f rgb2hsv(const cv::Vec4f& rgba)
{
	cv::Vec4f hsva;
	cv::Vec3f& hsv = *reinterpret_cast<cv::Vec3f*>(&hsva);
	hsv = rgb2hsv(*reinterpret_cast<const cv::Vec3f*>(&rgba));
	hsva[3] = rgba[3];

	return hsva;
}

cv::Vec4f hsv2rgb(const cv::Vec4f& hsva)
{
	cv::Vec4f rgba;
	cv::Vec3f& rgb = *reinterpret_cast<cv::Vec3f*>(&rgba);
	rgb = hsv2rgb(*reinterpret_cast<const cv::Vec3f*>(&hsva));
	rgba[3] = hsva[3];

	return rgba;
}

/*
 * An excerpt from a discussion on #gimp that sheds some light on the ideas
 * behind the algorithm that is being used here.
 *
 <clahey>   so if a1 > c1, a2 > c2, and a3 > c2 and a1 - c1 > a2-c2, a3-c3,
 then a1 = b1 * alpha + c1 * (1-alpha)
 So, maximizing alpha without taking b1 above 1 gives
 a1 = alpha + c1(1-alpha) and therefore alpha = (a1-c1) / (1-c1).
 <sjburges> clahey: btw, the ordering of that a2, a3 in the white->alpha didn't
 matter
 <clahey>   sjburges: You mean that it could be either a1, a2, a3 or
 a1, a3, a2?
 <sjburges> yeah
 <sjburges> because neither one uses the other
 <clahey>   sjburges: That's exactly as it should be.  They are both just
 getting reduced to the same amount, limited by the the darkest
 color.
 <clahey>   Then a2 = b2 * alpha + c2 * (1- alpha).  Solving for b2 gives
 b2 = (a1-c2)/alpha + c2.
 <sjburges> yeah
 <clahey>   That gives us are formula for if the background is darker than the
 foreground? Yep.
 <clahey>   Next if a1 < c1, a2 < c2, a3 < c3, and c1-a1 > c2-a2, c3-a3, and
 by our desired result a1 = b1 * alpha + c1 * (1-alpha),
 we maximize alpha without taking b1 negative gives
 alpha = 1 - a1 / c1.
 <clahey>   And then again, b2 = (a2-c2) / alpha + c2 by the same formula.
 (Actually, I think we can use that formula for all cases, though
 it may possibly introduce rounding error.
 <clahey>   sjburges: I like the idea of using floats to avoid rounding error.
 Good call.
*/
cv::Vec4f color2alpha(const cv::Vec4f& color, const cv::Vec3f& from)
{
	float alpha[4];
	cv::Vec4f dst(color);

	alpha[3] = dst[3];
	for(int i = 0; i < 3; ++i)
	{
		if(from[i] < 0.00001f)
			alpha[i] = dst[i];
		else if(dst[i] > from[i] + 0.00001f)
			alpha[i] = (dst[i] - from[i]) / (1.0f - from[i]);
		else if(dst[i] < from[i] - 0.00001f)
			alpha[i] = (from[i] - dst[i]) / (from[i]);
		else
			alpha[i] = 0.0f;
	}

	if(alpha[0] > alpha[1])
	{
		if(alpha[0] > alpha[2])
			dst[3] = alpha[0];
		else
			dst[3] = alpha[2];
	}
	else if (alpha[1] > alpha[2])
		dst[3] = alpha[1];
	else
		dst[3] = alpha[2];

	if(dst[3] >= 0.00001f)
	{
		for(int i = 0; i < 3; ++i)
			dst[i] = (dst[i] - color[i]) / dst[3] + color[i];

		dst[3] *= alpha[3];
	}

	return dst;
}


} /* namespace venus */