/** @file SubstanceMaterial.h
	@brief Header for the Procedural Material Interface
	@author Emmanuel ROCHE
	@date 22/04/2017
	@copyright Emmanuel ROCHE. All rights reserved.
*/
#ifndef GEM_SUBSTANCE_SUBSTANCEMATERIAL_H
#define GEM_SUBSTANCE_SUBSTANCEMATERIAL_H
#pragma once

#include "Substance/IProceduralMaterial.h"
#include "Substance/framework/package.h"

#if defined(USE_SUBSTANCE)

class GraphInstance;

/**/
class SubstanceMaterial : public IProceduralMaterial
{
public:
	SubstanceMaterial(const char* path);
	virtual ~SubstanceMaterial();

	/// Get the smtl file path for this material.
	virtual const char* GetPath() const;

	/// Get the substance archive path for this material.
	virtual const char* GetSourcePath() const;

	/// Get the number of graph instances contained in this material.
	virtual int GetGraphInstanceCount() const;

	/// Retrieve a specific graph instance by index.
	virtual IGraphInstance* GetGraphInstance(int index);

	/// Reimport Substance SBSAR from Disk
	virtual void ReimportSubstance();

	// Retrieve the package from this material:
	SubstanceAir::PackageDesc* getPackage() const { return _package; }

	// Retrieve a default input value:
	bool getDefaultInputValue(const AZStd::string& key, GraphValueVariant& val);

	// Save this material to file:
	bool save(const char* basePath, const char* path);

	// Write substance base path:
	void writeSubstanceTexture(const AZStd::string& basePath, const AZStd::string& fbase, const AZStd::string& otype, unsigned int id);

protected:
	// Helper method used to load the data from XML:
	void LoadMaterialFromXML();

	// smtl path:
	AZStd::string _smtlPath;

	// sbsar path:
	AZStd::string _sbsarPath;

	// Package desc:
	SubstanceAir::PackageDesc* _package;

	// map of graph instances:
	typedef std::map<int, GraphInstance*> GraphInstanceMap;
	GraphInstanceMap _graphInstances;

	typedef std::map<AZStd::string, GraphValueVariant> ValueMap;
	ValueMap _defValues;
};

#endif // USE_SUBSTANCE

#endif //GEM_SUBSTANCE_IPROCEDURALMATERIAL_H
