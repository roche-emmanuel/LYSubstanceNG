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
/** @file QProceduralMaterialEditorMainWindow.cpp
    @brief Procedural Material Dialog Source
    @author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
    @date 09-14-2015
    @copyright Allegorithmic. All rights reserved.
*/
#include "stdafx.h"

#if defined(USE_SUBSTANCE)

#include "QProceduralMaterialEditorMainWindow.h"
#include "OutputPreviewWidget.h"
#include <CryExtension/CryCreateClassInstance.h>
#include "ProceduralMaterialScanner.h"
#include "GraphInputWidgets.h"
#include "Substance/IProceduralMaterial.h"
#include "Util.h"
#include "StringUtils.h"

#include <QProceduralMaterialEditorMainWindow.moc>

//--------------------------------------------------------------------------------------------
//required includes for material manager
struct IRenderMesh;

#include "Util/smartptr.h"
#include "Util/TRefCountBase.h"
#include "Util/Variable.h"
#include "BaseLibrary.h"
#include "Material/MaterialManager.h"
#include "GraphInstance.h"
#include <Substance/framework/renderer.h>

static void QAlertMessageBox(const QString& caption, const QString& text)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(caption);
    msgBox.setText(text);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

//--------------------------------------------------------------------------------------------
const int QProceduralMaterialEditorMainWindow::FULLPATHNAME_ROLE = Qt::UserRole + 1;
const int QProceduralMaterialEditorMainWindow::FILENAME_ROLE = Qt::UserRole + 2;

const char* QProceduralMaterialEditorMainWindow::PROPERTY_INPUT = "_smtlInput";

//--------------------------------------------------------------------------------------------
QProceduralMaterialEditorMainWindow::QProceduralMaterialEditorMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , Ui::QProceduralMaterialEditorMainWindow()
    , m_CurrentMaterial(nullptr)
    , m_QueueRenderGraph(nullptr)
    , m_RenderUID(INVALID_PROCEDURALMATERIALRENDERUID)
{
    setupUi(this);

    //tab signals
    connect(tabProperties, &QTabWidget::currentChanged, this, &QProceduralMaterialEditorMainWindow::OnTabPropertiesCurrentChanged);

    //preview widget
    m_PreviewWidget = new QWidget;
    scrollPreview->setWidget(m_PreviewWidget);

    //menu items
    connect(action_Import_Substance, &QAction::triggered, this, &QProceduralMaterialEditorMainWindow::OnFileImportSubstanceTriggered);
    connect(action_Export_Textures, &QAction::triggered, this, &QProceduralMaterialEditorMainWindow::OnFileExportTexturesTriggered);
    connect(action_Delete_Substance, &QAction::triggered, this, &QProceduralMaterialEditorMainWindow::OnFileDeleteSubstanceTriggered);
    connect(action_Save, &QAction::triggered, this, &QProceduralMaterialEditorMainWindow::OnFileSave);
    connect(action_Save_As, &QAction::triggered, this, &QProceduralMaterialEditorMainWindow::OnFileSaveAs);

    action_Save->setEnabled(false);

    //setup undo system
    m_UndoStack = new QUndoStack(this);

    QAction* undoAction = m_UndoStack->createUndoAction(this);
    QAction* redoAction = m_UndoStack->createRedoAction(this);
    undoAction->setShortcut(QKeySequence::Undo);
    redoAction->setShortcut(QKeySequence::Redo);
    menu_Edit->addAction(undoAction);
    menu_Edit->addAction(redoAction);

    menu_Edit->addSeparator();

    m_ReimportSubstanceAction = new QAction(this);
    m_ReimportSubstanceAction->setText(tr("Reimport Substance"));
    menu_Edit->addAction(m_ReimportSubstanceAction);

    connect(m_ReimportSubstanceAction, &QAction::triggered, this, &QProceduralMaterialEditorMainWindow::OnEditReimportSubstance);

    //register with editor for events
    GetIEditor()->RegisterNotifyListener(this);

    //instance tree view model
    m_StandardModel = new QStandardItemModel;

    treeFiles->setModel(m_StandardModel);
    connect(treeFiles->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QProceduralMaterialEditorMainWindow::OnTreeViewSelectionChanged);

    //add statusbar label and progress bar
    m_StatusBarLabel = new QLabel(statusBar());
    statusBar()->addPermanentWidget(m_StatusBarLabel);

    m_StatusBarProgress = new QProgressBar(statusBar());
    m_StatusBarProgress->setValue(0);
    m_StatusBarProgress->setTextVisible(false);
    statusBar()->addPermanentWidget(m_StatusBarProgress);

    //start file scann
    CProceduralMaterialScanner::Instance()->StartScan();

    //configure widgets which will enable/disable depending on material status
    m_RequireSubstanceObjects.push_back(action_Delete_Substance);
    m_RequireSubstanceObjects.push_back(action_Save_As);
    m_RequireSubstanceObjects.push_back(action_Export_Textures);
    m_RequireSubstanceObjects.push_back(m_ReimportSubstanceAction);
    m_RequireSubstanceObjects.push_back(edit_Substance);

    //start off with nothing selected
    DisplayProceduralMaterial(nullptr);
}

