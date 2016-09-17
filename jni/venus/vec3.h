#ifndef MATH_VEC3_H_
#define MATH_VEC3_H_

#include "scalar.h"

#include <iostream>

namespace venus {

template <typename T>
class vec3
{
public:
// https://msdn.microsoft.com/en-us/library/2c8f766e.aspx
#ifdef _WIN32
#  pragma warning(push)
#  pragma warning(disable: 4201)  // warning C4201: nonstandard extension used: nameless struct/union
#endif

	union
	{
		T _data[3];
		struct { T x, y, z; };  // vertex
		struct { T r, g, b; };  // color
		struct { T u, v, w; };  // texture
		struct { T s, t, p; };  // texture
		struct { T i, j, k; };  // index
		struct { T start, step, stop; }; // arithmetic progression
		struct { T pitch, roll, yaw; };
		struct { T rho, theta, phi; };  // polar coordinate
		struct { T constant, linear, quadratic; };  // attenuation
	};

#ifdef _WIN32
#  pragma warning(pop)
#endif

	typedef T value_type;

public:
	vec3()  = default;
	~vec3() = default;

	explicit vec3(const T *array): x(array[0]), y(array[1]), z(array[2]) { }
	explicit vec3(T scalar): x(scalar), y(scalar), z(scalar) { }
	vec3(T x, T y, T z): x(x), y(y), z(z) { }

	vec3(const vec3<T>& v): x(v.x), y(v.y), z(v.z) { }
	vec3<T>& operator =(const vec3<T>& rhs)
	{
		if(this != &rhs)
		{
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
		}
		return *this;
	}

	vec3<T>& operator =(const vec3<T>&& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}

	T& operator[] (std::size_t i) { return _data[i]; }
	const T& operator[] (std::size_t i) const { return _data[i]; }

