#include <assert.h>

#include <Windows.h>

#include <opencv2/highgui.hpp>

#include "example/utility.h"

const std::string PROJECT_DIR    = dirname(__FILE__) + "/../../";
const std::string CLASSIFIER_DIR = PROJECT_DIR + "res/raw/";

std::string dirname(const std::string& path)
{
#if _WIN32
	char SEPERATOR = '\\';
#else
	char SEPERATOR = '/';
#endif

	size_t end = path.find_last_not_of(SEPERATOR);
	if(end == std::string::npos)
		return path.substr(0, 1);  // [0, 1)

	size_t position = path.find_last_of(SEPERATOR, end);
	if(position == 0)
		return path.substr(0, 1);  // [0, 1)
	else if(position == std::string::npos)
		return std::string(".");  // current directory

	return path.substr(0, position);  // [0, position)
}

std::vector<std::string> listFiles(const std::string& dir, const std::string& ext/* = "*.*" */)
{
	// extension validation, filter "/\:?" characters.
	for(const char ch: ext)
		if(!isalnum(ch) && ch != '.' && ch != '*')
		{
			assert(false);  // invalid suffix
			return {};
		}

	std::vector<std::string> names;
	std::string search_path = dir + '/' + ext;
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// read all (real) files in current folder except other 2 default folder . and ..
			if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				names.push_back(fd.cFileName);
		} while(::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}

void testSamples(const std::string& dir, const std::function<void(const cv::Mat&)>& func)
{
	std::vector<std::string> filenames = listFiles(dir);
	for(const std::string& filename: filenames)
	{
		std::string path = dir + filename;
		cv::Mat image = cv::imread(path, cv::IMREAD_UNCHANGED);

		func(image);
		cv::waitKey();
	}
}
