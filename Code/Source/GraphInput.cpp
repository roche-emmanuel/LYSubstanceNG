
/** @file GraphInput.cpp
	@brief Source File for the Substance graph input implementation
	@author Emmanuel ROCHE
	@date 22/04/2017
	@copyright Emmanuel ROCHE. All rights reserved.
*/
#include "StdAfx.h"

#if defined(USE_SUBSTANCE)
#include "GraphInput.h"
#include "GraphInstance.h"
#include "SubstanceMaterial.h"
#include <AzCore/IO/SystemFile.h>

using namespace SubstanceAir;

GraphInput::GraphInput(::GraphInstance* parent, unsigned int id) : 
	_parent(parent),
	_id(id)
{
	logDEBUG("Creating GraphInput object.");

	// Create the output instance from our graph instance:
	auto graph = parent->getInstance();
	_instance = graph->findInput(id);
	if(!_instance) {
		logERROR("Cannot find input with ID=%d"<< id);
		return;
	}
}

GraphInput::~GraphInput()
{
	logDEBUG("Deleting GraphInput object.");
}

IGraphInstance* GraphInput::GetGraphInstance() const
{
	return _parent;
}

GraphInputID GraphInput::GetGraphInputID() const
{
	return _id;
}

const char* GraphInput::GetDescription() const
{
	return _instance->mDesc.mGuiDescription.c_str();
}

const char* GraphInput::GetLabel() const
{
	return _instance->mDesc.mLabel.c_str();
}

const char* GraphInput::GetName() const
{
	return _instance->mDesc.mIdentifier.c_str();
}

const char* GraphInput::GetGroupName() const
{
	return _instance->mDesc.mGuiGroup.c_str();
}

GraphInputType GraphInput::GetInputType() const
{
	return (GraphInputType)_instance->mDesc.mType;
}

GraphInputWidgetType GraphInput::GetInputWidgetType() const
{
	return (GraphInputWidgetType)_instance->mDesc.mGuiWidget;
}

GraphValueVariant GraphInput::GetValue() const
{
	if(_instance->mDesc.isNumerical()) {
		switch (_instance->mDesc.mType) {
		case Substance_IType_Float:
			{
				InputInstanceFloat* tinst = (InputInstanceFloat*)_instance;
				return GraphValueVariant(tinst->getValue());
			} 
		case Substance_IType_Float2:
			{
				InputInstanceFloat2* tinst = (InputInstanceFloat2*)_instance;
				return GraphValueVariant(tinst->getValue().x,
										 tinst->getValue().y);
			} 
		case Substance_IType_Float3:
			{
				InputInstanceFloat3* tinst = (InputInstanceFloat3*)_instance;
				return GraphValueVariant(tinst->getValue().x,
										 tinst->getValue().y,
										 tinst->getValue().z);
			} 
		case Substance_IType_Float4:
			{
				InputInstanceFloat4* tinst = (InputInstanceFloat4*)_instance;
				return GraphValueVariant(tinst->getValue().x,
										 tinst->getValue().y,
										 tinst->getValue().z,
										 tinst->getValue().w);
			} 
		case Substance_IType_Integer:
			{
				InputInstanceInt* tinst = (InputInstanceInt*)_instance;
				return GraphValueVariant(tinst->getValue());
			} 
		case Substance_IType_Integer2:
			{
				InputInstanceInt2* tinst = (InputInstanceInt2*)_instance;
				return GraphValueVariant(tinst->getValue().x,
										 tinst->getValue().y);
			} 
		case Substance_IType_Integer3:
			{
				InputInstanceInt3* tinst = (InputInstanceInt3*)_instance;
				return GraphValueVariant(tinst->getValue().x,
										 tinst->getValue().y,
										 tinst->getValue().z);
			} 
		case Substance_IType_Integer4:
			{
				InputInstanceInt4* tinst = (InputInstanceInt4*)_instance;
				return GraphValueVariant(tinst->getValue().x,
										tinst->getValue().y,
										tinst->getValue().z,
										tinst->getValue().w);
			} 
		default:
			logERROR("getValue(): Unexpected numerical input with type: "<<(int)_instance->mDesc.mType);
			return GraphValueVariant();
		}
	}
	else if(_instance->mDesc.isString()) {
		switch (_instance->mDesc.mType) {
		case Substance_IType_String:
			{
				InputInstanceString* tinst = (InputInstanceString*)_instance;
				return GraphValueVariant(tinst->getString().c_str());
			} 
		default:
			logERROR("getValue(): Unexpected string input with type: "<<(int)_instance->mDesc.mType);
			return GraphValueVariant();
		}
	}
	else {
		logERROR("getValue(): Unsupported (image ?) input with type: "<<(int)_instance->mDesc.mType);
		return GraphValueVariant();
	}
}

