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
/** @file QProceduralMaterialEditorMainWindow.h
	@brief Procedural Material Dialog Header
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#ifndef QPROCEDURALMATERIALEDITORMAINWINDOW_H
#define QPROCEDURALMATERIALEDITORMAINWINDOW_H

#if defined(USE_SUBSTANCE)

#include <QMainWindow>

#include "IEditor.h"

// This is built using the Qt UIC custom tool. RClick the .ui file > properties > Item Type = Qt MOC
#include <ui_QProceduralMaterialEditorMainWindow.h>

#include "Substance/SubstanceBus.h"

struct IGraphInput;
struct IGraphInstance;

class GIGraphInputHandler;
class QOutputPreviewWidget;

/*
*/
class QProceduralMaterialEditorMainWindow : public QMainWindow, public Ui::QProceduralMaterialEditorMainWindow, public IEditorNotifyListener
{
    Q_OBJECT

public:
    explicit QProceduralMaterialEditorMainWindow(QWidget *parent = 0);
    ~QProceduralMaterialEditorMainWindow();

	// you are required to implement this to satisfy the unregister/registerclass requirements on "RegisterQtViewPane"
	// make sure you pick a unique GUID
	static const GUID& GetClassID()
	{
		// {BFB74C7D-2D0B-44CA-9892-BDB57FF77FBA}
		static const GUID guid =
		{ 0xbfb74c7d, 0x2d0b, 0x44ca, { 0x98, 0x92, 0xbd, 0xb5, 0x7f, 0xf7, 0x7f, 0xba } };
		return guid;
	}

	inline QUndoStack* GetUndoStack() const { return m_UndoStack; }

	GIGraphInputHandler* GetGraphInputHandler(GraphInputID graphInputID);
	QOutputPreviewWidget* GetOutputPreviewWidget(GraphOutputID graphOutputID);
	void QueueRender(IGraphInstance* pGraph);

	//used to track modified materials in view window
	void IncrementMaterialModified(IProceduralMaterial* proceduralMaterial);
	void DecrementMaterialModified(IProceduralMaterial* proceduralMaterial);

	//IEditorNotifyListener
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected://signals
	void OnFileImportSubstanceTriggered();
	void OnFileExportTexturesTriggered();
	void OnFileDeleteSubstanceTriggered();
	void OnFileSave();
	void OnFileSaveAs();
	void OnEditReimportSubstance();
	void OnTabPropertiesCurrentChanged(int index);
	void OnTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
	void AddSingleLabel(QWidget* widget, const QString& text);
	void AddGraphInputWidgets(IGraphInput* input, QFormLayout* layout);
	void CreateMaterial(IProceduralMaterial* pProcMaterial);
	void CreateMaterialFromPath(const char* path);
	void CreateOutputPreviews(int graphIndex);
	void DisplayProceduralMaterial(const char* path);
	void UpdateOutputPreviews();
	void RefreshTreeView();
	const char* TranslateInputName(const char* name) const;
	QStandardItem* FindMaterialItem(const string& path) const;
	void EnableRequireSubstanceObjects(bool enabled);

private:
	static const int FULLPATHNAME_ROLE;
	static const int FILENAME_ROLE;

	static const char* PROPERTY_INPUT;

private:
	QStandardItemModel*										m_StandardModel;
	QLabel*													m_StatusBarLabel;
	QProgressBar*											m_StatusBarProgress;
	QWidget*												m_PreviewWidget;
	QUndoStack*												m_UndoStack;
	QAction*												m_ReimportSubstanceAction;

	IProceduralMaterial*									m_CurrentMaterial;
	IGraphInstance*											m_QueueRenderGraph;
	ProceduralMaterialRenderUID								m_RenderUID;
	std::map<GraphInputID, GIGraphInputHandler*>			m_GraphInputHandlerMap;
	std::map<IProceduralMaterial*, int>						m_MaterialModifiedCountMap;
	std::vector<QOutputPreviewWidget*>						m_OutputPreviewWidgets;
	std::vector<QObject*>									m_RequireSubstanceObjects;
};

#endif // USE_SUBSTANCE
#endif // QPROCEDURALMATERIALEDITORMAINWINDOW_H

