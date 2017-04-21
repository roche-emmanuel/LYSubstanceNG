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
/** @file SubstanceAPI.cpp
	@brief Source file for Substance API Implementation
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#include "StdAfx.h"

#if defined(USE_SUBSTANCE)
#include "ImageExtensionHelper.h"	

#include "SubstanceAPI.h"
#include "SubstanceGem.h"


//--------------------------------------------------------------------------------------------
class CSubstanceParameterXML : public ISubstanceParameterXML
{
public:
	CSubstanceParameterXML(const char* name, const char* value, int graphIndex)
		: m_Name(name)
		, m_Value(value)
		, m_GraphIndex(graphIndex)
	{
	}

	virtual const char* GetName() const
	{
		return m_Name.c_str();
	}

	virtual const char* GetValue() const
	{
		return m_Value.c_str();
	}

	virtual int GetGraphIndex() const
	{
		return m_GraphIndex;
	}

private:
	string	m_Name;
	string	m_Value;
	int		m_GraphIndex;
};

//----------------------------------------------------------------------------------------------------
class CSubstanceMaterialXMLData : public ISubstanceMaterialXMLData
{
public:
	CSubstanceMaterialXMLData(const char* source, const char* data, size_t dataSize)
		: m_Source(source)
		, m_Data(data)
		, m_DataSize(dataSize)
	{
	}
	virtual ~CSubstanceMaterialXMLData()
	{
		delete m_Data;
	}

	virtual void Release() override
	{
		delete this;
	}

	virtual const char* GetSource() const override
	{
		return m_Source.c_str();
	}

	virtual const char* GetSourceData() const override
	{
		return m_Data;
	}

	virtual size_t GetSourceDataSize() const override
	{
		return m_DataSize;
	}

	virtual void AddParameter(const CSubstanceParameterXML& parameter)
	{
		m_Parameters.push_back(parameter);
	}

	virtual const ISubstanceParameterXML* GetParameter(int index) const override
	{
		return &(m_Parameters[index]);
	}

	virtual int GetParameterCount() const override
	{
		return m_Parameters.size();
	}

	void AddOutputInfo(unsigned int uid, const SSubstanceOutputInfoXML& outputInfoXml)
	{
		m_OutputInfoMap[uid] = outputInfoXml;
	}

	virtual const SSubstanceOutputInfoXML* GetOutputInfo(unsigned int uid) const override
	{
		auto iter = m_OutputInfoMap.find(uid);
		if (iter != m_OutputInfoMap.end())
		{
			return &(iter->second);
		}
		return nullptr;
	}

private:
	std::string												m_Source;
	const char*												m_Data;
	size_t													m_DataSize;

	std::map<unsigned int, SSubstanceOutputInfoXML>			m_OutputInfoMap;
	std::vector<CSubstanceParameterXML>						m_Parameters;
};

//--------------------------------------------------------------------------------------------
class CFileContents : public IFileContents
{
public:
	CFileContents(void* data, size_t bytes)
		: m_Data(data)
		, m_Bytes(bytes)
	{
	}
	~CFileContents()
	{
		delete [] m_Data;
	}

	virtual void Release() override
	{
		delete this;
	}

	virtual void* GetData() override
	{
		return m_Data;
	}

	virtual size_t GetDataSize() const override
	{
		return m_Bytes;
	}

private:
	void* m_Data;
	size_t m_Bytes;
};

//----------------------------------------------------------------------------------------------------
void CSubstanceAPI::AddRefDeviceTexture(IDeviceTexture* pTexture)
{
	reinterpret_cast<ITexture*>(pTexture)->AddRef();
}

void* CSubstanceAPI::Alloc(size_t bytes)
{
	return new byte[bytes];
}

int CSubstanceAPI::GetCoreCount()
{
	return substance_coreCount;
}

int CSubstanceAPI::GetMemoryBudget()
{
	return substance_memoryBudget;
}

bool CSubstanceAPI::IsEditor() const
{
	return gEnv->IsEditor();
}

ISubstanceMaterialXMLData* CSubstanceAPI::LoadMaterialXML(const char* path, bool bParametersOnly)
{
    string resolvedPath(path);
    if (gEnv->IsEditor())
    {
        resolvedPath = "@devassets@/" + string(path);
    }

	//parse from xml
	XmlNodeRef mtlNode = GetISystem()->LoadXmlFromFile(resolvedPath);
	if (mtlNode)
	{
		const char* source;

		//get sbsar path
		if (!mtlNode->getAttr("Source", &source))
		{
			CryLogAlways("ProceduralMaterial: No Source parameter for material (%s)", path);
			return nullptr;
		}

		//load substance file
		CCryFile sbsarFile(source, "rb");

		if (!sbsarFile.GetHandle())
		{
			CryLogAlways("ProceduralMaterial: Unable to load substance (%s) in material (%s)", source, path);
			return nullptr;
		}

		//read archive data
		size_t dataSize = sbsarFile.GetLength();
		byte* data = new byte[dataSize];
		sbsarFile.ReadRaw(data, dataSize);

		CSubstanceMaterialXMLData* pXmlData = new CSubstanceMaterialXMLData(source, (const char*)data, dataSize);

		//process child nodes
		for (int i = 0; i < mtlNode->getChildCount(); i++)
		{
			XmlNodeRef child = mtlNode->getChild(i);

			if (!strcmp(child->getTag(), "Output"))
			{
				unsigned int uid;
				SSubstanceOutputInfoXML outputInfo = { true, true };

				child->getAttr("Enabled", outputInfo.bEnabled);
				child->getAttr("Compressed", outputInfo.bCompressed);

				if (child->getAttr("ID", uid))
				{
					pXmlData->AddOutputInfo(uid, outputInfo);
				}
			}
			else if (!strcmp(child->getTag(), "Parameter"))
			{
				const char* name;
				const char* value;
				int graphIndex;

				if (child->getAttr("Name", &name) &&
					child->getAttr("GraphIndex", graphIndex) &&
					child->getAttr("Value", &value))
				{
					pXmlData->AddParameter(CSubstanceParameterXML(name, value, graphIndex));
				}
			}
		}

		return pXmlData;
	}

	return nullptr;
}

bool CSubstanceAPI::LoadTextureXML(const char* path, unsigned int& outputID, const char** material)
{
    string resolvedPath(path);
    if (gEnv->IsEditor())
    {
        resolvedPath = "@devassets@/" + string(path);
    }

	XmlNodeRef mtlNode = GetISystem()->LoadXmlFromFile(resolvedPath);
	if (mtlNode)
	{
		if (!mtlNode->getAttr("OutputID", outputID))
		{
			CryLogAlways("ProceduralTexture: Malformed xml (no OutputID) for sub file (%s)", path);
			return false;
		}

		if (!mtlNode->getAttr("Material", material))
		{
			CryLogAlways("ProceduralTexture: Malformed xml (no Material) for sub file (%s)", path);
			return false;
		}

		return true;
	}
	
	return false;
}

void CSubstanceAPI::Log(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	gEnv->pLog->LogV(ILog::eAlways, msg, args);
	va_end(args);
}

bool CSubstanceAPI::ReadFile(const char* path, IFileContents** pContents)
{
    string resolvedPath(path);
    if (gEnv->IsEditor())
    {
        resolvedPath = "@devassets@/" + string(path);
    }

	//try to load substance archive
	CCryFile file(resolvedPath, "rb");

	if (!file.GetHandle())
	{
		CryLogAlways("CSubstanceAPI: Unable to open file (%s)", path);
		return false;
	}

	//load substance archive
	size_t dataSize = file.GetLength();
	byte* data = new byte[dataSize];
	file.ReadRaw(data, dataSize);

	*pContents = new CFileContents(data, dataSize);
	return true;
}

bool CSubstanceAPI::ReadTexture(const char* path, SSubstanceLoadData& loadData)
{
	if (IRenderer* pRenderer = gEnv->pRenderer)
	{
		ITexture* pTexture = pRenderer->EF_LoadTexture(path, FT_IGNORE_PRECACHE | FT_USAGE_READBACK | FT_DONT_RESIZE | FT_NOMIPS | FT_DONT_STREAM);
		CRY_ASSERT_MESSAGE(!pTexture->IsPostponed(), "Trying to load from postponed texture");
		CRY_ASSERT_MESSAGE(pTexture->GetFlags() & FT_USAGE_READBACK, "Trying to read from a texture that doesn't support CPU readbacks");

		if (pTexture->IsPostponed() || !(pTexture->GetFlags() & FT_USAGE_READBACK))
		{
			return false;
		}

		ETEX_Format texFormat = pTexture->GetTextureSrcFormat();

		switch (texFormat)
		{
		case eTF_BC1:				loadData.m_Format = SubstanceTex_BC1; break;
		case eTF_BC2:				loadData.m_Format = SubstanceTex_BC2; break;
		case eTF_BC3:				loadData.m_Format = SubstanceTex_BC3; break;
		case eTF_L8:				loadData.m_Format = SubstanceTex_L8; break;
		case eTF_PVRTC2:			loadData.m_Format = SubstanceTex_PVRTC2; break;
		case eTF_PVRTC4:			loadData.m_Format = SubstanceTex_PVRTC4; break;
		case eTF_R8G8B8A8:			loadData.m_Format = SubstanceTex_R8G8B8A8; break;
		case eTF_R16G16B16A16:		loadData.m_Format = SubstanceTex_R16G16B16A16; break;
		default:
     		const char* texFormatName = CImageExtensionHelper::NameForTextureFormat(texFormat);
     		CRY_ASSERT_MESSAGE(false, string().Format("Substance doesn't support the %s pixel format used by %s", texFormatName, pTexture->GetName()));
			return false;
		}

		ISubstanceLibAPI *pSubstanceLibAPI = nullptr;
		EBUS_EVENT_RESULT(pSubstanceLibAPI, SubstanceRequestBus, GetSubstanceLibAPI);
		if (pSubstanceLibAPI)
		{
			loadData.m_Width = pTexture->GetWidth();
			loadData.m_Height = pTexture->GetHeight();
			loadData.m_NumMips = pTexture->GetNumMips();
			loadData.m_bNormalMap = false;
			loadData.m_pTexture = nullptr;
	
			size_t bufferSize = pSubstanceLibAPI->CalcTextureSize(pTexture->GetWidth(), pTexture->GetHeight(), pTexture->GetNumMips(), loadData.m_Format);
			uint8* buffer = new uint8[bufferSize];

			int mipWidth = pTexture->GetWidth();
			int mipHeight = pTexture->GetHeight();
			size_t dataOffset = 0;
			for (int i = 0; i < pTexture->GetNumMips(); i++)
			{
				uint8* writeBuffer = buffer + dataOffset;
				size_t mipSize = pSubstanceLibAPI->CalcTextureSize(mipWidth, mipHeight, 1, loadData.m_Format);
				int nPitch;

				uint8* mipData = pTexture->LockData(nPitch, 0, i);
				CRY_ASSERT(mipData);
				if (mipData)
				{
					memcpy(writeBuffer, mipData, mipSize);
					pTexture->UnlockData(0, i);
				}

				dataOffset += mipSize;
				mipWidth = std::max(mipWidth >> 1, 1);
				mipHeight = std::max(mipHeight >> 1, 1);
			}

			loadData.m_pData = buffer;
			loadData.m_DataSize = bufferSize;
	
			return true;
		}
	}

	return false;
}

void CSubstanceAPI::ReleaseDeviceTexture(IDeviceTexture* pTexture)
{
	reinterpret_cast<ITexture*>(pTexture)->Release();
}

void CSubstanceAPI::ReleaseSubstanceLoadData(SSubstanceLoadData& loadData)
{
	if (loadData.m_pData)
	{
		delete [] loadData.m_pData;
	}
}

void CSubstanceAPI::ReloadDeviceTexture(IDeviceTexture* pTexture)
{
	reinterpret_cast<ITexture*>(pTexture)->Reload();
}

bool CSubstanceAPI::WriteFile(const char* path, const char* data, size_t bytes)
{
	//write smtl file
	CCryFile file("@devassets@/" + string(path), "w");
	if (!file.GetHandle())
	{
		CryLogAlways("Unable to create file (%s)", path);
		return false;
	}

	file.Write(data, bytes);
	file.Flush();
	file.Close();
	return true;
}

bool CSubstanceAPI::RemoveFile(const char* path)
{
    return gEnv->pFileIO->Remove("@devassets@/" + string(path));
}
#endif // USE_SUBSTANCE