QProceduralMaterialEditorMainWindow::~QProceduralMaterialEditorMainWindow()
{
    CProceduralMaterialScanner::Instance()->StopScan();

    GetIEditor()->UnregisterNotifyListener(this);

    delete m_StandardModel;
    delete m_StatusBarLabel;
}

void QProceduralMaterialEditorMainWindow::QueueRender(IGraphInstance* pGraph)
{
    m_QueueRenderGraph = pGraph;
}

void QProceduralMaterialEditorMainWindow::IncrementMaterialModified(IProceduralMaterial* proceduralMaterial)
{
    if (!m_MaterialModifiedCountMap.count(proceduralMaterial))
    {
        m_MaterialModifiedCountMap[proceduralMaterial] = 0;
    }

    m_MaterialModifiedCountMap[proceduralMaterial] += 1;

    if (QStandardItem* pItem = FindMaterialItem(proceduralMaterial->GetPath()))
    {
        QByteArray fileName = pItem->data(FILENAME_ROLE).toByteArray();
        pItem->setText(QString(fileName.data()).append("*"));
    }

    if (m_CurrentMaterial == proceduralMaterial)
    {
        action_Save->setEnabled(true);
    }
}

void QProceduralMaterialEditorMainWindow::DecrementMaterialModified(IProceduralMaterial* proceduralMaterial)
{
    m_MaterialModifiedCountMap[proceduralMaterial]--;

    if (m_MaterialModifiedCountMap[proceduralMaterial] == 0)
    {
        if (QStandardItem* pItem = FindMaterialItem(proceduralMaterial->GetPath()))
        {
            QByteArray fileName = pItem->data(FILENAME_ROLE).toByteArray();
            pItem->setText(fileName);
        }

        if (m_CurrentMaterial == proceduralMaterial)
        {
            action_Save->setEnabled(false);
        }
    }
}

GIGraphInputHandler* QProceduralMaterialEditorMainWindow::GetGraphInputHandler(GraphInputID graphInputID)
{
    auto iter = m_GraphInputHandlerMap.find(graphInputID);
    if (iter != m_GraphInputHandlerMap.end())
    {
        return iter->second;
    }
    return nullptr;
}

QOutputPreviewWidget* QProceduralMaterialEditorMainWindow::GetOutputPreviewWidget(GraphOutputID graphOutputID)
{
    for (auto iter = m_OutputPreviewWidgets.begin(); iter != m_OutputPreviewWidgets.end(); iter++)
    {
        QOutputPreviewWidget* pWidget = *iter;
        if (pWidget->GetOutput()->GetGraphOutputID() == graphOutputID)
        {
            return pWidget;
        }
    }

    return nullptr;
}

void QProceduralMaterialEditorMainWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnIdleUpdate:
    {
        RefreshTreeView();

        //update editor previews
        if (m_RenderUID == INVALID_PROCEDURALMATERIALRENDERUID)
        {
            //if an input change is pending, just update editor preview
            if (m_QueueRenderGraph)
            {
                std::vector<IGraphOutput*>  disabledOutputs;

                //mark all outputs as enabled for preview purposes
                for (int o = 0; o < m_QueueRenderGraph->GetOutputCount(); o++)
                {
                    IGraphOutput* pOutput = m_QueueRenderGraph->GetOutput(o);
                    if (!pOutput->IsEnabled())
                    {
                        pOutput->SetEnabled(true);
                        disabledOutputs.push_back(pOutput);
                    }
                }

                // Here we should request the rendering of our previews:
                logDEBUG("Rendering outputs for preview...");
                m_StatusBarProgress->setMaximum(0);
                SubstanceAir::Renderer renderer;
                auto graph = (GraphInstance*)m_QueueRenderGraph;
                renderer.push(*(graph->getInstance()));

                unsigned int res = renderer.run();
                logDEBUG("Preview: render job UID = "<<res);
                
                // We should now update the output previews:
                UpdateOutputPreviews();

                //render editor preview thumbnails
                // EBUS_EVENT(SubstanceRequestBus, QueueRender, m_QueueRenderGraph);
                // EBUS_EVENT_RESULT(m_RenderUID, SubstanceRequestBus, RenderEditorPreviewASync);
                // m_StatusBarProgress->setMaximum(0);

                m_QueueRenderGraph = nullptr;

                //restore output settings
                for (auto iter = disabledOutputs.begin(); iter != disabledOutputs.end(); iter++)
                {
                    (*iter)->SetEnabled(false);
                }
            }
            else
            {
                m_StatusBarProgress->setMaximum(1);
            }
        }
    }
    break;
    }
}

