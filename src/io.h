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
#ifndef XNV_IO_H__
#define XNV_IO_H__

#include <iostream>
#include <fstream>
#include <XnCppWrapper.h>
#include <hdf5.h>
#include <H5Cpp.h>

class DepthMapLogger
{
protected:
	H5::H5File    *p_h5_file_;
	std::ofstream log_stream_;
	bool          have_created_ds_;
	int64_t       frame_count_;

	H5::DataSet   depth_ds_;
	H5::DataSet   label_ds_;

	bool EnsureDatasets_(int w, int h);
public:
	DepthMapLogger();
	~DepthMapLogger();

	void Open(const char* h5_filename, const char* log_filename);
	void Close();

	// Dump the current depth map to the specified output stream.
	void DumpDepthMap(const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd);
};

#endif // XNV_IO_H__
