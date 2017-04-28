
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
#include "GraphInput.h"
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
	// Init the inputs:
	for(auto& in: _instance->getInputs()) {
		// Create a new output object:
		_inputs.push_back(new GraphInput(this, in->mDesc.mUid));
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
	for(auto& in: _inputs) {
		delete in;
	}
	_inputs.clear();
	
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
	int num = _instance->getInputs().size();
	logDEBUG("GraphInstance: GetInputCount = "<<num);
	return num;
}

IGraphInput* GraphInstance::GetInput(int index)
{
	logDEBUG("GraphInstance: GetInput: "<< index);
	return _inputs[index];
}

IGraphInput* GraphInstance::GetInputByName(const char* name)
{
	logDEBUG("GraphInstance: GetInputByName: "<< name);

	AZStd::string str(name);

	for(auto& in: _inputs) {
		if(str == in->GetName()) {
			return in;
		}
	}

	return nullptr;
}

IGraphInput* GraphInstance::GetInputByID(GraphInputID inputID)
{
	logDEBUG("GraphInstance: GetInputByID: "<< (int)inputID);

	for(auto& in: _inputs) {
		if(inputID == in->GetGraphInputID()) {
			return in;
		}
	}

	return nullptr;
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

bool GraphInstance::getDefaultInputValue(unsigned int id, GraphValueVariant& val)
{
	return _parent->getDefaultInputValue(string_format("%d_%d",_index, id), val);
}

#endif // USE_SUBSTANCE
