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
	typedef std::map<int, IGraphInstance*> GraphInstanceMap;
	GraphInstanceMap _graphInstances;
};

#endif // USE_SUBSTANCE

#endif //GEM_SUBSTANCE_IPROCEDURALMATERIAL_H
