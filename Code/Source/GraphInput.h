/** @file SubstanceMaterial.h
	@brief Header for the Substance graph input
	@author Emmanuel ROCHE
	@date 22/04/2017
	@copyright Emmanuel ROCHE. All rights reserved.
*/
#ifndef GEM_SUBSTANCE_GRAPHINPUT_H
#define GEM_SUBSTANCE_GRAPHINPUT_H
#pragma once

#include "Substance/IProceduralMaterial.h"
#include "Substance/framework/package.h"
#include "Substance/framework/input.h"

#if defined(USE_SUBSTANCE)

class GraphInstance;

/**/
class GraphInput : public IGraphInput
{
public:
	GraphInput(GraphInstance* parent, unsigned int id);
	virtual ~GraphInput();

	/// Get the parent graph instance.
	virtual IGraphInstance* GetGraphInstance() const;

	/// Get the graph input ID.
	virtual GraphInputID GetGraphInputID() const;

	/// Get the input description.
	virtual const char* GetDescription() const;

	/// Get the input label.
	virtual const char* GetLabel() const;

	/// Get the input name.
	virtual const char* GetName() const;

	/// Get the GUI Group for this input.
	virtual const char* GetGroupName() const;

	/// Get the input type.
	virtual GraphInputType GetInputType() const;

	/// Get the input type used for GUI display.
	virtual GraphInputWidgetType GetInputWidgetType() const;

	/// Retrieve the value of this input.
	virtual GraphValueVariant GetValue() const;

	/// Assign a new value to this input. You must call QueueRender/Render(A)Sync to update the output textures.
	virtual void SetValue(const GraphValueVariant& value);

	/// Get the minimum value for this input.
	virtual GraphValueVariant GetMinValue() const;

	/// Get the maximum value for this input.
	virtual GraphValueVariant GetMaxValue() const;

	/// Get enumeration count for combo box inputs.
	virtual int GetEnumCount() const;

	/// Get the enumeration value for combo box inputs.
	virtual GraphEnumValue GetEnumValue(int index);

protected:
	// Pointer on the parent graph instance:
	GraphInstance* _parent;

	// Input ID:
	unsigned int _id;

	// Substance output instance:
	SubstanceAir::InputInstanceBase* _instance;
};

#endif // USE_SUBSTANCE

#endif //GEM_SUBSTANCE_IPROCEDURALMATERIAL_H
