#include "parseCmdArgs.h"

using namespace cv;
using namespace std;
using namespace TCLAP;


Cmd_Parameters parseCmdArgs(int argc, const char** argv)
{
	Cmd_Parameters paras;

	try {
		CmdLine cmd("Command description message", ' ', "0.1");
		ValueArg<int> deviceIdArg("c", "cam", "Camera device id", false, 0, "integer", cmd);
		ValueArg<string> seqPathArg("f", "seq_file", "Path to sequence file", false, "", "file", cmd);
		ValueArg<string> initBbArg("b", "box", "Init Bounding Box", false, "-1,-1,-1,-1", "x,y,w,h", cmd);
		SwitchArg saveSw("s", "Save", "Save as video", cmd, false);
		
		// Parse the argv array.
		cmd.parse(argc, argv);

		paras.device = deviceIdArg.getValue();
		paras.sequencePath = seqPathArg.getValue();

		paras.save = saveSw.getValue();
		stringstream initBbSs(initBbArg.getValue());

		double initBb[4];

		for (int i = 0; i < 4; ++i)
		{
			string singleValueStr;
			getline(initBbSs, singleValueStr, ',');
			initBb[i] = static_cast<double>(stod(singleValueStr.c_str()));//stod:convert string to double
		}

		paras.initBb = Rect_<double>(initBb[0], initBb[1], initBb[2], initBb[3]);
	}
	catch (ArgException &argException)
	{
		cerr << "Command Line Argument Exception: " << argException.what() << endl;
		exit(-1);
	}
	// TODO: properly check every argument and throw exceptions accordingly
	catch (...)
	{
		cerr << "Command Line Argument Exception!" << endl;
		exit(-1);
	}

	return paras;
}