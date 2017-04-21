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
/** @file OutputPreviewWidget.h
	@brief Custom widget for texture output previews
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#ifndef SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_OUTPUTPREVIEWWIDGET_H
#define SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_OUTPUTPREVIEWWIDGET_H
#pragma once

#if defined(USE_SUBSTANCE)

#include "Substance/IProceduralMaterial.h"

class QOutputPreviewWidget;
class QProceduralMaterialEditorMainWindow;

/**/
class COutputEnabledUndoCommand : public QUndoCommand
{
public:
	COutputEnabledUndoCommand(QOutputPreviewWidget* pWidget, QProceduralMaterialEditorMainWindow* pMainWindow, bool enabled);

	virtual void undo();
	virtual void redo();

private:
	IGraphInstance* CommitValue(bool enabled);

private:
	QProceduralMaterialEditorMainWindow*		m_MainWindow;
	GraphInstanceID								m_GraphInstanceID;
	GraphOutputID								m_GraphOutputID;
	bool										m_NewValue;
};

/**/
class QOutputPreviewWidget : public QWidget
{
public:
	QOutputPreviewWidget(IGraphOutput* pOutput);

	void OnOutputChanged();
	void SyncUndoValue();

	inline IGraphOutput* GetOutput() const { return m_Output; }

public://slots
	void OnEnabledWidgetStateChanged(int checkstate);

private:
	IGraphOutput*		m_Output;

	QLabel*				m_PreviewWidget;
	QCheckBox*			m_EnabledWidget;
	
	QImage				m_PreviewImage;
};

#endif // USE_SUBSTANCE
#endif //SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_OUTPUTPREVIEWWIDGET_H
