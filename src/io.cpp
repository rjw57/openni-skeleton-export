/*****************************************************************************
*                                                                            *
*  Copyright (C) 2014 Rich Wareham <rich.openni@richwareham.com>             *
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
// Support for saving frames to disk
//---------------------------------------------------------------------------

#include "io.h"

extern xn::UserGenerator g_UserGenerator;
extern xn::DepthGenerator g_DepthGenerator;

// Dump joint data to output
void DumpJoint(std::ostream& os, XnUserID player, XnSkeletonJoint eJoint);

void DumpDepthMap(std::ostream& os, const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd)
{
	// Start a depth map outputting the current frame id
	os << "FRAME:" << dmd.FrameID() << '\n';

	// Dump each user in turn
	char strLabel[50] = "";
	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	g_UserGenerator.GetUsers(aUsers, nUsers);
	for (int i = 0; i < nUsers; ++i)
	{
		// Write current user id
		os << "USER:" << aUsers[i] << '\n';

		// Write state (if any)
		if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i]))
		{
			os << "STATE:Tracking\n";
		}
		else if (g_UserGenerator.GetSkeletonCap().IsCalibrating(aUsers[i]))
		{
			// Calibrating
			os << "STATE:Calibrating\n";
		}
		else
		{
			// Nothing
			os << "STATE:Looking\n";
		}

		// Are we actually tracking a user?
		if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i]))
		{
			// Try to dump all joints
			DumpJoint(os, aUsers[i], XN_SKEL_HEAD);
			DumpJoint(os, aUsers[i], XN_SKEL_NECK);
			DumpJoint(os, aUsers[i], XN_SKEL_TORSO);
			DumpJoint(os, aUsers[i], XN_SKEL_WAIST);

			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_COLLAR);
			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_SHOULDER);
			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_ELBOW);
			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_WRIST);
			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_HAND);
			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_FINGERTIP);

			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_COLLAR);
			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_SHOULDER);
			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_ELBOW);
			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_WRIST);
			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_HAND);
			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_FINGERTIP);

			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_HIP);
			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_KNEE);
			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_ANKLE);
			DumpJoint(os, aUsers[i], XN_SKEL_LEFT_FOOT);

			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_HIP);
			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_KNEE);
			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_ANKLE);
			DumpJoint(os, aUsers[i], XN_SKEL_RIGHT_FOOT);
		}

		// Finished recording user
		os << "ENDUSER\n";
	}
	os << "ENDFRAME" << '\n';
}

void DumpJoint(std::ostream& os, XnUserID player, XnSkeletonJoint eJoint)
{
	// Check the user is being tracked
	if (!g_UserGenerator.GetSkeletonCap().IsTracking(player))
	{
		return;
	}

	// Check the joint is actually there
	if (!g_UserGenerator.GetSkeletonCap().IsJointActive(eJoint))
	{
		return;
	}

	// Extract joint positions
	XnSkeletonJointPosition joint;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint, joint);

	XnPoint3D pt, imagePt;
	pt = joint.position;

	g_DepthGenerator.ConvertRealWorldToProjective(1, &pt, &imagePt);

	os << "JOINT:" << eJoint << ',' <<
		joint.fConfidence << ',' <<
		pt.X << ',' << pt.Y << ',' << pt.Z << ',' <<
		imagePt.X << ',' << imagePt.Y << '\n';
}