void GraphInput::SetValue(const GraphValueVariant& value)
{
	if(_instance->mDesc.isNumerical()) {
		switch (_instance->mDesc.mType) {
		case Substance_IType_Float:
			{
				InputInstanceFloat* tinst = (InputInstanceFloat*)_instance;
				tinst->setValue((float)value);
				break;
			} 
		case Substance_IType_Float2:
			{
				InputInstanceFloat2* tinst = (InputInstanceFloat2*)_instance;
				auto ptr = (const float*)value;
				tinst->setValue(SubstanceAir::Vec2Float(ptr[0],ptr[1]));
				break;
			} 
		case Substance_IType_Float3:
			{
				InputInstanceFloat3* tinst = (InputInstanceFloat3*)_instance;
				auto ptr = (const float*)value;
				tinst->setValue(SubstanceAir::Vec3Float(ptr[0],ptr[1],ptr[2]));
				break;
			} 
		case Substance_IType_Float4:
			{
				InputInstanceFloat4* tinst = (InputInstanceFloat4*)_instance;
				auto ptr = (const float*)value;
				tinst->setValue(SubstanceAir::Vec4Float(ptr[0],ptr[1],ptr[2],ptr[3]));
				break;
			} 
		case Substance_IType_Integer:
			{
				InputInstanceInt* tinst = (InputInstanceInt*)_instance;
				tinst->setValue((int)value);
				break;
			} 
		case Substance_IType_Integer2:
			{
				InputInstanceInt2* tinst = (InputInstanceInt2*)_instance;
				auto ptr = (const int*)value;
				tinst->setValue(SubstanceAir::Vec2Int(ptr[0],ptr[1]));
				break;
			} 
		case Substance_IType_Integer3:
			{
				InputInstanceInt3* tinst = (InputInstanceInt3*)_instance;
				auto ptr = (const int*)value;
				tinst->setValue(SubstanceAir::Vec3Int(ptr[0],ptr[1],ptr[2]));
				break;
			} 
		case Substance_IType_Integer4:
			{
				InputInstanceInt4* tinst = (InputInstanceInt4*)_instance;
				auto ptr = (const int*)value;
				tinst->setValue(SubstanceAir::Vec4Int(ptr[0],ptr[1],ptr[2],ptr[3]));
				break;
			} 
		default:
			logERROR("setValue(): Unexpected numerical input with type: "<<(int)_instance->mDesc.mType);
			break;
		}
	}
	else if(_instance->mDesc.isString()) {
		switch (_instance->mDesc.mType) {
		case Substance_IType_String:
			{
				InputInstanceString* tinst = (InputInstanceString*)_instance;
				tinst->setString((const char*)value);
				break;
			} 
		default:
			logERROR("Unexpected string input with type: "<<(int)_instance->mDesc.mType);
			break;
		}
	}
	else {
		logERROR("Unsupported (image ?) input with type: "<<(int)_instance->mDesc.mType);
	}
}

GraphValueVariant GraphInput::GetMinValue() const
{
	if(_instance->mDesc.isNumerical()) {
		switch (_instance->mDesc.mType) {
		case Substance_IType_Float:
			{
				InputInstanceFloat* tinst = (InputInstanceFloat*)_instance;
				return GraphValueVariant(tinst->getDesc().mMinValue);
			} 
		case Substance_IType_Float2:
			{
				InputInstanceFloat2* tinst = (InputInstanceFloat2*)_instance;
				return GraphValueVariant(tinst->getDesc().mMinValue.x,
										 tinst->getDesc().mMinValue.y);
			} 
		case Substance_IType_Float3:
			{
				InputInstanceFloat3* tinst = (InputInstanceFloat3*)_instance;
				return GraphValueVariant(tinst->getDesc().mMinValue.x,
										 tinst->getDesc().mMinValue.y,
										 tinst->getDesc().mMinValue.z);
			} 
		case Substance_IType_Float4:
			{
				InputInstanceFloat4* tinst = (InputInstanceFloat4*)_instance;
				return GraphValueVariant(tinst->getDesc().mMinValue.x,
										 tinst->getDesc().mMinValue.y,
										 tinst->getDesc().mMinValue.z,
										 tinst->getDesc().mMinValue.w);
			} 
		case Substance_IType_Integer:
			{
				InputInstanceInt* tinst = (InputInstanceInt*)_instance;
				return GraphValueVariant(tinst->getDesc().mMinValue);
			} 
		case Substance_IType_Integer2:
			{
				InputInstanceInt2* tinst = (InputInstanceInt2*)_instance;
				return GraphValueVariant(tinst->getDesc().mMinValue.x,
										 tinst->getDesc().mMinValue.y);
			} 
		case Substance_IType_Integer3:
			{
				InputInstanceInt3* tinst = (InputInstanceInt3*)_instance;
				return GraphValueVariant(tinst->getDesc().mMinValue.x,
										 tinst->getDesc().mMinValue.y,
										 tinst->getDesc().mMinValue.z);
			} 
		case Substance_IType_Integer4:
			{
				InputInstanceInt4* tinst = (InputInstanceInt4*)_instance;
				return GraphValueVariant(tinst->getDesc().mMinValue.x,
										tinst->getDesc().mMinValue.y,
										tinst->getDesc().mMinValue.z,
										tinst->getDesc().mMinValue.w);
			} 
		default:
			logERROR("getMinValue(): Unexpected numerical input with type: "<<(int)_instance->mDesc.mType);
			return GraphValueVariant();
		}
	}
	else {
		logERROR("getMinValue(): Unsupported input with type: "<<(int)_instance->mDesc.mType);
		return GraphValueVariant();
	}
}