void QProceduralMaterialEditorMainWindow::OnFileImportSubstanceTriggered()
{
    AZStd::string gameFolder = Path::GetEditingGameDataFolder();

    QString filter(tr("Substances (*.sbsar);;All Files (*.*)"));
    QString sbsarFile = QFileDialog::getOpenFileName(this, tr("Select Substance File"), gameFolder.c_str(), filter);

    QFileInfo sbsarInfo(sbsarFile);
    if (sbsarInfo.exists())
    {
        string stdSbsarFileAbs = sbsarFile.toStdString().c_str();

        //make sure item is in the game folder!
        if (strstr(CryStringUtils::ToLower(stdSbsarFileAbs).c_str(), CryStringUtils::ToLower(PathUtil::ToUnixPath(gameFolder.c_str())).c_str()) == nullptr)
        {
            QMessageBox msgBox;
            msgBox.setText(tr("Substance must be imported from game folder"));
            msgBox.exec();
            return;
        }

        // Keep the absolute file path here instead of relative:
        sbsarFile = Util::AbsolutePathToGamePath(sbsarFile);

        string stdSbarFile = string(sbsarFile.toStdString().c_str());
        string sbsarName = PathUtil::GetFileName(stdSbarFile);
        string smtlFile = PathUtil::AddSlash(PathUtil::GetPath(stdSbarFile)) + sbsarName + string(".smtl");

        //check for reimport first
        IProceduralMaterial* pMaterial = nullptr;
        EBUS_EVENT_RESULT(pMaterial, SubstanceRequestBus, GetMaterialFromPath, smtlFile.c_str(), false);
        if (pMaterial)
        {
            pMaterial->ReimportSubstance();

            if (m_CurrentMaterial == pMaterial)
            {
                DisplayProceduralMaterial(m_CurrentMaterial->GetPath());
            }

            return;
        }

        bool createdMaterial = false;
        AZStd::string basePath = Path::GetEditingGameDataFolder().c_str();

        EBUS_EVENT_RESULT(createdMaterial, SubstanceRequestBus, CreateProceduralMaterial, basePath.c_str(), stdSbarFile.c_str(), smtlFile.c_str());
        if (!createdMaterial)
        {
            QMessageBox msgBox;
            msgBox.setText(tr("Failed to create Substance"));
            msgBox.exec();
            return;
        }

        //re-issue file scan
        CProceduralMaterialScanner::Instance()->StartScan();

        // Create the material:
        CreateMaterialFromPath(smtlFile.c_str());
    }
}

void QProceduralMaterialEditorMainWindow::CreateMaterialFromPath(const char* path)
{
    //re-issue file scan
    CProceduralMaterialScanner::Instance()->StartScan();

    IProceduralMaterial* pMaterial = nullptr;
    EBUS_EVENT_RESULT(pMaterial, SubstanceRequestBus, GetMaterialFromPath, path, true);
    if (pMaterial)
    {
        CreateMaterial(pMaterial);
        // Delete the material interface:
        delete pMaterial;
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Failed to create Substance Material"));
        msgBox.exec();
        return;
    }
}

bool HasGlossyInAlpha(const SGraphOutputEditorPreview& editorPreview)
{
    // Loop through the image data
    byte* data = static_cast<byte*>(editorPreview.Data);
    if (data)
    {
        if (editorPreview.BytesPerPixel == 4)
        {
            for (int y = editorPreview.Height - 1; y >= 0; y--)
            {
                for (int x = editorPreview.Width - 1; x >= 0; x--)
                {
                    int stride = editorPreview.Width * editorPreview.BytesPerPixel;
                    int offset = stride * y + x * editorPreview.BytesPerPixel;

                    // If any pixel has a non-white alpha, return true
                    int alphaValue = *(data + offset + 3);
                    if (alphaValue != 255)
                    {
                        return true;
                    }
                }
            }
        }
    }
    // If the entire alpha channel is filled with white, then there is no glossmap in the alpha channel
    return false;
}

bool TryGenerateLumberyardSuffix(string& label, GraphOutputChannel outputChannel, const SGraphOutputEditorPreview& editorPreview)
{
    switch (outputChannel)
    {
    case GraphOutputChannel::Diffuse:
        label = "diff";
        return true;
    case GraphOutputChannel::Specular:
        label = "spec";
        return true;
    case GraphOutputChannel::Normal:
        if (HasGlossyInAlpha(editorPreview))
        {
            label = "ddna";
        }
        else
        {
            label = "ddn";
        }
        return true;
    case GraphOutputChannel::Displacement:
        label = "displ";
        return true;
    default:
        break;
    }
    return false;
}

