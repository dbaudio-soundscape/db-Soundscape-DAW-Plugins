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


#pragma once

#include "Gui.h"
#include "SurfaceSlider.h"
#include <utility>	//<USE std::unique_ptr


namespace dbaudio
{


/**
 * Class CPluginEditor, a component that acts as the GUI for the AudioProcessor. 
 */
class CPluginEditor :
	public AudioProcessorEditor,
	public TextEditor::Listener,
	public Slider::Listener,
	public ComboBox::Listener,
	public Button::Listener,
	private Timer
{
public:
	CPluginEditor(CPlugin&);
	~CPluginEditor() override;

	void paint(Graphics&) override;
	void resized() override;
	void ToggleOverlay(AOverlay::OverlayType type);
	void UpdateGui(bool init);

private:
	CAudioParameterFloat* GetParameterForSlider(CSlider* slider);
	void sliderValueChanged(Slider *slider) override;
	void sliderDragStarted(Slider* slider) override;
	void sliderDragEnded(Slider* slider) override;
	void textEditorFocusLost(TextEditor &) override;
	void textEditorReturnKeyPressed(TextEditor &) override;
	void comboBoxChanged(ComboBox *comboBox) override;
	void buttonClicked(Button*) override;
	void timerCallback() override;

	/**
	 * Horizontal slider for X axis.
	 */
	std::unique_ptr<CSlider>	m_xSlider;

	/**
	 * Vertical slider for Y axis.
	 */
	std::unique_ptr<CSlider>	m_ySlider;

	/**
	 * Slider for ReverbSendGain
	 */
	std::unique_ptr<CSlider>	m_reverbSendGainSlider;

	/**
	 * Slider for SourceSpread
	 */
	std::unique_ptr<CSlider>	m_sourceSpreadSlider;

	/**
	 * ComboBox for DelayMode
	 */
	std::unique_ptr<ComboBox>	m_delayModeComboBox;

	/**
	 * X axis slider label
	 */
	std::unique_ptr<CLabel>	m_xAxisLabel;

	/**
	 * Y axis slider label
	 */
	std::unique_ptr<CLabel>	m_yAxisLabel;

	/**
	 * ReverbSendGain slider label
	 */
	std::unique_ptr<CLabel>	m_reverbSendGainLabel;

	/**
	 * SourceSpread slider label
	 */
	std::unique_ptr<CLabel>	m_sourceSpreadLabel;

	/**
	 * DelayMode ComboBox label
	 */
	std::unique_ptr<CLabel>	m_delayModeLabel;

	/*
	 * App version label
	 */
	std::unique_ptr<CLabel>	m_versionLabel;

	/*
	 * App name
	 */
	std::unique_ptr<CLabel>	m_nameLabel;

	/*
	 * Positioning Area label
	 */
	std::unique_ptr<CLabel>	m_posAreaLabel;

	/*
	 * Source ID label
	 */
	std::unique_ptr<CLabel>	m_sourceIdLabel;

	/*
	 * DS100 IP Address label
	 */
	std::unique_ptr<CLabel>	m_ipAddressLabel;

	/*
	 * Send/receive rate label
	 */
	std::unique_ptr<CLabel>	m_rateLabel;

	/*
	 * ComboBox selector for the positioning area
	 */
	std::unique_ptr<ComboBox>	m_areaSelector;

	/*
	 * Text editor for the source ID (matrix input)
	 */
	std::unique_ptr<CDigital>	m_sourceIdDigital;

	/*
	 * Text editor for the DS100 IP Address
	 */
	std::unique_ptr<CTextEditor>	m_ipAddressTextEdit;

	/*
	 * Text editor for the OSC send/receive rate in ms.
	 */
	std::unique_ptr<CTextEditor>	m_rateTextEdit;

	/*
	 * Button used as Online indicator LED.
	 */
	std::unique_ptr<CButton> m_onlineLed;

	/*
	 * Button for OSC Send mode (part of a radio button pair)
	 */
	std::unique_ptr<CButton> m_oscModeSend;
	/*
	 * Button for OSC Receive mode (part of a radio button pair)
	 */
	std::unique_ptr<CButton> m_oscModeReceive;

	/*
	 * 2D Slider component.
	 */
	std::unique_ptr<CSurfaceSlider> m_surfaceSlider;

	/*
	 * Logo image.
	 */
	Image m_dbLogo;

	/**
	 * Button to open the overview window.
	 */
	std::unique_ptr<CImageButton> m_overviewButton;

	/**
	 * Button to open the overview multi-slider overlay (Console-only).
	 */
	std::unique_ptr<CPathButton> m_overviewMultiSliderButton;

	/**
	 * Pointer to the GUI overlay. 
	 * This can be an overview table, or an "about" overlay.
	 */
	std::unique_ptr<AOverlay> m_overlay;

	/**
	 * Plug-in display name label. On the hosts which support updateTrackProperties or changeProgramName, 
	 * this will show the track's name where this Plug-in is located.
	 */
	std::unique_ptr<CLabel>	m_displayNameLabel;

	/**
	 * Button to open the "About" overlay.
	 */
	std::unique_ptr<CDiscreteButton> m_aboutButton;

	/**
	 * Used to allow some tolerance when switching between fast and slow refresh rates for the GUI. 
	 * Once this counter reaches GUI_UPDATE_DELAY_TICKS, and no parameters have changed, the GUI will
	 * switch to GUI_UPDATE_RATE_SLOW. Switches to GUI_UPDATE_RATE_FAST happen immediately after any change.
	 */
	int m_ticksSinceLastChange = 0;

	/**
	 * Keep track of the user's preferred Plug-In window size, and use it when opening a fresh window.
	 */
	static Point<int> m_pluginWindowSize;

#ifdef JUCE_DEBUG
	/**
	 * Special textfield used for displaying debugging messages.
	 */
	std::unique_ptr<TextEditor>	m_debugTextEdit;
#endif

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPluginEditor)
};


} // namespace dbaudio