GraphValueVariant GraphInput::GetMaxValue() const
{
	if(_instance->mDesc.isNumerical()) {
		switch (_instance->mDesc.mType) {
		case Substance_IType_Float:
			{
				InputInstanceFloat* tinst = (InputInstanceFloat*)_instance;
				return GraphValueVariant(tinst->getDesc().mMaxValue);
			} 
		case Substance_IType_Float2:
			{
				InputInstanceFloat2* tinst = (InputInstanceFloat2*)_instance;
				return GraphValueVariant(tinst->getDesc().mMaxValue.x,
										 tinst->getDesc().mMaxValue.y);
			} 
		case Substance_IType_Float3:
			{
				InputInstanceFloat3* tinst = (InputInstanceFloat3*)_instance;
				return GraphValueVariant(tinst->getDesc().mMaxValue.x,
										 tinst->getDesc().mMaxValue.y,
										 tinst->getDesc().mMaxValue.z);
			} 
		case Substance_IType_Float4:
			{
				InputInstanceFloat4* tinst = (InputInstanceFloat4*)_instance;
				return GraphValueVariant(tinst->getDesc().mMaxValue.x,
										 tinst->getDesc().mMaxValue.y,
										 tinst->getDesc().mMaxValue.z,
										 tinst->getDesc().mMaxValue.w);
			} 
		case Substance_IType_Integer:
			{
				InputInstanceInt* tinst = (InputInstanceInt*)_instance;
				return GraphValueVariant(tinst->getDesc().mMaxValue);
			} 
		case Substance_IType_Integer2:
			{
				InputInstanceInt2* tinst = (InputInstanceInt2*)_instance;
				return GraphValueVariant(tinst->getDesc().mMaxValue.x,
										 tinst->getDesc().mMaxValue.y);
			} 
		case Substance_IType_Integer3:
			{
				InputInstanceInt3* tinst = (InputInstanceInt3*)_instance;
				return GraphValueVariant(tinst->getDesc().mMaxValue.x,
										 tinst->getDesc().mMaxValue.y,
										 tinst->getDesc().mMaxValue.z);
			} 
		case Substance_IType_Integer4:
			{
				InputInstanceInt4* tinst = (InputInstanceInt4*)_instance;
				return GraphValueVariant(tinst->getDesc().mMaxValue.x,
										tinst->getDesc().mMaxValue.y,
										tinst->getDesc().mMaxValue.z,
										tinst->getDesc().mMaxValue.w);
			} 
		default:
			logERROR("getMaxValue(): Unexpected numerical input with type: "<<(int)_instance->mDesc.mType);
			return GraphValueVariant();
		}
	}
	else {
		logERROR("getMaxValue(): Unsupported input with type: "<<(int)_instance->mDesc.mType);
		return GraphValueVariant();
	}
}

int GraphInput::GetEnumCount() const
{
	if(_instance->mDesc.isNumerical() && _instance->mDesc.mType==Substance_IType_Integer) {
		InputInstanceInt* tinst = (InputInstanceInt*)_instance;
		return (int)tinst->getDesc().mEnumValues.size();
	}

	logERROR("GetEnumCount(): Unexpectedinput with type: "<<(int)_instance->mDesc.mType);
	return 0;
}

GraphEnumValue GraphInput::GetEnumValue(int index)
{
	GraphEnumValue res;
	if(_instance->mDesc.isNumerical() && _instance->mDesc.mType==Substance_IType_Integer) {
		InputInstanceInt* tinst = (InputInstanceInt*)_instance;
		auto val = tinst->getDesc().mEnumValues[index];
		res.label = val.second.c_str();
		res.value = GraphValueVariant(tinst->getValue());
		return res;
	}

	logERROR("GetEnumValue(): Unexpectedinput with type: "<<(int)_instance->mDesc.mType);
	return res;
}

#endif // USE_SUBSTANCE
