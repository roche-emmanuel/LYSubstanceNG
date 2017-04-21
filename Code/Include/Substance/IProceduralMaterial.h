/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
/** @file IProceduralMaterial.h
	@brief Header for the Procedural Material Interface
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#ifndef GEM_SUBSTANCE_IPROCEDURALMATERIAL_H
#define GEM_SUBSTANCE_IPROCEDURALMATERIAL_H
#pragma once

#if defined(USE_SUBSTANCE)

struct IGraphInstance;
struct IGraphInput;
struct IGraphOutput;

#define PROCEDURALMATERIAL_EXTENSION	"smtl"
#define PROCEDURALTEXTURE_EXTENSION		"sub"

/// Encodes a procedural material and graph index
typedef uint32 GraphInstanceID;
#define INVALID_GRAPHINSTANCEID ((GraphInstanceID)(0))

/// Encodes a graphinput UID
typedef uint32 GraphInputID;
#define INVALID_GRAPHINPUTID ((GraphInputID)(0))

/// Encodes a graphoutput UID
typedef uint32 GraphOutputID;
#define INVALID_GRAPHOUTPUTID ((GraphOutputID)(0))

/// A unique render batch ID
typedef unsigned int ProceduralMaterialRenderUID;
#define INVALID_PROCEDURALMATERIALRENDERUID ((ProceduralMaterialRenderUID)0)

/// Maps to substance unique ID's
typedef unsigned int SubstanceUID;

/// References a procedural material ID in CSubstanceSystem
typedef uint16 ProceduralMaterialID;
#define INVALID_PROCEDURALMATERIALID ((ProceduralMaterialID)0)

/**/
struct IProceduralMaterial
{
	virtual ~IProceduralMaterial() {}

	/// Get the smtl file path for this material.
	virtual const char* GetPath() const = 0;

	/// Get the substance archive path for this material.
	virtual const char* GetSourcePath() const = 0;

	/// Get the number of graph instances contained in this material.
	virtual int GetGraphInstanceCount() const = 0;

	/// Retrieve a specific graph instance by index.
	virtual IGraphInstance* GetGraphInstance(int index) = 0;

	/// Reimport Substance SBSAR from Disk
	virtual void ReimportSubstance() = 0;
};

/**/
struct IGraphInstance
{
	virtual ~IGraphInstance() {}

	/// Get the parent procedural material.
	virtual IProceduralMaterial* GetProceduralMaterial() const = 0;

	/// Get the name of this graph.
	virtual const char* GetName() const = 0;

	/// Get the GraphInstanceID.
	virtual GraphInstanceID GetGraphInstanceID() const = 0;

	/// Get the number of input parameters.
	virtual int GetInputCount() const = 0;

	/// Get an input parameter by index.
	virtual IGraphInput* GetInput(int index) = 0;

	/// Get an input parameter by name.
	virtual IGraphInput* GetInputByName(const char* name) = 0;

	/// Get a graph input by ID
	virtual IGraphInput* GetInputByID(GraphInputID inputID) = 0;

	/// Get the number of output object.
	virtual int GetOutputCount() const = 0;

	/// Get an output object by index.
	virtual IGraphOutput* GetOutput(int index) = 0;

	/// Get an output object by ID
	virtual IGraphOutput* GetOutputByID(GraphOutputID outputID) = 0;
};

/**/
enum class GraphInputType
{
	None,
	Float1,
	Float2,
	Float3,
	Float4,
	Integer1,
	Integer2,
	Integer3,
	Integer4,
	Image
};

/**/
enum class GraphInputWidgetType
{
	None,
	Slider,
	Angle,
	Color,
	Boolean,
	Combobox
};

/**/
struct GraphValueVariant
{
public:
	GraphValueVariant() { clear(); }
	GraphValueVariant(const GraphValueVariant& cpy) { memcpy(this, &cpy, sizeof(GraphValueVariant)); }
	GraphValueVariant(int vx, int vy = 0, int vz = 0, int vw = 0) { nValue[0] = vx; nValue[1] = vy; nValue[2] = vz; nValue[3] = vw; }
	GraphValueVariant(int* varr) { nValue[0] = varr[0]; nValue[1] = varr[1]; nValue[2] = varr[2]; nValue[3] = varr[3]; }
	GraphValueVariant(float vx, float vy = 0.0f, float vz = 0.0f, float vw = 0.0f) { fValue[0] = vx; fValue[1] = vy; fValue[2] = vz; fValue[3] = vw; }
	GraphValueVariant(float* varr) { fValue[0] = varr[0]; fValue[1] = varr[1]; fValue[2] = varr[2]; fValue[3] = varr[3]; }
	GraphValueVariant(double cdValue) { clear(); fValue[0] = (float)cdValue; }
	GraphValueVariant(const char* szValue) { clear(); stringValue = szValue; }
	GraphValueVariant(const Vec2i& vec) { clear(); nValue[0] = vec.x; nValue[1] = vec.y; }
	GraphValueVariant(const Vec3i& vec) { clear(); nValue[0] = vec.x; nValue[1] = vec.y; nValue[2] = vec.z; }
	GraphValueVariant(const Vec4i& vec) { clear(); nValue[0] = vec.x; nValue[1] = vec.y; nValue[2] = vec.z; nValue[3] = vec.w; }
	GraphValueVariant(const Vec2& vec) { clear(); fValue[0] = vec.x; fValue[1] = vec.y; }
	GraphValueVariant(const Vec3& vec) { clear(); fValue[0] = vec.x; fValue[1] = vec.y; fValue[2] = vec.z; }
	GraphValueVariant(const Vec4& vec) { clear(); fValue[0] = vec.x; fValue[1] = vec.y; fValue[2] = vec.z; fValue[3] = vec.w; }

