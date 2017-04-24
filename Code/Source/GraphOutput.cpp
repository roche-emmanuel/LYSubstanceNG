
/** @file GraphOutput.cpp
	@brief Source File for the Substance graph output implementation
	@author Emmanuel ROCHE
	@date 22/04/2017
	@copyright Emmanuel ROCHE. All rights reserved.
*/
#include "StdAfx.h"

#if defined(USE_SUBSTANCE)
#include "GraphOutput.h"
#include "GraphInstance.h"
#include "SubstanceMaterial.h"
#include <AzCore/IO/SystemFile.h>

GraphOutput::GraphOutput(GraphInstance* parent, GraphOutputID id) : 
	_parent(parent),
	_id(id)
{
	AZ_TracePrintf("GraphOutput", "Creating GraphOutput object.");

	// Create the output instance from our graph instance:
	auto graph = parent->getInstance();
	_instance = graph->findOutput(id);
	if(!_instance) {
		AZ_TracePrintf("GraphOutput", "ERROR: Cannot find output with ID=%d", id);
		return;
	}

	// use the sbsar path and add the proper ending to the path:
	AZStd::string fbase = _parent->GetProceduralMaterial()->GetSourcePath();
	fbase = fbase.substr(0,fbase.size()-6);

	AZStd::string otype = "";
	switch(_instance->mDesc.mChannel) {
	case SubstanceAir::Channel_Diffuse: 
		otype = "diffuse"; break;
	case SubstanceAir::Channel_Normal: 
		otype = "normal"; break;
	case SubstanceAir::Channel_Specular: 
		otype = "specular"; break;
	case SubstanceAir::Channel_Height: 
		otype = "height"; break;
	default:
		AZ_TracePrintf("GraphOutput", "Unknown output of type %d.", (int)_instance->mDesc.mChannel);
		otype = "unknown"; break;
	}

	_outputPath = string_format("%s_%s.sub", fbase.c_str(),otype.c_str()).c_str();
}

GraphOutput::~GraphOutput()
{
	AZ_TracePrintf("GraphOutput", "Deleting GraphOutput object.");
}

IGraphInstance* GraphOutput::GetGraphInstance() const
{
	return _parent;
}

GraphOutputID GraphOutput::GetGraphOutputID() const
{
	return _id;
}

const char* GraphOutput::GetLabel() const
{
	logDEBUG("Retrieving GraphOutput label.");
	return _instance->mDesc.mLabel.c_str();
}

const char* GraphOutput::GetPath() const
{
	logDEBUG("Returning output path:" << _outputPath.c_str());
	return _outputPath.c_str();
}

bool GraphOutput::IsEnabled() const
{
	return _instance->mEnabled;
}

void GraphOutput::SetEnabled(bool bEnabled)
{
	_instance->mEnabled = bEnabled;
}

bool GraphOutput::IsDirty() const
{
	return _instance->isDirty();
}

void GraphOutput::SetDirty()
{
	_instance->flagAsDirty();
}

bool GraphOutput::GetEditorPreview(SGraphOutputEditorPreview& preview)
{
	logDEBUG("Trying to generate Output preview for id "<<(int)_id);
	return false;
}

GraphOutputChannel GraphOutput::GetChannel() const
{
	return (GraphOutputChannel)_instance->mDesc.mChannel;
}

#endif // USE_SUBSTANCE
