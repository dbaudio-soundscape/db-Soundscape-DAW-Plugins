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


#include "Gui.h"


namespace dbaudio
{


/*
===============================================================================
 Class CDbStyle
===============================================================================
*/

/**
 * Get the desired color.
 * @param color Desired color code.
 */
Colour CDbStyle::GetDbColor(DbColor color)
{
	switch (color)
	{
	case WindowColor:
		return Colour(27, 27, 27);
	case DarkLineColor:
		return Colour(49, 49, 49);
	case DarkColor:
		return Colour(67, 67, 67);
	case MidColor:
		return Colour(83, 83, 83);
	case ButtonColor:
		return Colour(125, 125, 125);
	case LightColor:
		return Colour(201, 201, 201);
	case TextColor:
		return Colour(238, 238, 238);
	case DarkTextColor:
		return Colour(180, 180, 180);
	case HighlightColor:
		return Colour(115, 140, 155);
	case FaderGreenColor:
		return Colour(140, 180, 90);
	case ButtonBlueColor:
		return Colour(27, 120, 163);
	default:
		break;
	}

	jassertfalse;
	return Colours::black;
}


/*
===============================================================================
 Class CSlider
===============================================================================
*/

static constexpr int CSLIDER_THUMB_WIDTH = 17;		//< The width or thickness of the slider's grabber or thumb.
static constexpr int CSLIDER_THUMB_LENGTH = 23;		//< The length of the slider's grabber or thumb.
static constexpr int CSLIDER_SLIDER_WIDTH = 9;		//< The width or thickness of the slider.

/**
 * Object constructor.
 */
CSlider::CSlider()
{
	InitStyle();
}

/**
 * Object constructor.
 * @param name	The component name.
 */
CSlider::CSlider(const String& name)
	: Slider(name)
{
	InitStyle();
}

/**
 * Object constructor.
 * @param style			Component style.
 * @param textBoxPos	Position of the slider's textbox.
 */
CSlider::CSlider(SliderStyle style, TextEntryBoxPosition textBoxPos)
	: Slider(style, textBoxPos)
{
	InitStyle();
}

/**
 * Object destructor.
 */
CSlider::~CSlider()
{
}

/**
 * Set custom colors.
 */
void CSlider::InitStyle()
{
	setColour(Slider::textBoxTextColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	setColour(Slider::textBoxBackgroundColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
	setColour(Slider::thumbColourId, CDbStyle::GetDbColor(CDbStyle::ButtonColor));
	setColour(Slider::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
	setColour(Slider::trackColourId, CDbStyle::GetDbColor(CDbStyle::FaderGreenColor));
	setColour(Slider::textBoxOutlineColourId, CDbStyle::GetDbColor(CDbStyle::WindowColor));
	setColour(Slider::textBoxHighlightColourId, CDbStyle::GetDbColor(CDbStyle::LightColor));
}

/**
 * Called when a mouse button is pressed.
 * @param e		Details about the position and status of the mouse event, including the source component in which it occurred
 */
void CSlider::mouseDown(const MouseEvent& e)
{
	// Get mouse position and scale it between 0 and 1.
	Point<int> pos = e.getMouseDownPosition();

	if (GetUpperSliderRect().contains(pos) || 
		GetLowerSliderRect().contains(pos) || 
		GetThumbPath().contains(static_cast<float>(pos.getX()), static_cast<float>(pos.getY())))
	{
		// Accept click -> pass on to base class implementation.
		Slider::mouseDown(e);
	}
}

/**
 * Reimplemented paint function.
 * @param g		A graphics context, used for drawing a component or image.
 */
void CSlider::paint(Graphics &g)
{
	// Paint bounding rect just for testing.
	//Rectangle<int> bounds = getLocalBounds();
	//g.setColour(CDbStyle::GetDbColor(ButtonBlueColor));
	//g.drawRect(bounds, 1);

	Rectangle<int> lowerSliderRect = GetLowerSliderRect();
	Rectangle<int> upperSliderRect = GetUpperSliderRect();

	// Slider areas
	g.setColour(CDbStyle::GetDbColor(CDbStyle::FaderGreenColor));
	g.fillRect(lowerSliderRect);
	g.setColour(CDbStyle::GetDbColor(CDbStyle::ButtonColor));
	g.fillRect(upperSliderRect);
	g.setColour(CDbStyle::GetDbColor(CDbStyle::WindowColor));
	g.drawRect(lowerSliderRect, 1);
	g.drawRect(upperSliderRect, 1);

	// Thumb (slider knob)
	Path thumbPath = GetThumbPath();
	g.setColour(CDbStyle::GetDbColor(CDbStyle::WindowColor));
	g.strokePath(thumbPath, PathStrokeType(2));
	g.setColour(CDbStyle::GetDbColor(CDbStyle::ButtonColor));
	g.fillPath(thumbPath);

	// Thumb arrows
	Path arrowPath = GetThumbArrowsPath();
	g.setColour(CDbStyle::GetDbColor(CDbStyle::TextColor));
	g.fillPath(arrowPath);
}

/**
 * Get the bounding rectangle of the lower section (i.e. the green part) of the slider area.
 * For higher slider values this rectangle will be longer / higher.
 * @return	A rectangle representing the bounding area.
 */
Rectangle<int> CSlider::GetLowerSliderRect()
{
	Rectangle<int> ret;
	Rectangle<int> bounds = getLocalBounds();
	double v = getValue();

	if (getSliderStyle() == Slider::LinearHorizontal)
	{
		ret = Rectangle<int>(CSLIDER_THUMB_WIDTH / 2, 9, 
			static_cast<int>(v * (bounds.getWidth() - CSLIDER_THUMB_WIDTH)),
			CSLIDER_SLIDER_WIDTH);
	}
	else if (getSliderStyle() == Slider::LinearVertical)
	{
		int len = static_cast<int>(v * (bounds.getHeight() - CSLIDER_THUMB_WIDTH));
		int pos = (bounds.getHeight() - len) - (CSLIDER_THUMB_WIDTH / 2);
		ret = Rectangle<int>(bounds.getWidth() - 18, pos, CSLIDER_SLIDER_WIDTH, len);
	}

	return ret;
}

/**
 * Get the bounding rectangle of the higher section (i.e. the gray part) of the slider area.
 * For lower slider values this rectangle will be longer / higher.
 * @return	A rectangle representing the bounding area.
 */
Rectangle<int> CSlider::GetUpperSliderRect()
{
	Rectangle<int> ret;
	Rectangle<int> bounds = getLocalBounds();
	double v = getValue();

	if (getSliderStyle() == Slider::LinearHorizontal)
	{
		int len = static_cast<int>((1.0 - v) * (bounds.getWidth() - CSLIDER_THUMB_WIDTH));
		int pos = (bounds.getWidth() - len) - (CSLIDER_THUMB_WIDTH / 2);
		ret = Rectangle<int>(pos, 9, len, CSLIDER_SLIDER_WIDTH);
	}
	else if (getSliderStyle() == Slider::LinearVertical)
	{
		ret = Rectangle<int>(bounds.getWidth() - 18, CSLIDER_THUMB_WIDTH / 2,
			CSLIDER_SLIDER_WIDTH, static_cast<int>((1.0 - v) * (bounds.getHeight() - CSLIDER_THUMB_WIDTH)));
	}

	return ret;
}

/**
 * Get the drawing path for the slider's thumb grabber.
 * @return	The path.
 */
Path CSlider::GetThumbPath()
{
	Path ret;
	Rectangle<int> bounds = getLocalBounds();
	double v = getValue();
	float startX;
	float startY;

	if (getSliderStyle() == Slider::LinearHorizontal)
	{
		startX = 1.0f + static_cast<float>(v * (bounds.getWidth() - CSLIDER_THUMB_WIDTH));
		startY = 3.0f;

		ret.startNewSubPath(startX, startY + 2);
		ret.lineTo(startX + static_cast<int>(CSLIDER_THUMB_WIDTH / 2) - 0.5f, startY - 2);
		ret.lineTo(startX + CSLIDER_THUMB_WIDTH - 2, startY + 2);
		ret.lineTo(startX + CSLIDER_THUMB_WIDTH - 2, CSLIDER_THUMB_LENGTH);
		ret.lineTo(startX, CSLIDER_THUMB_LENGTH);
		ret.closeSubPath();
	}
	else if (getSliderStyle() == Slider::LinearVertical)
	{
		startX = static_cast<float>(bounds.getWidth() - CSLIDER_THUMB_LENGTH);
		startY = 1.0f + static_cast<float>((bounds.getHeight() - CSLIDER_THUMB_WIDTH) * (1.0 - v));

		ret.startNewSubPath(startX, startY);
		ret.lineTo(startX + CSLIDER_THUMB_LENGTH - 5, startY);
		ret.lineTo(startX + CSLIDER_THUMB_LENGTH - 1, startY + (CSLIDER_THUMB_WIDTH / 2));
		ret.lineTo(startX + CSLIDER_THUMB_LENGTH - 5, startY + CSLIDER_THUMB_WIDTH - 2);
		ret.lineTo(startX, startY + CSLIDER_THUMB_WIDTH - 2);
		ret.closeSubPath();
	}

	return ret;
}

/**
 * Get the drawing path for the slider's little thumb arrows.
 * @return	The path.
 */
Path CSlider::GetThumbArrowsPath()
{
	Path ret;
	Rectangle<int> bounds = getLocalBounds();
	double v = getValue();
	float startX;
	float startY;

	if (getSliderStyle() == Slider::LinearHorizontal)
	{
		startX = 1.0f + static_cast<float>(v * (bounds.getWidth() - CSLIDER_THUMB_WIDTH));
		startY = 3.0f;

		ret.addTriangle(	startX + 1.5f, startY + 10.5f,	// P1
							startX + 6, startY + 7,			// P2
							startX + 6, startY + 14);		// P3

		ret.addTriangle(	startX + 13.5f, startY + 10.5f,	// P1
							startX + 9, startY + 14,		// P2
							startX + 9, startY + 7);		// P3
	}
	else if (getSliderStyle() == Slider::LinearVertical)
	{
		startX = static_cast<float>(bounds.getWidth() - CSLIDER_THUMB_LENGTH);
		startY = 1.0f + static_cast<float>((bounds.getHeight() - CSLIDER_THUMB_WIDTH) * (1.0 - v));

		ret.addTriangle(	startX + 9.5f, startY + 1.5f,	// P1
							startX + 13, startY + 6,		// P2
							startX + 6, startY + 6);		// P3

		ret.addTriangle(	startX + 9.5f, startY + 13.5f,	// P1
							startX + 6, startY + 9,			// P2
							startX + 13, startY + 9);		// P3
	}

	return ret;
}


/*
===============================================================================
 Class CKnob
===============================================================================
*/

/**
 * Object constructor.
 */
CKnob::CKnob()
{
	InitStyle();
}

/**
 * Object constructor.
 * @param name	The component name.
 */
CKnob::CKnob(const String& name)
	: CSlider(name)
{
	InitStyle();
}

/**
 * Object constructor.
 * @param style			Component style.
 * @param textBoxPos	Position of the slider's textbox.
 */
CKnob::CKnob(SliderStyle style, TextEntryBoxPosition textBoxPos)
	: CSlider(style, textBoxPos)
{
	InitStyle();
}

/**
 * Object destructor.
 */
CKnob::~CKnob()
{
}

/**
 * Set custom colors.
 */
void CKnob::InitStyle()
{
	setColour(Slider::textBoxTextColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	setColour(Slider::textBoxBackgroundColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
	setColour(Slider::thumbColourId, CDbStyle::GetDbColor(CDbStyle::ButtonColor));
	setColour(Slider::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
	setColour(Slider::trackColourId, CDbStyle::GetDbColor(CDbStyle::FaderGreenColor));
	setColour(Slider::textBoxOutlineColourId, CDbStyle::GetDbColor(CDbStyle::WindowColor));
	setColour(Slider::textBoxHighlightColourId, CDbStyle::GetDbColor(CDbStyle::LightColor));
}

/**
 * Called when a mouse button is pressed.
 * @param e		Details about the position and status of the mouse event, including the source component in which it occurred
 */
void CKnob::mouseDown(const MouseEvent& e)
{
	// Skip the CSlider implementation.
	Slider::mouseDown(e);
}

/**
 * Reimplemented paint function.
 * @param g		A graphics context, used for drawing a component or image.
 */
void CKnob::paint(Graphics &g)
{
	// Skip the CSlider implementation.
	Slider::paint(g);
}


/*
===============================================================================
 Class CTextEditor
===============================================================================
*/

/**
 * Object constructor.
 * @param componentName		The name to pass to the component for it to use as its name .
 * @param passwordCharacter	Used as a replacement for all characters that are drawn on screen.
 */
CTextEditor::CTextEditor(const String& componentName, juce_wchar passwordCharacter)
	: TextEditor(componentName, passwordCharacter)
{
	m_suffix = "";
	InitStyle();
}

/**
 * Object destructor.
 */
CTextEditor::~CTextEditor()
{
}

/**
 * Reimplemented paint function to draw units.
 * @param g		A graphics context, used for drawing a component or image.
 */
void CTextEditor::paint(Graphics &g)
{
	// First let base implementation paint the component.
	TextEditor::paint(g);

	// If a suffix has been defined (suffix string not empty), paint the units suffix.
	// Only display units while the TextEditor does NOT have keyboard focus.
	if (!hasKeyboardFocus(true) && m_suffix.isNotEmpty())
	{
		float suffixWidth = (m_suffix.length() * 7.0f) + 6.0f; // Width of strings in pixels.
		float contentWidth = (getText().length() * 7.0f) + 6.0f;
		Rectangle<float> textArea(contentWidth, static_cast<float>(getLocalBounds().getY()), suffixWidth, static_cast<float>(getLocalBounds().getHeight()));

		g.setColour(CDbStyle::GetDbColor(CDbStyle::TextColor));
		g.drawText(m_suffix, textArea, Justification::centred);
	}
}

/**
 * Set custom colors and config.
 */
void CTextEditor::InitStyle()
{
	setMultiLine(false);
	setReturnKeyStartsNewLine(false);
	setCaretVisible(true);
	setInputRestrictions(16, ".0123456789");
	setColour(TextEditor::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::DarkColor));
	setColour(TextEditor::textColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	setColour(TextEditor::outlineColourId, CDbStyle::GetDbColor(CDbStyle::WindowColor));
	setColour(TextEditor::focusedOutlineColourId, CDbStyle::GetDbColor(CDbStyle::LightColor));
	setColour(TextEditor::highlightedTextColourId, CDbStyle::GetDbColor(CDbStyle::HighlightColor));
}

/**
 * Add a suffix. Per default no suffix is appended to the displayed text.
 * @param suffix	Suffix to append to displayed text, such as units.
 */
void CTextEditor::SetSuffix(String suffix)
{
	m_suffix = suffix;
}


/*
===============================================================================
 Class CButton
===============================================================================
*/

/**
 * Object constructor.
 * @param componentName		The name to pass to the component for it to use as its name .
 */
CButton::CButton(const String& componentName)
	: ToggleButton(componentName)
{
	m_cornerRadius = 2;
}

/**
 * Object destructor.
 */
CButton::~CButton()
{
}

/**
 * Set the corner radius of the button. If this function is not used, 
 * the default corner radius used is 2.
 * @param radius	The new corner radius.
 */
void CButton::SetCornerRadius(float radius)
{
	m_cornerRadius = radius;
	repaint();
}

/**
 * Reimplemented paint function.
 * @param g					A graphics context, used for drawing a component or image.
 * @param isMouseOverButton	True if the mouse is over the button.
 * @param isButtonDown		True if the button is being pressed.
 */
void CButton::paintButton(Graphics &g, bool isMouseOverButton, bool isButtonDown)
{
	Rectangle<int> bounds = getLocalBounds();
	Rectangle<float> buttonRectF = Rectangle<float>(2.5f, 2.5f, bounds.getWidth() - 4.0f, bounds.getHeight() - 4.0f);
	bool on = getToggleState();
	bool enabled = isEnabled();

	// Button's main colour
	if (on)
	{
		Colour col = CDbStyle::GetDbColor(CDbStyle::ButtonBlueColor);
		if (isButtonDown)
			col = col.brighter(0.1f);
		else if (isMouseOverButton)
			col = col.brighter(0.05f);
		g.setColour(col);
	}
	else
	{
		Colour col = CDbStyle::GetDbColor(CDbStyle::ButtonColor);
		if (!enabled)
			col = col.darker(0.5f);
		else if (isButtonDown)
			col = CDbStyle::GetDbColor(CDbStyle::ButtonBlueColor).brighter(0.05f);
		else if (isMouseOverButton)
			col = col.brighter(0.05f);
		g.setColour(col);
	}

	g.fillRoundedRectangle(buttonRectF, m_cornerRadius);
	g.setColour(CDbStyle::GetDbColor(CDbStyle::WindowColor));
	g.drawRoundedRectangle(buttonRectF, m_cornerRadius, 1);
	g.setColour(CDbStyle::GetDbColor(CDbStyle::TextColor));
	g.drawText(getName(), bounds, Justification::centred, false);
}


/*
===============================================================================
 Class CPathButton
===============================================================================
*/

/**
 * Object constructor.
 * @param path		Path which will be drawn on the button.
 */
CPathButton::CPathButton(const Path& path)
	: CButton(String()),
	m_path(path)
{
}

/**
 * Object destructor.
 */
CPathButton::~CPathButton()
{
}

/**
 * Reimplemented paint function.
 * @param g					A graphics context, used for drawing a component or image.
 * @param isMouseOverButton	True if the mouse is over the button.
 * @param isButtonDown		True if the button is being pressed.
 */
void CPathButton::paintButton(Graphics &g, bool isMouseOverButton, bool isButtonDown)
{
	// Actual button is painted by base class
	CButton::paintButton(g, isMouseOverButton, isButtonDown);

	// Use applyTransform to move the path so that it is centered within the button's area.
	// NOTE: Make a local copy of the path, since applyTransform modifies the original.
	Path tmpPath(m_path);
	float xOffset = ((getLocalBounds().getWidth() / 2) - (tmpPath.getBounds().getWidth() / 2)) + 1;
	float yOffset = ((getLocalBounds().getHeight() / 2) - (tmpPath.getBounds().getHeight() / 2)) + 1;
	tmpPath.applyTransform(AffineTransform::translation(xOffset, yOffset));

	// Draw the path
	g.setColour(CDbStyle::GetDbColor(CDbStyle::TextColor));
	g.strokePath(tmpPath, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
}


/*
===============================================================================
 Class CImageButton
===============================================================================
*/

/**
 * Object constructor.
 * @param image		Image which will be place on the button.
 */
CImageButton::CImageButton(const Image& image)
	: CButton(String()),
	m_image(image)
{
}

/**
 * Object destructor.
 */
CImageButton::~CImageButton()
{
}

/**
 * Reimplemented paint function.
 * @param g					A graphics context, used for drawing a component or image.
 * @param isMouseOverButton	True if the mouse is over the button.
 * @param isButtonDown		True if the button is being pressed.
 */
void CImageButton::paintButton(Graphics &g, bool isMouseOverButton, bool isButtonDown)
{
	// Actual button is painted by base class
	CButton::paintButton(g, isMouseOverButton, isButtonDown);

	// Paint image centered on buttons rectangle.
	int w = getLocalBounds().getWidth();
	int h = getLocalBounds().getHeight();
	int iw = m_image.getBounds().getWidth();
	int ih = m_image.getBounds().getHeight();
	g.drawImage(m_image, ((w - iw) / 2) + 1, ((h - ih) / 2) + 1, iw, ih, 0, 0, iw, ih);
}


/*
===============================================================================
 Class CDiscreteButton
===============================================================================
*/

/**
 * Object constructor.
 * @param image		Image which will be place on the button.
 */
CDiscreteButton::CDiscreteButton(const Image& image)
	: CImageButton(image)
{
}

/**
 * Object destructor.
 */
CDiscreteButton::~CDiscreteButton()
{
}

/**
 * Reimplemented paint function, to only show as a button when pressed, or mouseOver'ed. 
 * @param g					A graphics context, used for drawing a component or image.
 * @param isMouseOverButton	True if the mouse is over the button.
 * @param isButtonDown		True if the button is being pressed.
 */
void CDiscreteButton::paintButton(Graphics &g, bool isMouseOverButton, bool isButtonDown)
{
	Rectangle<int> bounds = getLocalBounds();
	int w = bounds.getWidth();
	int h = bounds.getHeight();

	// Only draw the actual button if it is being pressed, toggled on, or with mouseOver.
	bool on = getToggleState();
	if (on || isButtonDown || isMouseOverButton)
	{
		Rectangle<float> buttonRectF = Rectangle<float>(2.5f, 2.5f, w - 4.0f, h - 4.0f);
		Colour col = CDbStyle::GetDbColor(CDbStyle::ButtonBlueColor);

		if (!on)
		{
			if (isButtonDown)
				col = col.brighter(0.05f);
			else if (isMouseOverButton)
				col = CDbStyle::GetDbColor(CDbStyle::ButtonColor).brighter(0.05f);
		}
		else
		{
			if (isButtonDown)
				col = col.brighter(0.1f);
			else if (isMouseOverButton)
				col = col.brighter(0.05f);
		}

		g.setColour(col);
		g.fillRoundedRectangle(buttonRectF, m_cornerRadius);
		g.setColour(CDbStyle::GetDbColor(CDbStyle::WindowColor));
		g.drawRoundedRectangle(buttonRectF, m_cornerRadius, 1);
		g.setColour(CDbStyle::GetDbColor(CDbStyle::TextColor));
	}

	// Paint image centered on buttons rectangle.
	int iw = m_image.getBounds().getWidth();
	int ih = m_image.getBounds().getHeight();
	g.drawImage(m_image, ((w - iw) / 2) + 1, ((h - ih) / 2) + 1, iw, ih, 0, 0, iw, ih);
}


/*
===============================================================================
 Class CDiscreteButton
===============================================================================
*/

/**
 * Object constructor.
 * @param componentName		The name to pass to the component for it to use as its name .
 * @param labelText			The text to be shown on the label.
 */
CLabel::CLabel(const String& componentName, const String& labelText)
	: Label(componentName, labelText)
{
	InitStyle();
}

/**
 * Object destructor.
 */
CLabel::~CLabel()
{
}

/**
 * Set custom colors and config.
 */
void CLabel::InitStyle()
{
	setFont(Font(14, Font::plain));
	setColour(Label::textColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	setJustificationType(Justification::centred);
}


/*
===============================================================================
 Class CDigital
===============================================================================
*/

/**
 * Object constructor.
 */
CDigital::CDigital(const String& componentName)
	: Component(componentName)
{
	// Create and configure components inside this container.
	addAndMakeVisible(m_editor);
	m_plus.setName("+");
	m_plus.setEnabled(true);
	addAndMakeVisible(m_plus);
	m_minus.setName("-");
	m_minus.setEnabled(true);
	addAndMakeVisible(m_minus);
}

/**
 * Object destructor.
 */
CDigital::~CDigital()
{
}

/**
 * Set the allowed range of values.
 * @param min	Min allowed value.
 * @param max	Max allowed value.
 */
void CDigital::SetRange(int min, int max)
{
	m_min = min;
	m_max = max;
}

/**
 * Set the new value of the control. Depending on the new value, it may become 
 * adjusted to the current allowed range.
 * @param newValue	New value to set.
 */
void CDigital::SetValue(int newValue)
{
	if (newValue <= m_min)
	{
		// Can't go lower, disable "-" button.
		newValue = m_min;
		m_plus.setEnabled(true);
		m_minus.setEnabled(false);
	}
	else if (newValue >= m_max)
	{
		// Can't go higher, disable "+" button.
		newValue = m_max;
		m_plus.setEnabled(false);
		m_minus.setEnabled(true);
	}
	else
	{
		m_plus.setEnabled(true);
		m_minus.setEnabled(true);
	}

	m_editor.setText(String(newValue));
}

/**
 * Set listeners for the textfield and the buttons. These listeners will be automatically signalled when
 * any changes to the textField or the buttons take place, respectively.
 * @param textListener		Pointer to the TextEditor::Listener.
 * @param buttonListener	Pointer to the Button::Listener.
 */
void CDigital::AddListeners(TextEditor::Listener* textListener, Button::Listener* buttonListener)
{
	m_editor.addListener(textListener);
	m_plus.addListener(buttonListener);
	m_minus.addListener(buttonListener);
}

/**
 * Reimplemented from Component, gets called whenever the component is resized. Here we adjust the size
 * and position of sub-elements.
 */
void CDigital::resized()
{
	int w = getLocalBounds().getWidth();
	int h = getLocalBounds().getHeight();
	m_editor.setBounds(0, 2, static_cast<int>((w * 0.45f) + 1.0f), h - 4);
	m_minus.setBounds(static_cast<int>((w * 0.45f) - 2.0f), 0, static_cast<int>((w * 0.275f) + 3.0f), h - 1);
	m_plus.setBounds(static_cast<int>((w * 0.725f) - 3.0f), 0, static_cast<int>((w * 0.275f) + 3.0f), h - 1);
}


/*
===============================================================================
 Class AOverlay
===============================================================================
*/

/**
 * Class constructor.
 */
AOverlay::AOverlay(OverlayType type)
	: Component()
{
	m_overlayType = type;
}

/**
 * Class destructor.
 */
AOverlay::~AOverlay()
{
}

/**
 * Get this overlay's type.
 */
AOverlay::OverlayType AOverlay::GetOverlayType() const
{
	return m_overlayType;
}


} // namespace dbaudio
