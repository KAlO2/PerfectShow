#ifndef MATH_VEC2_H_
#define MATH_VEC2_H_

#include <stdint.h>  // for int32_t

namespace venus {

template <typename T>
class vec2
{
public:

// https://msdn.microsoft.com/en-us/library/2c8f766e.aspx
#ifdef _WIN32
#  pragma warning(push)
#  pragma warning(disable: 4201)  // warning C4201: nonstandard extension used: nameless struct/union
#endif
	union
	{
		T data[2];
		struct { T x, y; }; // 2D coordinate
		struct { T i, j; }; // index, integer pair
		struct { T s, t; }; // texture
		struct { T u, v; }; // texture
		
		struct { T gray, alpha;   }; // color
		struct { T rho, theta;    }; // polar coordinate
		struct { T width, height; }; // size
		struct { T column, row;   }; // grid, index, notice the order
		struct { T min, max;      }; // 1D AABB
		struct { T slice, stack;  }; // cut shape
	};

#ifdef _WIN32
#  pragma warning(pop)
#endif

	typedef T value_type;

public:
	vec2()  = default;
	~vec2() = default;

	explicit vec2(const T *array): x(array[0]), y(array[1]) { }
	explicit vec2(T scalar): x(scalar), y(scalar) { }
	vec2(T x, T y): x(x), y(y) { }

	vec2(const vec2<T>& v): x(v.x), y(v.y) {}

	vec2<T>& operator =(const vec2<T>& rhs)
	{
		if(this != &rhs)
		{
			x = rhs.x;
			y = rhs.y;
		}
		return *this;
	}