	// assignment operators
	vec3<T>& operator +=(const vec3<T>& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	vec3<T>& operator -=(const vec3<T>& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	vec3<T>& operator *=(T value)            { x *= value; y *= value; z *= value; return *this; }
	vec3<T>& operator /=(T value)            { x /= value; y /= value; z /= value; return *this; }

	// unary operators
	vec3<T> operator +() const { return vec3<T>(+x, +y, +z); }
	vec3<T> operator -() const { return vec3<T>(-x, -y, -z); }

	// binary operators, multiply and divide operations for scalar
	friend vec3<T> operator +(const vec3<T>& lhs, const vec3<T>& rhs) { vec3<T> result(lhs); result += rhs; return result; }
	friend vec3<T> operator -(const vec3<T>& lhs, const vec3<T>& rhs) { vec3<T> result(lhs); result -= rhs; return result; }
	friend vec3<T> operator *(const vec3<T>& lhs, const vec3<T>& rhs) { return vec3<T>(lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z); }
	friend vec3<T> operator *(const vec3<T>& lhs, T value) { return vec3<T>(lhs.x*value, lhs.y*value, lhs.z*value); }
	friend vec3<T> operator *(T value, const vec3<T>& rhs) { return vec3<T>(rhs.x*value, rhs.y*value, rhs.z*value); }
	friend vec3<T> operator /(const vec3<T>& lhs, T value) { return vec3<T>(lhs.x/value, lhs.y/value, lhs.z/value); }

	friend bool operator <(const vec3<T>& lhs, const vec3<T>& rhs)
	{
		// Operator < and strict weak ordering
		if(lhs.x != rhs.x) return lhs.x < rhs.x;
		if(lhs.y != rhs.y) return lhs.y < rhs.y;
		if(lhs.z != rhs.z) return lhs.z < rhs.z;

		return false;
	}

	bool operator ==(const vec3<T>& rhs) const
	{
		if(this == &rhs)
			return true;
		return fuzzyEqual(x, rhs.x) && fuzzyEqual(y, rhs.y) && fuzzyEqual(z, rhs.z);
	}
	bool operator !=(const vec3<T>& rhs) const { return !(*this == rhs); }
	friend bool fuzzyEqual(const vec3<T>& lhs, const vec3<T>& rhs) { return lhs == rhs; }

	T length2() const { return x*x + y*y + z*z; }
	T length() const { return std::sqrt(x*x + y*y + z*z); }
	void normalize() { *this /= length(); }

	/**
	 * give the vector v a spherical coordinate system(rho, theta, phi) view.
	 * http://en.wikipedia.org/wiki/Spherical_coordinate_system
	 * r ≥ 0, 0° ≤ θ ≤ 180° (π rad), 0° ≤ φ < 360° (2π rad)
	 * However, the azimuth φ is often restricted to the interval (−180°, +180°],
	 * or (−π, +π] in radians, instead of [0, 360°). This is the standard
	 * convention for geographic longitude.
	 *
	 * @return vec3(rho, theta, phi) where rho >= 0, theta in [-pi/2, pi/2], phi in [-pi, pi].
	 */
	//FIXME
	vec3<T> polar() const
	{
		T _rho = length(), _theta = 0, _phi = 0;
		if(!isZero<T>(_rho))
		{
			_theta = std::acos(z/rho);
//			_theta == 0 means vector parallel to y axis in OpenGL
			_phi = (isZero<T>(theta))?  0 : std::atan2(y, x);
		}
		return vec3<T>(_rho, _theta, _phi);
	}

	vec3<T> project(vec3<T>& direction, bool normalized = false) const
	{
		if(!normalized)
			direction.normalize();
		return dot(*this, direction) * direction;
	}

	inline vec3<T>& translate(const vec3<T>& vector)
	{
		*this += vector;
		return *this;
	}

	/**
	 * @brief It's equivalent to left multiplying matrix
	 *
	 * [1     0           0     ]
	 * [0 cos(angle) -sin(angle)]
	 * [0 sin(angle)  cos(angle)]
	 */
	vec3<T>& rotateX(T angle)
	{
		const T c = std::cos(angle);
		const T s = std::sin(angle);
		T _y = c*y - s*z;
		T _z = s*y + c*z;

		y = _y;
		z = _z;
		return *this;
	}

	/**
	 * @brief It's equivalent to left multiplying matrix
	 *
	 * [ cos(angle) 0 sin(angle)]
	 * [    0       1     0     ]
	 * [-sin(angle) 0 cos(angle)]
	 */
	vec3<T>& rotateY(T angle)
	{
		const T c = std::cos(angle);
		const T s = std::sin(angle);
		T _x = s*z + c*x;
		T _z = c*z - s*x;

		x = _x;
		z = _z;
		return *this;
	}

	/**
	 * @brief It's equivalent to left multiplying matrix
	 *
	 * [cos(angle) -sin(angle) 0]
	 * [sin(angle)  cos(angle) 0]
	 * [    0           0      1]
	 */
	vec3<T>& rotateZ(T angle)
	{
		const T c = std::cos(angle);
		const T s = std::sin(angle);
		T _x = c*x - s*y;
		T _y = s*x + c*y;

		x = _x;
		y = _y;
		return *this;
	}

	vec3<T>& rotate(const vec3<T>& v, T angle)
	{
		const T c = std::cos(angle), s = std::sin(angle), C = 1-c;
		const T &x = v.x, &y = v.y, &z = v.z;

		T _x = (   c + x*x*C)*this->x + (-z*s + x*y*C)*this->y + ( y*s + x*z*C)*this->z;
		T _y = ( z*s + y*x*C)*this->x + (   c + y*y*C)*this->y + (-x*s + y*z*C)*this->z;
		T _z = (-y*s + z*x*C)*this->x + ( x*s + z*y*C)*this->y + (   c + z*z*C)*this->z;

		this->x = _x;
		this->y = _y;
		this->z = _z;
		return *this;
	}

	friend T angle(const vec3<T>& v1, const vec3<T>& v2)
	{
		T numerator = dot(v1, v2);
		T denominator = v1.length() * v2.length();
//		assert(!isZero<T>(denominator));
		return std::acos(numerator/denominator);
	}

	friend T distance(const vec3<T>& pt1, const vec3<T>& pt2)
	{
		vec3<T> v = pt2 - pt1;
		return v.length();
	}

	friend T dot(const vec3<T>& v1, const vec3<T>& v2)
	{
		return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
	}

	friend vec3<T> cross(const vec3<T>& v1, const vec3<T>& v2)
	{
/*
		|  i    j    k   |
		| v1.x v1.y v1.z |
		| v2.x v2.y v2.z |
*/
		return vec3<T>(
			v1.y * v2.z - v2.y * v1.z,
			v1.z * v2.x - v2.z * v1.x,
			v1.x * v2.y - v2.x * v1.y);
	}

	template <typename charT, class traits>
	friend std::basic_istream<charT, traits>& operator >>(
			std::basic_istream<charT, traits>& is, vec3<T>& v)
	{
		T _x, _y, _z;
		charT ch0, ch1, ch2, ch3;

		// only '(_x, _y, _z)' format is allowed
		is >> ch0 >> _x >> ch1 >> _y >> ch2 >> _z >> ch3;
		if(ch0 == '(' && ch1 == ',' && ch2 == ','  && ch3 == ')')
			v = vec3<T>(_x, _y, _z);
		else
			is.setstate(std::ios_base::failbit);

		return is;
	}

	template <typename charT, class traits>
	friend std::basic_ostream<charT, traits>& operator <<(
			std::basic_ostream<charT, traits>& os, const vec3<T>& v)
	{
		std::basic_ostringstream<charT, traits> stream;
		stream.flags(os.flags());
		stream.imbue(os.getloc());
		stream.precision(os.precision());
		stream << '(' << v.x << ", " << v.y << ", " << v.z << ')';
		return os << stream.str();
	}
};

typedef vec3<int32_t> vec3i;
typedef vec3<float>   vec3f;
typedef vec3<double>  vec3d;

} /* namespace venus */
#endif /* MATH_VEC3_H_ */
