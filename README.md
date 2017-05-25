# PerfectShow

Welcome to PerfectShow, an image processing application for Android platform.

Up until now, I release functionalities about make-up.

This project mainly use [stasm](http://www.milbo.users.sonic.net/stasm/) for face detecting, [dlib](http://dlib.net/) can be more superior and robust, and it use [OpenCV](http://opencv.org/) for color blending and image processing, so you might as well have a good grasp of this field.

# Usage
Run the command below to pull OpenCV module in Android Studio project, then build the project.
```sh
$ git submodule update --init --recursive
```

For Win32/Linux project, [cmake](https://cmake.org/download/) (CLI mode or GUI mode) is needed to generate native makefiles. Out-of-source build is recommended, as you can build multiple variants in separate directories. Compilers need to support some C++11 features, *Visual Studio* 2013 or newer works fine.
```sh
$ cd iShow/src/main/cpp/jni
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
```


# Screenshot
<img src="./doc/seamless-cloning.gif" alt="seamless cloning" width="42%" height="42%">
<img src="./doc/ui-makeup.jpg" alt="makeup UI" width="42%" height="42%">


# Contact
If you like this project, share your appreciation by following me in [GitHub](https://github.com/KAlO2).
If you have any problems about this project, you can file an issue [here](https://github.com/KAlO2/PerfectShow/issues).


# License
```
Copyright© 2016-2017  Martin Taylor

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
