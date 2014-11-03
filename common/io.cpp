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

// A structure describing a joint
struct Joint {
	int id;
	float confidence;
	float x, y, z; // real-world
	float u, v, w; // projective
};

// Dump joint data to output
bool DumpJoint(XnUserID player, XnSkeletonJoint eJoint, Joint& out_joint);

// Convert joint id to a human-friendly string
const char* NameJoint(XnSkeletonJoint joint);

// An array of all joint types
const XnSkeletonJoint g_JointTypes[] = {
	XN_SKEL_HEAD, XN_SKEL_NECK, XN_SKEL_TORSO, XN_SKEL_WAIST,
	XN_SKEL_LEFT_COLLAR, XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_WRIST,
	XN_SKEL_LEFT_HAND, XN_SKEL_LEFT_FINGERTIP, XN_SKEL_RIGHT_COLLAR, XN_SKEL_RIGHT_SHOULDER,
	XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_WRIST, XN_SKEL_RIGHT_HAND, XN_SKEL_RIGHT_FINGERTIP,
	XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_ANKLE, XN_SKEL_LEFT_FOOT,
	XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_ANKLE, XN_SKEL_RIGHT_FOOT
};
const int g_NumJointTypes = 24;

DepthMapLogger::DepthMapLogger()
	: p_h5_file_(NULL), p_frames_group_(NULL)
	, joint_dt_(sizeof(Joint))
{
	// Create memory datatype for joints
	joint_dt_.insertMember(H5std_string("id"), HOFFSET(Joint, id), PredType::NATIVE_INT);
	joint_dt_.insertMember(H5std_string("confidence"), HOFFSET(Joint, confidence),
			PredType::NATIVE_FLOAT);
	joint_dt_.insertMember(H5std_string("x"), HOFFSET(Joint, x), PredType::NATIVE_FLOAT);
	joint_dt_.insertMember(H5std_string("y"), HOFFSET(Joint, y), PredType::NATIVE_FLOAT);
	joint_dt_.insertMember(H5std_string("z"), HOFFSET(Joint, z), PredType::NATIVE_FLOAT);
	joint_dt_.insertMember(H5std_string("u"), HOFFSET(Joint, u), PredType::NATIVE_FLOAT);
	joint_dt_.insertMember(H5std_string("v"), HOFFSET(Joint, v), PredType::NATIVE_FLOAT);
	joint_dt_.insertMember(H5std_string("w"), HOFFSET(Joint, w), PredType::NATIVE_FLOAT);
}

DepthMapLogger::~DepthMapLogger()
{
	Close();
}

void DepthMapLogger::Open(const char* h5_filename)
{
	// Ensure closed
	Close();

	// Open new ones
	p_h5_file_ = new H5File(h5_filename, H5F_ACC_TRUNC);

	// Create new group for storing frames
	p_frames_group_ = new Group(p_h5_file_->createGroup("frames"));
}

void DepthMapLogger::Close()
{
	// this invalidates all the rest of the datasets as well
	if(p_frames_group_) { delete p_frames_group_; }
	if(p_h5_file_) { delete p_h5_file_; }

	// Reset pointer
	p_h5_file_ = NULL;
	p_frames_group_ = NULL;
}

