#pragma once

#include <fstream>
#include "parseCmdArgs.h"
#include <opencv2/highgui/highgui.hpp>
#include "dsst_tracker.h"

class TrackerRun
{

public:
	TrackerRun(Cmd_Parameters cmd);
	virtual ~TrackerRun();
	bool start();

private:
	bool init();
	bool run();
	bool update();
	void printResults(const cv::Rect_<double>& boundingBox, bool isConfident, double fps);

private:
	cv::Mat _image;
	cf_tracking::DsstTracker* _tracker;
	cv::VideoCapture _cap;
	cv::VideoWriter _wtr;
	Cmd_Parameters _cmdParas;
	cv::Rect_<float> _boundingBox;
	std::ofstream _resultsFile;
	std::string _windowTitle;

	int _frameIdx;
	//bool _isPaused = false;
	bool _exit = false;
	bool _hasInitBox = false;
	bool _isTrackerInitialzed = false;
	bool _targetOnFrame = false;
};