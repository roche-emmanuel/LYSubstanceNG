
/** @file SubstanceMaterial.cpp
	@brief Source File for the Substance material implementation
	@author Emmanuel ROCHE
	@date 22/04/2017
	@copyright Emmanuel ROCHE. All rights reserved.
*/
#include "StdAfx.h"

#if defined(USE_SUBSTANCE)
#include "SubstanceMaterial.h"
#include "GraphInstance.h"
#include "GraphOutput.h"
#include "GraphInput.h"
#include <AzCore/IO/SystemFile.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

SubstanceMaterial::SubstanceMaterial(const char* path) : _smtlPath(path),
	_package(nullptr)
{
	AZ_TracePrintf("SubstanceGem", "Creating SubstanceMaterial object.");
	LoadMaterialFromXML();
}

SubstanceMaterial::~SubstanceMaterial()
{
	AZ_TracePrintf("SubstanceGem", "Deleting SubstanceMaterial object.");
	// Destroy the graph instances:
	for(auto& inst: _graphInstances) {
		delete inst.second;
	}
	_graphInstances.clear();

	// Destroy the package:
	if(_package) {
		delete _package;
		_package = nullptr;
	}
}

void SubstanceMaterial::LoadMaterialFromXML()
{
	auto resolvedPath = getAbsoluteAssetPath(_smtlPath);

	XmlNodeRef mtlNode = GetISystem()->LoadXmlFromFile(resolvedPath.c_str());
	if(!mtlNode) {
		AZ_TracePrintf("SubstanceGem", "ERROR: Cannot load material XML from file %s.", resolvedPath.c_str());
		return;
	}

	// Read the content of the XML file:
	const char* source;
	
	//get sbsar path
	if (!mtlNode->getAttr("Source", &source))
	{
		CryLogAlways("ERROR: ProceduralMaterial: No Source parameter for material (%s)", resolvedPath.c_str());
		return;
	}
	
	_sbsarPath = source;

	// Open the sbsar file:
	CCryFile sbsarFile(source, "rb");
	if (!sbsarFile.GetHandle())
	{
		CryLogAlways("ERROR: ProceduralMaterial: Unable to load substance (%s) in material (%s)", source, resolvedPath.c_str());
		return;
	}
	
	//read archive data
	size_t dataSize = sbsarFile.GetLength();
	byte* data = new byte[dataSize];
	sbsarFile.ReadRaw(data, dataSize);

	// parse the package:
	_package = new SubstanceAir::PackageDesc(data, dataSize);
	delete [] data;

	// Check the package is valid:
	if(!_package->isValid()) {
		CryLogAlways("ERROR: ProceduralMaterial: Package read from %s is invalid.", source);
		return;
	}

	//process child nodes
	for (int i = 0; i < mtlNode->getChildCount(); i++)
	{
		XmlNodeRef child = mtlNode->getChild(i);

		if (!strcmp(child->getTag(), "Parameter"))
		{
			const char* id;
			int type;

			if (child->getAttr("ID", &id) &&
				child->getAttr("Type", type))
			{
				// Retrieve the graph:
				logDEBUG("Setting up parameter with ID="<<id<<" of type "<<type);
				float xf, yf, zf, wf;
				int xi, yi, zi, wi;
				const char* str;
				AZStd::string key(id);

				switch(type) {
				case GraphInputType::Float1:
					child->getAttr("x", xf);
					_defValues[key] = GraphValueVariant(xf);
					break;
				case GraphInputType::Float2:
					child->getAttr("x", xf);
					child->getAttr("y", yf);
					_defValues[key] = GraphValueVariant(xf, yf);
					break;
				case GraphInputType::Float3:
					child->getAttr("x", xf);
					child->getAttr("y", yf);
					child->getAttr("z", zf);
					_defValues[key] = GraphValueVariant(xf, yf, zf);
					break;
				case GraphInputType::Float4:
					child->getAttr("x", xf);
					child->getAttr("y", yf);
					child->getAttr("z", zf);
					child->getAttr("w", wf);
					_defValues[key] = GraphValueVariant(xf, yf, zf, wf);
					break;
				case GraphInputType::Integer1:
					child->getAttr("x", xi);
					_defValues[key] = GraphValueVariant(xi);
					break;
				case GraphInputType::Integer2:
					child->getAttr("x", xi);
					child->getAttr("y", yi);
					_defValues[key] = GraphValueVariant(xi, yi);
					break;
				case GraphInputType::Integer3:
					child->getAttr("x", xi);
					child->getAttr("y", yi);
					child->getAttr("z", zi);
					_defValues[key] = GraphValueVariant(xi, yi, zi);
					break;
				case GraphInputType::Integer4:
					child->getAttr("x", xi);
					child->getAttr("y", yi);
					child->getAttr("z", zi);
					child->getAttr("w", wi);
					_defValues[key] = GraphValueVariant(xi, yi, zi, wi);
					break;
				case GraphInputType::String:
					child->getAttr("str", &str);
					_defValues[key] = GraphValueVariant(str);
					break;
				default:
					logERROR("Unsupported input type: "<<type);
					break;
				}			
			}
		}
	}
}