	// assignment operators
	vec2<T>& operator +=(const vec2<T>& rhs) { x += rhs.x; y += rhs.y; return *this; }
	vec2<T>& operator -=(const vec2<T>& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
	vec2<T>& operator *=(T value)            { x *= value; y *= value; return *this; }
	vec2<T>& operator /=(T value)            { x /= value; y /= value; return *this; }

	/**
	 * @brief unary operators
	 * @{
	 */
	inline vec2<T> operator +() const { return vec2<T>(+x, +y); }
	inline vec2<T> operator -() const { return vec2<T>(-x, -y); }
	/** @} */

	/**
	 * binary operators, add subtract, multiply and divide operations for scalar
	 * @{
	 */
	friend vec2<T> operator +(const vec2<T>& lhs, const vec2<T>& rhs) { vec2<T> result(lhs); result += rhs; return result; }
	friend vec2<T> operator -(const vec2<T>& lhs, const vec2<T>& rhs) { vec2<T> result(lhs); result -= rhs; return result; }
	friend vec2<T> operator *(const vec2<T>& lhs, T value) { return vec2<T>(lhs.x*value, lhs.y*value); }
	friend vec2<T> operator *(T value, const vec2<T>& rhs) { return vec2<T>(rhs.x*value, rhs.y*value); }
	friend vec2<T> operator /(const vec2<T>& lhs, T value) { return vec2<T>(lhs.x/value, lhs.y/value); }
	/** @} */

	friend bool operator <(const vec2<T>& lhs, const vec2<T>& rhs)
	{
	    if(lhs.x != rhs.x)
	        return lhs.x < rhs.x;
	    return lhs.y < rhs.y;
	}

	bool operator ==(const vec2<T>& rhs) const
	{
		if(this == &rhs)
			return true;

		return fuzzyEqual<T>(x, rhs.x) && fuzzyEqual<T>(y, rhs.y);
	}

	bool operator !=(const vec2<T>& rhs) const { return !(*this == rhs); }
	friend bool fuzzyEqual(const vec2<T>& lhs, const vec2<T>& rhs) { return lhs == rhs; }

	T length2() const { return x*x + y*y; }
	T length() const { return std::sqrt(x*x + y*y); }
	void normalize() { *this /= length(); }

	// ceil/floor/fract

	/**
	 * polar coordinate system (rho, theta)
	 *
	 * @param rho [out], the magnitude of the vector.
	 * @param theta [out], the return value is in the range [-pi, pi].
	 */
	vec2<T> polar() const
	{
		T _rho = length();
		T _theta = std::atan2(y, x);
		return vec2<T>(_rho, _theta);
	}

	vec2<T> project(vec2<T>& direction, bool normalized = false) const
	{
		if(!normalized)
			direction.normalize();
		return dot(*this, direction) * direction;
	}

	vec2<T>& translate(const vec2<T>& vector)
	{
		*this += vector;
		return *this;
	}

	vec2<T>& rotate(T angle)
	{
		// complex<T>(x,y) * complex<T>(cos(angle), sin(angle));
		const T c = std::cos(angle);
		const T s = std::sin(angle);
		T _x = c*x - s*y;
		T _y = s*x + c*y;

		x = _x;
		y = _y;
		return *this;
	}

	friend T angle(const vec2<T>& v1, const vec2<T>& v2)
	{
		T numerator = dot(v1, v2);
		T denominator = v1.length() * v2.length();
//		assert(!isZero<T>(denominator));
		return std::acos(numerator/denominator);
	}

	friend T distance(const vec2<T>& pt1, const vec2<T>& pt2)
	{
		vec2<T> v = pt2 - pt1;
		return v.length();
	}

	friend T dot(const vec2<T>& v1, const vec2<T>& v2)
	{
		return v1.x * v2.x + v1.y * v2.y;
	}

	/**
	 * The cross product of the two vectors in 2D.
	 * If the result is 0, the points are colinear; if it is positive, the two
	 * vectors constitute a "left turn" or counter-clockwise orientation,
	 * otherwise a "right turn" or clockwise orientation.
	 *
	 * http://www.euclideanspace.com/maths/algebra/vectors/vecGeometry/vec2d/index.htm
	 * This may be useful to calculate triangle's directed area, which has both
	 * quantity and direction.
	 */
	friend T cross(const vec2<T>& v1, const vec2<T>& v2)
	{
/*
		|  i    j   k |
		| v1.x v1.y 0 |
		| v2.x v2.y 0 |
*/
		return v1.x * v2.y - v2.x * v1.y;
	}

	template <typename charT, class traits>
	friend std::basic_istream<charT, traits>& operator >>(
			std::basic_istream<charT, traits>& is, vec2<T>& v)
	{
/*
		charT ch;
		is >> ch;
		if(ch == '(')
		{
			is >> _x >> ch;
			if(ch == ',')
			{
				is >> _y >> ch;
				if(ch == ')')
					v = vec2<U>(_x, _y);
				else
					is.setstate(std::ios_base::failbit);
			}
			else if(ch == ')')
				v = _x;
			else
				is.setstate(std::ios_base::failbit);
		}
		else
		{
			is.putback(ch);
			is >> _x;
			v = _x;
		}
*/
		T _x, _y;
		charT ch0, ch1, ch2;

		// only '(_x, _y)' format is allowed
		is >> ch0 >> _x >> ch1 >> _y >> ch2;
		if(ch0 == '(' && ch1 == ',' && ch2 == ')')
			v = vec2<T>(_x, _y);
		else
			is.setstate(std::ios_base::failbit);

		return is;
	}

	template <typename charT, class traits>
	friend std::basic_ostream<charT, traits>& operator <<(
			std::basic_ostream<charT, traits>& os, const vec2<T>& v)
	{
		std::basic_ostringstream<charT, traits> stream;
		stream.flags(os.flags());
		stream.imbue(os.getloc());
		stream.precision(os.precision());
		stream << '(' << v.x << ", " << v.y << ')';
		return os << stream.str();
	}

};

typedef vec2<int32_t> vec2i;
typedef vec2<float>   vec2f;
typedef vec2<double>  vec2d;

} /* namespace venus */
#endif /* MATH_VEC2_H_ */
