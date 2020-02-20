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

#include "../JuceLibraryCode/JuceHeader.h"


namespace dbaudio
{


/**
 * A style for d&b software look and feel.
 */
class CDbStyle
{
public:
	/**
	 * d&b Color codes
	 */
	enum DbColor
	{
		WindowColor,		// 27 27 27	- Window background
		DarkLineColor,		// 49 49 49 - Dark lines between table rows
		DarkColor,			// 67 67 67	- Dark
		MidColor,			// 83 83 83	- Mid
		ButtonColor,		// 125 125 125 - Button off
		LightColor,			// 201 201 201	- Light
		TextColor,			// 238 238 238 - Text
		DarkTextColor,		// 180 180 180 - Dark text
		HighlightColor,		// 115 140 155 - Highlighted text
		FaderGreenColor,	// 140 180 90 - Green sliders
		ButtonBlueColor,	// 28 122 166 - Button Blue
	};

	CDbStyle() {};
	virtual ~CDbStyle() = default;

	static Colour GetDbColor(DbColor color);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CDbStyle)
};


/**
 * Class CSlider, a custom Slider
 */
class CSlider : public Slider
{
public:
	CSlider();
	explicit CSlider(const String& componentName);
	CSlider(SliderStyle style, TextEntryBoxPosition textBoxPosition);
	~CSlider() override;

protected:
	void InitStyle();
	Rectangle<int> GetUpperSliderRect();
	Rectangle<int> GetLowerSliderRect();
	Rectangle<int> GetThumbRect();
	Path GetThumbPath();
	Path GetThumbArrowsPath();

	void mouseDown(const MouseEvent &) override;
	void paint(Graphics &) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CSlider)
};


/**
 * Class CKnob, a custom CSlider
 */
class CKnob : public CSlider
{
public:
	CKnob();
	explicit CKnob(const String& componentName);
	CKnob(SliderStyle style, TextEntryBoxPosition textBoxPosition);
	~CKnob() override;

protected:
	void InitStyle();

	void mouseDown(const MouseEvent &) override;
	void paint(Graphics &) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CKnob)
};


/**
 * class CTextEditor, a custom TextEditor
 */
class CTextEditor : public TextEditor
{
public:
	explicit CTextEditor(const String& componentName = String(), juce_wchar passwordCharacter = 0);
	~CTextEditor() override;

	void SetSuffix(String suffix);

protected:
	void InitStyle();
	void paint(Graphics &) override;

	String m_suffix;		/**< Suffix to append to displayed text, such as units. */

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CTextEditor)
};


/**
 * class CButton, a custom ToggleButton
 */
class CButton : public ToggleButton
{
public:
	explicit CButton(const String& componentName = String());
	~CButton() override;
	void SetCornerRadius(float radius);

protected:
	void paintButton(Graphics &, bool isMouseOverButton, bool isButtonDown) override;
	float m_cornerRadius;		/**< Determines the corner radius of the button. */

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CButton)
};


/**
 * class CPathButton, a custom CButton
 */
class CPathButton : public CButton
{
public:
	explicit CPathButton(const Path& path = Path());
	~CPathButton() override;

protected:
	void paintButton(Graphics &, bool isMouseOverButton, bool isButtonDown) override;
	Path		m_path;		/**< Path to draw on top of the button. */

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPathButton)
};


/**
 * class CImageButton, a custom CButton
 */
class CImageButton : public CButton
{
public:
	explicit CImageButton(const Image& image = Image());
	~CImageButton() override;

protected:
	void paintButton(Graphics &, bool isMouseOverButton, bool isButtonDown) override;
	Image		m_image;		/**< Image to draw on top of the button. */

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CImageButton)
};


/**
 * class CDiscreteButton, a custom CImageButton which is more discrete 
 * (Only looks like an actual button if it is being pressed, toggled on, or with mouseOver)
 */
class CDiscreteButton : public CImageButton
{
public:
	explicit CDiscreteButton(const Image& image);
	~CDiscreteButton() override;

protected:
	void paintButton(Graphics &, bool isMouseOverButton, bool isButtonDown) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CDiscreteButton)
};


/**
 * class CLabel, a custom Label
 */
class CLabel : public Label
{
public:
	explicit CLabel(const String& componentName = String(), const String& labelText = String());
	~CLabel() override;

protected:
	void InitStyle();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CLabel)
};


/**
 * Class CDigital, a custom component which behaves like a R1 RCT_Digital control. 
 * It has an input text field and also a + and a - buttons to increase/decrease the value.
 */
class CDigital : public Component
{
public:
	explicit CDigital(const String& componentName = String());
	~CDigital() override;
	
	void SetRange(int min, int max);
	void SetValue(int newValue);
	void AddListeners(TextEditor::Listener*, Button::Listener*);

	void resized() override;

private:
	/**
	 * Text editor.
	 */
	CTextEditor				m_editor;

	/**
	 * "+" button.
	 */
	CImageButton			m_plus;

	/**
	 * "-" button.
	 */
	CImageButton			m_minus;

	/**
	 * Minimum allowed value.
	 */
	int m_min = -99;

	/**
	 * Maximum allowed value.
	 */
	int m_max = 99;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CDigital)
};


/**
 * Abstract class AOverlay.
 * Must be reimplemented to provide a GUI overlay.
 */
class AOverlay : public Component
{
public:

	/**
	 * Overlay types. There can only be one active at the time.
	 */
	enum OverlayType
	{
		OT_Unknown = 0,
		OT_Overview,
		OT_MultiSlide,
		OT_About
	};

	explicit AOverlay(OverlayType type);
	~AOverlay() override;

	OverlayType GetOverlayType() const;
	virtual void UpdateGui(bool init) = 0;

private:
	/**
	 * Type of overlay as specified by the OverlayType enum.
	 */
	OverlayType	m_overlayType;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AOverlay)
};


} // namespace dbaudio
