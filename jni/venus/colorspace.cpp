#include "venus/colorspace.h"
#include "venus/scalar.h"
#include "venus/compiler.h"

#include <algorithm>

static constexpr float HSL_UNDEFINED = -1.0F;

namespace venus {

void rgb2hsv(const float* rgb, float* hsv)
{
	const float &r = rgb[0], &g = rgb[1], &b = rgb[2];
	float &h = hsv[0], &s = hsv[1], &v = hsv[2];

	float min, max;
	minmax(min, max, rgb[0], rgb[1], rgb[2]);

	v = max;
	float delta = max - min;
	if(delta > 0.0001F)
    {
		s = delta / max;

		if(r == max)
        {
			h = (g - b) / delta;
			if(h < 0.0F)
				h += 6.0F;
		}
		else if(g == max)
			h = 2.0F + (b - r) / delta;
		else
			h = 4.0F + (r - g) / delta;
		
		h /= 6;
	}
	else
	{
		s = 0.0F;
		h = 0.0F;
    }
}

void hsv2rgb(const float* hsv, float* rgb)
{
	const float& h = hsv[0], &s = hsv[1], &v = hsv[2];
	float &r = rgb[0], &g = rgb[1], &b = rgb[2];

	if(s == 0.0F)
		b = g = r = v;
	else
    {
		float hue = h;
		if(hue == 1.0F)
			hue = 0.0F;
		hue *= 6.0F;

		int i = static_cast<int>(hue);
		float f = hue - i;
		float w = v * (1.0F - s);
		float q = v * (1.0F - (s * f));
		float t = v * (1.0F - (s * (1.0F - f)));

		switch(i)
		{
		case 0: r = v; g = t; b = w; break;
		case 1: r = q; g = v; b = w; break;
		case 2: r = w; g = v; b = t; break;
		case 3: r = w; g = q; b = v; break;
		case 4: r = t; g = w; b = v; break;
		case 5: r = v; g = w; b = q; break;
		}
	}
}

void rgb2hsl(const float* rgb, float* hsl)
{
	const float &r = rgb[0], &g = rgb[1], &b = rgb[2];
	float& h = hsl[0], &s = hsl[1], &l = hsl[2];

	float min, max;
	minmax(min, max, r, g, b);

	l = (max + min) / 2;
	if(max == min)
	{
		s = 0.0F;
		h = HSL_UNDEFINED;
	}
	else
	{
		if(l <= 0.5F)
			s = (max - min) / (max + min);
		else
			s = (max - min) / (2.0F - max - min);

		float delta = max - min;
		if(delta == 0.0F)
			delta = 1.0F;

		if(r == max)
			h = (g - b) / delta;
		else if(g == max)
			h = 2.0F + (b - r) / delta;
		else
			h = 4.0F + (r - g) / delta;

		h /= 6;
		if(h < 0.0F)
			h += 1.0F;
	}
}

static inline float hsl_value(float n1, float n2, float hue)
{
	float val;

	if (hue > 6.0F)
		hue -= 6.0F;
	else if (hue < 0.0F)
		hue += 6.0F;

	if (hue < 1.0F)
		val = n1 + (n2 - n1) * hue;
	else if (hue < 3.0F)
		val = n2;
	else if (hue < 4.0F)
		val = n1 + (n2 - n1) * (4.0F - hue);
	else
		val = n1;

	return val;
}

void hsl2rgb(const float* hsl, float* rgb)
{
	const float& h = hsl[0], &s = hsl[1], &l = hsl[2];
	float &r = rgb[0], &g = rgb[1], &b = rgb[2];

	if(s == 0.0F)  // achromatic case
		b = g = r = l;
	else
	{
		float m2;
		if (l <= 0.5F)
			m2 = l * (1.0F + s);
		else
			m2 = l + s - l * s;
		
		float m1 = 2.0F * l - m2;

		r = hsl_value(m1, m2, h * 6.0F + 2.0F);
		g = hsl_value(m1, m2, h * 6.0F);
		b = hsl_value(m1, m2, h * 6.0F - 2.0F);
    }
}

void rgb2cmyk(const float* rgb, const float& pullout, float* cmyk)
{
	const float &r = rgb[0], &g = rgb[1], &b = rgb[2];
	float &c = cmyk[0], &m = cmyk[1], &y = cmyk[2], &k = cmyk[3];

	c = 1.0F - r;
	m = 1.0F - g;
	y = 1.0F - b;
	k = 1.0F;

	if(c < k)  k = c;
	if(m < k)  k = m;
	if(y < k)  k = y;

	k *= pullout;
	if(k < 1.0F)
	{
		c = (c - k) / (1.0F - k);
		m = (m - k) / (1.0F - k);
		y = (y - k) / (1.0F - k);
	}
	else
	{
		c = 0.0F;
		m = 0.0F;
		y = 0.0F;
	}
}

void cmyk2rgb(const float* cmyk, float* rgb)
{
	const float &c = cmyk[0], &m = cmyk[1], &y = cmyk[2], &k = cmyk[3];
	float &r = rgb[0], &g = rgb[1], &b = rgb[2];

	if(k < 1.0F)
	{
		r = 1.0F - c * (1.0F - k) - k;
		g = 1.0F - m * (1.0F - k) - k;
		b = 1.0F - y * (1.0F - k) - k;
	}
	else
	{
		r = 0.0F;
		g = 0.0F;
		b = 0.0F;
	}
}

void color2alpha(const float* color, const float* src, float* dst)
{
	float alpha[4];

	if(src != dst)
		for(int i = 0; i < 4; ++i)
			dst[i] = src[i];

	alpha[3] = dst[3];
	const float EPS = 0.00001F;
	for(int i = 0; i < 3; ++i)
	{
		if(color[i] < EPS)
			alpha[i] = dst[i];
		else if(dst[i] > color[i] + EPS)
			alpha[i] = (dst[i] - color[i]) / (1.0F - color[i]);
		else if(dst[i] < color[i] - EPS)
			alpha[i] = (color[i] - dst[i]) / color[i];
		else //(dst[i] == color[i])
			alpha[i] = 0.0F;
	}

	// alpha3 = max(alpha[0], alpha[1], alpha[2])
	dst[3] = std::max(std::max(alpha[0], alpha[1]), alpha[2]);

	if(dst[3] < EPS)
		return;
	
	for(int i = 0; i < 3; ++i)
		dst[i] = (dst[i] - color[i]) / dst[3] + color[i];

	dst[3] *= alpha[3];
}


} /* namespace venus */