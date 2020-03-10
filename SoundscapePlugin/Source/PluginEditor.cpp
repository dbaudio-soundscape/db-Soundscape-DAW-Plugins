/*
===============================================================================

Copyright (C) 2019 d&b audiotechnik GmbH & Co. KG. All Rights Reserved.

This file is part of the Soundscape VST, AU, and AAX Plug-in.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY d&b audiotechnik GmbH & Co. KG "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===============================================================================
*/


#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Overview.h"
#include "Parameters.h"


namespace dbaudio
{


/**
 * Rate at which the GUI will refresh, after parameter changes have been detected.
 * 33 ms translates to about 30 frames per second.
 */
static constexpr int GUI_UPDATE_RATE_FAST = 33;

/**
 * Rate at which the GUI will refresh, when no parameter changes have taken place for a while.
 */
static constexpr int GUI_UPDATE_RATE_SLOW = 120;

/**
 * After this number of timer callbacks without parameter changes, the timer will switch to GUI_UPDATE_RATE_SLOW.
 */
static constexpr int GUI_UPDATE_DELAY_TICKS = 15;

/*
 * Default Plug-In window size.
 */
static constexpr Point<int> GUI_DEFAULT_PLUGIN_WINDOW_SIZE(488, 380);

/*
 * Initialize user's Plug-In window size with default values.
 */
Point<int> CPluginEditor::m_pluginWindowSize = GUI_DEFAULT_PLUGIN_WINDOW_SIZE;

/*
===============================================================================
 Class CPluginEditor
===============================================================================
*/

/**
 * Object constructor.
 * This is the base class for the component that acts as the GUI for an AudioProcessor.
 * @param parent	The audio processor object to act as parent.
 */
CPluginEditor::CPluginEditor(CPlugin& parent)
	: AudioProcessorEditor(&parent)
{
	m_surfaceSlider = std::make_unique<CSurfaceSlider>(&parent);
	m_surfaceSlider->setWantsKeyboardFocus(true);
	addAndMakeVisible(m_surfaceSlider.get());

	const Array<AudioProcessorParameter*>& params = parent.getParameters();
	if (params.size() >= 2)
	{
		//--- X Slider ---//
		AudioParameterFloat* param = dynamic_cast<AudioParameterFloat*> (params[ParamIdx_X]);
		m_xSlider = std::make_unique<CSlider>(param->name);
		m_xSlider->setRange(param->range.start, param->range.end, param->range.interval);
		m_xSlider->setSliderStyle(Slider::LinearHorizontal);
		m_xSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 80, 20);
		m_xSlider->addListener(this);
		addAndMakeVisible(m_xSlider.get());

		// Label for X slider
		m_xAxisLabel = std::make_unique<CLabel>(param->name, param->name);
		addAndMakeVisible(m_xAxisLabel.get());

		//--- Y Slider ---//
		param = dynamic_cast<AudioParameterFloat*> (params[ParamIdx_Y]);
		m_ySlider = std::make_unique<CSlider>(param->name);
		m_ySlider->setRange(param->range.start, param->range.end, param->range.interval);
		m_ySlider->setSliderStyle(Slider::LinearVertical);
		m_ySlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
		m_ySlider->addListener(this);
		addAndMakeVisible(m_ySlider.get());

		// Label for Y slider
		m_yAxisLabel = std::make_unique<CLabel>(param->name, param->name);
		addAndMakeVisible(m_yAxisLabel.get());

		if (params.size() == ParamIdx_MaxIndex)
		{
			//--- ReverbSendGain Slider ---//
			param = dynamic_cast<AudioParameterFloat*> (params[ParamIdx_ReverbSendGain]);
			m_reverbSendGainSlider = std::make_unique<CKnob>(param->name);
			m_reverbSendGainSlider->setRange(param->range.start, param->range.end, param->range.interval);
			m_reverbSendGainSlider->setSliderStyle(Slider::Rotary);
			m_reverbSendGainSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 80, 20);
			m_reverbSendGainSlider->addListener(this);
			addAndMakeVisible(m_reverbSendGainSlider.get());

			// Label for ReverbSendGain
			m_reverbSendGainLabel = std::make_unique<CLabel>(param->name, param->name);
			addAndMakeVisible(m_reverbSendGainLabel.get());

			//--- SourceSpread Slider ---//
			param = dynamic_cast<AudioParameterFloat*> (params[ParamIdx_SourceSpread]);
			m_sourceSpreadSlider = std::make_unique<CKnob>(param->name);
			m_sourceSpreadSlider->setRange(param->range.start, param->range.end, param->range.interval);
			m_sourceSpreadSlider->setSliderStyle(Slider::Rotary);
			m_sourceSpreadSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 80, 20);
			m_sourceSpreadSlider->addListener(this);
			addAndMakeVisible(m_sourceSpreadSlider.get());

