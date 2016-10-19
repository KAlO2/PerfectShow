#ifndef VENUS_COMPILER_H_
#define VENUS_COMPILER_H_

// Visual Studio start to support noexcept from VS2015
#if defined(_WIN32) && _MSC_VER < 1900/* VS2015 */
#  define noexcept
#  define constexpr const
#endif

#define NELEM(array) (sizeof(array)/sizeof(array[0]))

// OpenCV use BGRA instead of common RGBA memory layout.
#ifdef ANDROID
#  define USE_OPENCV_BGRA_LAYOUT 0
#else
#  define USE_OPENCV_BGRA_LAYOUT 1
#endif

namespace venus {

#if __cplusplus >= 201103L
/*
 * @see C++11 feature about user literial, http://en.cppreference.com/w/cpp/language/user_literal
 * note that float or double type are not allowed on literal operators
 */
constexpr long double operator "" _deg(long double degree)
{
	return degree * static_cast<long double>(M_PI)/180;
}

constexpr long double operator "" _deg(unsigned long long int degree)
{
	return degree * static_cast<long double>(M_PI)/180;
}
#endif

template<typename Enumeration>
constexpr auto int_cast(const Enumeration value) -> typename std::underlying_type<Enumeration>::type
{
	return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

} /* namespace venus */
#endif /* VENUS_COMPILER_H_ */