void QProceduralMaterialEditorMainWindow::OnFileExportTexturesTriggered()
{
    if (m_CurrentMaterial)
    {
        AZStd::string gameFolder = Path::GetEditingGameDataFolder();

        QFileDialog saveDialog;
        saveDialog.setFileMode(QFileDialog::DirectoryOnly);
        saveDialog.setNameFilter(tr("All Files (*.*)"));

        QString saveDir;

        if (saveDialog.exec())
        {
            saveDir = saveDialog.selectedFiles().first();
        }

        QFileInfo saveDirInfo(saveDir);
        string saveName = saveDir.toStdString().c_str();

        if (!saveDirInfo.exists())
        {
            QAlertMessageBox(tr("Unable to export"), tr("Cannot save to unknown directory"));
            return;
        }

        string materialName = PathUtil::GetFileName(m_CurrentMaterial->GetPath());

        for (int g = 0; g < m_CurrentMaterial->GetGraphInstanceCount(); g++)
        {
            bool bMultiGraph = (m_CurrentMaterial->GetGraphInstanceCount() > 1) ? true : false;
            IGraphInstance* pGraph = m_CurrentMaterial->GetGraphInstance(g);

            for (int o = 0; o < pGraph->GetOutputCount(); o++)
            {
                IGraphOutput* pOutput = pGraph->GetOutput(o);
                SGraphOutputEditorPreview editorPreview;

                if (!pOutput->IsEnabled())
                {
                    continue;
                }

                if (!pOutput->GetEditorPreview(editorPreview))
                {
                    QAlertMessageBox(tr("Unable to save file"), tr("Unable to read editor preview"));
                    continue;
                }

                // Convert to the Lumberyard required filename endings _spec, _ddn, _ddna, etc.
                string label = "";
                if (!TryGenerateLumberyardSuffix(label, pOutput->GetChannel(), editorPreview))
                {
                    // Use the label specified in Substance Designer if there is no Lumberyard suffix that corresponds to the Substance output channel
                    label = pOutput->GetLabel();
                }

                string filename = label + string(".tga");

                if (bMultiGraph)
                {
                    filename = pGraph->GetName() + string("_") + filename;
                }

                //create full file path
                string fullPath = PathUtil::AddSlash(saveName) + materialName + string("_") + filename;

                CCryFile outFile(fullPath.c_str(), "wb");

                if (!outFile.GetHandle())
                {
                    QAlertMessageBox(tr("Unable to save file"), QString("Cannot open file \'") + QString(fullPath.c_str()) + QString("\' for saving"));
                    continue;
                }

                //write header (don't use a struct here so we don't have to worry about packing rules)
                byte tgaHeader[18];

                static const int kTGA_RGB = 2;
                static const int kTGA_L = 3;

                const int width = editorPreview.Width;
                const int height = editorPreview.Height;

                memset(tgaHeader, 0, sizeof(tgaHeader));
                tgaHeader[2] = (editorPreview.BytesPerPixel == 1) ? kTGA_L : kTGA_RGB;
                tgaHeader[12] = width & 0x00FF;
                tgaHeader[13] = (width & 0xFF00) / 256;
                tgaHeader[14] = height & 0x0FF;
                tgaHeader[15] = (height & 0xFF00) / 256;
                tgaHeader[16] = editorPreview.BytesPerPixel * 8;

                outFile.Write(tgaHeader, sizeof(tgaHeader));

                for (int y = height - 1; y >= 0; y--)
                {
                    int stride = width * editorPreview.BytesPerPixel;
                    int offset = stride * y;
                    outFile.Write(static_cast<byte*>(editorPreview.Data) + offset, stride);
                }

                outFile.Flush();
                outFile.Close();
            }
        }
    }
}

