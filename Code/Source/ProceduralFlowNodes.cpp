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
/** @file ProceduralFlowNodes.cpp
	@brief Source File for the Procedural Flow Graph System
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#include "StdAfx.h"

#if defined(USE_SUBSTANCE)
#include "ProceduralFlowNodes.h"
#include <Substance/SubstanceBus.h>
#include "SubstanceAPI.h"

//--------------------------------------------------------------------------------------------
class CProceduralMaterialFlowNodeBase : public CFlowBaseNode<eNCT_Instanced>
{
public:
	CProceduralMaterialFlowNodeBase() {}

	IGraphInstance* GetGraphInstance(GraphInstanceID graphInstanceID) const
	{
		IGraphInstance* pGraph = nullptr;
		// EBUS_EVENT_RESULT(pGraph, SubstanceRequestBus, GetGraphInstance, graphInstanceID);
		return pGraph;
	}
};

//--------------------------------------------------------------------------------------------
class CProceduralMaterialFlowNodeGetGraphInstanceID : public CProceduralMaterialFlowNodeBase
{
public:
	CProceduralMaterialFlowNodeGetGraphInstanceID(SActivationInfo* pActInfo)
		: CProceduralMaterialFlowNodeBase()
	{
	}

	void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<string>("ProceduralMaterial", ""),
			InputPortConfig<int>("GraphIndex", 0),
			InputPortConfig_Void("Get", _HELP("Get GraphInstanceID From Material and GraphIndex")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<GraphInstanceID>("Result", "Obtain GraphInstanceID"),
			{ 0 }
		};

		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = "Get Graph Instance ID";
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo) override
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, eI_Get))
			{
				GraphInstanceID graphInstanceID = INVALID_GRAPHINSTANCEID;
				const char* szMaterialPath = GetPortString(pActInfo, eI_Material);
				const int graphIndex = GetPortInt(pActInfo, eI_GraphIndex);

				IProceduralMaterial* pMaterial = nullptr;
				EBUS_EVENT_RESULT(pMaterial, SubstanceRequestBus, GetMaterialFromPath, szMaterialPath, false);
				if (pMaterial)
				{
					ISubstanceLibAPI *pSubstanceLibAPI = nullptr;
					EBUS_EVENT_RESULT(pSubstanceLibAPI, SubstanceRequestBus, GetSubstanceLibAPI);
					if (pSubstanceLibAPI)
					{
						graphInstanceID = pSubstanceLibAPI->EncodeGraphInstanceID(pMaterial, graphIndex);
					}
				}

				ActivateOutput(pActInfo, eO_Result, graphInstanceID);
			}
			break;
		}
	}

	IFlowNodePtr Clone(SActivationInfo *pActInfo) override
	{
		return new CProceduralMaterialFlowNodeGetGraphInstanceID(pActInfo);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

private:
	enum InputPorts
	{
		eI_Material = 0,
		eI_GraphIndex,
		eI_Get,
	};

	enum OutputPorts
	{
		eO_Result = 0,
	};
};
REGISTER_FLOW_NODE("ProceduralMaterial:GetGraphInstanceID", CProceduralMaterialFlowNodeGetGraphInstanceID)

//--------------------------------------------------------------------------------------------
class CProceduralMaterialFlowNodeInputBase : public CProceduralMaterialFlowNodeBase
{
public:
	CProceduralMaterialFlowNodeInputBase()
		: CProceduralMaterialFlowNodeBase()
	{
	}
};

//--------------------------------------------------------------------------------------------
template<typename T> T GetFlowNodeDefaultValue();
template<> int GetFlowNodeDefaultValue<int>() { return 0; }
template<> float GetFlowNodeDefaultValue<float>() { return 0.0f; }

template<typename T, int DIM>
class TProceduralMaterialFlowNode_SetInput : public CProceduralMaterialFlowNodeInputBase
{
public:
	TProceduralMaterialFlowNode_SetInput(SActivationInfo* pActInfo)
		: CProceduralMaterialFlowNodeInputBase()
	{
	}

	void GetConfiguration(SFlowNodeConfig& config) override 
	{
		static const SInputPortConfig inputGraphInstanceID = InputPortConfig<GraphInstanceID>("GraphInstanceID", "");
		static const SInputPortConfig inputParameter = InputPortConfig<string>("ParameterName", "");
		static const SInputPortConfig inputApply = InputPortConfig<SFlowSystemVoid>("Apply");

		static const SInputPortConfig inputValues[] = {
			InputPortConfig<T>("Value1", GetFlowNodeDefaultValue<T>(), "Value 1"),
			InputPortConfig<T>("Value2", GetFlowNodeDefaultValue<T>(), "Value 2"),
			InputPortConfig<T>("Value3", GetFlowNodeDefaultValue<T>(), "Value 3"),
			InputPortConfig<T>("Value4", GetFlowNodeDefaultValue<T>(), "Value 4"),
		};

		//assemble the input ports
		static SInputPortConfig in_config[3+DIM+1];

		in_config[0] = inputGraphInstanceID;
		in_config[1] = inputParameter;
		
		for (int i = 0;i < DIM;i++)
		{
			in_config[2+i] = inputValues[i];
		}

		in_config[3+DIM-1] = inputApply;
		in_config[3+DIM] = { 0 };

		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_Void("Done", "Triggered when node input completes"),
			{ 0 }
		};

		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = "Set Substance Input";
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo) override
	{
		switch (event)
		{
		case eFE_Initialize:
			break;
		case eFE_Activate:
			if (IsPortActive(pActInfo, eI_Apply))
			{
				GraphInstanceID graphInstance = GetPortGraphInstanceID(pActInfo, eI_GraphInstanceID);
				const string& parameterName = GetPortString(pActInfo, eI_ParameterName);
				CRY_ASSERT(graphInstance != INVALID_GRAPHINSTANCEID);

				if (IGraphInstance* pGraphInstance = GetGraphInstance(graphInstance))
				{
					if (IGraphInput* pInput = pGraphInstance->GetInputByName(parameterName.c_str()))
					{
						T values[DIM];

						memset(values, 0, sizeof(values));

						for (int i = 0; i < DIM; i++)
						{
							values[i] = *(GetPortAny(pActInfo, eI_ValueStart + i).GetPtr<T>());
						}

						pInput->SetValue(values);
					}
					else
					{
						CRY_ASSERT_MESSAGE(false, "Input Not Found for GraphInstance");
					}
				}
				else
				{
					CRY_ASSERT_MESSAGE(false, "Invalid GraphInstanceID provided");
				}

				ActivateOutput(pActInfo, eO_OnActionStart, true);
			}
			break;
		}
	}

private:
	enum OutputPorts
	{
		eO_OnActionStart = 0,
	};

	static const unsigned int eI_GraphInstanceID = 0;
	static const unsigned int eI_ParameterName = 1;
	static const unsigned int eI_ValueStart = 2;
	static const unsigned int eI_Apply = eI_ValueStart + DIM;
};

//--------------------------------------------------------------------------------------------
#define REGISTER_PROCEDURALMATERIAL_SETINPUT_FLOW_NODE(className, name, type, dims) \
	class CProceduralMaterialFloatNode_SetInput##type##dims : public TProceduralMaterialFlowNode_SetInput < type, dims > \
	{ \
	public: \
		CProceduralMaterialFloatNode_SetInput##type##dims(SActivationInfo* pActInfo) \
			: TProceduralMaterialFlowNode_SetInput<type, dims>(pActInfo) \
		{ \
		} \
		IFlowNodePtr Clone(SActivationInfo *pActInfo) override \
		{ \
			return new CProceduralMaterialFloatNode_SetInput##type##dims(pActInfo); \
		} \
		void GetMemoryUsage(ICrySizer * s) const \
		{ \
			s->Add(*this); \
		} \
	}; \
	REGISTER_FLOW_NODE(className, CProceduralMaterialFloatNode_SetInput##type##dims)

REGISTER_PROCEDURALMATERIAL_SETINPUT_FLOW_NODE("ProceduralMaterial:SetInputFloat", Float, float, 1)
REGISTER_PROCEDURALMATERIAL_SETINPUT_FLOW_NODE("ProceduralMaterial:SetInputFloat2", Float2, float, 2)
REGISTER_PROCEDURALMATERIAL_SETINPUT_FLOW_NODE("ProceduralMaterial:SetInputFloat3", Float3, float, 3)
REGISTER_PROCEDURALMATERIAL_SETINPUT_FLOW_NODE("ProceduralMaterial:SetInputFloat4", Float4, float, 4)
REGISTER_PROCEDURALMATERIAL_SETINPUT_FLOW_NODE("ProceduralMaterial:SetInputInt", Integer, int, 1)
REGISTER_PROCEDURALMATERIAL_SETINPUT_FLOW_NODE("ProceduralMaterial:SetInputInt2", Integer2, int, 2)
REGISTER_PROCEDURALMATERIAL_SETINPUT_FLOW_NODE("ProceduralMaterial:SetInputInt3", Integer3, int, 3)
REGISTER_PROCEDURALMATERIAL_SETINPUT_FLOW_NODE("ProceduralMaterial:SetInputInt4", Integer4, int, 4)

template<typename T, int DIM>
class TProceduralMaterialFlowNode_GetInput : public CProceduralMaterialFlowNodeInputBase
{
public:
	TProceduralMaterialFlowNode_GetInput(SActivationInfo* pActInfo)
		: CProceduralMaterialFlowNodeInputBase()
	{
	}

	void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<GraphInstanceID>("GraphInstanceID", ""),
			InputPortConfig<string>("ParameterName", ""),
			InputPortConfig<SFlowSystemVoid>("Get", _HELP("Get parameters")),
			{ 0 }
		};

		static const SOutputPortConfig outputDone = OutputPortConfig_Void("Done", "Triggered when node input completes");

		static const SOutputPortConfig outputValues[] = {
			OutputPortConfig<T>("Value1", "Value 1"),
			OutputPortConfig<T>("Value2", "Value 2"),
			OutputPortConfig<T>("Value3", "Value 3"),
			OutputPortConfig<T>("Value4", "Value 4"),
		};
		
		static SOutputPortConfig out_config[DIM + 2];

		for (int i = 0;i < DIM;i++)
		{
			out_config[i] = outputValues[i];
		}

		out_config[DIM] = outputDone;
		out_config[DIM + 1] = { 0 };

		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = "Get Substance Input";
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo) override
	{
		switch (event)
		{
		case eFE_Initialize:
			break;
		case eFE_Activate:
			if (IsPortActive(pActInfo, eI_Get))
			{
				GraphInstanceID graphInstance = GetPortGraphInstanceID(pActInfo, eI_GraphInstanceID);
				const string& parameterName = GetPortString(pActInfo, eI_ParameterName);

				CRY_ASSERT(graphInstance != INVALID_GRAPHINSTANCEID);

				if (IGraphInstance* pGraphInstance = GetGraphInstance(graphInstance))
				{
					if (IGraphInput* pInput = pGraphInstance->GetInputByName(parameterName.c_str()))
					{
                        const T* pInputValues = static_cast<const T*>(pInput->GetValue());
						for (int i = 0; i < DIM; i++)
						{
							ActivateOutput(pActInfo, i, pInputValues[i]);
						}
					}
					else
					{
						CRY_ASSERT_MESSAGE(false, "Input Not Found for GraphInstance");
					}
				}
				else
				{
					CRY_ASSERT_MESSAGE(false, "Invalid GraphInstanceID provided");
				}

				ActivateOutput(pActInfo, eO_Done, true);
			}
			break;
		}
	}

private:
	enum InputPorts
	{
		eI_GraphInstanceID = 0,
		eI_ParameterName,
		eI_Get,
	};

	static const unsigned int eO_Done = DIM;
};

//--------------------------------------------------------------------------------------------
#define REGISTER_PROCEDURALMATERIAL_GETINPUT_FLOW_NODE(className, name, type, dims) \
	class CProceduralMaterialFloatNode_GetInput##type##dims : public TProceduralMaterialFlowNode_GetInput < type, dims > \
	{ \
	public: \
		CProceduralMaterialFloatNode_GetInput##type##dims(SActivationInfo* pActInfo) \
			: TProceduralMaterialFlowNode_GetInput<type, dims>(pActInfo) \
		{ \
		} \
		IFlowNodePtr Clone(SActivationInfo *pActInfo) override \
		{ \
			return new CProceduralMaterialFloatNode_GetInput##type##dims(pActInfo); \
		} \
		void GetMemoryUsage(ICrySizer * s) const \
		{ \
			s->Add(*this); \
		} \
	}; \
	REGISTER_FLOW_NODE(className, CProceduralMaterialFloatNode_GetInput##type##dims)

REGISTER_PROCEDURALMATERIAL_GETINPUT_FLOW_NODE("ProceduralMaterial:GetInputFloat", Float, float, 1)
REGISTER_PROCEDURALMATERIAL_GETINPUT_FLOW_NODE("ProceduralMaterial:GetInputFloat2", Float2, float, 2)
REGISTER_PROCEDURALMATERIAL_GETINPUT_FLOW_NODE("ProceduralMaterial:GetInputFloat3", Float3, float, 3)
REGISTER_PROCEDURALMATERIAL_GETINPUT_FLOW_NODE("ProceduralMaterial:GetInputFloat4", Float4, float, 4)
REGISTER_PROCEDURALMATERIAL_GETINPUT_FLOW_NODE("ProceduralMaterial:GetInputInt", Integer, int, 1)
REGISTER_PROCEDURALMATERIAL_GETINPUT_FLOW_NODE("ProceduralMaterial:GetInputInt2", Integer2, int, 2)
REGISTER_PROCEDURALMATERIAL_GETINPUT_FLOW_NODE("ProceduralMaterial:GetInputInt3", Integer3, int, 3)
REGISTER_PROCEDURALMATERIAL_GETINPUT_FLOW_NODE("ProceduralMaterial:GetInputInt4", Integer4, int, 4)


//--------------------------------------------------------------------------------------------
class CProceduralMaterialFlowNode_GetInputImage : public CProceduralMaterialFlowNodeInputBase
{
public:
	CProceduralMaterialFlowNode_GetInputImage(SActivationInfo* pActInfo)
		: CProceduralMaterialFlowNodeInputBase()
	{
	}

	void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<GraphInstanceID>("GraphInstanceID", ""),
			InputPortConfig<string>("ParameterName", ""),
			InputPortConfig<SFlowSystemVoid>("Get", _HELP("Get input value")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<string>("Value", _HELP("Path to image input")),
			OutputPortConfig_Void("Done", _HELP("Triggered when node input completes")),
			{ 0 }
		};

		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = "Get Substance Input Image";
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo) override
	{
		switch (event)
		{
		case eFE_Initialize:
			break;
		case eFE_Activate:
			if (IsPortActive(pActInfo, eI_Get))
			{
				GraphInstanceID graphInstanceID = GetPortGraphInstanceID(pActInfo, eI_GraphInstanceID);
				string parameterName = GetPortString(pActInfo, eI_ParameterName);
				bool bInputFound = false;

				CRY_ASSERT(graphInstanceID != INVALID_GRAPHINSTANCEID);

				if (IGraphInstance* pGraphInstance = GetGraphInstance(graphInstanceID))
				{
					if (IGraphInput* pInput = pGraphInstance->GetInputByName(parameterName.c_str()))
					{
						CRY_ASSERT(pInput->GetInputType() == GraphInputType::Image);
						if (pInput->GetInputType() == GraphInputType::Image)
						{
							const char* pPath = pInput->GetValue();
							ActivateOutput(pActInfo, eO_Value, string(pPath));
						}
					}
					else
					{
						CRY_ASSERT_MESSAGE(false, "Input Not Found in GetInputImage");
					}
				}
				else
				{
					CRY_ASSERT_MESSAGE(false, "Invalid GraphInstanceID in GetInputImage");
				}

				ActivateOutput(pActInfo, eO_Done, true);
			}
			break;
		}
	}

	IFlowNodePtr Clone(SActivationInfo *pActInfo) override
	{
		return new CProceduralMaterialFlowNode_GetInputImage(pActInfo);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

private:
	enum InputPorts
	{
		eI_GraphInstanceID = 0,
		eI_ParameterName,
		eI_Get,
	};

	enum OutputPorts
	{
		eO_Value = 0,
		eO_Done,
	};
};
REGISTER_FLOW_NODE("ProceduralMaterial:GetInputImage", CProceduralMaterialFlowNode_GetInputImage)

//--------------------------------------------------------------------------------------------
class CProceduralMaterialFlowNode_SetInputImage : public CProceduralMaterialFlowNodeInputBase
{
public:
	CProceduralMaterialFlowNode_SetInputImage(SActivationInfo* pActInfo)
		: CProceduralMaterialFlowNodeInputBase()
	{
	}

	void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<GraphInstanceID>("GraphInstanceID", ""),
			InputPortConfig<string>("ParameterName", ""),
			InputPortConfig<string>("Texture", ""),
			InputPortConfig<SFlowSystemVoid>("Apply"),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_Void("Done", "Triggered when node input completes"),
			{ 0 }
		};

		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = "Set Substance Input Image";
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo) override
	{
		switch (event)
		{
		case eFE_Initialize:
			break;
		case eFE_Activate:
			if (IsPortActive(pActInfo, eI_Apply))
			{
				GraphInstanceID graphInstanceID = GetPortGraphInstanceID(pActInfo, eI_GraphInstanceID);
				string parameterName = GetPortString(pActInfo, eI_ParameterName);
				string texture = GetPortString(pActInfo, eI_Texture);
				bool bInputFound = false;

				CRY_ASSERT(graphInstanceID != INVALID_GRAPHINSTANCEID);

				if (IGraphInstance* pGraphInstance = GetGraphInstance(graphInstanceID))
				{
					if (IGraphInput* pInput = pGraphInstance->GetInputByName(parameterName.c_str()))
					{
						CRY_ASSERT(pInput->GetInputType() == GraphInputType::Image);
						if (pInput->GetInputType() == GraphInputType::Image)
						{
							pInput->SetValue(texture.c_str());
						}
					}
					else
					{
						CRY_ASSERT_MESSAGE(false, "Input Not Found in SetInputImage");
					}
				}
				else
				{
					CRY_ASSERT_MESSAGE(false, "Invalid GraphInstanceID in SetInputImage");
				}

				ActivateOutput(pActInfo, eO_Done, true);
			}
			break;
		}
	}

	IFlowNodePtr Clone(SActivationInfo *pActInfo) override
	{
		return new CProceduralMaterialFlowNode_SetInputImage(pActInfo);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

private:
	enum InputPorts
	{
		eI_GraphInstanceID = 0,
		eI_ParameterName,
		eI_Texture,
		eI_Apply,
	};

	enum OutputPorts
	{
		eO_Done = 0,
	};
};
REGISTER_FLOW_NODE("ProceduralMaterial:SetInputImage", CProceduralMaterialFlowNode_SetInputImage)

//--------------------------------------------------------------------------------------------
class CProceduralMaterialFlowNodeQueueGraphInstance : public CProceduralMaterialFlowNodeBase
{
public:
	CProceduralMaterialFlowNodeQueueGraphInstance(SActivationInfo* pActInfo)
		: CProceduralMaterialFlowNodeBase()
	{
	}

	void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<GraphInstanceID>("GraphInstanceID", ""),
			InputPortConfig_Void("Add", _HELP("Add GraphInstanceID to the Queue")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_Void("Done"),
			{ 0 }
		};

		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = "Queue Graph Instance for RenderSync/RenderASync";
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo) override
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, eI_Add))
			{
				GraphInstanceID graphInstanceID = GetPortGraphInstanceID(pActInfo, eI_GraphInstanceID);

				// IGraphInstance* pGraph = nullptr;
				// EBUS_EVENT_RESULT(pGraph, SubstanceRequestBus, GetGraphInstance, graphInstanceID);
				// if (pGraph)
				// {
				// 	EBUS_EVENT(SubstanceRequestBus, QueueRender, pGraph);
				// }

				ActivateOutput(pActInfo, eO_Done, true);
			}
			break;
		}
	}

	IFlowNodePtr Clone(SActivationInfo *pActInfo) override
	{
		return new CProceduralMaterialFlowNodeQueueGraphInstance(pActInfo);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

private:
	enum InputPorts
	{
		eI_GraphInstanceID = 0,
		eI_Add,
	};

	enum OutputPorts
	{
		eO_Done = 0,
	};
};
REGISTER_FLOW_NODE("ProceduralMaterial:QueueGraphInstance", CProceduralMaterialFlowNodeQueueGraphInstance)

//--------------------------------------------------------------------------------------------
class CProceduralMaterialFlowNodeRenderBase : public CProceduralMaterialFlowNodeBase
{
public:
	CProceduralMaterialFlowNodeRenderBase()
		: CProceduralMaterialFlowNodeBase()
	{
	}

	void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("Render", _HELP("Begin rendering graph instances")),
			{ 0 }
		};

		config.pInputPorts = in_config;
		config.pOutputPorts = GetOutputPortConfig();
		config.sDescription = GetDescription();
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo) override
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, eI_Render))
			{
				DoRender(pActInfo);
			}
			break;
		case eFE_Update:
			DoUpdate(pActInfo);
			break;
		}
	}

protected:
	virtual const SOutputPortConfig* GetOutputPortConfig() = 0;
	virtual void DoRender(SActivationInfo* pActInfo) = 0;
	virtual void DoUpdate(SActivationInfo* pActInfo) = 0;
	virtual const char* GetDescription() const = 0;

private:
	enum InputPorts
	{
		eI_Render = 0,
	};
};

class CProceduralMaterialFlowNodeRenderSync : public CProceduralMaterialFlowNodeRenderBase
{
public:
	CProceduralMaterialFlowNodeRenderSync(SActivationInfo* pActInfo)
		: CProceduralMaterialFlowNodeRenderBase()
	{
	}

	IFlowNodePtr Clone(SActivationInfo *pActInfo) override
	{
		return new CProceduralMaterialFlowNodeRenderSync(pActInfo);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

protected:
	virtual const SOutputPortConfig* GetOutputPortConfig() override
	{
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_Void("RenderComplete", _HELP("Called when Rendering command has completed")),
			{ 0 }
		};
		return out_config;
	}

	virtual void DoRender(SActivationInfo* pActInfo) override
	{
		EBUS_EVENT(SubstanceRequestBus,RenderSync);
		ActivateOutput(pActInfo, eO_RenderComplete, true);
	}

	virtual void DoUpdate(SActivationInfo* pActInfo) override
	{
	}

	virtual const char* GetDescription() const override
	{
		return "Render Queued Graphs Synchronously";
	}

private:
	enum OutputPorts
	{
		eO_RenderComplete = 0
	};
};
REGISTER_FLOW_NODE("ProceduralMaterial:RenderSync", CProceduralMaterialFlowNodeRenderSync)

#endif // USE_SUBSTANCE
