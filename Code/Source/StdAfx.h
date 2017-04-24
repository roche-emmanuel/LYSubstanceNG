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
/** @file StdAfx.h
	@brief Precompiled Header
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#ifndef GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_SOURCE_STDAFX_H
#define GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_SOURCE_STDAFX_H
#pragma once

#define eCryModule eCryM_Game

#include <platform.h>

#ifdef USE_SUBSTANCE
#include <CryWindows.h> // needed for UINT defines used by CImageExtensionHelper
#include <CryName.h>
#include <I3DEngine.h>
#include <ISerialize.h>
#include <IGem.h>
#define GEMSUBSTANCE_EXPORTS

#if defined (GEMSUBSTANCE_EXPORTS)
#define GEMSUBSTANCE_API DLL_EXPORT
#else
#define GEMSUBSTANCE_API DLL_IMPORT
#endif

#define logDEBUG(msg) { \
    std::ostringstream os; \
    os.precision(9); \
    os << std::fixed << msg; \
    AZ_TracePrintf("SubstanceNG", os.str().c_str()); \
}

#define logERROR(msg) { \
    std::ostringstream os; \
    os.precision(9); \
    os << std::fixed << msg; \
    AZ_Error("SubstanceNG", false, os.str().c_str()); \
}

#define CHECK(cond,msg) if(!(cond)) { logERROR(msg); return; }
#define CHECK_RET(cond,ret,msg) if(!(cond)) { THROW_MSG(msg); return ret; }

#include "Substance/IProceduralMaterial.h"

template<typename ... Args>
AZStd::string string_format( const AZStd::string& format, Args ... args )
{
	size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
	std::unique_ptr<char[]> buf( new char[ size ] );
	snprintf( buf.get(), size, format.c_str(), args ... );
	return AZStd::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}


//CVars
extern int substance_coreCount;
extern int substance_memoryBudget;

#define DIMOF(x) (sizeof(x)/sizeof(x[0]))
#endif

#endif//GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_SOURCE_STDAFX_H