void QProceduralMaterialEditorMainWindow::OnFileDeleteSubstanceTriggered()
{
    if (m_CurrentMaterial)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Delete Substance"));
        msgBox.setText(QString("Are you sure you wish to delete \'") + QString(m_CurrentMaterial->GetPath()) + QString("\' and all associated files?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes)
        {
            CMaterialManager* pMatMan = GetIEditor()->GetMaterialManager();

            //delete mtl files
            for (int g = 0; g < m_CurrentMaterial->GetGraphInstanceCount(); g++)
            {
                IGraphInstance* pGraph = m_CurrentMaterial->GetGraphInstance(g);

                string smtlName = PathUtil::GetFileName(m_CurrentMaterial->GetPath());
                string mtlPath = PathUtil::AddSlash(PathUtil::GetPath(m_CurrentMaterial->GetPath())) + smtlName;

                if (m_CurrentMaterial->GetGraphInstanceCount() > 1)
                {
                    mtlPath = mtlPath + string("_") + string(pGraph->GetName());
                }

                mtlPath = mtlPath + string(".mtl");

                if (CMaterial* pMaterial = pMatMan->LoadMaterial(mtlPath.c_str(), false))
                {
                    pMatMan->DeleteMaterial(pMaterial);
                }
            }

            //remove material data
            EBUS_EVENT(SubstanceRequestBus, RemoveProceduralMaterial, m_CurrentMaterial);

            //close procedural material display
            DisplayProceduralMaterial(nullptr);

            //re-issue file scan
            CProceduralMaterialScanner::Instance()->StartScan();
        }
    }
}

void QProceduralMaterialEditorMainWindow::OnFileSave()
{
    if (m_CurrentMaterial)
    {
        bool saved = false;
        AZStd::string basePath = Path::GetEditingGameDataFolder().c_str();

        EBUS_EVENT_RESULT(saved, SubstanceRequestBus, SaveProceduralMaterial, m_CurrentMaterial, basePath.c_str(), nullptr);
        if (!saved)
        {
            QAlertMessageBox(tr("Failed to save Substance"), tr("Failed to save Substance"));
            return;
        }

        CreateMaterialFromPath(m_CurrentMaterial->GetPath());

        //reset asterisk
        m_MaterialModifiedCountMap[m_CurrentMaterial] = 0;
        if (QStandardItem* pItem = FindMaterialItem(m_CurrentMaterial->GetPath()))
        {
            QByteArray fileName = pItem->data(FILENAME_ROLE).toByteArray();
            pItem->setText(fileName);
        }
    }
}

void QProceduralMaterialEditorMainWindow::OnFileSaveAs()
{
    if (m_CurrentMaterial)
    {
        AZStd::string gameFolder = Path::GetEditingGameDataFolder();

        QString filter(tr("SMTL (*.smtl);;All Files (*.*)"));
        QString smtlFile = QFileDialog::getSaveFileName(this, tr("Select File"), gameFolder.c_str(), filter);

        QFileInfo smltInfo(smtlFile);
        if (smltInfo.exists())
        {
            QMessageBox msgBox;
            msgBox.setText(tr("Cannot overwrite existing file"));
            msgBox.exec();
            return;
        }

        smtlFile = Util::AbsolutePathToGamePath(smtlFile);
        std::string stdSmtlFile = smtlFile.toStdString();

        bool saved = false;
        AZStd::string basePath = Path::GetEditingGameDataFolder().c_str();

        EBUS_EVENT_RESULT(saved, SubstanceRequestBus, SaveProceduralMaterial, m_CurrentMaterial, basePath.c_str(), stdSmtlFile.c_str());
        if (!saved)
        {
            QMessageBox msgBox;
            msgBox.setText(tr("Failed to save Substance"));
            msgBox.exec();
            return;
        }

        CreateMaterialFromPath(stdSmtlFile.c_str());

        //re-issue file scan
        CProceduralMaterialScanner::Instance()->StartScan();
    }
}

void QProceduralMaterialEditorMainWindow::OnEditReimportSubstance()
{
    if (m_CurrentMaterial != nullptr)
    {
        m_CurrentMaterial->ReimportSubstance();

        DisplayProceduralMaterial(m_CurrentMaterial->GetPath());
    }
}

void QProceduralMaterialEditorMainWindow::CreateMaterial(IProceduralMaterial* pProcMaterial)
{
    //save a material for each graph
    for (int g = 0; g < pProcMaterial->GetGraphInstanceCount(); g++)
    {
        IGraphInstance* pGraph = pProcMaterial->GetGraphInstance(g);

        string smtlName = PathUtil::GetFileName(pProcMaterial->GetPath());
        string mtlPath = PathUtil::AddSlash(PathUtil::GetPath(pProcMaterial->GetPath())) + smtlName;
        mtlPath = CryStringUtils::ToLower(PathUtil::ToUnixPath(mtlPath));
        mtlPath = Path::FullPathToGamePath(mtlPath.c_str());

        if (pProcMaterial->GetGraphInstanceCount() > 1)
        {
            mtlPath = mtlPath + string("_") + string(pGraph->GetName());
        }

        mtlPath = mtlPath + string(".mtl");

        // Check if we already have a material with this name:
        auto matman = GetIEditor()->GetMaterialManager();
        logDEBUG("Looking for material with name: "<<mtlPath.c_str());

        CMaterial* pEditorMaterial = matman->LoadMaterial(mtlPath.c_str(), false);
        if(!pEditorMaterial) {
            logDEBUG("Creating material: "<<mtlPath.c_str());
            pEditorMaterial = matman->CreateMaterial(mtlPath.c_str());
            
            if(!pEditorMaterial) {
                logERROR("Cannot create material "<<mtlPath.c_str());
                return;
            }
        }
    
        pEditorMaterial->SetSurfaceTypeName("mat_invulnerable");

        SInputShaderResources& shaderResources = pEditorMaterial->GetShaderResources();
        // shaderResources.Cleanup();
        
        struct GraphOutputTextureMap
        {
            GraphOutputChannel srcType;
            EEfResTextures dstType;
        };

        GraphOutputTextureMap gotm[] = {
            { GraphOutputChannel::Diffuse, EFTT_DIFFUSE },
            { GraphOutputChannel::Normal, EFTT_NORMALS },
            { GraphOutputChannel::Specular, EFTT_SPECULAR },
            { GraphOutputChannel::Environment, EFTT_ENV },
            { GraphOutputChannel::Opacity, EFTT_OPACITY },
            { GraphOutputChannel::Height, EFTT_HEIGHT },
            { GraphOutputChannel::Emissive, EFTT_EMITTANCE },
        };

        //assign textures
        for (auto o = 0; o < pGraph->GetOutputCount(); o++)
        {
            IGraphOutput* pOutput = pGraph->GetOutput(o);

            for (int gc = 0; gc < DIMOF(gotm); gc++)
            {
                if (gotm[gc].srcType == pOutput->GetChannel())
                {
                    SEfResTexture& resTexture = shaderResources.m_Textures[gotm[gc].dstType];

                    // logDEBUG("Path 1: "<<pOutput->GetPath());
                    resTexture.m_Name = CryStringUtils::ToLower(PathUtil::ToUnixPath(pOutput->GetPath()));
                    // logDEBUG("Path 2: "<<resTexture.m_Name.c_str());
                    resTexture.m_Name = Path::FullPathToGamePath(resTexture.m_Name.c_str());
                    // logDEBUG("Requesting texture from path: "<< resTexture.m_Name.c_str());
                    //resTexture.m_Ext.m_nFrameUpdated = -1; // This doesn't help to refresh the textures.
                    break;
                }
            }
        }

        //save changes
        pEditorMaterial->Update();
        pEditorMaterial->Save();
    }
}

void QProceduralMaterialEditorMainWindow::CreateOutputPreviews(int graphIndex)
{
    //delete output previews
    for (auto iter = m_OutputPreviewWidgets.begin(); iter != m_OutputPreviewWidgets.end(); iter++)
    {
        delete *iter;
    }
    m_OutputPreviewWidgets.clear();

    if (m_CurrentMaterial)
    {
        m_PreviewWidget->setLayout(new QVBoxLayout);
        IGraphInstance* pGraph = m_CurrentMaterial->GetGraphInstance(graphIndex);
        for (int o = 0; o < pGraph->GetOutputCount(); o++)
        {
            IGraphOutput* pOutput = pGraph->GetOutput(o);
            QOutputPreviewWidget* pPreviewWidget = new QOutputPreviewWidget(pOutput);
            m_PreviewWidget->layout()->addWidget(pPreviewWidget);
            m_OutputPreviewWidgets.push_back(pPreviewWidget);
        }

        m_QueueRenderGraph = m_CurrentMaterial->GetGraphInstance(graphIndex);

        //flag all outputs as dirty so queue render has something to do
        for (int o = 0; o < m_QueueRenderGraph->GetOutputCount(); o++)
        {
            m_QueueRenderGraph->GetOutput(o)->SetDirty();
        }
    }
}

void QProceduralMaterialEditorMainWindow::OnTabPropertiesCurrentChanged(int index)
{
    //update output previews
    CreateOutputPreviews(index);
}

void QProceduralMaterialEditorMainWindow::OnTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    QModelIndexList modelIndexList = selected.indexes();

    if (modelIndexList.length())
    {
        QVariant data = modelIndexList[0].data(FULLPATHNAME_ROLE);

        if (data.type() == QVariant::ByteArray)
        {
            QByteArray qba = data.toByteArray();
            DisplayProceduralMaterial(qba.data());
        }
    }
}

