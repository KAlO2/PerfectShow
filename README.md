# PerfectShow

Welcome to PerfectShow, an image processing application for Android platform.

Up until now, I release functionalities about make-up.

Currently I use Eclipse project instead of Android Studio project, since the former IDE has a shallow directory hierarchy, and NDK building is easier to use.
This project mainly use OpenCV library for image processing, so you might as well have a good grasp of this field.

# Usage
1. Download Android OpenCV SDK from official site of [OpenCV](http://opencv.org/downloads.html), extract the archived package to somewhere *<OpenCV-android-sdk>*, then import the project under *<OpenCV-android-sdk>/sdk/java/* into Eclipse (Suppose you've got an Android development environment) and build it. For detailed steps, [this thread](http://docs.opencv.org/2.4/doc/tutorials/introduction/android_binary_package/dev_with_OCV_on_Android.html) walks you through how to do Android development with OpenCV.
2. Change the variable *OPENCV_ANDROID_SDK_HOME* in *jni/Android.mk* to the path *<OpenCV-android-sdk>* above, in our project, add a reference to the OpenCV Java SDK in Project -> Properties -> Android -> Library -> Add select OpenCV Library, then strike command to finish native code building, and last, build this Android project.
```sh
$ cd jni
$ ndk-build -j4
$ cd ..
$ ant debug
```


# Screenshot
<img src="./doc/seamless-cloning.gif" width="42%" height="42%">
<img src="./doc/ui-makeup.jpg"        width="42%" height="42%">

# Contact
If you like this project, share your appreciation by following me in [GitHub](https://github.com/KAlO2).
If you have any problems about this project, you can file an issue [here](https://github.com/KAlO2/PerfectShow/issues) or join QQ group 571444731 for consulting.

