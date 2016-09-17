#ifndef MATH_VEC4_H_
#define MATH_VEC4_H_

#include "scalar.h"

#include <iostream>

namespace venus {

/**
 * https://software.intel.com/en-us/articles/optimized-matrix-library-for-use-with-the-intel-pentiumr-4-processors-sse2-instructions/
 */
template <typename T>
class vec4
{
public:
// https://msdn.microsoft.com/en-us/library/2c8f766e.aspx
#ifdef _WIN32
#  pragma warning(push)
#  pragma warning(disable: 4201)  // warning C4201: nonstandard extension used: nameless struct/union
#endif

	union
	{
		T _data[4];
		struct {T x, y, z, w;}; // 4D homogeneous coordinate
		struct {T r, g, b, a;}; // color
		struct {T s, t, p, q;}; // texture
	};

#ifdef _WIN32
#  pragma warning(pop)
#endif

	typedef T value_type;

public:
	vec4()  = default;
	~vec4() = default;

	explicit vec4(const T *array): x(array[0]), y(array[1]), z(array[2]), w(array[3]) { }
	explicit vec4(T scalar): x(scalar), y(scalar), z(scalar), w(scalar) { }
	vec4(T x, T y, T z, T w): x(x), y(y), z(z), w(w) { }

	vec4(const vec4<T>& v): x(v.x), y(v.y), z(v.z), w(v.w) { }
	vec4<T>& operator =(const vec4<T>& rhs)
	{
		if(this != &rhs)
		{
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			w = rhs.w;
		}
		return *this;
	}

	T& operator[] (std::size_t i) { return _data[i]; }
	const T& operator[] (std::size_t i) const { return _data[i]; }

	// assignment operators
	vec4<T>& operator +=(const vec4<T>& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
	vec4<T>& operator -=(const vec4<T>& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
	vec4<T>& operator *=(T value)            { x *= value; y *= value; z *= value; w *= value; return *this; }
	vec4<T>& operator /=(T value)            { x /= value; y /= value; z /= value; w /= value; return *this; }

	// unary operators
	vec4<T> operator +() const { return vec4<T>(+x, +y, +z, +w); }
	vec4<T> operator -() const { return vec4<T>(-x, -y, -z, -w); }

	// binary operators, multiply and divide operations for scalar
	friend vec4<T> operator +(const vec4<T>& lhs, const vec4<T>& rhs) { vec4<T> result(lhs); result += rhs; return result; }
	friend vec4<T> operator -(const vec4<T>& lhs, const vec4<T>& rhs) { vec4<T> result(lhs); result -= rhs; return result; }
//	friend vec4<T> operator *(const vec4<T>& lhs, const vec4<T>& rhs) { return vec4<T>(lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z, lhs.w*rhs.w); }
	friend vec4<T> operator *(const vec4<T>& lhs, T value) { return vec4<T>(lhs.x*value, lhs.y*value, lhs.z*value, lhs.w*value); }
	friend vec4<T> operator *(T value, const vec4<T>& rhs) { return vec4<T>(rhs.x*value, rhs.y*value, rhs.z*value, rhs.w*value); }
	friend vec4<T> operator /(const vec4<T>& lhs, T value) { return vec4<T>(lhs.x/value, lhs.y/value, lhs.z/value, lhs.w/value); }

	friend bool operator <(const vec4<T>& lhs, const vec4<T>& rhs)
	{
		// Operator < and strict weak ordering
		if(lhs.x != rhs.x) return lhs.x < rhs.x;
		if(lhs.y != rhs.y) return lhs.y < rhs.y;
		if(lhs.z != rhs.z) return lhs.z < rhs.z;
		if(lhs.w != rhs.w) return lhs.w < rhs.w;
		return false;
	}

	bool operator ==(const vec4<T>& rhs) const
	{
		if(this == &rhs)
			return true;
		return fuzzyEqual(x, rhs.x) && fuzzyEqual(y, rhs.y) &&
				fuzzyEqual(z, rhs.z) && fuzzyEqual(w, rhs.w);
	}
	bool operator !=(const vec4<T>& rhs) const { return !(*this == rhs); }
	friend bool fuzzyEqual(const vec4<T>& lhs, const vec4<T>& rhs) { return lhs == rhs; }

	T length2() const { return x*x + y*y + z*z + w*w; }
	T length() const { return std::sqrt(x*x + y*y + z*z + w*w); }
	void normalize() { *this /= length(); }

	vec4<T>& translate(const vec4<T>& vector);
	vec4<T> project(vec4<T>& direction, bool normalized = false) const
	{
		if(!normalized)
			direction.normalize();
		return dot(*this, direction) * direction;
	}

	friend T angle(const vec4<T>& v1, const vec4<T>& v2)
	{
		T numerator = dot(v1, v2);
		T denominator = v1.length() * v2.length();
//		assert(!isZero<T>(denominator));
		return std::acos(numerator/denominator);
	}

	friend T distance(const vec4<T>& pt1, const vec4<T>& pt2)
	{
		vec4<T> v = pt2 - pt1;
		return v.length();
	}

	friend T dot(const vec4<T>& v1, const vec4<T>& v2)
	{
		return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
	}

	template <typename charT, class traits>
	friend std::basic_istream<charT, traits>& operator >>(
			std::basic_istream<charT, traits>& is, vec4<T>& v)
	{
		T _x, _y, _z, _w;
		charT ch0, ch1, ch2, ch3, ch4;

		// only '(_x, _y, _z, _w)' format is allowed
		is >> ch0 >> _x >> ch1 >> _y >> ch2 >> _z >> ch3 >> _w >> ch4;
		if(ch0 == '(' && ch1 == ',' && ch2 == ',' && ch3 == ',' && ch4 == ')')
			v = vec4<T>(_x, _y, _z, _w);
		else
			is.setstate(std::ios_base::failbit);

		return is;
	}

	template <typename charT, class traits>
	friend std::basic_ostream<charT, traits>& operator <<(
			std::basic_ostream<charT, traits>& os, const vec4<T>& v)
	{
		std::basic_ostringstream<charT, traits> stream;
		stream.flags(os.flags());
		stream.imbue(os.getloc());
		stream.precision(os.precision());
		stream << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')';
		return os << stream.str();
	}
};

typedef vec4<int32_t> vec4i;
typedef vec4<float>   vec4f;
typedef vec4<double>  vec4d;

} /* namespace venus */
#endif /* MATH_VEC4_H_ */
