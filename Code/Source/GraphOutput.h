/** @file SubstanceMaterial.h
	@brief Header for the Procedural Material Interface
	@author Emmanuel ROCHE
	@date 22/04/2017
	@copyright Emmanuel ROCHE. All rights reserved.
*/
#ifndef GEM_SUBSTANCE_GRAPHOUTPUT_H
#define GEM_SUBSTANCE_GRAPHOUTPUT_H
#pragma once

#include "Substance/IProceduralMaterial.h"
#include "Substance/framework/package.h"

#if defined(USE_SUBSTANCE)

class GraphInstance;

/**/
class GraphOutput : public IGraphOutput
{
public:
	GraphOutput(GraphInstance* parent, GraphOutputID id);
	virtual ~GraphOutput();

	/// Get the parent graph instance.
	virtual IGraphInstance* GetGraphInstance() const;

	/// Get the graph output ID.
	virtual GraphOutputID GetGraphOutputID() const;

	/// Get the output label.
	virtual const char* GetLabel() const;

	/// Get the path for this output.
	virtual const char* GetPath() const;

	/// Check if output is enabled for rendering
	virtual bool IsEnabled() const;

	/// Set output rendering on/off
	virtual void SetEnabled(bool bEnabled);

	/// Check if output texture needs to be updated
	virtual bool IsDirty() const;

	/// Force output texture to be updated at next render call.
	virtual void SetDirty();

	/// Retrieve editor preview buffer
	virtual bool GetEditorPreview(SGraphOutputEditorPreview& preview);

	/// Get the associated output channel.
	virtual GraphOutputChannel GetChannel() const;

	/// Retrieve the output instance:
	inline SubstanceAir::OutputInstance* getInstance() const { return _instance; }

protected:
	// Pointer on the parent graph instance:
	GraphInstance* _parent;

	// Output ID:
	GraphOutputID _id;

	// Substance output instance:
	SubstanceAir::OutputInstance* _instance;

	// outputpath for this graph output:
	AZStd::string _outputPath;
};

#endif // USE_SUBSTANCE

#endif //GEM_SUBSTANCE_IPROCEDURALMATERIAL_H
