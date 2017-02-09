# uncomment next line if you want to use Clang compiler
#NDK_TOOLCHAIN_VERSION := clang

# http://stackoverflow.com/questions/22922961/c11-cmath-functions-not-in-std-namespace-for-android-ndk-w-gcc-4-8-or-clang-3
# https://developer.android.com/ndk/guides/cpp-support.html
#APP_STL := gnustl_static # this will emit std::cbrt() missing error.
APP_STL := gnustl_shared
#LIBCXX_FORCE_REBUILD := true c++_static

APP_CFLAGS := -Wall
APP_CPPFLAGS := -Wall -std=c++11 -frtti -fexceptions -fPIC

# http://stackoverflow.com/questions/28926101/is-it-safe-to-support-only-armeabi-v7a-for-android-4-and-above
# Android 4.4+ requires an ARMv7 processor. Custom versions have been made for ARMv6 however.
APP_ABI := armeabi-v7a
APP_PLATFORM := android-14