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
/** @file GraphInputWidgets.h
	@brief Custom widgets to manipulate Procedural Material properties
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#ifndef SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_GRAPHINPUTWIDGETS_H
#define SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_GRAPHINPUTWIDGETS_H
#pragma once

#if defined(USE_SUBSTANCE)

#include "Substance/SubstanceBus.h"

class QProceduralMaterialEditorMainWindow;
class GIGraphInputHandler;

/**/
class CGraphUndoCommand : public QUndoCommand 
{
public:
	CGraphUndoCommand(GIGraphInputHandler* pInputHandler, QProceduralMaterialEditorMainWindow* pMainWindow, const GraphValueVariant& newValue);

	virtual void undo();
	virtual void redo();

private:
	IGraphInstance* CommitValue(const GraphValueVariant& value);

private:
	QProceduralMaterialEditorMainWindow*		m_MainWindow;
	GraphInstanceID								m_GraphInstanceID;
	GraphInputID								m_GraphInputID;

	GraphValueVariant							m_OldValue;
	GraphValueVariant							m_NewValue;

	std::string									m_OldValueStrBuf;
	std::string									m_NewValueStrBuf;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! GIGraphInputHandler handles value changes and undo changes for the various graph input widgets
class GIGraphInputHandler
{
public:
	GIGraphInputHandler(IGraphInput* pInput, QWidget* pWidget, QProceduralMaterialEditorMainWindow* pMainWindow);

	inline IGraphInput* GetGraphInput() const { return m_Input; }
	inline QWidget* GetWidget() const { return m_Widget; }

	void SetValue(const GraphValueVariant& value);

	virtual void SyncUndoValue() = 0;

private:
	IGraphInput*						m_Input;
	QWidget*							m_Widget;
	QProceduralMaterialEditorMainWindow*		m_MainWindow;
};

/**/
class GIAngle : public QWidget, public GIGraphInputHandler
{
	Q_OBJECT

public:
	GIAngle(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	virtual void SyncUndoValue() override;

private:
	void OnValueChanged(float value);

private:
	QDial*				m_Dial;
	QDoubleSpinBox*		m_SpinBox;

	float				m_MinValue;
	float				m_MaxValue;
};

/**/
class GICheckbox : public QCheckBox, public GIGraphInputHandler
{
	Q_OBJECT

public:
	GICheckbox(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	virtual void SyncUndoValue() override;

protected://slots
	void OnStateChanged(int index);
};

/**/
class GIColor : public QPushButton, public GIGraphInputHandler
{
	Q_OBJECT

public:
	GIColor(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	virtual void SyncUndoValue() override;

public://slots
	void OnClicked();

private:
	QColor GetColor();
	void SetColor(const QColor& color);

	void UpdateColor(const QColor& color);
};

/**/
class GIComboBox : public QComboBox, public GIGraphInputHandler
{
public:
	GIComboBox(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	virtual void SyncUndoValue() override;

public://slots
	void OnCurrentIndexChanged(int index);
};

/**/
class GIImageInput : public QWidget, public GIGraphInputHandler
{
	Q_OBJECT

public:
	GIImageInput(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	virtual void SyncUndoValue() override;

signals:
	void lastRunnableCompleted();

protected://slots
	void OnLastRunnableCompleted();
	void OnTextChanged();
	void OnToolButtonClicked();

private:
	class GIRunnable : public QRunnable
	{
	public:
		GIRunnable(GIImageInput* parent);

		virtual void run();
	private:
		GIImageInput*		m_Parent;
	};

private:
	QTextEdit*		m_TextEdit;
	QToolButton*	m_ToolButton;

	volatile int	m_RunnableCount;
};

/**/
class GIOutputSize : public QWidget, public GIGraphInputHandler
{
	Q_OBJECT
public:
	GIOutputSize(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	virtual void SyncUndoValue() override;

public://slots
	void OnLockButtonClicked();
	void OnComboBoxValueChanged(int valueIndex, int comboBoxIndex);

private:
	QPushButton*		m_LockButton;
	QComboBox*			m_ComboBoxes[2];

	bool				m_bLocked;
};

/**/
class GIRandomSeed : public QWidget, public GIGraphInputHandler
{
	Q_OBJECT

public:
	GIRandomSeed(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	virtual void SyncUndoValue() override;

public://slots
	void OnButtonClicked();
	QString FormatGraphValue() const;

private:
	QTextEdit*		m_TextEdit;
	QPushButton*	m_Button;
};

/**/
template<typename T, class QSPINBOXCLASS> class TGISliderBase : public QWidget
{
public:
	TGISliderBase(T value, T minValue, T maxValue);

	void SyncInputChanges(T value);

protected:
	virtual void EmitValueChanged(T value) = 0;

private:
	void OnSliderValueChanged(int value);
	void OnSpinBoxValueChanged(T value);
	T SliderValueToNativeValue(int sliderValue) const;
	int NativeValueToSliderValue(T nativeValue) const;
	int GetSliderMin() const;
	int GetSliderMax() const;

protected:
	QSlider*				m_Slider;
	QSPINBOXCLASS*			m_SpinBox;

	T						m_MinValue;
	T						m_MaxValue;
};

/**/
class GISliderIntBase : public TGISliderBase<int, QSpinBox>
{
	Q_OBJECT

public:
	typedef int valueChanged_type;

