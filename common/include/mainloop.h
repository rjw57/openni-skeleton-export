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
#ifndef XNV_MAINLOOP_H___
#define XNV_MAINLOOP_H___

#include <XnOpenNI.h>
#include <XnCppWrapper.h>

extern xn::Context g_Context;
extern xn::ScriptNode g_scriptNode;
extern xn::DepthGenerator g_DepthGenerator;
extern xn::UserGenerator g_UserGenerator;
extern xn::Player g_Player;

bool InitialiseContextFromRecording(const char* recordingFilename);
bool InitialiseContextFromXmlConfig(const char* xmlConfigFilename);
bool PreMainLoop();
void PostMainLoop();

#endif // XNV_MAINLOOP_H___
