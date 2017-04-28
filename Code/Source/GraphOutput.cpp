
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
	case SubstanceAir::Channel_Emissive: 
		otype = "emittance"; break;
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

int GraphOutput::GetBytesPerPixel(int format) const
{
	switch (format)
	{
	case Substance_PF_RGBA|Substance_PF_16b:					return 8;
	case Substance_PF_RGBx|Substance_PF_16b:					return 8;
	case Substance_PF_L|Substance_PF_16b:						return 2;
	case Substance_PF_RGBA:										return 4;
	case Substance_PF_L:										return 1;
	default:
		logERROR("Unsupported substance pixel format: "<<format);
		return 1;
	}
}

ETEX_Format GraphOutput::GetEngineFormat(int format) const
{
	switch (format)
	{
	case Substance_PF_RGBx|Substance_PF_16b:	
		return eTF_R16G16B16A16;	
	case Substance_PF_L|Substance_PF_16b:
		return eTF_R16;
	case Substance_PF_RGBA|Substance_PF_16b:
		return eTF_R16G16B16A16;
	case Substance_PF_RGBA:
		return eTF_R8G8B8A8;
	case Substance_PF_L:
		return eTF_L8;
	default:
		logERROR("Unsupported substance pixel format: "<<(int)format);
		return eTF_Unknown;
	}
}

bool GraphOutput::GetEditorPreview(SGraphOutputEditorPreview& preview)
{
	// We grab the output here and copy it into the preview structure:
	auto result = _instance->grabResult();
	if(!result) {
		logERROR("Invalid result in GetEditorPreview()");
		return false;
	}

	// Assign the data:
	auto stex = result->getTexture();
	logDEBUG("MipmapCount="<< (int)stex.mipmapCount);
	logDEBUG("Width="<< (int)stex.level0Width);
	logDEBUG("Height="<< (int)stex.level0Height);
	logDEBUG("PixelFormat="<< (int)stex.pixelFormat);
	logDEBUG("ChannelsOrder="<< (int)stex.channelsOrder);

	preview.Width = (int)stex.level0Width;
	preview.Height = (int)stex.level0Height;
	preview.BytesPerPixel = GetBytesPerPixel((int)stex.pixelFormat);
	preview.Data = stex.buffer;
	preview.Format = GetEngineFormat((int)stex.pixelFormat);
	preview.ChannelOrder = (int)stex.channelsOrder;

	return true;
}

GraphOutputChannel GraphOutput::GetChannel() const
{
	return (GraphOutputChannel)_instance->mDesc.mChannel;
}

#endif // USE_SUBSTANCE
