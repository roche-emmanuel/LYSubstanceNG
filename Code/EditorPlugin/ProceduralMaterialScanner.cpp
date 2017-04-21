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
#include "stdafx.h"

#if defined(USE_SUBSTANCE)

#include "ProceduralMaterialScanner.h"

#include "Util/PathUtil.h"
#include "ICryPak.h"

CProceduralMaterialScanner* CProceduralMaterialScanner::Instance()
{
	static CProceduralMaterialScanner s_instance;
	return &s_instance;
}

CProceduralMaterialScanner::CProceduralMaterialScanner()
	: m_bNewFiles(false)
	, m_bDone(false)
{
}

void CProceduralMaterialScanner::StartScan()
{
	// Kill last scan job and wait until it finishes
	GetISystem()->GetIThreadTaskManager()->UnregisterTask(this);

	// Prepare and start new job
	m_lock.Lock();
	m_filesForUser.clear();
	m_files.clear();
	m_bDone = false;
	m_lock.Unlock();

	SThreadTaskParams params;
	params.nFlags |= THREAD_TASK_BLOCKING;
	params.name = "Substance Files Scanning";
	GetISystem()->GetIThreadTaskManager()->RegisterTask(this, params);
}

void CProceduralMaterialScanner::StopScan()
{
	GetISystem()->GetIThreadTaskManager()->UnregisterTask(this);
}

bool CProceduralMaterialScanner::GetLoadedFiles(TFileVector& files)
{
	CryAutoCriticalSection lock(m_lock);

	if (m_bNewFiles)
	{
		files = m_filesForUser;
		m_filesForUser.clear();
		m_bNewFiles = false;
		return true;
	}

	return false;
}

void CProceduralMaterialScanner::AddFiles()
{
	CryAutoCriticalSection lock(m_lock);

    for (size_t i = 0; i < m_files.size(); i++)
    {
        m_files[i] = PathUtil::ToUnixPath(m_files[i]);
    }
    m_filesForUser.insert(m_filesForUser.begin(), m_files.begin(), m_files.end());
    m_files.clear();
    m_bNewFiles = true;
}

bool CProceduralMaterialScanner::ScanUpdateCallback(const CString& msg)
{
	Instance()->AddFiles();
	return true;
}

void CProceduralMaterialScanner::OnUpdate()
{
	m_lock.Lock();
	if (m_bDone)
	{
		m_lock.Unlock();
		// Spin until thread gets culled
		Sleep(10);
		return;
	}
	m_lock.Unlock();

	m_lock.Lock();
	m_files.reserve(5000); // Reserve space for 5000 files.			
	m_lock.Unlock();

	SDirectoryEnumeratorHelper dirHelper;
	dirHelper.ScanDirectoryRecursive(Path::GetEditingGameDataFolder().c_str(), "", "*.smtl", m_files);

	AddFiles();

	m_lock.Lock();
	m_bDone = true;
	m_lock.Unlock();
}
#endif // USE_SUBSTANCE
