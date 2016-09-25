#ifndef VENUS_COMMON_H_
#define VENUS_COMMON_H_

// Visual Studio start to support noexcept from VS2015
#if defined(_WIN32) && _MSC_VER < 1900/* VS2015 */
#  define noexcept
#  define constexpr const
#endif

#define NELEM(array) (sizeof(array)/sizeof(array[0]))

/**
 * @brief Clamps a value to a specified range [min, max]
 *
 * @param value The double in question
 * @param min The minimum of the range
 * @param max The maximum of the range
 * @return The clamped value
 */
template<typename T>
inline T clamp(const T& value, const T& min, const T& max)
{
	assert(min <= max && "invalid clamp range");
#if 0
	return std::min<T>(std::max<T>(value, min), max);
#else
	if(value < min) return min;
	if(value > max) return max;
	return value;
#endif
}

/*
template<typename T>
inline void clamp(T& value, const T& min, const T& max)
{
	assert(min <= max && "invalid clamp range");
	if(value < min) value = min;
	if(value > max) value = max;
}
*/

template<typename Enumeration>
constexpr auto integer_cast(const Enumeration value) -> typename std::underlying_type<Enumeration>::type
{
	return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

#endif /* VENUS_COMMON_H_ */
