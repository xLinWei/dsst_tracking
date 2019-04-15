#pragma once

#include <opencv2/highgui/highgui.hpp>
#include <tclap/CmdLine.h>

struct Cmd_Parameters {
	std::string sequencePath;
	cv::Rect initBb;
	int device;
	bool save;
};

Cmd_Parameters parseCmdArgs(int argc, const char** argv);