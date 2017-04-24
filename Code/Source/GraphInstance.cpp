
/** @file GraphInstance.cpp
	@brief Source File for the Substance graph instance implementation
	@author Emmanuel ROCHE
	@date 22/04/2017
	@copyright Emmanuel ROCHE. All rights reserved.
*/
#include "StdAfx.h"

#if defined(USE_SUBSTANCE)
#include "GraphInstance.h"
#include "GraphOutput.h"
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

	// Init the outputs:
	for(auto& out: _instance->getOutputs()) {
		// Create a new output object:
		_outputs.push_back(new GraphOutput(this, out->mDesc.mUid));
	}
}

GraphInstance::~GraphInstance()
{
	AZ_TracePrintf("GraphInstance", "Deleting GraphInstance object.");
	// Release all the outputs:
	for(auto& out: _outputs) {
		delete out;
	}
	_outputs.clear();
	
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
	// Number of outputs:
	int num = _instance->getOutputs().size();
	AZ_TracePrintf("GraphInstance", "GetOuputCount = %d", num);
	return num;
}

IGraphOutput* GraphInstance::GetOutput(int index)
{
	AZ_TracePrintf("GraphInstance", "GetOutput: %d", index);
	return _outputs[index];
}

IGraphOutput* GraphInstance::GetOutputByID(GraphOutputID outputID)
{
	AZ_TracePrintf("GraphInstance", "GetOutputByID: %d", (int)outputID);
	for(auto& out: _outputs) {
		if(out->GetGraphOutputID() == outputID) {
			return out;
		}
	}

	AZ_TracePrintf("GraphInstance", "Cannot find output with ID = %d", (int)outputID);
	return nullptr;
}

#endif // USE_SUBSTANCE