void QProceduralMaterialEditorMainWindow::AddSingleLabel(QWidget* widget, const QString& text)
{
    QLabel* label = new QLabel(widget);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    label->setGeometry(10, 10, 300, 25);
    //label->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    label->setText(text);
}

void QProceduralMaterialEditorMainWindow::AddGraphInputWidgets(IGraphInput* input, QFormLayout* layout)
{
    GIGraphInputHandler* widget = nullptr;

    if (!strcmp("$randomseed", input->GetName()))
    {
        widget = new GIRandomSeed(input, this);
    }
    else if (!strcmp("$outputsize", input->GetName()))
    {
        widget = new GIOutputSize(input, this);
    }
    else
    {
        switch (input->GetInputWidgetType())
        {
        case GraphInputWidgetType::Angle:
        {
            if (input->GetInputType() == GraphInputType::Float1)
            {
                widget = new GIAngle(input, this);
            }
        }
        break;
        case GraphInputWidgetType::Combobox:
        {
            widget = new GIComboBox(input, this);
        }
        break;
        case GraphInputWidgetType::Boolean:
        {
            widget = new GICheckbox(input, this);
        }
        break;
        case GraphInputWidgetType::Color:
        {
            if (input->GetInputType() == GraphInputType::Float3 || input->GetInputType() == GraphInputType::Float4)
            {
                widget = new GIColor(input, this);
            }
        }
        break;
        case GraphInputWidgetType::Slider:
            //we already use sliders below
            break;
        }

        //if there is no special widget type, fall back to baseline types
        if (widget == nullptr)
        {
            switch (input->GetInputType())
            {
            case GraphInputType::Integer1:
                widget = new GISliderInt(input, this);
                break;
            case GraphInputType::Integer2:
                widget = new GIVectorInt2(input, this);
                break;
            case GraphInputType::Integer3:
                widget = new GIVectorInt3(input, this);
                break;
            case GraphInputType::Integer4:
                widget = new GIVectorInt4(input, this);
                break;
            case GraphInputType::Float1:
                widget = new GISliderFloat(input, this);
                break;
            case GraphInputType::Float2:
                widget = new GIVectorFloat2(input, this);
                break;
            case GraphInputType::Float3:
                widget = new GIVectorFloat3(input, this);
                break;
            case GraphInputType::Float4:
                widget = new GIVectorFloat4(input, this);
                break;
            case GraphInputType::Image:
                widget = new GIImageInput(input, this);
                break;
            }
        }
    }

    //sanity check
    CRY_ASSERT(widget);

    layout->addRow(QString(TranslateInputName(input->GetLabel())).append(":"), widget->GetWidget());

    m_GraphInputHandlerMap[input->GetGraphInputID()] = widget;

    //adjust label alignment
    QLabel* label = qobject_cast<QLabel*>(layout->itemAt(layout->rowCount() - 1, QFormLayout::LabelRole)->widget());
    label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    label->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

void QProceduralMaterialEditorMainWindow::DisplayProceduralMaterial(const char* path)
{
    m_CurrentMaterial = nullptr;
    m_GraphInputHandlerMap.clear();

    CreateOutputPreviews(0);

    action_Save->setEnabled(false);

    EnableRequireSubstanceObjects(false);

    edit_Substance->setText("");

    m_ReimportSubstanceAction->setEnabled(false);

    if (!path)
    {
        for (int i = 0; i < tabProperties->count(); i++)
        {
            tabProperties->widget(i)->deleteLater();
        }

        QWidget* newTabWidget = new QWidget();
        tabProperties->addTab(newTabWidget, tr("N/A"));
        AddSingleLabel(newTabWidget, tr("No Substance Selected"));
        newTabWidget->show();
        tabProperties->setCurrentWidget(newTabWidget);

        m_StatusBarLabel->setText(tr("No Substance Selected"));

        return;
    }

    //clear tabs and previews
    for (int i = 0; i < tabProperties->count(); i++)
    {
        tabProperties->widget(i)->deleteLater();
    }

    IProceduralMaterial* pMaterial = nullptr;
    EBUS_EVENT_RESULT(pMaterial, SubstanceRequestBus, GetMaterialFromPath, path, true);
    if (pMaterial)
    {
        m_StatusBarLabel->setText(QString(path));

        edit_Substance->setText(QString(pMaterial->GetSourcePath()));

        for (int i = 0; i < pMaterial->GetGraphInstanceCount(); i++)
        {
            IGraphInstance* pGraph = pMaterial->GetGraphInstance(i);

            QScrollArea* scrollArea = new QScrollArea();
            scrollArea->setWidgetResizable(true);

            tabProperties->addTab(scrollArea, QString(pGraph->GetName()));

            QWidget* tabWidget = new QWidget;
            scrollArea->setWidget(tabWidget);

            QFormLayout* formLayout = new QFormLayout;
            tabWidget->setLayout(formLayout);

            //assemble inputs by groups
            std::map<QString, std::vector<IGraphInput*> > inputMap;
            AZ_TracePrintf("Default", "Number of inputs in Qproxxx == %d",pGraph->GetInputCount());
            for (int i = 0; i < pGraph->GetInputCount(); i++)
            {
                IGraphInput* pInput = pGraph->GetInput(i);

                //hide $normalformat
                if (!strcmp(pInput->GetName(), "$normalformat"))
                {
                    continue;
                }

                inputMap[QString(pInput->GetGroupName())].push_back(pInput);
            }

            //lay out widgets
            for (auto iter = inputMap.begin(); iter != inputMap.end(); iter++)
            {
                if (inputMap.size() > 1 && iter->first.length())
                {
                    QString labelText = "<b>";
                    labelText.append(iter->first);
                    labelText.append("</b>");

                    QLabel* label = new QLabel;
                    label->setText(labelText);
                    label->setIndent(20);

                    formLayout->addRow(new QWidget, new QWidget);
                    formLayout->addRow(label, new QWidget);
                    formLayout->addRow(new QWidget, new QWidget);
                }

                //add inputs to parent widget
                std::vector<IGraphInput*>& inputs = iter->second;

                for (auto iiter = inputs.begin(); iiter != inputs.end(); iiter++)
                {
                    AddGraphInputWidgets(*iiter, formLayout);
                }
            }

            tabWidget->show();
        }

        const int graphIndex = 0;

        tabProperties->setCurrentWidget(tabProperties->widget(graphIndex));

        m_CurrentMaterial = pMaterial;

        if (m_MaterialModifiedCountMap[m_CurrentMaterial] > 0)
        {
            action_Save->setEnabled(true);
        }

        EnableRequireSubstanceObjects(true);

        //create preview outputs
        CreateOutputPreviews(graphIndex);
    }
    else
    {
        QWidget* tabWidget = new QWidget();
        tabProperties->addTab(tabWidget, tr("N/A"));
        AddSingleLabel(tabWidget, tr("Failed to load Substance"));
        tabWidget->show();
        tabProperties->setCurrentWidget(tabWidget);

        m_StatusBarLabel->setText(tr("Failed to load Substance"));
    }
}

void QProceduralMaterialEditorMainWindow::UpdateOutputPreviews()
{
    //update preview widgets
    for (auto iter = m_OutputPreviewWidgets.begin(); iter != m_OutputPreviewWidgets.end(); iter++)
    {
        (*iter)->OnOutputChanged();
    }
}

void QProceduralMaterialEditorMainWindow::RefreshTreeView()
{
    TFileVector files;
    if (CProceduralMaterialScanner::Instance()->GetLoadedFiles(files))
    {
        m_StandardModel->clear();

        treeFiles->setSortingEnabled(false);

        for (TFileVector::iterator iter = files.begin(); iter != files.end(); iter++)
        {
            const string& path = *iter;
            QStandardItem* currentItem = m_StandardModel->invisibleRootItem();

            //split path using tokens
            int curPos = 0;
            string result = path.Tokenize("/", curPos);

            while (result.length())
            {
                if (strlen(PathUtil::GetExt(result.c_str())))
                {
                    QStandardItem* leaf = new QStandardItem(QIcon("://Icons/ProceduralMaterial_Icon.png"), result.c_str());
                    leaf->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                    leaf->setData(QByteArray(path.c_str()), FULLPATHNAME_ROLE);
                    leaf->setData(QByteArray(result.c_str()), FILENAME_ROLE);

                    currentItem->appendRow(leaf);

                    break;
                }

                QStandardItem* parent = currentItem;
                currentItem = nullptr;

                //find item matching this path element
                for (int i = 0; i < parent->rowCount(); i++)
                {
                    QStandardItem* item = parent->child(i);
                    if (item->text() == result)
                    {
                        currentItem = item;
                        break;
                    }
                }

                if (!currentItem)
                {
                    currentItem = new QStandardItem(QIcon("://Icons/Folder_Icon.png"), result.c_str());
                    currentItem->setFlags(Qt::ItemIsEnabled);
                    parent->appendRow(currentItem);
                }

                result = path.Tokenize("/", curPos);
            }
        }

        treeFiles->setSortingEnabled(true);
    }
}

const char* QProceduralMaterialEditorMainWindow::TranslateInputName(const char* name) const
{
    struct NameMap
    {
        const char* srcName;
        const char* dstName;
    };

    NameMap nameMap[] = {
        { "$outputsize", "Output Size" },
        { "$randomseed", "Random Seed" },
        { nullptr },
    };

    NameMap* curName = nameMap;
    while (curName->srcName)
    {
        if (!strcmp(curName->srcName, name))
        {
            return curName->dstName;
        }

        curName++;
    }

    return name;
}

QStandardItem* QProceduralMaterialEditorMainWindow::FindMaterialItem(const string& path) const
{
    QStandardItem* pCurrentItem = m_StandardModel->invisibleRootItem();

    //split path using tokens
    int curPos = 0;
    string result = path.Tokenize("/", curPos);

    while (result.length())
    {
        if (strlen(PathUtil::GetExt(result.c_str())))
        {
            for (int i = 0; i < pCurrentItem->rowCount(); i++)
            {
                QStandardItem* pItem = pCurrentItem->child(i);
                QByteArray fileName = pItem->data(FILENAME_ROLE).toByteArray();

                if (!strcmp(fileName.data(), result.c_str()))
                {
                    return pItem;
                }
            }

            return nullptr;
        }

        //find item matching this path element
        for (int i = 0; i < pCurrentItem->rowCount(); i++)
        {
            QStandardItem* pItem = pCurrentItem->child(i);
            if (pItem->text() == result)
            {
                pCurrentItem = pItem;
                break;
            }
        }

        result = path.Tokenize("/", curPos);
    }

    return nullptr;
}

void QProceduralMaterialEditorMainWindow::EnableRequireSubstanceObjects(bool enabled)
{
    for (QObject* pObject : m_RequireSubstanceObjects)
    {
        if (QWidget* pWidget = qobject_cast<QWidget*>(pObject))
        {
            pWidget->setEnabled(enabled);
        }
        else if (QAction* pAction = qobject_cast<QAction*>(pObject))
        {
            pAction->setEnabled(enabled);
        }
    }
}
#endif // USE_SUBSTANCE
