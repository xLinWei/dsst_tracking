#include "tracker_run.h"
#include "parseCmdArgs.h"
#include "init_box_selector.hpp"

using namespace cv;
using namespace std;


TrackerRun::TrackerRun(Cmd_Parameters cmd):
	_cmdParas(cmd), _windowTitle("Dsst_tracking")
{
	cf_tracking::DsstParameters paras;
	_tracker = new cf_tracking::DsstTracker(paras);
}


TrackerRun::~TrackerRun()
{
	_cap.release();
	if (_cmdParas.save) 
	{ 
		_wtr.release(); 
		if (_resultsFile.is_open())
			_resultsFile.close();
	}

	if (_tracker)
	{
		delete _tracker;
		_tracker = 0;
	}
}

bool TrackerRun::start()
{
	if (init() == false)
		return false;
	if (run() == false)
		return false;

	return true;
}

bool TrackerRun::init()
{
	if (_cmdParas.sequencePath.empty())
		_cap.open(_cmdParas.device);
	else
	{
		std::string sequenceExpansion = _cmdParas.sequencePath;
		_cap.open(sequenceExpansion);
	}
	if (!_cap.isOpened())
	{
		cerr << "Could not open device/sequence/video!" << endl;
		exit(-1);
	}

	namedWindow(_windowTitle.c_str());

	if (_cmdParas.save)
	{
		Size frame_size = Size(_cap.get(CAP_PROP_FRAME_WIDTH), _cap.get(CAP_PROP_FRAME_HEIGHT));//Size frame_size = Size(640, 480);
		_wtr.open("result.avi", CV_FOURCC('D', 'I', 'V', 'X'), 30, frame_size, true);//fps=30

		string outputFilePath("result.txt");
		_resultsFile.open(outputFilePath.c_str());
		if (!_resultsFile.is_open())
		{
			std::cerr << "Error: Unable to create results file: "
				<< outputFilePath.c_str()
				<< std::endl;
			return false;
		}
		_resultsFile.precision(std::numeric_limits<double>::digits10 - 4);
	}

	if (_cmdParas.initBb.width > 0 || _cmdParas.initBb.height > 0)
	{
		_boundingBox = _cmdParas.initBb;
		_hasInitBox = true;
	}

	_frameIdx = 0;
	return true;
}


bool TrackerRun::run()
{
	bool success = true;

	//std::cout << "Switch pause with 'p'" << std::endl;
	std::cout << "Select new target with 'r'" << std::endl;
	std::cout << "Quit with 'ESC'" << std::endl;

	while (true)
	{
		success = update();
		if (!success)
			break;
	}

	return true;
}


bool TrackerRun::update()
{
	int64 tStart = 0;
	int64 tDuration = 0;

	_cap >> _image;
	if (_image.empty())
		return false;
	++_frameIdx;

	if (!_isTrackerInitialzed)
	{
		if (!_hasInitBox)
		{
			Rect box;
			if (!InitBoxSelector::selectBox(_image, box))
				return false;

			_boundingBox = Rect_<float>(static_cast<double>(box.x),
				static_cast<float>(box.y),
				static_cast<float>(box.width),
				static_cast<float>(box.height));

			_hasInitBox = true;
		}

		tStart = getTickCount();
		_targetOnFrame = _tracker->reinit(_image, _boundingBox);
		tDuration = getTickCount() - tStart;

		if (_targetOnFrame)
			_isTrackerInitialzed = true;
	}
	else if (_isTrackerInitialzed)
	{
		tStart = getTickCount();
		_targetOnFrame = _tracker->update(_image, _boundingBox);
		tDuration = getTickCount() - tStart;
	}

	double fps = static_cast<double>(getTickFrequency() / tDuration);
	
	//----------------- show && save video-----------------------//
	//Mat hudImage;
	//_image.copyTo(hudImage);
	rectangle(_image, _boundingBox, Scalar(0, 0, 255), 2);
	/*Point_<double> center;
	center.x = _boundingBox.x + _boundingBox.width / 2;
	center.y = _boundingBox.y + _boundingBox.height / 2;
	circle(hudImage, center, 3, Scalar(0, 0, 255), 2);*/

	stringstream ss;
	ss << "FPS: " << fps;
	putText(_image, ss.str(), Point(20, 20), FONT_HERSHEY_TRIPLEX, 0.5, Scalar(255, 0, 0));

	/*ss.str("");
	ss.clear();
	ss << "#" << _frameIdx;
	putText(hudImage, ss.str(), Point(hudImage.cols - 60, 20), FONT_HERSHEY_TRIPLEX, 0.5, Scalar(255, 0, 0));*/

	imshow(_windowTitle.c_str(), _image);

	if (_cmdParas.save)
	{
		try
		{
			printResults(_boundingBox, _targetOnFrame, fps);
			_wtr << _image;
		}
		catch (runtime_error& runtimeError)
		{
			cerr << "Could not write output images: " << runtimeError.what() << endl;
		}
	}

	char c = (char)waitKey(10);
	if (c == 27)
	{
		_exit = true;
		return false;
	}

	switch (c)
	{
	/*case 'p':
		_isPaused = !_isPaused;
		break;*/
	case 'r':
		_hasInitBox = false;
		_isTrackerInitialzed = false;
		break;
	default:
		;
	}

	return true;

}

void TrackerRun::printResults(const cv::Rect_<double>& boundingBox, bool isConfident, double fps)
{
	if (_resultsFile.is_open())
	{
		if (boundingBox.width > 0 && boundingBox.height > 0 && isConfident)
		{
			_resultsFile << boundingBox.x << ","
				<< boundingBox.y << ","
				<< boundingBox.width << ","
				<< boundingBox.height << ","
				<< fps << std::endl;
		}
		else
		{
			_resultsFile << "NaN, NaN, NaN, NaN, " << fps << std::endl;
		}
	}
}