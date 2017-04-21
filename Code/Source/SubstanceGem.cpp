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


#if defined(USE_SUBSTANCE)
#include "SubstanceAPI.h"
#include <CryLibrary.h>
#include <I3DEngine.h>
#include <IRenderer.h>

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
	return m_SubstanceLibAPI->GetMaterialFromPath(CryStringUtils::ToLower(path).c_str(), bForceLoad);
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

bool SubstanceGem::CreateProceduralMaterial(const char* sbsarPath, const char* smtlPath)
{
	return m_SubstanceLibAPI->CreateProceduralMaterial(sbsarPath, smtlPath);
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

