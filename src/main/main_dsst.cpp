#include "parseCmdArgs.h"
#include "tracker_run.h"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
using namespace std;

int main(int argc, const char** argv)
{
	Cmd_Parameters cmd;
	cmd = parseCmdArgs(argc, argv);

	TrackerRun obj(cmd);
	if (!obj.start())
		return -1;

	return 0;
}

//-b 261,48,39,65  -f sample_sequence_compressed\sample_sequence_compressed.avi