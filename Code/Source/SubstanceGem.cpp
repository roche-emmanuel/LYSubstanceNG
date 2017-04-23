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
/** @file SubstanceGem.cpp
	@brief Source file for the Substance Gem Implementation.
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#include "StdAfx.h"
#include <platform_impl.h>
#include "SubstanceGem.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <AzCore/IO/SystemFile.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#if defined(USE_SUBSTANCE)
#include "SubstanceAPI.h"
#include <CryLibrary.h>
#include <I3DEngine.h>
#include <IRenderer.h>

#include <Substance/framework/package.h>
#include <SubstanceMaterial.h>

//Cvars
int substance_coreCount;
int substance_memoryBudget;
ICVar* substance_engineLibrary;

static const char* kSubstance_EngineLibrary_Default = "sse2";

//////////////////////////////////////////////////////////////////////////
struct CTextureLoadHandler_Substance : public ITextureLoadHandler
{
	CTextureLoadHandler_Substance(ISubstanceLibAPI* pAPI) : m_SubstanceLibAPI(pAPI)
	{
	}

	virtual bool SupportsExtension(const char* ext) const override
	{
		if (!strcmp(ext, PROCEDURALTEXTURE_EXTENSION))
		{
			return true;
		}

		return false;
	}

	ETEX_Format SubstanceFormatToEngineFormat(SubstanceTexFormat substanceFormat) const
	{
		switch (substanceFormat)
		{
		case SubstanceTex_BC1:				return eTF_BC1;
		case SubstanceTex_BC2:				return eTF_BC2;
		case SubstanceTex_BC3:				return eTF_BC3;
		case SubstanceTex_L8:				return eTF_L8;
		case SubstanceTex_PVRTC2:			return eTF_PVRTC2;
		case SubstanceTex_PVRTC4:			return eTF_PVRTC4;	
		case SubstanceTex_R8G8B8A8:			return eTF_R8G8B8A8;
		case SubstanceTex_R16G16B16A16:		return eTF_R16G16B16A16;
		default:
			return eTF_Unknown;
		}
	}

	virtual bool LoadTextureData(const char* path, STextureLoadData& loadData) override
	{
		if (m_SubstanceLibAPI)
		{
			SSubstanceLoadData subData;

			memset(&subData, 0, sizeof(subData));
			subData.m_pTexture = reinterpret_cast<IDeviceTexture*>(loadData.m_pTexture);
			if (m_SubstanceLibAPI->LoadTextureData(path, subData))
			{
				loadData.m_pData = subData.m_pData;
				loadData.m_DataSize = subData.m_DataSize;
				loadData.m_Width = subData.m_Width;
				loadData.m_Height = subData.m_Height;
				loadData.m_Format = SubstanceFormatToEngineFormat(subData.m_Format);
				loadData.m_NumMips = subData.m_NumMips;
				loadData.m_nFlags = (subData.m_bNormalMap) ? FT_TEX_NORMAL_MAP : 0;

				return true;
			}
		}

		return false;
	}

	virtual void Update() override
	{
		if (m_SubstanceLibAPI)
		{
			m_SubstanceLibAPI->Update();
		}
	}

private:
	ISubstanceLibAPI* m_SubstanceLibAPI;
};

//////////////////////////////////////////////////////////////////////////
void OnSubstanceRuntimeBudgetChangled(bool budgetChanged)
{
	ISubstanceLibAPI *pSubstanceLibAPI = nullptr;
	EBUS_EVENT_RESULT(pSubstanceLibAPI, SubstanceRequestBus, GetSubstanceLibAPI);
	if (pSubstanceLibAPI)
	{
		pSubstanceLibAPI->OnRuntimeBudgetChanged(budgetChanged);
	}
}

void OnCVarCoreCountChange(ICVar *pArgs)
{
	substance_coreCount = pArgs->GetIVal();
	OnSubstanceRuntimeBudgetChangled(false);
}

void OnCVarMemoryBudgetChange(ICVar* pArgs)
{
	substance_memoryBudget = pArgs->GetIVal();
	OnSubstanceRuntimeBudgetChangled(false);
}

void CommitRenderOptions(IConsoleCmdArgs* pArgs)
{
	OnSubstanceRuntimeBudgetChangled(true);
}

//////////////////////////////////////////////////////////////////////////
SubstanceGem::SubstanceGem() : CryHooksModule(), m_SubstanceLib(nullptr), m_SubstanceLibAPI(nullptr), m_TextureLoadHandler(nullptr) { }
SubstanceGem::~SubstanceGem() { }

void SubstanceGem::PostGameInitialize()
{
	RegisterConsole();

	if (LoadEngineLibrary())
	{
		RegisterTextureHandler();
		CryLogAlways("Substance Initialized");
	}
	else
	{
		CryLogAlways("Substance Not Initialized\n");
	}
}

bool SubstanceGem::LoadEngineLibrary()
{
	if (I3DEngine* p3DEngine = gEnv->p3DEngine)
	{
		const char* szLibrary = substance_engineLibrary->GetString();

		string szFullLibrary = string("substance_") + szLibrary;

		m_SubstanceLib = CryLoadLibrary(szFullLibrary.c_str());

		//if load failed, reset to default value and try again
		if (m_SubstanceLib == nullptr && strcmp(szLibrary, kSubstance_EngineLibrary_Default))
		{
			substance_engineLibrary->Set(kSubstance_EngineLibrary_Default);
			return LoadEngineLibrary();
		}

		//moment of truth
		if (m_SubstanceLib != nullptr)
		{
			if (ProcGetSubstanceAPI getSubstanceAPI = (ProcGetSubstanceAPI)CryGetProcAddress(m_SubstanceLib, "GetSubstanceAPI"))
			{
				getSubstanceAPI(&m_SubstanceAPI, &m_SubstanceLibAPI);

				if (m_SubstanceLibAPI)
				{
					SubstanceRequestBus::Handler::BusConnect();
					return true;
				}
			}
		}
	}

	return false;
}

void SubstanceGem::RegisterConsole()
{
	REGISTER_CVAR_CB(substance_coreCount, 32, 0, "Set how many CPU Cores are used for Substance (32 = All). Only relevant when using CPU based engines.", OnCVarCoreCountChange);
	REGISTER_CVAR_CB(substance_memoryBudget, 512, 0, "Set how much memory is used for Substance in MB", OnCVarMemoryBudgetChange);

	substance_engineLibrary = REGISTER_STRING("substance_engineLibrary", kSubstance_EngineLibrary_Default, VF_NULL, "Set engine to load for substance plugin (PC: sse2/d3d10/d3d11)");

	REGISTER_COMMAND("substance_commitRenderOptions", CommitRenderOptions, VF_NULL, "Apply cpu and memory changes immediately, rather than wait for next render call");
}

void SubstanceGem::RegisterTextureHandler()
{
	if (I3DEngine* p3DEngine = gEnv->p3DEngine)
	{
		m_TextureLoadHandler = new CTextureLoadHandler_Substance(m_SubstanceLibAPI);
		p3DEngine->AddTextureLoadHandler(m_TextureLoadHandler);
	}
}

void SubstanceGem::UnregisterTextureHandler()
{
	if (I3DEngine* p3DEngine = gEnv->p3DEngine)
	{
		if (m_TextureLoadHandler)
		{
			p3DEngine->RemoveTextureLoadHandler(m_TextureLoadHandler);
			delete m_TextureLoadHandler;
			m_TextureLoadHandler = nullptr;
		}
	}
}


void SubstanceGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
		RegisterExternalFlowNodes();
		break;	
	case ESYSTEM_EVENT_GAME_POST_INIT:
		PostGameInitialize();
		break;
	case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
	case ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED:
	{
		if (gEnv->IsEditor())
		{
			if (wparam == 0)
			{
				m_SubstanceLibAPI->OnExitSimulation();
			}
			else
			{
				m_SubstanceLibAPI->OnEnterSimulation();
			}
		}
	}
	break;
	case ESYSTEM_EVENT_FAST_SHUTDOWN:
	case ESYSTEM_EVENT_FULL_SHUTDOWN:
		if (I3DEngine* p3DEngine = gEnv->p3DEngine)
		{
			UnregisterTextureHandler();

			if (m_SubstanceLibAPI)
			{
				SubstanceRequestBus::Handler::BusDisconnect();
				m_SubstanceLibAPI->Release();
				m_SubstanceLibAPI = nullptr;
			}

			CryFreeLibrary(m_SubstanceLib);
			m_SubstanceLib = nullptr;
		}
		break;
	}
}

int SubstanceGem::GetMinimumOutputSize() const
{
	return m_SubstanceLibAPI->GetMinimumOutputSize();
}

int SubstanceGem::GetMaximumOutputSize() const
{
	return m_SubstanceLibAPI->GetMaximumOutputSize();
}

IProceduralMaterial* SubstanceGem::GetMaterialFromPath(const char* path, bool bForceLoad) const
{
	// using namespace AZ::IO;

    // char rpath[AZ_MAX_PATH_LEN] = { 0 };
    // gEnv->pFileIO->ResolvePath(path, rpath, AZ_MAX_PATH_LEN);

	string rootPath = gEnv->pFileIO->GetAlias("@root@");
	string assetPath = gEnv->pFileIO->GetAlias("@assets@");
	AZ_TracePrintf("SubstanceGem", "Root path: %s", rootPath.c_str());
	AZ_TracePrintf("SubstanceGem", "Asset path: %s", assetPath.c_str());

	// string resolvedPath(rpath);
	string resolvedPath;
    if (gEnv->IsEditor())
    {
        // resolvedPath = "@assets@/" + string(path);
		const char* resultValue = nullptr;
        EBUS_EVENT_RESULT(resultValue, AzToolsFramework::AssetSystemRequestBus, GetAbsoluteDevGameFolderPath);
		if(!resultValue) {
			AZ_TracePrintf("SubstanceGem", "No result from GetAbsoluteGameFolderPath()");
			return nullptr;
		}

		resolvedPath = string(resultValue)+"/"+string(path);
    }

	if(!AZ::IO::SystemFile::Exists(resolvedPath.c_str())) {
		AZ_TracePrintf("SubstanceGem", "Material file %s doesn't exist yet'", resolvedPath.c_str());
		return nullptr;
	}

	// Create a new substance material and return it:
	AZ_TracePrintf("SubstanceGem", "Loading procedural material from path: %s", path);
	SubstanceMaterial* mat = new SubstanceMaterial(path);
	return mat;
	// return m_SubstanceLibAPI->GetMaterialFromPath(CryStringUtils::ToLower(path).c_str(), bForceLoad);
}

IGraphInstance* SubstanceGem::GetGraphInstance(GraphInstanceID graphInstanceID) const
{
	return m_SubstanceLibAPI->GetGraphInstance(graphInstanceID);
}

ProceduralMaterialRenderUID SubstanceGem::RenderEditorPreviewASync()
{
	return m_SubstanceLibAPI->RenderEditorPreviewASync();
}

void SubstanceGem::RenderEditorPreviewSync()
{
	return m_SubstanceLibAPI->RenderEditorPreviewSync();
}

void SubstanceGem::QueueRender(IGraphInstance* pGraphInstance)
{
	m_SubstanceLibAPI->QueueRender(pGraphInstance);
}

ProceduralMaterialRenderUID SubstanceGem::RenderASync()
{
	return m_SubstanceLibAPI->RenderASync();
}

void SubstanceGem::RenderSync()
{
	m_SubstanceLibAPI->RenderSync();
}

bool SubstanceGem::HasRenderCompleted(ProceduralMaterialRenderUID uid) const
{
	return m_SubstanceLibAPI->HasRenderCompleted(uid);
}

bool SubstanceGem::CreateProceduralMaterial(const char* basePath, const char* sbsarPath, const char* smtlPath)
{
	AZ_TracePrintf("SubstanceGem", "Should create prodecural material which sbsarPath=%s, smtlPath=%s", sbsarPath, smtlPath);
	
	// AZStd::string gameFolder = Path::GetEditingGameDataFolder();
	AZStd::string fullPath = basePath;
	fullPath +=AZStd::string("/")+AZStd::string(sbsarPath);
	// AZStd::string fullPath = sbsarPath;
	AZ_TracePrintf("SubstanceGem", "using full sbsar path: %s", fullPath.c_str());

	// load the content of the sbsar file:
	AZ::IO::SystemFile file;

	bool res = file.Open(fullPath.c_str(),AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
	if(!res) {
		AZ_TracePrintf("SubstanceGem", "ERROR: Cannot open file %s.", fullPath.c_str());
		return false;
	}

	// Get the length of the file:
	auto len = file.Length();
	AZ_TracePrintf("SubstanceGem", "file size is: %d bytes.", len);

	// Prepare our buffer:
	char* data = new char[len];
	memset(data, 0, len);

	// Read the content of the file:
	auto rlen = file.Read(len, data);
	if(rlen != len) {
		AZ_TracePrintf("SubstanceGem", "ERROR: did not read expected number of bytes: %d != %d.", rlen, len);
		return false;	
	}

	// We may close the file:
	file.Close();

	// So here we try to load our package:
	SubstanceAir::PackageDesc* pdesc = nullptr;
	try {
		pdesc = new SubstanceAir::PackageDesc(data,len);
	}
	catch(...) {
		AZ_TracePrintf("SubstanceGem", "Exception occured when trying to create substance package.");
		return false;
	}

	// We can delete our buffer:
	delete [] data;

	// This package should not be valid:
	AZ_TracePrintf("SubstanceGem", "Substance package is: %s", pdesc->isValid() ? "VALID" : "INVALID");

	if(!pdesc->isValid()) {
		return false;
	}

	// Retrieve the graphs in this package:
	AZ_TracePrintf("SubstanceGem", "Substance package contains %d graphs.", pdesc->getGraphs().size());

	// We only consider the first graph available,
	// and we retrieve the outputs from it:
	auto graph = pdesc->getGraphs().front();

	auto outs = graph.mOutputs;
	AZ_TracePrintf("SubstanceGem", "Package main graph contains %d outputs.", outs.size());

	AZStd::string content = string_format("<ProceduralMaterial Source=\"%s\">\n", sbsarPath);
	
	// List the output IDs and usage:
	AZStd::string fbase = sbsarPath;
	fbase = fbase.substr(0,fbase.size()-6);

	for(auto& out: outs) {
		AZ_TracePrintf("SubstanceGem", "Found output of type %d with id=%d.", (int)out.mChannel, out.mUid);
		AZStd::string otype = "";
		switch(out.mChannel) {
		case SubstanceAir::Channel_Diffuse: 
			otype = "diffuse"; break;
		case SubstanceAir::Channel_Normal: 
			otype = "normal"; break;
		case SubstanceAir::Channel_Specular: 
			otype = "specular"; break;
		case SubstanceAir::Channel_Height: 
			otype = "height"; break;
		default:
			AZ_TracePrintf("SubstanceGem", "Ignoring output of type %d.", (int)out.mChannel);
			break;
		}
		if(!otype.empty()) {
			// Add a line in the output content:
			content += string_format("  <Output ID=\"%d\" Enabled=\"1\" Compressed=\"1\" File=\"%s_%s.sub\" />\n", out.mUid,fbase.c_str(),otype.c_str());
		}
	}

	// Todo: handle the input parameters here too.

	// close the parent tag:
	content += "</ProceduralMaterial>\n";

	AZ_TracePrintf("SubstanceGem", "Should write smtl content: %s", content.c_str());
	
	// Write this file:
	fullPath = basePath+AZStd::string("/")+AZStd::string(smtlPath);
	res = file.Open(fullPath.c_str(),AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY|AZ::IO::SystemFile::SF_OPEN_CREATE);
	if(!res) {
		AZ_TracePrintf("SubstanceGem", "ERROR: Cannot open file %s for writing.", fullPath.c_str());
		return false;
	}

	rlen = file.Write(content.c_str(), content.size());
	if(rlen != content.size()) {
		AZ_TracePrintf("SubstanceGem", "ERROR: did not write expected number of bytes: %d != %d.", rlen, content.size());
		return false;	
	}

	file.Close();

	AZ_TracePrintf("SubstanceGem", "SMTL file %s written successfully.", fullPath.c_str());

	return true;
	//return m_SubstanceLibAPI->CreateProceduralMaterial(sbsarPath, smtlPath);
}

bool SubstanceGem::SaveProceduralMaterial(IProceduralMaterial* pMaterial, const char* path)
{
	return m_SubstanceLibAPI->SaveProceduralMaterial(pMaterial, path);
}

void SubstanceGem::RemoveProceduralMaterial(IProceduralMaterial* pMaterial)
{
	m_SubstanceLibAPI->RemoveProceduralMaterial(pMaterial);
}

ISubstanceLibAPI* SubstanceGem::GetSubstanceLibAPI() const
{
	return m_SubstanceLibAPI;
}
#else
SubstanceGem::SubstanceGem() : CryHooksModule() { }
SubstanceGem::~SubstanceGem() { }
void SubstanceGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) { }
#endif // USE_SUBSTANCE

AZ_DECLARE_MODULE_CLASS(Substance_a2f08ba9713f485a8485d7588e5b120f, SubstanceGem)

