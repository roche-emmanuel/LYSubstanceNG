
/** @file GraphInstance.cpp
	@brief Source File for the Substance material implementation
	@author Emmanuel ROCHE
	@date 22/04/2017
	@copyright Emmanuel ROCHE. All rights reserved.
*/
#include "StdAfx.h"

#if defined(USE_SUBSTANCE)
#include "GraphInstance.h"
#include "SubstanceMaterial.h"
#include <AzCore/IO/SystemFile.h>

GraphInstance::GraphInstance(SubstanceMaterial* parent, int idx) : 
	_parent(parent),
	_index(idx)
{
	AZ_TracePrintf("GraphInstance", "Creating GraphInstance object.");
	// Create a new graph instance on the requested graph:
	if(!parent || parent->GetGraphInstanceCount()<=idx) {
		AZ_TracePrintf("GraphInstance", "ERROR: Invalid ProceduralMaterial parent or out of range graph index.");
		return;
	}

	SubstanceAir::PackageDesc* pdesc = parent->getPackage();
	_instance = new SubstanceAir::GraphInstance(pdesc->getGraphs()[0]);
}

GraphInstance::~GraphInstance()
{
	AZ_TracePrintf("GraphInstance", "Deleting GraphInstance object.");
	if(_instance) {
		delete _instance;
	}
}

IProceduralMaterial* GraphInstance::GetProceduralMaterial() const
{
	return _parent;
}

const char* GraphInstance::GetName() const
{
	return _instance->mDesc.mLabel.c_str();
}

GraphInstanceID GraphInstance::GetGraphInstanceID() const
{
	// We use the index as graph ID.
	return _index;
}

int GraphInstance::GetInputCount() const
{
	AZ_TracePrintf("GraphInstance", "GetInputCount == 0");
	// Number of inputs:
	// return _instance->getInputs().size();
	return 0;
}

IGraphInput* GraphInstance::GetInput(int index)
{
	AZ_TracePrintf("GraphInstance", "GetInput: %d", index);
	return nullptr;

	// return _instance->getInputs()[index];
}

IGraphInput* GraphInstance::GetInputByName(const char* name)
{
	AZ_TracePrintf("GraphInstance", "GetInputByName: %s", name);
	return nullptr;

	// for(auto& in: _instance->GetInputs()) {
	// 	if(in->mDesc.mIdentifier == name) {
	// 		return 
	// 	}
	// }
}

IGraphInput* GraphInstance::GetInputByID(GraphInputID inputID)
{
	AZ_TracePrintf("GraphInstance", "GetInputByID: %d", (int)inputID);
	return nullptr;

	// for(auto& in: _instance->GetInputs()) {
	// 	in->mDesc.mUid 
	// }
}

int GraphInstance::GetOutputCount() const
{
	AZ_TracePrintf("GraphInstance", "GetOuputCount == 0");
	return 0;

	// Number of outputs:
	// return _instance->getInputs().size();
}

IGraphOutput* GraphInstance::GetOutput(int index)
{
	AZ_TracePrintf("GraphInstance", "GetOutput: %d", index);
	return nullptr;
}

IGraphOutput* GraphInstance::GetOutputByID(GraphOutputID outputID)
{
	AZ_TracePrintf("GraphInstance", "GetOutputByID: %d", (int)outputID);
	return nullptr;
}

#endif // USE_SUBSTANCE
