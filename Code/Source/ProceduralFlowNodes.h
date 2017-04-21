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
/** @file ProceduralFlowNodes.h
	@brief Header for the Procedural Flow Graph System
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#ifndef GEM_SUBSTANCE_PROCEDURALFLOWNODES_H
#define GEM_SUBSTANCE_PROCEDURALFLOWNODES_H
#pragma once

#include "FlowSystem/Nodes/FlowBaseNode.h"

/// Cast a flowgraph port to a GraphInstanceID
ILINE GraphInstanceID GetPortGraphInstanceID(IFlowNode::SActivationInfo* pActInfo, int nPort)
{
	GraphInstanceID x = *(pActInfo->pInputPorts[nPort].GetPtr<GraphInstanceID>());
	return x;
}

#endif //GEM_SUBSTANCE_PROCEDURALFLOWNODES_H
