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

using namespace H5;

extern xn::UserGenerator g_UserGenerator;
extern xn::DepthGenerator g_DepthGenerator;

// Dump joint data to output
void DumpJoint(std::ostream& os, XnUserID player, XnSkeletonJoint eJoint);

// Convert joint id to a human-friendly string
const char* NameJoint(XnSkeletonJoint joint);

DepthMapLogger::DepthMapLogger()
	: p_h5_file_(NULL), have_created_ds_(false)
{ }

DepthMapLogger::~DepthMapLogger()
{
	Close();
}

void DepthMapLogger::Open(const char* h5_filename, const char* log_filename)
{
	// Ensure closed
	Close();

	// Open new ones
	log_stream_.open(log_filename, std::ofstream::out | std::ofstream::binary);
	p_h5_file_ = new H5File(h5_filename, H5F_ACC_TRUNC);
}

void DepthMapLogger::Close()
{
	if(log_stream_.is_open()) {
		log_stream_.close();
	}

	// this invalidates all the rest of the datasets as well
	if(p_h5_file_) {
		// destroy file
		delete p_h5_file_;
	}

	// Reset pointer
	p_h5_file_ = NULL;
	have_created_ds_ = false;
}

// Called to ensure that datasets are created. It is called when we know what
// the x- and y-sizes of the depth buffer are. Returns true iff dataset
// creation succeeded.
bool DepthMapLogger::EnsureDatasets_(int w, int h)
{
	if(!p_h5_file_) { return false; }
	if(have_created_ds_) { return true; }

	H5File &file(*p_h5_file_);

	hsize_t chunk_dims[3] = {64, 64, 1};
	uint16_t fill_value = 0;

	// Reset frame count
	frame_count_ = 0;

	// Create depth dataset

	DSetCreatPropList creat_props;
	creat_props.setChunk(3, chunk_dims);
	creat_props.setFillValue(PredType::NATIVE_UINT16, &fill_value);

	hsize_t rows(static_cast<hsize_t>(h)), cols(static_cast<hsize_t>(w));
	hsize_t creation_dims[3] = { rows, cols, 1 };
	hsize_t max_dims[3] = { rows, cols, H5S_UNLIMITED };
	DataSpace mem_space(3, creation_dims, max_dims);

	depth_ds_ = file.createDataSet("depth", PredType::NATIVE_UINT16, mem_space, creat_props);
	label_ds_ = file.createDataSet("label", PredType::NATIVE_UINT16, mem_space, creat_props);

	have_created_ds_ = true;
	return true;
}

void DepthMapLogger::DumpDepthMap(const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd)
{
	if(!EnsureDatasets_(dmd.XRes(), dmd.YRes())) { return; }

	// This frame's index is the previous frame count
	hsize_t this_frame_idx = static_cast<hsize_t>(frame_count_);

	// Increment frame count
	frame_count_ += 1;

	// Get depth and label buffers
	const uint16_t *p_depths = dmd.Data();
	const uint16_t *p_labels = smd.Data();

	hsize_t creation_dims[3] = { dmd.YRes(), dmd.XRes(), 1 };
	hsize_t max_dims[3] = { dmd.YRes(), dmd.XRes(), 1 };
	DataSpace mem_space(3, creation_dims, max_dims);

	// Extend depth and label dataset to have correct size
	hsize_t new_size[3] = { dmd.YRes(), dmd.XRes(), static_cast<hsize_t>(frame_count_) };
	depth_ds_.extend(new_size);
	label_ds_.extend(new_size);

	// Select appropriate hyperslab for depth data from depth dataset
	DataSpace depth_slab = depth_ds_.getSpace();
	hsize_t depth_offset[3] = { 0, 0, this_frame_idx };
	hsize_t depth_slab_size[3] = { dmd.YRes(), dmd.XRes(), 1 };
	depth_slab.selectHyperslab(H5S_SELECT_SET, depth_slab_size, depth_offset);

	// Write depth data into slab
	depth_ds_.write(p_depths, PredType::NATIVE_UINT16, mem_space, depth_slab);

	// Select appropriate hyperslab for label data from label dataset
	DataSpace label_slab = label_ds_.getSpace();
	hsize_t label_offset[3] = { 0, 0, this_frame_idx };
	hsize_t label_slab_size[3] = { dmd.YRes(), dmd.XRes(), 1 };
	label_slab.selectHyperslab(H5S_SELECT_SET, label_slab_size, label_offset);

	// Write label data into slab
	label_ds_.write(p_labels, PredType::NATIVE_UINT16, mem_space, label_slab);

	std::ostream& os(log_stream_);

	// Start a depth map outputting the current frame id
	os << "FRAME:" << "idx=" << this_frame_idx << ',' << "id=" << dmd.FrameID() << '\n';

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

	os << "JOINT:" << "id=" << eJoint << ',' <<
		"name=" << NameJoint(eJoint) << ',' <<
		"confidence=" << joint.fConfidence << ',' <<
		"x=" << pt.X << ',' << "y=" << pt.Y << ',' << "z=" << pt.Z << ',' <<
		"u=" << imagePt.X << ',' << "v=" << imagePt.Y << '\n';
}

const char* NameJoint(XnSkeletonJoint joint)
{
	switch(joint)
	{
		case XN_SKEL_HEAD:
			return "head";
		case XN_SKEL_NECK:
			return "neck";
		case XN_SKEL_TORSO:
			return "torso";
		case XN_SKEL_WAIST:
			return "waist";
		case XN_SKEL_LEFT_COLLAR:
			return "leftcollar";
		case XN_SKEL_LEFT_SHOULDER:
			return "leftshoulder";
		case XN_SKEL_LEFT_ELBOW:
			return "elbow";
		case XN_SKEL_LEFT_WRIST:
			return "leftwrist";
		case XN_SKEL_LEFT_HAND:
			return "lefthand";
		case XN_SKEL_LEFT_FINGERTIP:
			return "leftfingertip";
		case XN_SKEL_RIGHT_COLLAR:
			return "rightcollar";
		case XN_SKEL_RIGHT_SHOULDER:
			return "rightshoulder";
		case XN_SKEL_RIGHT_ELBOW:
			return "rightelbow";
		case XN_SKEL_RIGHT_WRIST:
			return "rightwrist";
		case XN_SKEL_RIGHT_HAND:
			return "righthand";
		case XN_SKEL_RIGHT_FINGERTIP:
			return "rightfingertip";
		case XN_SKEL_LEFT_HIP:
			return "lefthip";
		case XN_SKEL_LEFT_KNEE:
			return "leftknee";
		case XN_SKEL_LEFT_ANKLE:
			return "leftankle";
		case XN_SKEL_LEFT_FOOT:
			return "leftfoot";
		case XN_SKEL_RIGHT_HIP:
			return "righthip";
		case XN_SKEL_RIGHT_KNEE:
			return "rightknee";
		case XN_SKEL_RIGHT_ANKLE:
			return "rightankle";
		case XN_SKEL_RIGHT_FOOT:
			return "rightfoot";
		default:
			return "unknown joint";
	}
}