void SubstanceMaterial::writeSubstanceTexture(const AZStd::string& basePath, const AZStd::string& fbase, const AZStd::string& otype, unsigned int id)
{
	// Open a file for writing:
	AZ::IO::SystemFile file;

	AZStd::string fullPath = basePath+AZStd::string("/")+fbase+"_"+otype+".sub";
	bool res = file.Open(fullPath.c_str(),AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY|AZ::IO::SystemFile::SF_OPEN_CREATE);
	if(!res) {
		logERROR("Cannot open file " << fullPath.c_str() << " for writing.");
		return;
	}

	// prepare the content to write:
	AZStd::string content = string_format("<ProceduralTexture Material=\"%s.smtl\" OutputID=\"%d\" />", fbase.c_str(), id);

	auto rlen = file.Write(content.c_str(), content.size());
	if(rlen != content.size()) {
		logERROR("Did not write expected number of bytes: "<< rlen << " != " << content.size());
		return;	
	}

	file.Close();
	logDEBUG("Written substance texture file: " << fullPath.c_str());
}

bool SubstanceMaterial::save(const char* basePath, const char* path)
{
	// Prepare the content to save:
	AZStd::string content = string_format("<ProceduralMaterial Source=\"%s\">\n", GetSourcePath());
	
	// List the output IDs and usage:
	AZStd::string smtlPath = GetPath();
	if(path) {
		smtlPath = path;
	}

	AZStd::string fbase = smtlPath;
	fbase = fbase.substr(0,fbase.size()-5);

	// iterate on all the graphs:
	int ng = GetGraphInstanceCount();
	for(int i = 0; i<ng; ++i) {
		GraphInstance* graph = (GraphInstance*)GetGraphInstance(i);
		int nout = graph->GetOutputCount();
		for(int j=0; j<nout; ++j) {
			GraphOutput* out = (GraphOutput*)graph->GetOutput(j);
			AZStd::string otype = "";
			switch(out->GetChannel()) {
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
				AZ_TracePrintf("SubstanceGem", "Ignoring output of type %d.", (int)out->GetChannel());
				break;
			}
			if(!otype.empty()) {
				// Add a line in the output content:
				content += string_format("  <Output ID=\"%d\" Enabled=\"1\" Compressed=\"1\" File=\"%s_%s.sub\" />\n", (unsigned int)out->GetGraphOutputID(),fbase.c_str(),otype.c_str());

				writeSubstanceTexture(basePath, fbase, otype, out->GetGraphOutputID());
			}
		}

		// Process the inputs:
		int nin = graph->GetInputCount();
		for(int j=0; j<nin; ++j) {
			GraphInput* in = (GraphInput*)graph->GetInput(j);
			AZStd::string key = string_format("%d_%d",(int)graph->GetGraphInstanceID(), (int)in->GetGraphInputID());

			AZStd::string data = "";
			const float* fval;
			const int* ival;
			const char* str;
			int type = (int)in->GetInputType();

			switch(type) {
			case GraphInputType::Float1:
				fval = (const float*)(in->GetValue()); 
				data = string_format("x=\"%f\"", fval[0]);
				break;
			case GraphInputType::Float2:
				fval = (const float*)(in->GetValue()); 
				data = string_format("x=\"%f\" y=\"%f\"", fval[0], fval[1]);
				break;
			case GraphInputType::Float3:
				fval = (const float*)(in->GetValue()); 
				data = string_format("x=\"%f\" y=\"%f\" z=\"%f\"", fval[0], fval[1], fval[2]);
				break;
			case GraphInputType::Float4:
				fval = (const float*)(in->GetValue()); 
				data = string_format("x=\"%f\" y=\"%f\" z=\"%f\" w=\"%f\"", fval[0], fval[1], fval[2], fval[3]);
				break;
			case GraphInputType::Integer1:
				ival = (const int*)(in->GetValue()); 
				data = string_format("x=\"%d\"", ival[0]);
				break;
			case GraphInputType::Integer2:
				ival = (const int*)(in->GetValue()); 
				data = string_format("x=\"%d\" y=\"%d\"", ival[0], ival[1]);
				break;
			case GraphInputType::Integer3:
				ival = (const int*)(in->GetValue()); 
				data = string_format("x=\"%d\" y=\"%d\" z=\"%d\"", ival[0], ival[1], ival[2]);
				break;
			case GraphInputType::Integer4:
				ival = (const int*)(in->GetValue()); 
				data = string_format("x=\"%d\" y=\"%d\" z=\"%d\" w=\"%d\"", ival[0], ival[1], ival[2], ival[3]);
				break;
			case GraphInputType::String:
				str = (const char*)(in->GetValue());
				data = string_format("str=\"%s\"", str);
				break;
			default:
				logERROR("Unsupported input type: "<<type);
				break; 
			}

			content += string_format("  <Parameter ID=\"%s\" Type=\"%d\" %s />\n", key.c_str(),type,data.c_str());
		}
	}

	// Todo: handle the input parameters here too.

	// close the parent tag:
	content += "</ProceduralMaterial>\n";

	// Use the default path is non is provided:

	AZ::IO::SystemFile file;
	AZStd::string fullPath = basePath+AZStd::string("/")+AZStd::string(smtlPath);
	bool res = file.Open(fullPath.c_str(),AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY|AZ::IO::SystemFile::SF_OPEN_CREATE);
	if(!res) {
		logERROR("Cannot open file %s for writing."<< fullPath.c_str());
		return false;
	}

	auto rlen = file.Write(content.c_str(), content.size());
	if(rlen != content.size()) {
		logERROR("Did not write expected number of bytes: "<< rlen<<" != "<< content.size());
		return false;	
	}

	file.Close();

	logDEBUG("ProceduralMaterial saved to file: "<<fullPath.c_str());
	return true;
}

bool SubstanceMaterial::getDefaultInputValue(const AZStd::string& key, GraphValueVariant& val)
{
	if(_defValues.count(key)>0) {
		val = _defValues[key];
		return true;
	}

	return false;
}

const char* SubstanceMaterial::GetPath() const
{
	return _smtlPath.c_str();
}

const char* SubstanceMaterial::GetSourcePath() const
{
	return _sbsarPath.c_str();
}

int SubstanceMaterial::GetGraphInstanceCount() const
{
	if(_package && _package->isValid()) {
		return _package->getGraphs().size();
	}

	return 0;
}

IGraphInstance* SubstanceMaterial::GetGraphInstance(int index)
{
	if(_graphInstances.count(index)==0) {
		// create the graph instance if needed:
		AZ_TracePrintf("SubstanceGem", "Creating graph instance at index %d.", index);
		_graphInstances[index] = new GraphInstance(this, index);
	}

	return _graphInstances[index];
}

void SubstanceMaterial::ReimportSubstance()
{
	AZ_TracePrintf("SubstanceGem", "SubstanceMaterial::ReimportSubstance() not doing anything.");
}

#endif // USE_SUBSTANCE