	//cast getters
	inline operator float() const { return fValue[0]; }
	inline operator const float*() const { return fValue; }
	inline operator double() const { return (double)fValue[0]; }
	inline operator int() const { return nValue[0]; }
	inline operator const int*() const { return nValue; }
	inline operator const char*() const { return stringValue; }
	inline operator Vec2i() const { return Vec2i(nValue[0], nValue[1]); }
	inline operator Vec3i() const { return Vec3i(nValue[0], nValue[1], nValue[2]); }
	inline operator Vec4i() const { return Vec4i(nValue[0], nValue[1], nValue[2], nValue[3]); }
	inline operator Vec2() const { return Vec2(fValue[0], fValue[1]); }
	inline operator Vec3() const { return Vec3(fValue[0], fValue[1], fValue[2]); }
	inline operator Vec4() const { return Vec4(fValue[0], fValue[1], fValue[2], fValue[3]); }

	//comparison
	inline bool operator==(const GraphValueVariant& rhs) const
	{
		return (nValue[0]==rhs.nValue[0]&&nValue[1]==rhs.nValue[1]&&nValue[2]==rhs.nValue[2]&&nValue[3]==rhs.nValue[3]);
	}
	inline bool operator!=(const GraphValueVariant& rhs) const
	{
		return !(*this == rhs);
	}

private:
	inline void clear()
	{
		memset(this, 0, sizeof(GraphValueVariant));
	}

	union
	{
		const char* stringValue;
		int nValue[4];
		float fValue[4];
	};
};

/**/
struct GraphEnumValue
{
	const char* label;
	GraphValueVariant value;
};

/**/
struct IGraphInput
{
	virtual ~IGraphInput() {}
	
	/// Get the parent graph instance.
	virtual IGraphInstance* GetGraphInstance() const = 0;

	/// Get the graph input ID.
	virtual GraphInputID GetGraphInputID() const = 0;

	/// Get the input description.
	virtual const char* GetDescription() const = 0;

	/// Get the input label.
	virtual const char* GetLabel() const = 0;

	/// Get the input name.
	virtual const char* GetName() const = 0;

	/// Get the GUI Group for this input.
	virtual const char* GetGroupName() const = 0;

	/// Get the input type.
	virtual GraphInputType GetInputType() const = 0;

	/// Get the input type used for GUI display.
	virtual GraphInputWidgetType GetInputWidgetType() const = 0;

	/// Retrieve the value of this input.
	virtual GraphValueVariant GetValue() const = 0;

	/// Assign a new value to this input. You must call QueueRender/Render(A)Sync to update the output textures.
	virtual void SetValue(const GraphValueVariant& value) = 0;

	/// Get the minimum value for this input.
	virtual GraphValueVariant GetMinValue() const = 0;

	/// Get the maximum value for this input.
	virtual GraphValueVariant GetMaxValue() const = 0;

	/// Get enumeration count for combo box inputs.
	virtual int GetEnumCount() const = 0;

	/// Get the enumeration value for combo box inputs.
	virtual GraphEnumValue GetEnumValue(int index) = 0;
};

/**/
struct SGraphOutputEditorPreview
{
	int		Width;
	int		Height;
	int		BytesPerPixel;
	void*	Data;
};

enum class GraphOutputChannel
{
	Diffuse,
	BaseColor,
	Opacity,
	Emissive,
	Ambient,
	AmbientOcclusion,
	Mask,
	Normal,
	Bump,
	Height,
	Displacement,
	Specular,
	SpecularLevel,
	SpecularColor,
	Glossiness,
	Roughness,
	AnisotropyLevel,
	AnisotropyAngle,
	Transmissive,
	Reflection,
	Refraction,
	Environment,
	IOR,
	Scattering0,
	Scattering1,
	Scattering2,
	Scattering3,
	Metallic,
	Any,
	Unknown
};

/**/
struct IGraphOutput
{
	virtual ~IGraphOutput() {}

	/// Get the parent graph instance.
	virtual IGraphInstance* GetGraphInstance() const = 0;

	/// Get the graph output ID.
	virtual GraphOutputID GetGraphOutputID() const = 0;

	/// Get the output label.
	virtual const char* GetLabel() const = 0;

	/// Get the path for this output.
	virtual const char* GetPath() const = 0;

	/// Check if output is enabled for rendering
	virtual bool IsEnabled() const = 0;

	/// Set output rendering on/off
	virtual void SetEnabled(bool bEnabled) = 0;

	/// Check if output texture needs to be updated
	virtual bool IsDirty() const = 0;

	/// Force output texture to be updated at next render call.
	virtual void SetDirty() = 0;

	/// Retrieve editor preview buffer
	virtual bool GetEditorPreview(SGraphOutputEditorPreview& preview) = 0;

	/// Get the associated output channel.
	virtual GraphOutputChannel GetChannel() const = 0;
};
#endif // USE_SUBSTANCE

#endif //GEM_SUBSTANCE_IPROCEDURALMATERIAL_H