			// Label for SourceSpread
			m_sourceSpreadLabel = std::make_unique<CLabel>(param->name, param->name);
			addAndMakeVisible(m_sourceSpreadLabel.get());

			//--- DelayMode ComboBox ---//
			AudioParameterChoice* choiceParam = dynamic_cast<AudioParameterChoice*> (params[ParamIdx_DelayMode]);
			m_delayModeComboBox = std::make_unique<ComboBox>(choiceParam->name);
			m_delayModeComboBox->setEditableText(false);
			m_delayModeComboBox->addItem("Off",	1);
			m_delayModeComboBox->addItem("Tight", 2);
			m_delayModeComboBox->addItem("Full", 3);
			m_delayModeComboBox->setColour(ComboBox::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::DarkColor));
			m_delayModeComboBox->setColour(ComboBox::textColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
			m_delayModeComboBox->setColour(ComboBox::outlineColourId, CDbStyle::GetDbColor(CDbStyle::WindowColor));
			m_delayModeComboBox->setColour(ComboBox::buttonColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
			m_delayModeComboBox->setColour(ComboBox::arrowColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
			m_delayModeComboBox->addListener(this);
			addAndMakeVisible(m_delayModeComboBox.get());

			// Label for DelayMode
			m_delayModeLabel = std::make_unique<CLabel>(choiceParam->name, choiceParam->name);
			addAndMakeVisible(m_delayModeLabel.get());
		}
	}

	m_areaSelector = std::make_unique<ComboBox>("Coordinate mapping");
	m_areaSelector->setEditableText(false);
	m_areaSelector->addItem("1", 1);
	m_areaSelector->addItem("2", 2);
	m_areaSelector->addItem("3", 3);
	m_areaSelector->addItem("4", 4);
	m_areaSelector->addListener(this);
	m_areaSelector->setColour(ComboBox::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::DarkColor));
	m_areaSelector->setColour(ComboBox::textColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	m_areaSelector->setColour(ComboBox::outlineColourId, CDbStyle::GetDbColor(CDbStyle::WindowColor));
	m_areaSelector->setColour(ComboBox::buttonColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
	m_areaSelector->setColour(ComboBox::arrowColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	addAndMakeVisible(m_areaSelector.get());

	m_posAreaLabel = std::make_unique<CLabel>("Coordinate mapping label", "Mapping:");
	addAndMakeVisible(m_posAreaLabel.get());

	m_sourceIdDigital = std::make_unique<CDigital>("Source Id");
	m_sourceIdDigital->SetRange(1, 64);
	m_sourceIdDigital->AddListeners(this, this);
	addAndMakeVisible(m_sourceIdDigital.get());

	m_sourceIdLabel = std::make_unique<CLabel>("Source Id Label", "Input:");
	addAndMakeVisible(m_sourceIdLabel.get());

	m_ipAddressTextEdit = std::make_unique<CTextEditor>("IP Address");
	m_ipAddressTextEdit->addListener(this);
	addAndMakeVisible(m_ipAddressTextEdit.get());
	m_ipAddressLabel = std::make_unique<CLabel>("IP Address Label", "IP Address:");
	addAndMakeVisible(m_ipAddressLabel.get());

	m_onlineLed = std::make_unique<CButton>("");
	m_onlineLed->setEnabled(false);
	m_onlineLed->SetCornerRadius(10);
	addAndMakeVisible(m_onlineLed.get());

	m_rateTextEdit = std::make_unique<CTextEditor>("OSC Send Rate");
	m_rateTextEdit->SetSuffix("ms");
	m_rateTextEdit->addListener(this);
	addAndMakeVisible(m_rateTextEdit.get());
	m_rateLabel = std::make_unique<CLabel>("OSC Send Rate", "Interval:");
	addAndMakeVisible(m_rateLabel.get());

	// d&b logo and Plugin version label
	m_dbLogo = ImageCache::getFromMemory(BinaryData::logo_dbaudio_15x15_png, BinaryData::logo_dbaudio_15x15_pngSize);
	String versionString(JUCE_STRINGIFY(JUCE_APP_VERSION));

	m_versionLabel = std::make_unique<CLabel>("PluginVersion", versionString);
	m_versionLabel->setFont(Font(11.5, Font::plain));
	addAndMakeVisible(m_versionLabel.get());
	m_nameLabel = std::make_unique<CLabel>("PluginName", "Soundscape");
	m_nameLabel->setFont(Font(11.5, Font::plain));
	m_nameLabel->setColour(Label::textColourId, CDbStyle::GetDbColor(CDbStyle::DarkTextColor));
	addAndMakeVisible(m_nameLabel.get());

	// OSC communication mode
	m_oscModeSend = std::make_unique<CButton>("Tx");
	m_oscModeSend->setEnabled(true);
	m_oscModeSend->addListener(this);
	addAndMakeVisible(m_oscModeSend.get());
	m_oscModeReceive = std::make_unique<CButton>("Rx");
	m_oscModeReceive->setEnabled(true);
	m_oscModeReceive->addListener(this);
	addAndMakeVisible(m_oscModeReceive.get());

	// Overview button
	Image burgerImg = ImageCache::getFromMemory(BinaryData::icon_hamburger_16x16_png, BinaryData::icon_hamburger_16x16_pngSize);
	m_overviewButton = std::make_unique<CImageButton>(burgerImg);
	m_overviewButton->setEnabled(true);
	m_overviewButton->addListener(this);
	addAndMakeVisible(m_overviewButton.get());

	// No overlay (Overview or About) to start with.
	m_overlay = nullptr;

	// Label for Plugin' display name.
	m_displayNameLabel = std::make_unique<CLabel>("DisplayName");
	m_displayNameLabel->setJustificationType(Justification(Justification::centredLeft));
	m_displayNameLabel->setColour(Label::textColourId, CDbStyle::GetDbColor(CDbStyle::DarkTextColor));
	addAndMakeVisible(m_displayNameLabel.get());

	// About button
	Image aboutImg = ImageCache::getFromMemory(BinaryData::icon_help_16x16_png, BinaryData::icon_help_16x16_pngSize);
	m_aboutButton = std::make_unique<CDiscreteButton>(aboutImg);
	m_aboutButton->setEnabled(true);
	m_aboutButton->addListener(this);
	addAndMakeVisible(m_aboutButton.get());

#ifdef JUCE_DEBUG
	// Special testfield for displaying debugging messages.
	m_debugTextEdit = std::make_unique<TextEditor>("Debug");
	m_debugTextEdit->setMultiLine(true, true);
	m_debugTextEdit->setReadOnly(true);
	m_debugTextEdit->setScrollbarsShown(true);
	addAndMakeVisible(m_debugTextEdit.get());
#endif

	if (parent.IsTargetHostAvidConsole())
	{
		// Overview Multi-slider button, only needed on consoles where no extra Overview window is used.
		Path iconPath;
		CTabbedComponent::GetIconPath(CTabbedComponent::OTI_MultiSlider, Point<float>(14, 10), 2.0f, iconPath);
		m_overviewMultiSliderButton = std::make_unique<CPathButton>(iconPath);
		m_overviewMultiSliderButton->setEnabled(true);
		m_overviewMultiSliderButton->addListener(this);
		addAndMakeVisible(m_overviewMultiSliderButton.get());

		// Larger GUI for consoles.
		if (m_pluginWindowSize == GUI_DEFAULT_PLUGIN_WINDOW_SIZE)
		{
			m_pluginWindowSize = Point<int>(684, 544);
		}
	}

	// Backup user's window size, since setResizeLimits() calls resized(), which overwrites it.
	Point<int> tmpSize(m_pluginWindowSize);
	setResizeLimits(488, 380, 1920, 1080);
	m_pluginWindowSize = tmpSize;

	// Resize the new plugin's window to the same as the last.
	setSize(m_pluginWindowSize.getX(), m_pluginWindowSize.getY());

	// Allow resizing of plugin window.
	setResizable(true, true);

	// Start GUI-refreshing timer.
	startTimer(GUI_UPDATE_RATE_FAST);
}

/**
 * Object destructor.
 */
CPluginEditor::~CPluginEditor()
{
	stopTimer();
}

/**
 * Helper function to get the pointer to a plugin parameter based on the slider assigned to it.
 * @param slider	The slider object for which the parameter is desired.
 * @return			The desired plugin parameter.
 */
CAudioParameterFloat* CPluginEditor::GetParameterForSlider(CSlider* slider)
{
	const Array<AudioProcessorParameter*>& params = getAudioProcessor()->getParameters();
	if (slider == m_xSlider.get())
		return dynamic_cast<CAudioParameterFloat*> (params[ParamIdx_X]);
	else if (slider == m_ySlider.get())
		return dynamic_cast<CAudioParameterFloat*> (params[ParamIdx_Y]);
	else if (slider == m_reverbSendGainSlider.get())
		return dynamic_cast<CAudioParameterFloat*> (params[ParamIdx_ReverbSendGain]);
	else if (slider == m_sourceSpreadSlider.get())
		return dynamic_cast<CAudioParameterFloat*> (params[ParamIdx_SourceSpread]);

	// Should not make it this far.
	jassertfalse;
	return nullptr;
}

/**
 * Callback function for changes to our sliders. Called when the slider's value is changed.
 * This may be caused by dragging it, or by typing in its text entry box, or by a call to Slider::setValue().
 * You can find out the new value using Slider::getValue().
 * @param slider	Slider object which was dragged by user.
 */
void CPluginEditor::sliderValueChanged(Slider* slider)
{
	CPlugin* plugin = dynamic_cast<CPlugin*>(getAudioProcessor());
	if (plugin)
	{
		AutomationParameterIndex paramIdx = ParamIdx_MaxIndex;
		if (slider == m_xSlider.get())
			paramIdx = ParamIdx_X;
		else if (slider == m_ySlider.get())
			paramIdx = ParamIdx_Y;
		else if (slider == m_reverbSendGainSlider.get())
			paramIdx = ParamIdx_ReverbSendGain;
		else if (slider == m_sourceSpreadSlider.get())
			paramIdx = ParamIdx_SourceSpread;

		plugin->SetParameterValue(DCS_Gui, paramIdx, static_cast<float>(slider->getValue()));
	}
}

/**
 * Called when the slider is about to be dragged.
 * This is called when a drag begins, then it's followed by multiple calls to sliderValueChanged(),
 * and then sliderDragEnded() is called after the user lets go.
 * @param slider	Slider object which was dragged by user.
 */
void CPluginEditor::sliderDragStarted(Slider* slider)
{
	if (CAudioParameterFloat* param = GetParameterForSlider(static_cast<CSlider*>(slider)))
		param->BeginGuiGesture();
}

/**
 * Called after a drag operation has finished.
 * @param slider	Slider object which was dragged by user.
 */
void CPluginEditor::sliderDragEnded(Slider* slider)
{
	if (CAudioParameterFloat* param = GetParameterForSlider(static_cast<CSlider*>(slider)))
		param->EndGuiGesture();
}

/**
 * Callback function for changes to our textEditors.
 * @param textEditor	The TextEditor object whose content has just changed.
 */
void CPluginEditor::textEditorFocusLost(TextEditor& textEditor)
{
	CTextEditor *myEditor = static_cast<CTextEditor*>(&textEditor);
	CPlugin* pro = dynamic_cast<CPlugin*>(getAudioProcessor());
	if (pro && myEditor)
	{
		// SourceId changed
		if (m_sourceIdDigital.get() == dynamic_cast<CDigital*>(myEditor->getParentComponent()))
		{
			pro->SetSourceId(DCS_Gui, myEditor->getText().getIntValue());
		}

		// IP Address changed
		else if (myEditor == m_ipAddressTextEdit.get())
		{
			// IP address validation: If IPAddress::toString() returns the same string
			// which was entered, it is a valid IP address.
			IPAddress ip(myEditor->getText());
			if (ip.toString() == myEditor->getText())
				pro->SetIpAddress(DCS_Gui, myEditor->getText());
			else
				myEditor->setText(pro->GetIpAddress(), false);
		}

		// OSC message rate has changed
		else if (myEditor == m_rateTextEdit.get())
		{
			pro->SetMessageRate(DCS_Gui, myEditor->getText().getIntValue());
		}
	}
}

/**
 * Callback function for Enter key presses on textEditors.
 * @param textEditor	The TextEditor object whose where enter key was pressed.
 */
void CPluginEditor::textEditorReturnKeyPressed(TextEditor& textEditor)
{
	ignoreUnused(textEditor);

	// Remove keyboard focus from this editor. 
	// Function textEditorFocusLost will then take care of setting values.
	m_surfaceSlider->grabKeyboardFocus();
}

/**
 * Called when a ComboBox has its selected item changed. 
 * @param comboBox	The combo box which has changed.
 */
void CPluginEditor::comboBoxChanged(ComboBox *comboBox)
{
	CPlugin* pro = dynamic_cast<CPlugin*>(getAudioProcessor());
	if (pro)
	{
		if (comboBox == m_areaSelector.get())
		{
			pro->SetMappingId(DCS_Gui, comboBox->getSelectedId());
		}

		else if (comboBox == m_delayModeComboBox.get())
		{
			pro->SetParameterValue(DCS_Gui, ParamIdx_DelayMode, float(comboBox->getSelectedId() - 1));
		}
	}
}

/**
 * Called when a button has been clicked.
 * @param button	The button whose status changed.
 */
void CPluginEditor::buttonClicked(Button *button)
{
	CPlugin* pro = dynamic_cast<CPlugin*>(getAudioProcessor());
	if (pro)
	{
		// Rx / Tx Buttons
		if (((button == m_oscModeSend.get()) || (button == m_oscModeReceive.get())) && (m_oscModeSend != nullptr) && (m_oscModeReceive != nullptr))
		{
			ComsMode oldMode = pro->GetComsMode();
			ComsMode newFlag = (button == m_oscModeSend.get()) ? CM_Tx : CM_Rx;

			if (button->getToggleState() == true)
				oldMode |= newFlag;
			else
				oldMode &= ~newFlag;

			pro->SetComsMode(DCS_Gui, oldMode);
		}
		
		// Overview button
		else if (button == m_overviewButton.get())
		{
			if (pro->IsTargetHostAvidConsole())
			{
				// Show / hide the Overview overlay
				ToggleOverlay(AOverlay::OT_Overview);
			}
			else
			{
				// Signal goes all the way to the Overview manager -> This will open the overview window.
				pro->OnOverviewButtonClicked();

				// Un-toggle button.			
				m_overviewButton->setToggleState(false, NotificationType::dontSendNotification);
			}
		}

		else if (button == m_overviewMultiSliderButton.get())
		{
			if (pro->IsTargetHostAvidConsole())
			{
				// Show / hide the Overview Multi-slider overlay.
				ToggleOverlay(AOverlay::OT_MultiSlide);

				// Set the selected coordinate mapping on the Overview slider to this Plug-in's setting.
				COverviewManager* ovrMgr = COverviewManager::GetInstance();
				if (ovrMgr)
					ovrMgr->SetSelectedMapping(pro->GetMappingId());
			}
		}

		// "About" button
		else if (button == m_aboutButton.get())
		{
			// Show / hide the About overlay
			ToggleOverlay(AOverlay::OT_About);
		}

		// Digital +/- buttons
		else if (m_sourceIdDigital.get() == dynamic_cast<CDigital*>(button->getParentComponent()))
		{
			// Increase or decrease the current sourceId by 1, depending on which button in the CDigital was pressed.
			int newSourceId = pro->GetSourceId();
			if (button->getName() == String("+"))
				newSourceId++;
			else
				newSourceId--;

			pro->SetSourceId(DCS_Gui, newSourceId);

			// Un-toggle button.			
			button->setToggleState(false, NotificationType::dontSendNotification);
		}
	}
}

/**
 * Create and show the speficied overlay if not already present, otherwise it is removed.
 * If a different overlay is specified as already exists, the existing one is replaced
 * and the corresponding GUI buttons are automatically toggled off.
 * @param type	Desired overlay to turn on or off.
 */
void CPluginEditor::ToggleOverlay(AOverlay::OverlayType type)
{
	AOverlay::OverlayType previousType = AOverlay::OT_Unknown;

	// Overview already exists, remove and delete it.
	if (m_overlay != nullptr)
	{
		previousType = m_overlay->GetOverlayType();

		// Toggle off the existing overlay's button, if is still turned on.
		CButton* tmpButton = nullptr;
		switch (previousType)
		{
			case AOverlay::OT_Overview:
				tmpButton = m_overviewButton.get();
				break;
			case AOverlay::OT_MultiSlide:
				tmpButton = m_overviewMultiSliderButton.get();
				break;
			case AOverlay::OT_About:
				tmpButton = m_aboutButton.get();
				break;
			default:
				jassertfalse;
				break;
		}
		if ((tmpButton != nullptr) && (tmpButton->getToggleState() == true))
			tmpButton->setToggleState(false, dontSendNotification);

		// Remove and delete old overlay.
		removeChildComponent(m_overlay.get());
		m_overlay.reset(nullptr); // Set scoped pointer to 0
	}

	// Create the new specified overlay.
	if (previousType != type)
	{
		switch (type)
		{
			case AOverlay::OT_Overview:
				m_overlay = std::make_unique<COverviewTableContainer>();
				break;

			case AOverlay::OT_MultiSlide:
				m_overlay = std::make_unique<COverviewMultiSurface>();
				break;

			case AOverlay::OT_About:
				// Slightly different About overlay depending on host format. 
				switch (PluginHostType::getPluginLoadedAs())
				{
					case AudioProcessor::wrapperType_AAX:
						m_overlay = std::make_unique<CAboutOverlayAAX>();
						break;
					case AudioProcessor::wrapperType_VST:
					case AudioProcessor::wrapperType_VST3: 
						m_overlay = std::make_unique<CAboutOverlayVST>();
						break;
					case AudioProcessor::wrapperType_AudioUnit:
					case AudioProcessor::wrapperType_AudioUnitv3:
						m_overlay = std::make_unique<CAboutOverlayAU>();
						break;
					default:
						jassertfalse;
						break;
				}
				break;
			default:
				jassertfalse;
				break;
		}

		// Initialize overlay GUI with current param values.
		m_overlay->UpdateGui(true);

		addAndMakeVisible(m_overlay.get());
		resized();
	}
}

/**
* Method which gets called when a region of a component needs redrawing, either because the
* component's repaint() method has been called, or because something has happened on the
* screen that means a section of a window needs to be redrawn.
* @param g		Graphics context that must be used to do the drawing operations.
*/
void CPluginEditor::paint(Graphics& g)
{
	int w = getLocalBounds().getWidth();
	int h = getLocalBounds().getHeight();

	// Bars above and below
	g.setColour(CDbStyle::GetDbColor(CDbStyle::MidColor));
	g.fillRect(getLocalBounds());

	// Background
	g.setColour(CDbStyle::GetDbColor(CDbStyle::DarkColor));
	g.fillRect(Rectangle<int>(0, 43, w, h - 87));

	// Little lines between version and logo
	g.setColour(CDbStyle::GetDbColor(CDbStyle::ButtonColor));
	g.fillRect(Rectangle<int>(w - 35, 6, 1, 30));
	g.fillRect(Rectangle<int>(w - 102, 6, 1, 30));

	// Add d&b logo 
	g.drawImage(m_dbLogo, getLocalBounds().getWidth() - 25, 15, 15, 15, 0, 0, 15, 15);

	// Draw a thin frame around window. Looks naked without.
	CPlugin* pro = dynamic_cast<CPlugin*>(getAudioProcessor());
	if (pro && pro->IsTargetHostAvidConsole())
	{
		g.setColour(CDbStyle::GetDbColor(CDbStyle::DarkLineColor));
		g.drawRect(Rectangle<int>(0, 0, w, h), 1);
	}
}

/**
* Called when this component's size has been changed.
* This is generally where you'll want to lay out the positions of any subcomponents in your editor.
*/
void CPluginEditor::resized()
{
	Rectangle<int> bounds = getLocalBounds();
	int w = bounds.getWidth();
	int h = bounds.getHeight();
	int xOffset = w - 468;
	int yOffset = h - 370;
	int vStartPos = 10;
	int vStartPos2 = h - 35;
	int hSecondPos = w - 106;

	// 2D Surface
	m_surfaceSlider->setBounds(125, vStartPos + 45, 200 + xOffset, 200 + yOffset);

	// X Slider
	m_xSlider->setBounds(125, 215 + yOffset + 45, 200 + xOffset, 50);
	m_xAxisLabel->setBounds(160 + (xOffset / 2), 242 + yOffset + 45, 25, 25);

	// Y Slider
	m_ySlider->setBounds(Rectangle<int>(20, 10 + 45, 100, 200 + yOffset));
	m_yAxisLabel->setBounds(Rectangle<int>(45, 120 + (yOffset / 2), 25, 25));

	// ReverbSendGain Slider
	m_reverbSendGainLabel->setBounds(Rectangle<int>(hSecondPos, h - 332, 72, 25));
	m_reverbSendGainSlider->setBounds(Rectangle<int>(hSecondPos, h - 314, 72, 55 + 20));

	// SourceSpread Slider
	m_sourceSpreadLabel->setBounds(Rectangle<int>(hSecondPos, h - 228, 72, 25));
	m_sourceSpreadSlider->setBounds(Rectangle<int>(hSecondPos, h - 211, 72, 55 + 20));

	// DelayMode ComboBox
	m_delayModeLabel->setBounds(Rectangle<int>(hSecondPos, h - 125, 72, 25));
	m_delayModeComboBox->setBounds(Rectangle<int>(hSecondPos, h - 104, 72, 25));

	// ----

	// Input number
	m_sourceIdLabel->setBounds(Rectangle<int>(5, vStartPos, 54, 25));
	m_sourceIdDigital->setBounds(Rectangle<int>(56, vStartPos - 2, 32 + 64, 25 + 4));

	// Mapping selector
	m_posAreaLabel->setBounds(Rectangle<int>(93 + 70, vStartPos, 72, 25));
	m_areaSelector->setBounds(Rectangle<int>(163 + 70, vStartPos, 50, 25));

	// Send / Receive buttons
	if (m_oscModeSend && m_oscModeReceive)
	{
		m_oscModeSend->setBounds(Rectangle<int>(w - 190, vStartPos, 35, 25));
		m_oscModeReceive->setBounds(Rectangle<int>(w - 154, vStartPos, 35, 25));
	}

	// Ip Address
	m_ipAddressLabel->setBounds(Rectangle<int>(5, vStartPos2, 75, 25));
	m_ipAddressTextEdit->setBounds(Rectangle<int>(80, vStartPos2, 140, 25));

	// Rate
	m_rateLabel->setBounds(Rectangle<int>(233, vStartPos2, 65, 25));
	m_rateTextEdit->setBounds(Rectangle<int>(296, vStartPos2, 50, 25));

	// Online
	m_onlineLed->setBounds(Rectangle<int>(w - 40, vStartPos2, 24, 24));

	// Overview
	m_overviewButton->setBounds(Rectangle<int>(w - 85, vStartPos2 - 2, 35, 27));

	// Multi-slider button is only used for Consoles
	if (m_overviewMultiSliderButton)
	{
		m_overviewMultiSliderButton->setBounds(Rectangle<int>(w - 130, vStartPos2 - 2, 35, 27));

		// About button
		m_aboutButton->setBounds(Rectangle<int>(w - 175, vStartPos2 - 2, 35, 27));
	}
	else
	{
		// About button
		m_aboutButton->setBounds(Rectangle<int>(w - 130, vStartPos2 - 2, 35, 27));
	}

	// Resize the GUI Overlay (if any)
	if (m_overlay)
	{
		m_overlay->setBounds(Rectangle<int>(0, 44, w, getLocalBounds().getHeight() - 89));
		m_overlay->toFront(true);
	}

	// Name and Version label
	m_nameLabel->setBounds(w - 105, 3, 75, 25);
	m_versionLabel->setBounds(w - 103, 21, 42, 15); 
	m_displayNameLabel->setBounds(5, 242 + yOffset + 45, 160 + (xOffset / 2), 25);

#ifdef JUCE_DEBUG	
	m_debugTextEdit->setBounds(125 + 20, vStartPos + 65, 160 + xOffset, 160 + yOffset);
#endif

	// Remember the user's preferred Plug-In window size.
	m_pluginWindowSize.setXY(w, h);
}

/**
 * Timer callback function, which will be called at regular intervals to update the GUI.
 * Reimplemented from base class Timer.
 */
void CPluginEditor::timerCallback()
{
	// If there is an overlay currenly active, update it.
	if (m_overlay)
		m_overlay->UpdateGui(false);

	// Also update the regular GUI.
	UpdateGui(false);
}

/**
 * Update GUI elements with the current parameter values.
 * @param init	True to ignore any changed flags and update parameters
 *				in the GUI anyway. Good for when opening the GUI for the first time.
 */
void CPluginEditor::UpdateGui(bool init)
{
	ignoreUnused(init); // No need to use this here so far.

	bool somethingChanged = false;

	CPlugin* pro = dynamic_cast<CPlugin*>(getAudioProcessor());
	if (pro)
	{
		const Array<AudioProcessorParameter*>& params = pro->getParameters();
		AudioParameterFloat* fParam;

		// See if any parameters changed since the last timer callback.
		somethingChanged = (pro->GetParameterChanged(DCS_Gui, DCT_AutomationParameters) ||
							pro->GetParameterChanged(DCS_Gui, DCT_PluginInstanceConfig) ||
							pro->GetParameterChanged(DCS_Gui, DCT_OscConfig));

		if (pro->PopParameterChanged(DCS_Gui, DCT_SourcePosition))
		{
			// Update position of X slider.
			fParam = dynamic_cast<AudioParameterFloat*>(params[ParamIdx_X]);
			if (fParam)
				m_xSlider->setValue(fParam->get(), dontSendNotification);

			// Update position of Y slider.
			fParam = dynamic_cast<AudioParameterFloat*>(params[ParamIdx_Y]);
			if (fParam)
				m_ySlider->setValue(fParam->get(), dontSendNotification);

			// Update the nipple position on the 2D-Slider.
			m_surfaceSlider->repaint();
		}

		if (pro->PopParameterChanged(DCS_Gui, DCT_ReverbSendGain))
		{
			// Update ReverbSendGain slider
			fParam = dynamic_cast<AudioParameterFloat*>(params[ParamIdx_ReverbSendGain]);
			if (fParam)
				m_reverbSendGainSlider->setValue(fParam->get(), dontSendNotification);
		}

		if (pro->PopParameterChanged(DCS_Gui, DCT_SourceSpread))
		{
			// Update SourceSpread slider
			fParam = dynamic_cast<AudioParameterFloat*>(params[ParamIdx_SourceSpread]);
			if (fParam)
				m_sourceSpreadSlider->setValue(fParam->get(), dontSendNotification);
		}

		if (pro->PopParameterChanged(DCS_Gui, DCT_DelayMode))
		{
			// Update DelayMode combo box
			AudioParameterChoice* cParam = dynamic_cast<AudioParameterChoice*>(params[ParamIdx_DelayMode]);
			if (cParam)
			{
				// Need to add 1 because the parameter's indeces go from 0 to 2, while the combo box's ID's go from 1 to 3.
				m_delayModeComboBox->setSelectedId(cParam->getIndex() + 1, dontSendNotification);
			}
		}

		if (pro->PopParameterChanged(DCS_Gui, DCT_SourceID))
		{
			// Update SourceID
			m_sourceIdDigital->SetValue(pro->GetSourceId());

			// Update the displayName (Host probably called updateTrackProperties or changeProgramName)
			m_displayNameLabel->setText(pro->getProgramName(0), dontSendNotification);
		}

		if (pro->PopParameterChanged(DCS_Gui, DCT_MappingID))
		{
			// Update MappingID
			m_areaSelector->setSelectedId(pro->GetMappingId(), dontSendNotification);
		}

		if (pro->PopParameterChanged(DCS_Gui, DCT_ComsMode))
		{
			// Update Rx/Tx (Check for existence of Rx/Tx buttons, not all platforms require them).
			if (m_oscModeSend && m_oscModeReceive)
			{
				ComsMode newMode = pro->GetComsMode();
				m_oscModeSend->setToggleState(((newMode & CM_Tx) == CM_Tx), dontSendNotification);
				m_oscModeReceive->setToggleState(((newMode & CM_Rx) == CM_Rx), dontSendNotification);
			}
		}

		if (pro->PopParameterChanged(DCS_Gui, DCT_IPAddress))
		{
			// Update IP address field
			m_ipAddressTextEdit->setText(pro->GetIpAddress());
		}

		if (pro->PopParameterChanged(DCS_Gui, DCT_MessageRate))
		{
			// Update message rate field
			m_rateTextEdit->setText(String(pro->GetMessageRate()));
		}

		if (pro->PopParameterChanged(DCS_Gui, DCT_Online))
		{
			// Update online status
			m_onlineLed->setToggleState(pro->GetOnline(), dontSendNotification);
		}

#ifdef JUCE_DEBUG
		if (pro->PopParameterChanged(DCS_Gui, DCT_DebugMessage))
		{
			// Append newest messages.
			m_debugTextEdit->moveCaretToEnd();
			m_debugTextEdit->insertTextAtCaret(pro->FlushDebugMessages());
		}
#endif
	}

	// Whenever the Multi-slider overlay is active, switch to fast refresh for smoother movements 
	// even if nothing changes in this plugin (there may be position changes in other plugins).
	if ((m_overlay != nullptr) &&
		(m_overlay->GetOverlayType() == AOverlay::OT_MultiSlide))
		somethingChanged = true;

	if (somethingChanged)
	{
		// At least one parameter was changed -> reset counter to prevent switching to "slow" refresh rate too soon.
		m_ticksSinceLastChange = 0;

		// Parameters have changed in the plugin: Switch to frequent GUI refreshing rate
		if (getTimerInterval() == GUI_UPDATE_RATE_SLOW)
		{
			startTimer(GUI_UPDATE_RATE_FAST);
			DBG("CPluginEditor::timerCallback: Switching to GUI_UPDATE_RATE_FAST");
		}
	}

	else
	{
		// No parameter changed since last timer callback -> increase counter.
		if (m_ticksSinceLastChange < GUI_UPDATE_DELAY_TICKS)
			m_ticksSinceLastChange++;

		// Once counter has reached a certain limit: Switch to lazy GUI refreshing rate
		else if (getTimerInterval() == GUI_UPDATE_RATE_FAST)
		{
			DBG("CPluginEditor::timerCallback(): Switching to GUI_UPDATE_RATE_SLOW");
			startTimer(GUI_UPDATE_RATE_SLOW);
		}
	}
}


} // namespace dbaudio
