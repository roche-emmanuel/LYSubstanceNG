
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
