#ifndef MATH_SCALAR_H_
#define MATH_SCALAR_H_

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include <limits>
#include <type_traits>
#include <assert.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288L
#endif

namespace venus {

#ifdef USE_DOUBLE_PRECISION
  typedef double real;
#else
  typedef float real;
#endif // USE_DOUBLE_PRECISION

// template variable is brought in C++14
// @see http://en.cppreference.com/w/cpp/language/variable_template
#if __cplusplus >= 201103L
template <typename T>
class scalar
{
public:
// @see http://en.cppreference.com/w/cpp/preprocessor/replace for __cplusplus macro.

	static constexpr T tolerance = 1.0E-5;
	static constexpr T pi = static_cast<T>(M_PI);
	static constexpr T e = static_cast<T>(M_E);
};
#endif

template <typename T>
inline bool isZero(const T& value)
{
//	static_assert(!std::is_floating_point<T>::value, "limited to scalar floating-point type only");
	static_assert(std::is_integral<T>::value, "limited to integral type only");
	return value == 0;
}

/*
	template specialization for real types. We limit difference precisions with difference tolerances.
	Intuitively, when you fully specialize something, it doesn't depend on a template parameter any
	more, so unless you make the specialization inline, you need to put it in a .cpp file instead of
	a .h or you end up violating the one definition rule (http://en.wikipedia.org/wiki/One_Definition_Rule).
	Note that when you partially specialize templates, the partial specializations do still depend on
	one or more template parameters, so they still go in a .h file.
*/
template <>
inline bool isZero<float>(const float& value)
{
	return std::abs(value) <= std::sqrt(std::numeric_limits<float>::epsilon());
}

template <>
inline bool isZero<double>(const double& value)
{
	// std::cbrt(x) is not equivalent to std::pow(x, 1.0/3) because std::pow cannot raise a
	// negative base to a fractional exponent.
#ifdef ANDROID  // gnustl_static misses std::cbrt()
	return std::abs(value) <= std::pow(std::numeric_limits<double>::epsilon(), 1.0/3);
#else
	return std::abs(value) <= std::cbrt(std::numeric_limits<double>::epsilon());
#endif
}

template <>
inline bool isZero<long double>(const long double& value)
{
	return std::abs(value) <= std::pow(std::numeric_limits<long double>::epsilon(), 1.0/4);
}

/**
 * @brief Compare two floating point values for equality, with a permissible
 * amount of error. Oftentimes you only care if floats are close enough and
 * this function lets you make that determination. For vector and matrix with
 * float type, operator == is your choice.
 *
 * (Because of rounding errors inherent in floating point arithmetic, direct
 * comparison of floats is often inadvisable.
 * http://en.wikipedia.org/wiki/Floating_point#Accuracy_problems )
 *
 * @param a The first floating value
 * @param b The second floating value
 * @return Whether the two floating values are within epsilon of each other
 */
template <typename T>
inline bool fuzzyEqual(const T& a, const T& b)
{
	// http://floating-point-gui.de/errors/comparison/
	static_assert(std::is_floating_point<T>::value, "limited to scalar floating-point type only");
	if(a == b)  // shortcut, handles infinities
		return true;
//	else if(a == T(0) || b == T(0) || isZero<T>(a - b))
	return isZero<T>(a - b);
}

template <>
inline bool fuzzyEqual<int>(const int& a, const int& b)
{
	return a == b;
}

template <>
inline bool fuzzyEqual<std::size_t>(const std::size_t& a, const std::size_t& b)
{
	return a == b;
}

template <typename T>
inline bool isPowerOfTwo(T n)
{
	static_assert(std::is_integral<T>::value, "limited to integral type only");
	return (n > 0) && ((n&(n-1)) == 0);
}

/**
 * @brief Convert degrees to radians
 *
 * @param degrees The angle in degrees
 * @return The angle in radians
 */
template <typename T>
inline T deg2rad(const T& degrees)
{
	static_assert(std::is_floating_point<T>::value, "limited to floating-point type only");
	return degrees/(180/static_cast<T>(M_PI));
}

/**
 * @brief Convert radians to degrees
 *
 * @param radians The angle in radians
 * @return The angle in degrees
 */
template <typename T>
inline T rad2deg(const T& radians)
{
	static_assert(std::is_floating_point<T>::value, "limited to floating-point type only");
	return radians*(180/static_cast<T>(M_PI));
}

/**
 * @brief Whether the two line segments on one axis overlaps
 *
 * @param l1min The lower bound of the first line segment
 * @param l1max The upper bound of the first line segment
 * @param l2min The lower bound of the second line segment
 * @param l2max The upper bound of the second line segment
 * @return true if the two interval [l1min, l1max] and
 *   [l2min, l2max] overlaps, otherwise false.
 */
template <typename T>
inline bool overlap(const T& l1min, const T& l1max,
		const T& l2min, const T& l2max)
{
	const T len = (l1max-l1min) + (l2max-l2min);
	return (l2max - l1min < len) && (l1max - l2min < len);
}

/**
 * @brief Clamps a value to a specified range [min, max]
 *
 * @param value The double in question
 * @param min The minimum of the range
 * @param max The maximum of the range
 * @return The clamped value
 */
template <typename T>
const T& clamp(const T& value, const T& min, const T& max)
{
	assert(min <= max && "invalid clamp range");
#if 0
	return std::min<T>(std::max<T>(value, min), max);
#else
	if(value < min)  return min;
	if(value > max)  return max;
	return value;
#endif
}

template <typename T>
inline T clamp(const T& value)
{
	static_assert(std::is_floating_point<T>::value, "limited to floating-point type only");
	return clamp<T>(value, T(0), T(1));
}

/**
 * Get minimum, maximum value from triple elements.
 */
template<typename T>
void minmax(T& min, T& max, const T& _1, const T& _2, const T& _3)
{
	if(_1 < _2) { min = _1; max = _2; }
	else        { min = _2; max = _1; }

	if(min > _3)      min = _3;
	else if(max < _3) max = _3;
}

/**
 * @brief Linearly interpolates between two values. Works for any classes that
 * define addition, subtraction, and multiplication (by a float) operators.
 *
 * http://en.wikipedia.org/wiki/Lerp_(computing)
 *
 * @param from   The starting value
 * @param to     The ending value
 * @param amount The amount to interpolate, range [0, 1]
 *
 * @return The interpolated value
 */
template <typename T>
inline T lerp(const T& from, const T& to, const float& amount)
{
	float x = (to - from) * amount;
	if(std::is_integral<T>::value)
		return from + static_cast<T>(x + 0.5F);  // round for integers
	else
		return from + static_cast<T>(x);
}

template <typename T>
inline T lerp(const T& from, const T& to, const uint8_t& amount)
{
//	return from + ((to - from) * amount + 127) / 255;
	return (from * (255 - amount) + to * amount + 127) / 255;
}

/**
 * Smoothly step between two values. Works for any classes that lerp would work
 * for (and is essentially a drop-in replacement). Often looks visually better
 * than a simple linear interpolation as it gives ease-in and ease-out.
 *
 * http://www.fundza.com/rman_shaders/smoothstep/index.html
 *
 * @param from   The starting value
 * @param to     The ending value
 * @param amount The amount to interpolate, range [0, 1]
 *
 * @return The interpolated value
 */
template <typename T>
inline T smoothStep(const T& from, const T& to, const float& amount)
{
	float num = clamp<float>(amount, 0, 1);
	return lerp<T>(from, to, num*num*(3-2*num));
}

/**
 * Normal distribution, also known as Gaussian distribution.
 * <pre>
 *                1                (x - μ)^2
 *  f(x) = -------------- * exp(- -----------)
 *          sqrt(2*pi)*σ             2*σ^2
 * 
 * P(μ- σ < X ≤ μ+ σ) = 68.3%,
 * P(μ-2σ < X ≤ μ+2σ) = 95.4%,
 * P(μ-3σ < X ≤ μ+3σ) = 99.7%.
 * </pre>
 */
template <typename T>
inline T gaussian(const T& x, const T& miu = T(0), const T& sigma = T(1))
{
	T tmp = (x - miu)/sigma;
	constexpr T two_pi = 2*M_PI;
	return std::exp(tmp*tmp/T(-2)) / std::sqrt(two_pi) / sigma;
}

} /* namespace venus */
#endif /* MATH_SCALAR_H_ */
