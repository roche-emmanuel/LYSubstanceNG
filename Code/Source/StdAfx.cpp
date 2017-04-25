/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
/** @file StdAfx.cpp
	@brief Precompiled Header
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#include "StdAfx.h"
#include <AzCore/IO/SystemFile.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

AZStd::string getAbsoluteAssetPath(const AZStd::string& src)
{
	AZStd::string resolvedPath;
 
    if (gEnv->IsEditor())
    {
        // resolvedPath = "@assets@/" + string(_smtlPath.c_str());
		const char* resultValue = nullptr;
        EBUS_EVENT_RESULT(resultValue, AzToolsFramework::AssetSystemRequestBus, GetAbsoluteDevGameFolderPath);
		if(!resultValue) {
			logERROR("getAbsoluteAssetPath: no result from GetAbsoluteGameFolderPath()");
			return AZStd::string();
		}

		resolvedPath = AZStd::string(resultValue)+"/"+src;
    }
	else {
		char rpath[AZ_MAX_PATH_LEN] = { 0 };
		gEnv->pFileIO->ResolvePath(src.c_str(), rpath, AZ_MAX_PATH_LEN);

		resolvedPath = rpath;
	}

	return resolvedPath;
}