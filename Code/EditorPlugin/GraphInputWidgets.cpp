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
/** @file GraphInputWidgets.cpp
	@brief Custom widgets to manipulate Procedural Material properties
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#include "StdAfx.h"

#if defined(USE_SUBSTANCE)

#include "GraphInputWidgets.h"
#include "QProceduralMaterialEditorMainWindow.h"
#include "Util.h"

#include <GraphInputWidgets.moc>

static const int CONTROL_HEIGHT = 23;

//these helpers assume the slider is on a 0-1000 scale
static int DoubleToSlider(double value, double minvalue, double maxvalue)
{
	double reScaledValue = (value - minvalue) / (maxvalue - minvalue);
	return (int)(reScaledValue * 1000.0);
}

static double SliderToDouble(int value, double minvalue, double maxvalue)
{
	double reScaledValue = (double)value / 1000.0;
	return (reScaledValue * (maxvalue - minvalue)) + minvalue;
}

//--------------------------------------------------------------------------------------------
CGraphUndoCommand::CGraphUndoCommand(
		GIGraphInputHandler* pInputHandler,
		QProceduralMaterialEditorMainWindow* pMainWindow, 
		const GraphValueVariant& newValue
	)
	: QUndoCommand()
{
	m_MainWindow = pMainWindow;

	IGraphInput* pInput = pInputHandler->GetGraphInput();
	m_OldValue = pInput->GetValue();
	m_NewValue = newValue;
	m_GraphInstanceID = pInput->GetGraphInstance()->GetGraphInstanceID();
	m_GraphInputID = pInput->GetGraphInputID();

	//string types need their values copied
	if (pInput->GetInputType() == GraphInputType::Image)
	{
		m_OldValueStrBuf = (const char*)m_OldValue;
		m_NewValueStrBuf = (const char*)m_NewValue;

		m_OldValue = m_OldValueStrBuf.c_str();
		m_NewValue = m_NewValueStrBuf.c_str();
	}
}

void CGraphUndoCommand::undo()
{
	if (IGraphInstance* pGraph = CommitValue(m_OldValue))
	{
		m_MainWindow->DecrementMaterialModified(pGraph->GetProceduralMaterial());
	}
}

void CGraphUndoCommand::redo()
{
	if (IGraphInstance* pGraph = CommitValue(m_NewValue))
	{
		m_MainWindow->IncrementMaterialModified(pGraph->GetProceduralMaterial());
	}
}

IGraphInstance* CGraphUndoCommand::CommitValue(const GraphValueVariant& value)
{
	if (GIGraphInputHandler* pInputHandler = m_MainWindow->GetGraphInputHandler(m_GraphInputID))
	{
		IGraphInstance* pGraph = pInputHandler->GetGraphInput()->GetGraphInstance();

		pInputHandler->GetGraphInput()->SetValue(value);
		m_MainWindow->QueueRender(pGraph);

		pInputHandler->SyncUndoValue();

		return pGraph;
	}
	// else
	// {
	// 	IGraphInstance* pGraph = nullptr;
	// 	EBUS_EVENT_RESULT(pGraph, SubstanceRequestBus, GetGraphInstance, m_GraphInstanceID);
	// 	if (pGraph)
	// 	{
	// 		AZ_TracePrintf("Default", "Number of inputs in GraphInputWidgets == %d",pGraph->GetInputCount());

	// 		for (int i = 0; i < pGraph->GetInputCount(); i++)
	// 		{
	// 			IGraphInput* pInput = pGraph->GetInput(i);
	// 			if (pInput->GetGraphInputID() == m_GraphInputID)
	// 			{
	// 				pInput->SetValue(value);
	// 				m_MainWindow->QueueRender(pGraph);
	// 				return pGraph;
	// 			}
	// 		}
	// 	}
	// }

	return nullptr;
}

//--------------------------------------------------------------------------------------------
GIGraphInputHandler::GIGraphInputHandler(IGraphInput* pInput, QWidget* pWidget, QProceduralMaterialEditorMainWindow* pMainWindow)
	: m_Input(pInput)
	, m_Widget(pWidget)
	, m_MainWindow(pMainWindow)
{
}

void GIGraphInputHandler::SetValue(const GraphValueVariant& value)
{
	//add undo command
	if (m_MainWindow)
	{
		m_MainWindow->GetUndoStack()->push(new CGraphUndoCommand(this, m_MainWindow, value));
	}
}

//--------------------------------------------------------------------------------------------
GIAngle::GIAngle(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: QWidget()
	, GIGraphInputHandler(pInput, this, pMainWindow)
{
	setLayout(new QHBoxLayout);

	m_MinValue = pInput->GetMinValue();
	m_MaxValue = pInput->GetMaxValue();

	if (m_MinValue == 0.0f && m_MinValue == m_MaxValue)
	{
		m_MaxValue = 1000.0f;
	}

	m_Dial = new QDial;
	m_Dial->setMinimum(0);
	m_Dial->setMaximum(1000);
	m_Dial->setSingleStep(1);
	m_Dial->setFixedWidth(40);
	m_Dial->setFixedHeight(40);
	m_Dial->setValue(DoubleToSlider(pInput->GetValue(), m_MinValue, m_MaxValue));
	connect(m_Dial, &QDial::valueChanged, [this](int value) {
		OnValueChanged(SliderToDouble(value, m_MinValue, m_MaxValue));
	});
	layout()->addWidget(m_Dial);

	m_SpinBox = new QDoubleSpinBox;
	m_SpinBox->setMinimum(m_MinValue);
	m_SpinBox->setMaximum(m_MaxValue);
	m_SpinBox->setSingleStep(0.1);
	m_SpinBox->setValue(pInput->GetValue());
	connect(m_SpinBox,  static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](double value) {
		OnValueChanged(value);
	});
	layout()->addWidget(m_SpinBox);	
}

void GIAngle::OnValueChanged(float value)
{
	SetValue(value);
}

void GIAngle::SyncUndoValue()
{
	m_Dial->blockSignals(true);
	m_Dial->setValue(DoubleToSlider(GetGraphInput()->GetValue(), m_MinValue, m_MaxValue));
	m_Dial->blockSignals(false);

	m_SpinBox->blockSignals(true);
	m_SpinBox->setValue(GetGraphInput()->GetValue());
	m_SpinBox->blockSignals(false);
}

//--------------------------------------------------------------------------------------------
GICheckbox::GICheckbox(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: QCheckBox()
	, GIGraphInputHandler(pInput, this, pMainWindow)
{
	setCheckState((int)pInput->GetValue() ? Qt::Checked : Qt::Unchecked);

	connect(this, &QCheckBox::stateChanged, this, &GICheckbox::OnStateChanged);
}

void GICheckbox::OnStateChanged(int checkState)
{
	SetValue((Qt::Checked == checkState) ? 1 : 0);
}

void GICheckbox::SyncUndoValue()
{
	blockSignals(true);
	setChecked((int)GetGraphInput()->GetValue() ? true : false);
	blockSignals(false);
}


//--------------------------------------------------------------------------------------------
GIColor::GIColor(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: QPushButton()
	, GIGraphInputHandler(pInput, this, pMainWindow)
{
	setAutoFillBackground(true);
	setFlat(true);
	UpdateColor(GetColor());

	connect(this, &QAbstractButton::clicked, this, &GIColor::OnClicked);
}

void GIColor::OnClicked()
{
	QColorDialog* qcd = new QColorDialog(GetColor(), this);
	
	if (GetGraphInput()->GetInputType() == GraphInputType::Float4)
	{
		qcd->setOption(QColorDialog::ShowAlphaChannel);
	}

	connect(qcd, &QColorDialog::colorSelected, [this](const QColor& color) {
		SetColor(color);
		UpdateColor(color);
	});

	//display dialog
	qcd->exec();
}

QColor GIColor::GetColor()
{
	Vec4 rgba;

	if (GetGraphInput()->GetInputType() == GraphInputType::Float3)
	{
		Vec3 rgb = GetGraphInput()->GetValue();
		rgba.x = rgb.x;
		rgba.y = rgb.y;
		rgba.z = rgb.z;
		rgba.w = 1.0f;
	}
	else
	{
		rgba = GetGraphInput()->GetValue();
	}

	int r = rgba.x * 255.0f;
	int g = rgba.y * 255.0f;
	int b = rgba.z * 255.0f;
	int a = rgba.w * 255.0f;

	return QColor(r, g, b, a);
}

void GIColor::SetColor(const QColor& color)
{
	float r = (float)color.red() / 255.0f;
	float g = (float)color.green() / 255.0f;
	float b = (float)color.blue() / 255.0f;
	float a = (float)color.alpha() / 255.0f;
	Vec4 rgba(r, g, b, a);

	SetValue(rgba);
}

void GIColor::UpdateColor(const QColor& color)
{
	QPalette pal = palette();
	pal.setColor(QPalette::Button, color);
	setPalette(pal);
}


void GIColor::SyncUndoValue()
{
	UpdateColor(GetColor());
}

//--------------------------------------------------------------------------------------------
GIComboBox::GIComboBox(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: QComboBox()
	, GIGraphInputHandler(pInput, this, pMainWindow)
{
	GraphValueVariant currentValue = GetGraphInput()->GetValue();
	for (int i = 0; i < GetGraphInput()->GetEnumCount(); i++)
	{
		GraphEnumValue enumValue = GetGraphInput()->GetEnumValue(i);
		addItem(enumValue.label);

		if (enumValue.value == currentValue)
			setCurrentIndex(i);
	}

	connect(this, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &GIComboBox::OnCurrentIndexChanged);
}

void GIComboBox::OnCurrentIndexChanged(int index)
{
	SetValue(GetGraphInput()->GetEnumValue(index).value);
}

void GIComboBox::SyncUndoValue()
{
	blockSignals(true);

	GraphValueVariant currentValue = GetGraphInput()->GetValue();
	for (int i = 0; i < GetGraphInput()->GetEnumCount(); i++)
	{
		GraphEnumValue enumValue = GetGraphInput()->GetEnumValue(i);

		if (enumValue.value == currentValue)
		{
			setCurrentIndex(i);
			break;
		}
	}

	blockSignals(false);
}


//--------------------------------------------------------------------------------------------
GIImageInput::GIImageInput(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: QWidget()
	, GIGraphInputHandler(pInput, this, pMainWindow)
	, m_RunnableCount(0)
{
	setLayout(new QHBoxLayout);

	m_TextEdit = new QTextEdit;
	m_TextEdit->setText((const char*)pInput->GetValue());
	m_TextEdit->setFixedHeight(CONTROL_HEIGHT);
	connect(m_TextEdit, &QTextEdit::textChanged, this, &GIImageInput::OnTextChanged);

	m_ToolButton = new QToolButton;
	m_ToolButton->setText(tr("..."));
	m_ToolButton->setFixedHeight(CONTROL_HEIGHT);
	connect(m_ToolButton, &QToolButton::clicked, this, &GIImageInput::OnToolButtonClicked);

	layout()->addWidget(m_TextEdit);
	layout()->addWidget(m_ToolButton);

	connect(this, &GIImageInput::lastRunnableCompleted, this, &GIImageInput::OnLastRunnableCompleted);
}

void GIImageInput::OnTextChanged()
{
	//we issue a thread sleep when the text changes, so that 
	//way we only apply those changes when the user stops typing.
	CryInterlockedIncrement(&m_RunnableCount);
	QThreadPool::globalInstance()->start(new GIRunnable(this));
}

void GIImageInput::SyncUndoValue()
{
	m_TextEdit->blockSignals(true);
	m_TextEdit->setText((const char*)GetGraphInput()->GetValue());
	m_TextEdit->blockSignals(false);
}

void GIImageInput::OnToolButtonClicked()
{
	AZStd::string gameFolder = Path::GetEditingGameDataFolder();

	QString dir(gameFolder.c_str());
	QString filter(tr("Images (*.dds);;All Files (*.*)"));
	QString filename = QFileDialog::getOpenFileName(this, tr("Select Image"), dir, filter);
	if (!filename.isEmpty())
	{
		m_TextEdit->setText(Util::AbsolutePathToGamePath(filename));
	}
}

void GIImageInput::OnLastRunnableCompleted()
{
	QByteArray qba = m_TextEdit->toPlainText().toLatin1();
	SetValue(qba.data());
}

GIImageInput::GIRunnable::GIRunnable(GIImageInput* parent)
	: QRunnable()
	, m_Parent(parent)
{
}

void GIImageInput::GIRunnable::run()
{
	QThread::msleep(500);
	if (CryInterlockedDecrement(&m_Parent->m_RunnableCount) == 0)
	{
		//this will be called on main thread
		Q_EMIT m_Parent->lastRunnableCompleted();
	}
}

//--------------------------------------------------------------------------------------------
GIRandomSeed::GIRandomSeed(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: QWidget()
	, GIGraphInputHandler(pInput, this, pMainWindow)
{
	setLayout(new QHBoxLayout);

	m_TextEdit = new QTextEdit;
	m_TextEdit->setText(FormatGraphValue());
	m_TextEdit->setEnabled(false);
	m_TextEdit->setFixedHeight(CONTROL_HEIGHT);
	layout()->addWidget(m_TextEdit);

	m_Button = new QPushButton;
	m_Button->setText(tr("Randomize"));
	m_Button->setFixedWidth(100);
	m_Button->setFixedHeight(CONTROL_HEIGHT);
	connect(m_Button, &QPushButton::clicked, this, &GIRandomSeed::OnButtonClicked);
	layout()->addWidget(m_Button);
}

void GIRandomSeed::OnButtonClicked()
{
	SetValue(qrand());
}

void GIRandomSeed::SyncUndoValue()
{
	m_TextEdit->setText(FormatGraphValue());
}

QString GIRandomSeed::FormatGraphValue() const
{
	return QString("0x").append(QString::number((int)GetGraphInput()->GetValue(), 16).toUpper());
}

//--------------------------------------------------------------------------------------------
GIOutputSize::GIOutputSize(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: QWidget()
	, GIGraphInputHandler(pInput, this, pMainWindow)
	, m_bLocked(true)
{
	setLayout(new QHBoxLayout);

	m_LockButton = new QPushButton;
	m_LockButton->setIcon(QIcon("://Icons/Locked_Icon.png"));
	m_LockButton->setFixedWidth(CONTROL_HEIGHT);
	connect(m_LockButton, &QPushButton::clicked, this, &GIOutputSize::OnLockButtonClicked);
	layout()->addWidget(m_LockButton);

	Vec2i value = pInput->GetValue();

	uint32 minOutputSize = 0;
	EBUS_EVENT_RESULT(minOutputSize, SubstanceRequestBus, GetMinimumOutputSize);

	uint32 maxOutputSize = 0;
	EBUS_EVENT_RESULT(maxOutputSize, SubstanceRequestBus, GetMaximumOutputSize);

	for (int i = 0;i < 2;i++)
	{
		QComboBox* comboBox = new QComboBox;

		//populate combobox
		for (uint32 s = minOutputSize;s <= maxOutputSize;s *= 2)
		{
			uint32 l2v = IntegerLog2(s);
			comboBox->addItem(QString::number(s), l2v);

			if (l2v == value[i])
				comboBox->setCurrentIndex(comboBox->count()-1);
		}

		connect(comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, i](int index) {
			OnComboBoxValueChanged(i, index);
		});
		
		m_ComboBoxes[i] = comboBox;
		layout()->addWidget(m_ComboBoxes[i]);
	}
}

void GIOutputSize::OnLockButtonClicked()
{
	m_bLocked = !m_bLocked;

	if (m_bLocked)
		m_LockButton->setIcon(QIcon("://Icons/Locked_Icon.png"));
	else 
		m_LockButton->setIcon(QIcon("://Icons/Unlocked_Icon.png"));
}

void GIOutputSize::SyncUndoValue()
{
	Vec2i value = GetGraphInput()->GetValue();

	for (int i = 0;i < 2;i++)
	{
		QComboBox* comboBox = m_ComboBoxes[i];

		for (int c = 0;c < comboBox->count();c++)
		{
			if (comboBox->itemData(c) == value[i])
			{
				comboBox->blockSignals(true);
				comboBox->setCurrentIndex(c);
				comboBox->blockSignals(false);
				break;
			}
		}
	}
}

void GIOutputSize::OnComboBoxValueChanged(int inputValueIndex, int comboBoxIndex)
{
	Vec2i value = GetGraphInput()->GetValue();
	int elemValue = m_ComboBoxes[inputValueIndex]->itemData(comboBoxIndex).toInt();

	value[inputValueIndex] = elemValue;

	if (m_bLocked)
	{
		value[!inputValueIndex] = elemValue;
	}

	SetValue(value);
}

//--------------------------------------------------------------------------------------------
template<typename T, class QSPINBOXCLASS> TGISliderBase<T,QSPINBOXCLASS>::TGISliderBase(T value, T minValue, T maxValue)
	: QWidget()
	, m_MinValue(minValue)
	, m_MaxValue(maxValue)
	, m_Slider(nullptr)
{
	setLayout(new QHBoxLayout);

	bool bHasMinMax = (minValue != maxValue) ? true : false;
	QLabel* minLabel;
	QLabel* maxLabel;

	m_SpinBox = new QSPINBOXCLASS;
	m_SpinBox->setValue(value);
	connect(m_SpinBox, static_cast<void (QSPINBOXCLASS::*)(T)>(&QSPINBOXCLASS::valueChanged), this, &TGISliderBase<T,QSPINBOXCLASS>::OnSpinBoxValueChanged);

	if (bHasMinMax)
	{
		minLabel = new QLabel();
		minLabel->setFixedWidth(25);
		minLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
		minLabel->setText(QString().setNum(minValue));
		layout()->addWidget(minLabel);

		m_Slider = new QSlider(Qt::Horizontal);
		m_Slider->setMinimum(GetSliderMin());
		m_Slider->setMaximum(GetSliderMax());
		m_Slider->setValue(NativeValueToSliderValue(value));
		connect(m_Slider, &QSlider::valueChanged, this, &TGISliderBase<T, QSPINBOXCLASS>::OnSliderValueChanged);
		layout()->addWidget(m_Slider);

		maxLabel = new QLabel();
		maxLabel->setFixedWidth(25);
		maxLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
		maxLabel->setText(QString().setNum(maxValue));
		layout()->addWidget(maxLabel);

		m_SpinBox->setMinimum(minValue);
		m_SpinBox->setMaximum(maxValue);
		m_SpinBox->setFixedWidth(50);
	}

	layout()->addWidget(m_SpinBox);
}

template<typename T, class QSPINBOX> void TGISliderBase<T,QSPINBOX>::SyncInputChanges(T value)
{
	if (m_Slider)
	{
		m_Slider->blockSignals(true);
		m_Slider->setValue(NativeValueToSliderValue(value));
		m_Slider->blockSignals(false);
	}

	m_SpinBox->blockSignals(true);
	m_SpinBox->setValue(value);
	m_SpinBox->blockSignals(false);
}

template<typename T, class QSPINBOX> void TGISliderBase<T,QSPINBOX>::OnSliderValueChanged(int value)
{
	T nvalue = SliderValueToNativeValue(value);
	SyncInputChanges(nvalue);
	EmitValueChanged(nvalue);
}

template<typename T, class QSPINBOX> void TGISliderBase<T,QSPINBOX>::OnSpinBoxValueChanged(T value)
{
	SyncInputChanges(value);
	EmitValueChanged(value);
}

/* Specializations for Slider Value Conversion */
template<> int TGISliderBase<int,QSpinBox>::NativeValueToSliderValue(int nativeValue) const
	{ return nativeValue; }