	GISliderIntBase(int value, int minValue, int maxValue);

signals:
	void valueChanged(int value);

protected:
	virtual void EmitValueChanged(int value);
};

/**/
class GISliderInt : public GISliderIntBase, public GIGraphInputHandler
{
	Q_OBJECT

public:
	GISliderInt(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	virtual void SyncUndoValue() override;

public://slots
	void OnValueChanged(int value);
};

/**/
class GISliderFloatBase : public TGISliderBase<double, QDoubleSpinBox>
{
	Q_OBJECT

public:
	typedef float valueChanged_type;

	GISliderFloatBase(float value, float minValue, float maxValue);

signals:
	void valueChanged(float value);

protected:
	virtual void EmitValueChanged(double value);
};

/**/
class GISliderFloat : public GISliderFloatBase, public GIGraphInputHandler
{
	Q_OBJECT

public:
	GISliderFloat(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	virtual void SyncUndoValue() override;

public://slots
	void OnValueChanged(float value);
};

/**/
template<typename T, int DIM, typename SLIDER> class TGIVectorBase : public QWidget, public GIGraphInputHandler
{
public:
	TGIVectorBase(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow);

	//GIGraphInputHandler
	virtual void SyncUndoValue() override;

protected:
	SLIDER* m_Sliders[DIM];
};

/**/
#define REGISTER_GIVECTOR_TYPE(name, t, dim, slider) \
	class name : public TGIVectorBase<t, dim, slider> \
	{ \
		Q_OBJECT \
	public: \
		name(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow); \
	};

REGISTER_GIVECTOR_TYPE(GIVectorInt2, Vec2i, 2, GISliderIntBase)
REGISTER_GIVECTOR_TYPE(GIVectorInt3, Vec3i, 3, GISliderIntBase)
REGISTER_GIVECTOR_TYPE(GIVectorInt4, Vec4i, 4, GISliderIntBase)

REGISTER_GIVECTOR_TYPE(GIVectorFloat2, Vec2, 2, GISliderFloatBase)
REGISTER_GIVECTOR_TYPE(GIVectorFloat3, Vec3, 3, GISliderFloatBase)
REGISTER_GIVECTOR_TYPE(GIVectorFloat4, Vec4, 4, GISliderFloatBase)

#endif // USE_SUBSTANCE
#endif //SUBSTANCE_PROCEDURALMATERIALEDITORPLUGIN_GRAPHINPUTWIDGETS_H
