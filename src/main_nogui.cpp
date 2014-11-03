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
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnPropNames.h>

#include "arghelpers.h"
#include "io.h"
#include "optionparser.h"

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
xn::Context g_Context;
xn::ScriptNode g_scriptNode;
xn::DepthGenerator g_DepthGenerator;
xn::UserGenerator g_UserGenerator;
xn::Player g_Player;

XnBool g_bNeedPose = FALSE;
XnChar g_strPose[20] = "";

DepthMapLogger g_Log;

//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
bool InitialiseContextFromRecording(const char* recordingFilename);
bool InitialiseContextFromXmlConfig(const char* xmlConfigFilename);
bool EnsureDepthGenerator();

void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& /*generator*/, XnUserID nId, void* /*pCookie*/);
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& /*generator*/, XnUserID nId, void* /*pCookie*/);
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& /*capability*/, const XnChar* strPose, XnUserID nId, void* /*pCookie*/);
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& /*capability*/, XnUserID nId, void* /*pCookie*/);
void XN_CALLBACK_TYPE UserCalibration_CalibrationComplete(xn::SkeletonCapability& /*capability*/, XnUserID nId, XnCalibrationStatus eStatus, void* /*pCookie*/);

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

#define SAMPLE_XML_PATH "../Data/SamplesConfig.xml"

#define CHECK_RC(nRetVal, what)										\
	if (nRetVal != XN_STATUS_OK)									\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));\
		return nRetVal;												\
	}

#define CHECK_RC_RETURNING(rv, nRetVal, what)										\
	if (nRetVal != XN_STATUS_OK)									\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));\
		return rv;												\
	}

// Command-line option description
enum optionIndex { UNKNOWN, HELP, CAPTURE, PLAYBACK, LOG, DURATION, };
const option::Descriptor g_Usage[] =
{
	{ UNKNOWN,  0, "",   "",         option::Arg::None,	"Usage:\n"
							  	"  skeletonexport_nongui [options]\n\n"
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

	double duration(0.); // >0 only if a duration has been requested
	if (options[DURATION]) {
		// We know this will succeed because the argument has been checked.
		duration = static_cast<double>(strtol(options[DURATION].arg, NULL, 10));
		if (duration < 0.) {
			std::cerr << "Duration must be positive.\n";
			return EXIT_FAILURE;
		}
	}

	EnsureDepthGenerator();

	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
	if (nRetVal != XN_STATUS_OK)
	{
		nRetVal = g_UserGenerator.Create(g_Context);
		CHECK_RC(nRetVal, "Find user generator");
	}

	XnCallbackHandle hUserCallbacks, hCalibrationStart, hCalibrationComplete, hPoseDetected, hCalibrationInProgress, hPoseInProgress;
	if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
	{
		printf("Supplied user generator doesn't support skeleton\n");
		return 1;
	}
	nRetVal = g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
	CHECK_RC(nRetVal, "Register to user callbacks");
	nRetVal = g_UserGenerator.GetSkeletonCap().RegisterToCalibrationStart(UserCalibration_CalibrationStart, NULL, hCalibrationStart);
	CHECK_RC(nRetVal, "Register to calibration start");
	nRetVal = g_UserGenerator.GetSkeletonCap().RegisterToCalibrationComplete(UserCalibration_CalibrationComplete, NULL, hCalibrationComplete);
	CHECK_RC(nRetVal, "Register to calibration complete");

	if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration())
	{
		g_bNeedPose = TRUE;
		if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
		{
			printf("Pose required, but not supported\n");
			return 1;
		}
		nRetVal = g_UserGenerator.GetPoseDetectionCap().RegisterToPoseDetected(UserPose_PoseDetected, NULL, hPoseDetected);
		CHECK_RC(nRetVal, "Register to Pose Detected");
		g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
	}

	g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

	nRetVal = g_Context.StartGeneratingAll();
	CHECK_RC(nRetVal, "StartGenerating");

	if (options[LOG]) {
		std::string logfile_prefix(options[LOG].arg);
		std::string h5_logfile(logfile_prefix + ".h5"), txt_logfile(logfile_prefix + ".txt");
		std::cout << "Logging to " << h5_logfile << " and " << txt_logfile << '\n';
		g_Log.Open(h5_logfile.c_str(), txt_logfile.c_str());
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
	g_scriptNode.Release();
	g_DepthGenerator.Release();
	g_UserGenerator.Release();
	g_Player.Release();
	g_Context.Release();

	return EXIT_SUCCESS;
}

bool InitialiseContextFromRecording(const char* recordingFilename)
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = g_Context.Init();
	CHECK_RC_RETURNING(false, nRetVal, "Init");
	nRetVal = g_Context.OpenFileRecording(recordingFilename, g_Player);
	if (nRetVal != XN_STATUS_OK)
	{
		printf("Can't open recording %s: %s\n", recordingFilename, xnGetStatusString(nRetVal));
		return false;
	}
	return true;
}

