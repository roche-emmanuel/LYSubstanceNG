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
/** @file OutputPreviewWidget.cpp
	@brief Custom widget for texture output previews
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#include "StdAfx.h"

#if defined(USE_SUBSTANCE)

#include "OutputPreviewWidget.h"
#include "QProceduralMaterialEditorMainWindow.h"
#include <I3DEngine.h>

static const int PREVIEW_WIDGET_SIZE_SCALAR = 145;
static const QSize PREVIEW_WIDGET_SIZE(PREVIEW_WIDGET_SIZE_SCALAR, PREVIEW_WIDGET_SIZE_SCALAR);

//----------------------------------------------------------------------------------------------------
COutputEnabledUndoCommand::COutputEnabledUndoCommand(QOutputPreviewWidget* pWidget, QProceduralMaterialEditorMainWindow* pMainWindow, bool enabled)
	: QUndoCommand()
	, m_MainWindow(pMainWindow)
	, m_NewValue(enabled)
{
	IGraphOutput* pOutput = pWidget->GetOutput();
	IGraphInstance* pGraphInstance = pOutput->GetGraphInstance();

	m_GraphInstanceID = pGraphInstance->GetGraphInstanceID();
	m_GraphOutputID = pOutput->GetGraphOutputID();
}

void COutputEnabledUndoCommand::undo()
{
	if (IGraphInstance* pGraph = CommitValue(!m_NewValue))
	{
		m_MainWindow->DecrementMaterialModified(pGraph->GetProceduralMaterial());
	}
}

void COutputEnabledUndoCommand::redo()
{
	if (IGraphInstance* pGraph = CommitValue(m_NewValue))
	{
		m_MainWindow->IncrementMaterialModified(pGraph->GetProceduralMaterial());
	}
}

IGraphInstance* COutputEnabledUndoCommand::CommitValue(bool enabled)
{
	if (QOutputPreviewWidget* pWidget = m_MainWindow->GetOutputPreviewWidget(m_GraphOutputID))
	{
		pWidget->GetOutput()->SetEnabled(enabled);
		pWidget->SyncUndoValue();

		return pWidget->GetOutput()->GetGraphInstance();
	}
	// else
	// {
	// 	IGraphInstance* pGraph = nullptr;
	// 	EBUS_EVENT_RESULT(pGraph, SubstanceRequestBus, GetGraphInstance, m_GraphInstanceID);
	// 	if (pGraph)
	// 	{
	// 		for (int i = 0; i < pGraph->GetOutputCount(); i++)
	// 		{
	// 			IGraphOutput* pOutput = pGraph->GetOutput(i);
	// 			if (pOutput->GetGraphOutputID() == m_GraphOutputID)
	// 			{
	// 				pOutput->SetEnabled(enabled);
	// 				return pGraph;
	// 			}
	// 		}
	// 	}
	// }

	return nullptr;
}

//----------------------------------------------------------------------------------------------------
QOutputPreviewWidget::QOutputPreviewWidget(IGraphOutput* pOutput)
	: QWidget()
	, m_Output(pOutput)
{
	setLayout(new QVBoxLayout);

	m_PreviewWidget = new QLabel;
	m_PreviewWidget->setFrameShape(QFrame::Box);
	m_PreviewWidget->setFrameShadow(QFrame::Raised);
	m_PreviewWidget->setMinimumSize(PREVIEW_WIDGET_SIZE);
	m_PreviewWidget->setMaximumSize(PREVIEW_WIDGET_SIZE);
	layout()->addWidget(m_PreviewWidget);

	m_EnabledWidget = new QCheckBox;
	m_EnabledWidget->setText(pOutput->GetLabel());
	m_EnabledWidget->setCheckState(pOutput->IsEnabled() ? Qt::Checked : Qt::Unchecked);
	layout()->addWidget(m_EnabledWidget);
	connect(m_EnabledWidget, &QCheckBox::stateChanged, this, &QOutputPreviewWidget::OnEnabledWidgetStateChanged);
}

void QOutputPreviewWidget::OnEnabledWidgetStateChanged(int checkstate)
{
	//add undo command
	QWidget* main = parentWidget();
	while (main->parentWidget())
	{
		main = main->parentWidget();
	}

	if (QProceduralMaterialEditorMainWindow* mainWindow = qobject_cast<QProceduralMaterialEditorMainWindow*>(main))
	{
		bool enabled = (checkstate == Qt::Checked) ? true : false;
		mainWindow->GetUndoStack()->push(new COutputEnabledUndoCommand(this, mainWindow, enabled));
	}
}

void QOutputPreviewWidget::OnOutputChanged()
{
	SGraphOutputEditorPreview preview;
	if (m_Output->GetEditorPreview(preview))
	{
		m_PreviewImage = QImage(preview.Width, preview.Height, QImage::Format_RGB32);

		uchar pR, pG, pB;
		unsigned short* sptr;

		logDEBUG("Loading preview for "<< m_Output->GetLabel()<<" of size "<<preview.Width<<"x"<<preview.Height<<", with format: "<<preview.Format<<", bytesPerPixel: "<<preview.BytesPerPixel);

		// This conversion below assumes that we have BGR data,
		// But this is not the case any more, so this should be ignored.
		for (int y = 0;y < preview.Height;y++)
		{
			QRgb* pScanline = (QRgb*)m_PreviewImage.scanLine(y);

			for (int x = 0; x < preview.Width;x++)
			{
				uchar* base = (uchar*)(preview.Data) + (x*preview.BytesPerPixel) + (y*preview.BytesPerPixel*preview.Width);
				switch(preview.Format) {
				case eTF_R16:
					sptr = (unsigned short*)base;
					pR = (uchar)(255.0*sptr[0]/65535.0);
					pG = pR;
					pB = pR;
					break;
				case eTF_R16G16B16A16:
					sptr = (unsigned short*)base;
					pR = (uchar)(255.0*sptr[0]/65535.0);
					pG = (uchar)(255.0*sptr[1]/65535.0);
					pB = (uchar)(255.0*sptr[2]/65535.0);
					break;
				case eTF_R8G8B8A8:
					pR = base[0];
					pG = base[1];
					pB = base[2];
					break;
				case eTF_L8:
					pR = base[0];
					pG = base[1];
					pB = base[2];
					break;
				}
				// uchar* psR = (uchar*)(preview.Data) + (x*preview.BytesPerPixel) + (y*preview.BytesPerPixel*preview.Width);
				// uchar* psG = psR + 1;
				// uchar* psB = psG + 1;

				// *(pScanline + x) = qRgb(*psR, *psG, *psB);
				*(pScanline + x) = qRgb(pR, pG, pB);
			}
		}

		logDEBUG("Done loading preview for "<< m_Output->GetLabel());

		QPixmap pixMap = QPixmap::fromImage(m_PreviewImage);
		pixMap = pixMap.scaled(PREVIEW_WIDGET_SIZE, Qt::KeepAspectRatio);
		m_PreviewWidget->setPixmap(pixMap);
	}
}

void QOutputPreviewWidget::SyncUndoValue()
{
	m_EnabledWidget->blockSignals(true);
	m_EnabledWidget->setChecked(m_Output->IsEnabled());
	m_EnabledWidget->blockSignals(false);
}
#endif // USE_SUBSTANCE
