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
/** @file ProceduralMaterialEditorPlugin.h
	@brief Library hook to register plugin features
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#ifndef SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_PROCEDURALMATERIALSCANNER_H
#define SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_PROCEDURALMATERIALSCANNER_H
#pragma once

#if defined(USE_SUBSTANCE)

#include <IThreadTask.h>

typedef std::vector<string> TFileVector;

class CProceduralMaterialScanner : public IThreadTask
{
public:
	static CProceduralMaterialScanner *Instance();

	CProceduralMaterialScanner();

	void StartScan();
	void StopScan();

	bool GetLoadedFiles(TFileVector& files);

private:
	void AddFiles();

	static bool ScanUpdateCallback(const CString& msg);

	virtual void OnUpdate();

	virtual void Stop() {}
	virtual struct SThreadTaskInfo* GetTaskInfo() { return &m_TaskInfo; };

private:
	CryCriticalSection m_lock;
	TFileVector m_filesForUser;
	TFileVector m_files;
	bool m_bNewFiles;
	bool m_bDone;

protected:
	SThreadTaskInfo m_TaskInfo;
};

#endif // USE_SUBSTANCE
#endif //SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_PROCEDURALMATERIALSCANNER_H
