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