
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
