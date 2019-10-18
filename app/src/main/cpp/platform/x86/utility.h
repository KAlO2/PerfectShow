#ifndef EXAMPLE_UTILITY_
#define EXAMPLE_UTILITY_

#include <functional>
#include <string>
#include <vector>

#include <opencv2/core/mat.hpp>

extern const std::string PROJECT_DIR;
extern const std::string CLASSIFIER_DIR;

/*
	path      dirname  basename
	/usr/lib  /usr     lib
	/usr/     /        usr
	usr       .        usr
	/         /        /
	.         .        .
	..        .        ..
*/
std::string basename(const std::string& path);
std::string dirname(const std::string& path);

/**
 * @param[in] dir  directory
 * @param[in] ext  pattern *.* matches all the files, you can use *.png to filter PNG images and so on.
 * @return files with the same type.
 */
std::vector<std::string> listFiles(const std::string& dir, const std::string& ext = "*.*");


void testSamples(const std::string& dir, const std::function<void(const cv::Mat&)>& func);


#endif /* EXAMPLE_UTILITY_ */