template<> int TGISliderBase<int,QSpinBox>::SliderValueToNativeValue(int sliderValue) const
	{ return sliderValue; }
template<> int TGISliderBase<int, QSpinBox>::GetSliderMin() const
	{ return m_MinValue; }
template<> int TGISliderBase<int, QSpinBox>::GetSliderMax() const
	{ return m_MaxValue; }
template<> int TGISliderBase<double, QDoubleSpinBox>::NativeValueToSliderValue(double nativeValue) const
{
	return DoubleToSlider(nativeValue, m_MinValue, m_MaxValue);
}
template<> double TGISliderBase<double, QDoubleSpinBox>::SliderValueToNativeValue(int sliderValue) const
{
	return SliderToDouble(sliderValue, m_MinValue, m_MaxValue);
}
template<> int TGISliderBase<double, QDoubleSpinBox>::GetSliderMin() const
	{ return 0; }
template<> int TGISliderBase<double, QDoubleSpinBox>::GetSliderMax() const
	{ return 1000; }

//--------------------------------------------------------------------------------------------
GISliderIntBase::GISliderIntBase(int value, int minValue, int maxValue)
	: TGISliderBase<int, QSpinBox>(value, minValue, maxValue)
{
}

void GISliderIntBase::EmitValueChanged(int value)
{
	Q_EMIT valueChanged(value);
}