void DepthMapLogger::DumpDepthMap(const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd)
{
	static char name_str[20], comment_str[255];

	// Don't do anything if the h5 file is not open
	if(!p_h5_file_ || !p_frames_group_) { return; }

	// References to various bits of the HDF5 output
	H5File &file(*p_h5_file_);
	Group &frames_group(*p_frames_group_);

	// This frame's index is the number of frames we've previously saved
	hsize_t this_frame_idx = frames_group.getNumObjs();

	// Create this frame's group
	snprintf(name_str, 20, "frame_%06lld", this_frame_idx);
	snprintf(comment_str, 255, "Data for frame %lld", this_frame_idx);
	Group this_frame_group(frames_group.createGroup(name_str));
	this_frame_group.setComment(".", comment_str);

	// Create attributes for this group
	Attribute idx_attr = this_frame_group.createAttribute("idx", PredType::NATIVE_HSIZE, DataSpace());
	idx_attr.write(PredType::NATIVE_HSIZE, &this_frame_idx);

	// Create this frame's datasets
	DSetCreatPropList creat_props;
	uint16_t fill_value(0);
	creat_props.setFillValue(PredType::NATIVE_UINT16, &fill_value);

	hsize_t rows(static_cast<hsize_t>(dmd.YRes())), cols(static_cast<hsize_t>(dmd.XRes()));
	hsize_t creation_dims[2] = { rows, cols };
	hsize_t max_dims[2] = { rows, cols };
	DataSpace mem_space(2, creation_dims, max_dims);

	DataSet depth_ds(this_frame_group.createDataSet(
		"depth", PredType::NATIVE_UINT16, mem_space, creat_props));
	DataSet label_ds(this_frame_group.createDataSet(
		"label", PredType::NATIVE_UINT16, mem_space, creat_props));

	// Get depth and label buffers
	const uint16_t *p_depths = dmd.Data();
	const uint16_t *p_labels = smd.Data();

	// Write depth data
	depth_ds.write(p_depths, PredType::NATIVE_UINT16);

	// Write label data
	label_ds.write(p_labels, PredType::NATIVE_UINT16);

	// Convert non-zero depth values into 3D point positions
	XnPoint3D *pts = new XnPoint3D[rows*cols];
	uint16_t *pt_labels = new uint16_t[rows*cols];
	size_t n_pts(0);
	for(size_t depth_idx(0); depth_idx < rows*cols; ++depth_idx) {
		// Skip zero depth values
		if(p_depths[depth_idx] == 0) {
			continue;
		}

		// Store projective-values
		pts[n_pts].X = depth_idx % cols;
		pts[n_pts].Y = depth_idx / cols;
		pts[n_pts].Z = p_depths[depth_idx];
		pt_labels[n_pts] = p_labels[depth_idx];
		++n_pts;
	}
	g_DepthGenerator.ConvertProjectiveToRealWorld(n_pts, pts, pts);

	// Create points dataset
	hsize_t pts_creation_dims[2] = { n_pts, 3 };
	hsize_t pts_max_dims[2] = { n_pts, 3 };
	DataSpace pts_mem_space(2, pts_creation_dims, pts_max_dims);
	DataSet pts_ds(this_frame_group.createDataSet(
		"points", PredType::NATIVE_FLOAT, pts_mem_space, creat_props));
	hsize_t pt_labels_creation_dims[1] = { n_pts };
	hsize_t pt_labels_max_dims[1] = { n_pts };
	DataSpace pt_labels_mem_space(1, pt_labels_creation_dims, pt_labels_max_dims);
	DataSet pt_labels_ds(this_frame_group.createDataSet(
		"point_labels", PredType::NATIVE_UINT16, pt_labels_mem_space, creat_props));

	// Write points data
	pts_ds.write(pts, PredType::NATIVE_FLOAT);
	pt_labels_ds.write(pt_labels, PredType::NATIVE_UINT16);

	// Create groups to store detected users
	Group users_group(this_frame_group.createGroup("users"));

	// Dump each user in turn
	char strLabel[50] = "";
	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	g_UserGenerator.GetUsers(aUsers, nUsers);
	for (int i = 0; i < nUsers; ++i)
	{
		// Create a group for this user
		snprintf(name_str, 20, "user_%02d", aUsers[i]);
		Group this_user_group(users_group.createGroup(name_str));

		// Create attributes for this group
		Attribute user_idx_attr = this_user_group.createAttribute(
				"idx", PredType::NATIVE_UINT16, DataSpace());
		uint16_t this_user_idx(aUsers[i]);
		user_idx_attr.write(PredType::NATIVE_UINT16, &this_user_idx);


		// Write state (if any)
		H5std_string strwritebuf;
		if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i]))
		{
			strwritebuf = "tracking";
		}
		else if (g_UserGenerator.GetSkeletonCap().IsCalibrating(aUsers[i]))
		{
			// Calibrating
			strwritebuf = "calibrating";
		}
		else
		{
			// Nothing
			strwritebuf = "looking";
		}
		StrType strdatatype(PredType::C_S1, strwritebuf.size()); // of length 256 characters
		Attribute user_state_attr = this_user_group.createAttribute(
				"state", strdatatype, DataSpace());
		user_state_attr.write(strdatatype, strwritebuf);

		static Joint joints[g_NumJointTypes];
		int n_joints_found = 0;

		// Try to dump all joints
		for(int jt_idx=0; jt_idx < g_NumJointTypes; ++jt_idx)
		{
			if(DumpJoint(aUsers[i], g_JointTypes[jt_idx], joints[n_joints_found])) 
			{
				++n_joints_found;
			}
		}

		// Create joints dataset
		hsize_t joints_dim[] = { n_joints_found };
		DataSpace joints_space(1, joints_dim);
		DataSet joints_ds(this_user_group.createDataSet("joints", joint_dt_, joints_space));
		joints_ds.write(joints, joint_dt_);
	}
}

bool DumpJoint(XnUserID player, XnSkeletonJoint eJoint, Joint &out_joint)
{
	// Check the user is being tracked
	if (!g_UserGenerator.GetSkeletonCap().IsTracking(player))
	{
		return false;
	}

	// Check the joint is actually there
	if (!g_UserGenerator.GetSkeletonCap().IsJointActive(eJoint))
	{
		return false;
	}

	// Extract joint positions
	XnSkeletonJointPosition joint;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint, joint);

	XnPoint3D pt, imagePt;
	pt = joint.position;

	g_DepthGenerator.ConvertRealWorldToProjective(1, &pt, &imagePt);

	out_joint.id = eJoint;
	out_joint.confidence = joint.fConfidence;
	out_joint.x = pt.X;
	out_joint.y = pt.Y;
	out_joint.z = pt.Z;
	out_joint.u = imagePt.X;
	out_joint.v = imagePt.Y;
	out_joint.w = imagePt.Z;

	return true;
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
