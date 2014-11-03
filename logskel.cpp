/*****************************************************************************
*                                                                            *
*  OpenNI 1.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <cstdlib> // for EXIT_SUCCESS
#include <iostream>
#include <time.h>

#include <XnOpenNI.h>
#include <XnCppWrapper.h>

#include "arghelpers.h"
#include "io.h"
#include "mainloop.h"
#include "optionparser.h"

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

DepthMapLogger g_Log;

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

// Command-line option description
enum optionIndex { UNKNOWN, HELP, CAPTURE, PLAYBACK, LOG, DURATION, };
const option::Descriptor g_Usage[] =
{
	{ UNKNOWN,  0, "",   "",         option::Arg::None,	"Usage:\n"
							  	"  logskel [options]\n\n"
							  	"Options:" },
	{ HELP,     0, "h?", "help",     option::Arg::None, 	"  --help, -h, -?  \tPrint a brief usage summary." },
	{ CAPTURE,  0, "c",  "capture",  Arg::Required,		"  --capture, -c CONFIG  \tCapture from sensor using specified XML config." },
	{ PLAYBACK, 0, "p",  "playback", Arg::Required,		"  --playback, -p RECORDING  \tPlayback a .oni recording." },
	{ LOG,      0, "l",  "log",      Arg::Required,		"  --log, -l PREFIX  \tLog results to PREFIX.{h5,txt}." },
	{ DURATION, 0, "d",  "duration", Arg::Numeric,		"  --duration, -d SECONDS  \tRun main loop for the specified duration." },

	{ 0,0,0,0,0,0 } // Zero record marking end of array.
};

int main(int argc, char **argv)
{
	// Parse command-line options
	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
	option::Stats  stats(g_Usage, argc, argv);
	option::Option options[stats.options_max], buffer[stats.buffer_max];
	option::Parser parse(g_Usage, argc, argv, options, buffer);

	if (parse.error()) {
		return EXIT_FAILURE;
	}

	if (options[HELP]) {
		option::printUsage(std::cout, g_Usage);
		return EXIT_SUCCESS;
	}

	// Check command-line options for validity.
	if (options[CAPTURE] && options[PLAYBACK]) {
		std::cerr << "Error: only one of --playback and --capture may be specified.\n";
		option::printUsage(std::cerr, g_Usage);
		return EXIT_FAILURE;
	}

	double duration(0.); // >0 only if a duration has been requested
	if (options[DURATION]) {
		// We know this will succeed because the argument has been checked.
		duration = static_cast<double>(strtol(options[DURATION].arg, NULL, 10));
		if (duration < 0.) {
			std::cerr << "Duration must be positive.\n";
			return EXIT_FAILURE;
		}
	}

	if (options[LOG]) {
		std::string logfile_prefix(options[LOG].arg);
		std::string h5_logfile(logfile_prefix + ".h5"), txt_logfile(logfile_prefix + ".txt");
		std::cout << "Logging to " << h5_logfile << " and " << txt_logfile << '\n';
		g_Log.Open(h5_logfile.c_str(), txt_logfile.c_str());
	}

	// Set up capture device
	if (options[PLAYBACK])
	{
		if(!InitialiseContextFromRecording(options[PLAYBACK].arg))
		{
			return EXIT_FAILURE;
		}
	}
	else if(options[CAPTURE])
	{
		if(!InitialiseContextFromXmlConfig(options[CAPTURE].arg))
		{
			return EXIT_FAILURE;
		}
	} else {
		std::cerr << "Error: exactly one of --playback and --capture must be specified.\n";
		option::printUsage(std::cerr, g_Usage);
		return EXIT_FAILURE;
	}

	if(!PreMainLoop()) {
		return EXIT_FAILURE;
	}

	// Main event loop
	xn::SceneMetaData sceneMD;
	xn::DepthMetaData depthMD;
	std::cout << "---------------------------------------------------------------------------\n";
	std::cout << "Starting tracker. Press any key to exit.\n";
	std::cout << "---------------------------------------------------------------------------\n";
	time_t loop_start(time(NULL));
	while (!xnOSWasKeyboardHit())
	{
		// Was a particular duration requested?
		if ((duration > 0.) && (difftime(time(NULL), loop_start) >= duration)) {
			std::cout << "Loop has run for " << duration << " seconds.\n";
			break;
		}

		// Wait for an update
		g_Context.WaitOneUpdateAll(g_UserGenerator);

		// Process the data
		g_DepthGenerator.GetMetaData(depthMD);
		g_UserGenerator.GetUserPixels(0, sceneMD);

		// Log the data
		g_Log.DumpDepthMap(depthMD, sceneMD);
	}
	std::cout << '\n';
	std::cout << "---------------------------------------------------------------------------\n";
	std::cout << "Exiting tracker.\n";
	std::cout << "---------------------------------------------------------------------------\n";

	// Clean up all resources
	PostMainLoop();

	return EXIT_SUCCESS;
}
