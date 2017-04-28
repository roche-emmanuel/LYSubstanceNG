/** @file SubstanceMaterial.h
	@brief Header for the Procedural Material Interface
	@author Emmanuel ROCHE
	@date 22/04/2017
	@copyright Emmanuel ROCHE. All rights reserved.
*/
#ifndef GEM_SUBSTANCE_GRAPHINSTANCE_H
#define GEM_SUBSTANCE_GRAPHINSTANCE_H
#pragma once

#include "Substance/IProceduralMaterial.h"
#include "Substance/framework/package.h"

#if defined(USE_SUBSTANCE)

class SubstanceMaterial;

class GraphOutput;
class GraphInput;

/**/
class GraphInstance : public IGraphInstance
{
public:
	GraphInstance(SubstanceMaterial* parent, int idx);
	virtual ~GraphInstance();

	/// Get the parent procedural material.
	virtual IProceduralMaterial* GetProceduralMaterial() const;

	/// Get the name of this graph.
	virtual const char* GetName() const;

	/// Get the GraphInstanceID.
	virtual GraphInstanceID GetGraphInstanceID() const;

	/// Get the number of input parameters.
	virtual int GetInputCount() const;

	/// Get an input parameter by index.
	virtual IGraphInput* GetInput(int index);

	/// Get an input parameter by name.
	virtual IGraphInput* GetInputByName(const char* name);

	/// Get a graph input by ID
	virtual IGraphInput* GetInputByID(GraphInputID inputID);

	/// Get the number of output object.
	virtual int GetOutputCount() const;

	/// Get an output object by index.
	virtual IGraphOutput* GetOutput(int index);

	/// Get an output object by ID
	virtual IGraphOutput* GetOutputByID(GraphOutputID outputID);

	//! Retrieve the substance graph instance:
	inline SubstanceAir::GraphInstance* getInstance() const { return _instance; }

	//! Retrieve a default input value:
	bool getDefaultInputValue(unsigned int id, GraphValueVariant& val);

protected:
	// Pointer on the parent material:
	SubstanceMaterial* _parent;

	// Graph index in parent package:
	unsigned int _index;

	// Graph instance on the given graph:
	SubstanceAir::GraphInstance* _instance;

	// List of graph outputs:
	typedef std::vector<GraphOutput*> GraphOutputList;
	GraphOutputList _outputs;

	// List of graph inputs:
	typedef std::vector<GraphInput*> GraphInputList;
	GraphInputList _inputs;
};

#endif // USE_SUBSTANCE

#endif //GEM_SUBSTANCE_IPROCEDURALMATERIAL_H