//--------------------------------------------------------------------------------------------
GISliderInt::GISliderInt(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: GISliderIntBase(pInput->GetValue(), pInput->GetMinValue(), pInput->GetMaxValue())
	, GIGraphInputHandler(pInput, this, pMainWindow)
{
	connect(this, &GISliderIntBase::valueChanged, this, &GISliderInt::OnValueChanged);
}

void GISliderInt::OnValueChanged(int value)
{
	SetValue(value);
}

void GISliderInt::SyncUndoValue()
{
	SyncInputChanges(GetGraphInput()->GetValue());
}

//--------------------------------------------------------------------------------------------
GISliderFloatBase::GISliderFloatBase(float value, float minValue, float maxValue)
	: TGISliderBase<double, QDoubleSpinBox>(value, minValue, maxValue)
{
	m_SpinBox->setSingleStep(0.1);
}

void GISliderFloatBase::EmitValueChanged(double value)
{
	Q_EMIT valueChanged((float)value);
}

//--------------------------------------------------------------------------------------------
GISliderFloat::GISliderFloat(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: GISliderFloatBase(pInput->GetValue(), pInput->GetMinValue(), pInput->GetMaxValue())
	, GIGraphInputHandler(pInput, this, pMainWindow)
{
	connect(this, &GISliderFloatBase::valueChanged, this, &GISliderFloat::OnValueChanged);
}

void GISliderFloat::OnValueChanged(float value)
{
	SetValue(value);
}

void GISliderFloat::SyncUndoValue()
{
	SyncInputChanges(GetGraphInput()->GetValue());
}

//--------------------------------------------------------------------------------------------
template<typename T, int DIM, typename SLIDER> TGIVectorBase<T,DIM,SLIDER>::TGIVectorBase(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow)
	: QWidget()
	, GIGraphInputHandler(pInput, this, pMainWindow)
{
	T value = pInput->GetValue();
	T minValue = pInput->GetMinValue();
	T maxValue = pInput->GetMaxValue();

	//If we're going to display a 4D element with a slider, go to vertical layout
	if (DIM > 3 && minValue != maxValue)
		setLayout(new QVBoxLayout);
	else 
		setLayout(new QHBoxLayout);

	for (int i = 0;i < DIM;i++)
	{
		SLIDER* slider = new SLIDER(value[i], minValue[i], maxValue[i]);
		layout()->addWidget(slider);

		connect(slider, &SLIDER::valueChanged, [this, i](SLIDER::valueChanged_type value) {
			T theValue = GetGraphInput()->GetValue();
			theValue[i] = value;
			SetValue(theValue);
		});

		m_Sliders[i] = slider;
	}
}

template<typename T, int DIM, typename SLIDER> void TGIVectorBase<T,DIM,SLIDER>::SyncUndoValue()
{
	T value = GetGraphInput()->GetValue();
	for (int i = 0;i < DIM;i++)
	{
		m_Sliders[i]->SyncInputChanges(value[i]);
	}
}

//--------------------------------------------------------------------------------------------
#define IMPLEMENT_GIVECTOR_TYPE(name, type, dim, slider) \
	name::name(IGraphInput* pInput, QProceduralMaterialEditorMainWindow* pMainWindow) \
		: TGIVectorBase<type,dim,slider>(pInput, pMainWindow) \
	{ \
	}

IMPLEMENT_GIVECTOR_TYPE(GIVectorInt2, Vec2i, 2, GISliderIntBase)
IMPLEMENT_GIVECTOR_TYPE(GIVectorInt3, Vec3i, 3, GISliderIntBase)
IMPLEMENT_GIVECTOR_TYPE(GIVectorInt4, Vec4i, 4, GISliderIntBase)

IMPLEMENT_GIVECTOR_TYPE(GIVectorFloat2, Vec2, 2, GISliderFloatBase)
IMPLEMENT_GIVECTOR_TYPE(GIVectorFloat3, Vec3, 3, GISliderFloatBase)
IMPLEMENT_GIVECTOR_TYPE(GIVectorFloat4, Vec4, 4, GISliderFloatBase)

#endif // USE_SUBSTANCE
