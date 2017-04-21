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
/** @file SubstanceBus.h
	@brief Header for the Substance Gem.
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#ifndef GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_INCLUDE_SUBSTANCEBUS_H
#define GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_INCLUDE_SUBSTANCEBUS_H
#include <AzCore/EBus/EBus.h>

#if defined(USE_SUBSTANCE)
#include "IProceduralMaterial.h"

struct ISubstanceLibAPI;
#endif // USE_SUBSTANCE


//----------------------------------------------------------------------------------------------------
class SubstanceRequests : public AZ::EBusTraits
{
public:
	////////////////////////////////////////////////////////////////////////////
	// EBusTraits
	static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
	static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
	////////////////////////////////////////////////////////////////////////////

#if defined(USE_SUBSTANCE)
	/// Retrieve minimum output size of a texture.
	virtual int GetMinimumOutputSize() const = 0;

	/// Retrieve maximum output size of a texture.
	virtual int GetMaximumOutputSize() const = 0;

	/** Get a ProceduralMaterial object given a file path. 
	  * If bForceLoad is true, we will load the material from disk
	  */
	virtual IProceduralMaterial* GetMaterialFromPath(const char* path, bool bForceLoad) const = 0;

	/// Retrieve a graph instance given a GraphInstanceID.
	virtual IGraphInstance* GetGraphInstance(GraphInstanceID graphInstanceID) const = 0;

	/// Queue a GraphInstance for rendering.
	virtual void QueueRender(IGraphInstance* pGraphInstance) = 0;

	/// Renders all queued graphs asynchronously. Returns a handle so you can query for completion.
	virtual ProceduralMaterialRenderUID RenderASync() = 0;

	/// Renders all queued graphs synchronously.
	virtual void RenderSync() = 0;

	/// Used to query if an async render command has completed.
	virtual bool HasRenderCompleted(ProceduralMaterialRenderUID uid) const = 0;

	/// Special render mode used by editor plugin.
	virtual ProceduralMaterialRenderUID RenderEditorPreviewASync() = 0;

	/// Special render mode used by editor plugin and unit testing.
	virtual void RenderEditorPreviewSync() = 0;

	/// Given a substance archive and destination path, create the appropriate smtl/sub files.
	virtual bool CreateProceduralMaterial(const char* sbsarPath, const char* smtlPath) = 0;

	/// Save an existing procedural material to disk, serializing all property changes.
	virtual bool SaveProceduralMaterial(IProceduralMaterial* pMaterial, const char* path = nullptr) = 0;

	/// Remove a procedural material from disk
	virtual void RemoveProceduralMaterial(IProceduralMaterial* pMaterial) = 0;

	// Retrieve the Substance Library API
	virtual ISubstanceLibAPI* GetSubstanceLibAPI() const = 0;
#endif // USE_SUBSTANCE
};
using SubstanceRequestBus = AZ::EBus<SubstanceRequests>;

#endif// GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_INCLUDE_SUBSTANCEBUS_H