bool InitialiseContextFromXmlConfig(const char* xmlConfigFilename)
{
	XnStatus nRetVal = XN_STATUS_OK;
	xn::EnumerationErrors errors;

	nRetVal = g_Context.InitFromXmlFile(xmlConfigFilename, g_scriptNode, &errors);
	if (nRetVal == XN_STATUS_NO_NODE_PRESENT)
	{
		XnChar strError[1024];
		errors.ToString(strError, 1024);
		printf("%s\n", strError);
		return false;
	}
	else if (nRetVal != XN_STATUS_OK)
	{
		printf("Open failed: %s\n", xnGetStatusString(nRetVal));
		return false;
	}

	return true;
}

bool EnsureDepthGenerator()
{
	XnStatus nRetVal = XN_STATUS_OK;

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	if (nRetVal != XN_STATUS_OK)
	{
		printf("No depth generator found. Using a default one...");
		xn::MockDepthGenerator mockDepth;
		nRetVal = mockDepth.Create(g_Context);
		CHECK_RC_RETURNING(false, nRetVal, "Create mock depth");

		// set some defaults
		XnMapOutputMode defaultMode;
		defaultMode.nXRes = 320;
		defaultMode.nYRes = 240;
		defaultMode.nFPS = 30;
		nRetVal = mockDepth.SetMapOutputMode(defaultMode);
		CHECK_RC_RETURNING(false, nRetVal, "set default mode");

		// set FOV
		XnFieldOfView fov;
		fov.fHFOV = 1.0225999419141749;
		fov.fVFOV = 0.79661567681716894;
		nRetVal = mockDepth.SetGeneralProperty(XN_PROP_FIELD_OF_VIEW, sizeof(fov), &fov);
		CHECK_RC_RETURNING(false, nRetVal, "set FOV");

		XnUInt32 nDataSize = defaultMode.nXRes * defaultMode.nYRes * sizeof(XnDepthPixel);
		XnDepthPixel* pData = (XnDepthPixel*)xnOSCallocAligned(nDataSize, 1, XN_DEFAULT_MEM_ALIGN);

		nRetVal = mockDepth.SetData(1, 0, nDataSize, pData);
		CHECK_RC_RETURNING(false, nRetVal, "set empty depth map");

		g_DepthGenerator = mockDepth;
	}

	return true;
}

// Callback: New user was detected
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& /*generator*/, XnUserID nId, void* /*pCookie*/)
{
	XnUInt32 epochTime = 0;
	xnOSGetEpochTime(&epochTime);
	printf("%d New User %d\n", epochTime, nId);
	// New user found
	if (g_bNeedPose)
	{
		g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
	}
	else
	{
		g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}

// Callback: An existing user was lost
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& /*generator*/, XnUserID nId, void* /*pCookie*/)
{
	XnUInt32 epochTime = 0;
	xnOSGetEpochTime(&epochTime);
	printf("%d Lost user %d\n", epochTime, nId);	
}

// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& /*capability*/, const XnChar* strPose, XnUserID nId, void* /*pCookie*/)
{
	XnUInt32 epochTime = 0;
	xnOSGetEpochTime(&epochTime);
	printf("%d Pose %s detected for user %d\n", epochTime, strPose, nId);
	g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}

// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& /*capability*/, XnUserID nId, void* /*pCookie*/)
{
	XnUInt32 epochTime = 0;
	xnOSGetEpochTime(&epochTime);
	printf("%d Calibration started for user %d\n", epochTime, nId);
}

// Callback: Finished calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationComplete(xn::SkeletonCapability& /*capability*/, XnUserID nId, XnCalibrationStatus eStatus, void* /*pCookie*/)
{
	XnUInt32 epochTime = 0;
	xnOSGetEpochTime(&epochTime);
	if (eStatus == XN_CALIBRATION_STATUS_OK)
	{
		// Calibration succeeded
		printf("%d Calibration complete, start tracking user %d\n", epochTime, nId);		
		g_UserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		printf("%d Calibration failed for user %d\n", epochTime, nId);
		if(eStatus==XN_CALIBRATION_STATUS_MANUAL_ABORT)
		{
			printf("Manual abort occured, stop attempting to calibrate!");
			return;
		}
		if (g_bNeedPose)
		{
			g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
		}
		else
		{
			g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
}
