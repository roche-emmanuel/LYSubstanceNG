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
/** @file Util.h
	@brief Header for Utility functions
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#ifndef SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_UTIL_H
#define SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_UTIL_H
#pragma once

namespace Util
{
	inline QString ToUnixPath(const QString& strPath)
	{
		if (strPath.indexOf('\\') != -1)
		{
			QString path = strPath;
			path.replace('\\', '/');
			return path;
		}
		return strPath;
	}

	inline QString AbsolutePathToGamePath(const QString& filename)
	{
		QString dir = ToUnixPath(QString(Path::GetEditingGameDataFolder().c_str())).toLower();
		QString stdFilename = ToUnixPath(filename).toLower();
		
		int rpos = stdFilename.lastIndexOf(dir);
		if (rpos >= 0)
		{
			stdFilename = stdFilename.remove(0, rpos + dir.length() + 1);
		}

		return stdFilename;
	}
};

#endif //SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_UTIL_H
