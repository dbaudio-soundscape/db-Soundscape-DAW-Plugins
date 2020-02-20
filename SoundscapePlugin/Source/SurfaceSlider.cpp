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


#include "SurfaceSlider.h"
#include "Parameters.h"
#include "PluginProcessor.h"
#include "Controller.h"
#include "Gui.h"


namespace dbaudio
{


/**
 * Invalid PluginId used to signal when selection in CSurfaceMultiSlider is empty.
 */
static constexpr PluginId INVALID_PLUGIN_ID = 0xFFFFFFFF;


/*
===============================================================================
 Class CSurfaceSlider
===============================================================================
*/

/**
 * Object constructor.
 * @param parent	The audio processor object to act as parent.
 */
CSurfaceSlider::CSurfaceSlider(AudioProcessor* parent)
{
	m_parent = parent;
}

/**
 * Object destructor.
 */
CSurfaceSlider::~CSurfaceSlider()
{
}

/**
 * Reimplemented paint event function.
 * Components can override this method to draw their content. The paint() method gets called when 
 * a region of a component needs redrawing, either because the component's repaint() method has 
 * been called, or because something has happened on the screen that means a section of a window needs to be redrawn.
 * @param g		The graphics context that must be used to do the drawing operations. 
 */
void CSurfaceSlider::paint(Graphics& g)
{
	float w = static_cast<float>(getLocalBounds().getWidth());
	float h = static_cast<float>(getLocalBounds().getHeight());

	// Surface area
	Path outline;
	outline.addRectangle(0, 0, w, h);

	// X knob posiiton 
	float x = 0;
	const Array<AudioProcessorParameter*>& params = m_parent->getParameters();
	AudioParameterFloat* param = dynamic_cast<AudioParameterFloat*> (params[ParamIdx_X]);
	if (param)
		x = static_cast<float>(*param) * w;

	// Y knob position
	float y = 0;
	param = dynamic_cast<AudioParameterFloat*> (params[ParamIdx_Y]);
	if (param)
		y = h - (static_cast<float>(*param) * h);

	// Paint knob
	float knobSize = 10;
	outline.addEllipse(x - (knobSize / 2), y - (knobSize / 2), knobSize, knobSize);

	g.setColour(CDbStyle::GetDbColor(CDbStyle::MidColor));
	g.fillPath(outline);
	g.setColour(CDbStyle::GetDbColor(CDbStyle::ButtonColor));
	g.strokePath(outline, PathStrokeType(3)); // Stroke width

}

/**
 * Called when a mouse button is pressed. 
 * @param e		Details about the position and status of the mouse event, including the source component in which it occurred 
 */
void CSurfaceSlider::mouseDown(const MouseEvent& e)
{
	float w = static_cast<float>(getLocalBounds().getWidth());
	float h = static_cast<float>(getLocalBounds().getHeight());

	// Get mouse position and scale it between 0 and 1.
	Point<int> pos = e.getMouseDownPosition();
	float x = jmin<float>(1.0, jmax<float>(0.0, (static_cast<float>(pos.getX()) / w)));
	float y = 1.0f - jmin<float>(1.0, jmax<float>(0.0, (static_cast<float>(pos.getY()) / h)));

	CPlugin* plugin = dynamic_cast<CPlugin*>(m_parent);
	if (plugin)
	{
		// Set new X and Y values
		CAudioParameterFloat* param;
		param = dynamic_cast<CAudioParameterFloat*>(m_parent->getParameters()[ParamIdx_X]);
		param->BeginGuiGesture();
		plugin->SetParameterValue(DCS_Gui, ParamIdx_X, x);

		param = dynamic_cast<CAudioParameterFloat*>(m_parent->getParameters()[ParamIdx_Y]);
		param->BeginGuiGesture();
		plugin->SetParameterValue(DCS_Gui, ParamIdx_Y, y);
	}
}

/**
 * Called when the mouse is moved while a button is held down. 
 * @param e		Details about the position and status of the mouse event, including the source component in which it occurred
 */
void CSurfaceSlider::mouseDrag(const MouseEvent& e)
{
	float w = static_cast<float>(getLocalBounds().getWidth());
	float h = static_cast<float>(getLocalBounds().getHeight());

	// Get mouse position and scale it between 0 and 1.
	Point<int> pos = e.getPosition();
	float x = jmin<float>(1.0, jmax<float>(0.0, (static_cast<float>(pos.getX()) / w)));
	float y = 1.0f - jmin<float>(1.0, jmax<float>(0.0, (static_cast<float>(pos.getY()) / h)));

	CPlugin* plugin = dynamic_cast<CPlugin*>(m_parent);
	if (plugin)
	{
		// Set new X and Y values
		plugin->SetParameterValue(DCS_Gui, ParamIdx_X, x);
		plugin->SetParameterValue(DCS_Gui, ParamIdx_Y, y);
	}
}

/**
 * Called when the mouse button is released.
 * Reimplemented just to call EndGuiGesture() to inform the host.
 * @param e		Details about the position and status of the mouse event, including the source component in which it occurred
 */
void CSurfaceSlider::mouseUp(const MouseEvent& e)
{
	ignoreUnused(e);

	CAudioParameterFloat* param;
	param = dynamic_cast<CAudioParameterFloat*>(m_parent->getParameters()[ParamIdx_X]);
	param->EndGuiGesture();

	param = dynamic_cast<CAudioParameterFloat*>(m_parent->getParameters()[ParamIdx_Y]);
	param->EndGuiGesture();
}


/*
===============================================================================
 Class CSurfaceMultiSlider
===============================================================================
*/

/**
 * Object constructor.
 */
CSurfaceMultiSlider::CSurfaceMultiSlider()
{
	m_selected = INVALID_PLUGIN_ID;
}

/**
 * Object destructor.
 */
CSurfaceMultiSlider::~CSurfaceMultiSlider()
{
}

/**
 * Reimplemented paint event function.
 * Components can override this method to draw their content. The paint() method gets called when 
 * a region of a component needs redrawing, either because the component's repaint() method has 
 * been called, or because something has happened on the screen that means a section of a window needs to be redrawn.
 * @param g		The graphics context that must be used to do the drawing operations. 
 */
void CSurfaceMultiSlider::paint(Graphics& g)
{
	float w = static_cast<float>(getLocalBounds().getWidth());
	float h = static_cast<float>(getLocalBounds().getHeight());

	// Surface background area
	g.setColour(CDbStyle::GetDbColor(CDbStyle::MidColor));
	g.fillRect(Rectangle<float>(0.0f, 0.0f, w, h));

	// Draw grid
	const float dashLengths[2] = { 5.0f, 6.0f };
	const float lineThickness = 1.0f;
	g.setColour(CDbStyle::GetDbColor(CDbStyle::MidColor).brighter(0.15f));
	g.drawDashedLine(Line<float>(w * 0.25f, 0.0f, w * 0.25f, h), dashLengths, 2, lineThickness);
	g.drawDashedLine(Line<float>(w * 0.50f, 0.0f, w * 0.50f, h), dashLengths, 2, lineThickness);
	g.drawDashedLine(Line<float>(w * 0.75f, 0.0f, w * 0.75f, h), dashLengths, 2, lineThickness);
	g.drawDashedLine(Line<float>(0.0f, h * 0.25f, w, h * 0.25f), dashLengths, 2, lineThickness);
	g.drawDashedLine(Line<float>(0.0f, h * 0.50f, w, h * 0.50f), dashLengths, 2, lineThickness);
	g.drawDashedLine(Line<float>(0.0f, h * 0.75f, w, h * 0.75f), dashLengths, 2, lineThickness);

	// Surface frame
	g.setColour(CDbStyle::GetDbColor(CDbStyle::ButtonColor));
	g.drawRect(Rectangle<float>(0.0f, 0.0f, w, h), 1.5f);

	float knobSize = 10.0f;
	for (auto iter = m_cachedPositions.cbegin(); iter != m_cachedPositions.cend(); ++iter)
	{
		int inputNo((*iter).second.first);

		// Map the x/y coordinates to the pixel-wise dimensions of the surface area.
		Point<float> pt((*iter).second.second);
		float x = pt.x * w;
		float y = h - (pt.y * h);

		// Generate a color variant based on the input number, so make the nipples easier to tell from each other.
		Colour shade(juce::uint8(inputNo * 111), juce::uint8(inputNo * 222), juce::uint8(inputNo * 333));
		g.setColour(CDbStyle::GetDbColor(CDbStyle::DarkTextColor).interpolatedWith(shade, 0.3f));

		// Paint knob
		g.drawEllipse(Rectangle<float>(x - (knobSize / 2.0f), y - (knobSize / 2.0f), knobSize, knobSize), 3.0f);

		// Input number label
		g.setFont(Font(11.0, Font::plain));
		g.drawText(String(inputNo), Rectangle<float>(x - knobSize, y + 3, knobSize * 2.0f, knobSize * 2.0f), Justification::centred, true);
	}
}

/**
 * Called when a mouse button is pressed. 
 * @param e		Details about the position and status of the mouse event, including the source component in which it occurred 
 */
void CSurfaceMultiSlider::mouseDown(const MouseEvent& e)
{
	float w = static_cast<float>(getLocalBounds().getWidth());
	float h = static_cast<float>(getLocalBounds().getHeight());

	// Mouse click position (in pixel units)
	Point<float> mousePos(static_cast<float>(e.getMouseDownPosition().x), static_cast<float>(e.getMouseDownPosition().y));
	float knobSize = 15.0f;

	for (auto iter = m_cachedPositions.cbegin(); iter != m_cachedPositions.cend(); ++iter)
	{
		// Map the x/y coordinates to the pixel-wise dimensions of the surface area.
		Point<float> pt((*iter).second.second);
		float x = pt.x * w;
		float y = h - (pt.y * h);

		Path knobPath;
		knobPath.addEllipse(Rectangle<float>(x - (knobSize / 2.0f), y - (knobSize / 2.0f), knobSize, knobSize));

		// Check if the mouse click landed inside any of the knobs.
		if (knobPath.contains(mousePos))
		{
			// Set this source as "selected" and begin a drag gesture.
			m_selected = (*iter).first;

			CController* ctrl = CController::GetInstance();
			if (ctrl)
			{
				CPlugin* plugin = ctrl->GetProcessor(m_selected);
				jassert(plugin);
				if (plugin)
				{
					CAudioParameterFloat* param;
					param = dynamic_cast<CAudioParameterFloat*>(plugin->getParameters()[ParamIdx_X]);
					param->BeginGuiGesture();

					param = dynamic_cast<CAudioParameterFloat*>(plugin->getParameters()[ParamIdx_Y]);
					param->BeginGuiGesture();
				}
			}

			// Found a knob to select, skip the rest.
			break;
		}
	}
}

/**
 * Called when the mouse is moved while a button is held down. 
 * @param e		Details about the position and status of the mouse event, including the source component in which it occurred
 */
void CSurfaceMultiSlider::mouseDrag(const MouseEvent& e)
{
	if (m_selected != INVALID_PLUGIN_ID)
	{
		CController* ctrl = CController::GetInstance();
		if (ctrl)
		{
			CPlugin* plugin = ctrl->GetProcessor(m_selected);
			if (plugin)
			{
				// Get mouse pixel-wise position and scale it between 0 and 1.
				Point<int> pos = e.getPosition();
				float x = jmin<float>(1.0, jmax<float>(0.0, (static_cast<float>(pos.getX()) / getLocalBounds().getWidth())));
				float y = 1.0f - jmin<float>(1.0, jmax<float>(0.0, (static_cast<float>(pos.getY()) / getLocalBounds().getHeight())));

				plugin->SetParameterValue(DCS_Overview, ParamIdx_X, x);
				plugin->SetParameterValue(DCS_Overview, ParamIdx_Y, y);
			}
		}
	}
}

/**
 * Called when the mouse button is released.
 * Reimplemented just to call EndGuiGesture() to inform the host.
 * @param e		Details about the position and status of the mouse event, including the source component in which it occurred
 */
void CSurfaceMultiSlider::mouseUp(const MouseEvent& e)
{
	ignoreUnused(e);

	if (m_selected != INVALID_PLUGIN_ID)
	{
		CController* ctrl = CController::GetInstance();
		if (ctrl)
		{
			CPlugin* plugin = ctrl->GetProcessor(m_selected);
			if (plugin)
			{
				dynamic_cast<CAudioParameterFloat*>(plugin->getParameters()[ParamIdx_X])->EndGuiGesture();
				dynamic_cast<CAudioParameterFloat*>(plugin->getParameters()[ParamIdx_Y])->EndGuiGesture();

				// Get mouse pixel-wise position and scale it between 0 and 1.
				Point<int> pos = e.getPosition();
				float x = jmin<float>(1.0, jmax<float>(0.0, (static_cast<float>(pos.getX()) / getLocalBounds().getWidth())));
				float y = 1.0f - jmin<float>(1.0, jmax<float>(0.0, (static_cast<float>(pos.getY()) / getLocalBounds().getHeight())));

				plugin->SetParameterValue(DCS_Overview, ParamIdx_X, x);
				plugin->SetParameterValue(DCS_Overview, ParamIdx_Y, y);
			}
		}

		// De-select knob.
		m_selected = INVALID_PLUGIN_ID;
	}
}

/**
 * Update the local hash of plugins and their current coordinates.
 * @param positions	Map where the keys are the PluginIds of each source, while values are pairs of the corresponding 
 *					input number and position coordinates (0.0 to 1.0). 
 */
void CSurfaceMultiSlider::UpdatePositions(PositionCache positions)
{
	m_cachedPositions = positions;
}


} // namespace dbaudio
