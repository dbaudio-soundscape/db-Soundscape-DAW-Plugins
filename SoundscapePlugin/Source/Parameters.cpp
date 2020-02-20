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


#include "Parameters.h"


namespace dbaudio
{


/**
 * Number of CController::timerCallback() calls that are considered as the duration of a "gesture", when 
 * modifying a parameter via OSC. This is relevant for Touch automation.
 */
static constexpr int GESTURE_LENGTH_IN_TICKS = 8;


/*
===============================================================================
 Class CAudioParameterFloat
===============================================================================
*/

/**
 * Object constructor.
 * @param parameterID	Parameter unique ID
 * @param name			Parameter abbreviated name.
 * @param minValue		Minimum value, usually 0.0 (except for example for En-Space gain, -120.0).
 * @param maxValue		Maximum value, usually 1.0 (except for example for En-Space gain, +24.0).
 * @param stepSize		Smallest change amount which signifies an actual value change.
 * @param defaultValue	The parameter's default value when created.
 */
CAudioParameterFloat::CAudioParameterFloat(String parameterID, String name, float minValue, float maxValue, float stepSize, float defaultValue)
	: AudioParameterFloat(parameterID, name, minValue, maxValue, defaultValue)
{
	 // A float parameter is considered unchanged if it moves withing this tolerance range.
	 // Used in SetParameterValue() to deal with unfortunate rounding / precision errors caused by some DAWs.
	range.interval = stepSize;

	m_inGuiGesture = false;
	m_ticksSinceLastChange = GESTURE_LENGTH_IN_TICKS + 1;
}

/**
 * Object destructor.
 */
CAudioParameterFloat::~CAudioParameterFloat()
{
}

/**
 * Overriden from AudioParameterFloat. The base class implementation just calls AudioProcessor::getDefaultNumParameterSteps() which
 * returns 0x7fffffff. This results in a "Number of parameter steps exceeds limit" Error when analyzing the Plugin with Avid's
 * AAX-Validator tool DSH. 
 * @return	The number of steps this parameter has between 0.0f and 1.0f.
 */
int CAudioParameterFloat::getNumSteps() const
{ 
	return 0x7ff; 
}

/**
 * Called by GUI components (such as sliders or rotary knobs) when a drag or turn gesture starts. 
 * This will signal the host that a gesture has started, which is used for example for Touch automation.
 */
void CAudioParameterFloat::BeginGuiGesture()
{
	jassert(m_inGuiGesture == false);
	if (!m_inGuiGesture)
	{
		beginChangeGesture();
		m_inGuiGesture = true;
	}
}

/**
 * Called by GUI components (such as sliders or rotary knobs) when a drag or turn gesture ends. 
 * This will signal the host that a gesture has ended, which is used for example for Touch automation.
 */
void CAudioParameterFloat::EndGuiGesture()
{
	jassert(m_inGuiGesture == true);
	if (m_inGuiGesture)
	{
		endChangeGesture();
		m_inGuiGesture = false;

		// Ensure that the next CController::timerCallback() call does not trigger a endChangeGesture() call.
		m_ticksSinceLastChange = GESTURE_LENGTH_IN_TICKS + 1;
	}
}

/**
 * Called at every CController::timerCallback() call. 
 * Counts down the number of timer ticks that are considered as the duration of a "gesture", when 
 * modifying a parameter via OSC. This is relevant for Touch automation.
 */
void CAudioParameterFloat::Tick()
{
	const ScopedLock lock(m_mutex);

	// Ensure that user ist'n dragging a GUI control and already in the middle of a gesture.
	if (!m_inGuiGesture && (m_ticksSinceLastChange <= GESTURE_LENGTH_IN_TICKS))
	{
		// Don't let counter grow unnecessarily into infinity, 
		// we only need to know if maximum gesture length has been passed.
		m_ticksSinceLastChange++;

		// Once the tick counter passes the length of a gesture, 
		// we consider the gesture ended.
		if (m_ticksSinceLastChange > GESTURE_LENGTH_IN_TICKS)
			endChangeGesture();
	}
}

/**
 * Callback which takes place after the parameter's value has been changed.
 * This reimplementation is used to remember the parameter's previous value.
 * @param newValue	The new value, within the parameter's range (NOT normalized between 0.0f and 1.0f).
 */
void CAudioParameterFloat::valueChanged(float newValue)
{
	m_lastValue[1] = m_lastValue[0];
	m_lastValue[0] = newValue;
}

/**
 * Returns the value which was set before the current one.
 * @return	The previous value, within the parameter's range (NOT normalized between 0.0f and 1.0f).
 */
float CAudioParameterFloat::GetLastValue() const
{
	return m_lastValue[1];
}

/**
 * Pass a parameter change to the host.
 * Will also trigger the start of a gesture, if not already in the middle of one.
 * @param newValue	The new value, within the parameter's range (i.e. NOT normalized between 0.0f and 1.0f).
 */
void CAudioParameterFloat::SetParameterValue(float newValue)
{
	const ScopedLock lock(m_mutex);

	// Clip new value within allowed range for this parameter.
	newValue = jmax(jmin(newValue, range.end), range.start);

	// Check for an actual value change, taking precision errors into account.
	if ((newValue >= (get() + range.interval)) || (newValue <= (get() - range.interval)))
	{
		// If user ist'n dragging a GUI control and already in the middle of a gesture, 
		// signal the start of a gesture now.
		if (!m_inGuiGesture)
		{
			if (m_ticksSinceLastChange > GESTURE_LENGTH_IN_TICKS)
				beginChangeGesture();

			// Change taking place so reset tick counter.
			m_ticksSinceLastChange = 0;
		}

		// Map the newValue to the 0.0 to 1.0 range, and then
		// pass the parameter value change to base class.
		setValueNotifyingHost(range.convertTo0to1(newValue));
	}
}


/*
===============================================================================
 Class CAudioParameterChoice
===============================================================================
*/

/**
 * Object constructor.
 * @param parameterID			Parameter unique ID
 * @param name					Parameter abbreviated name.
 * @param choices				The set of choices to use 
 * @param defaultItemIndex		The index of the default choice
 * @param label					An optional label for the parameter's value
 * @param stringFromIndex		An optional lambda function that converts a choice index to a string with a maximum length. 
 *								This may be used by hosts to display the parameter's value.
 * @param indexFromString		An optional lambda function that parses a string and converts it into a choice index. 
 *								Some hosts use this to allow users to type in parameter values.
 */
CAudioParameterChoice::CAudioParameterChoice(	const String& parameterID, 
												const String& name,
												const StringArray& choices,
												int defaultItemIndex,
												const String& label,
												std::function<String(int, int)> stringFromIndex,
												std::function<int(const String&)> indexFromString)
	: AudioParameterChoice(parameterID, name, choices, defaultItemIndex, label, stringFromIndex, indexFromString)
{
	m_ticksSinceLastChange = GESTURE_LENGTH_IN_TICKS + 1;
}

/**
 * Object destructor.
 */
CAudioParameterChoice::~CAudioParameterChoice()
{
}

/**
 * Callback which takes place after the parameter's value has been changed.
 * This reimplementation is used to remember the parameter's previous value.
 * @param newValue	The new choice index.
 */
void CAudioParameterChoice::valueChanged(int newValue)
{
	m_lastIndex[1] = m_lastIndex[0];
	m_lastIndex[0] = newValue;
}

/**
 * Returns the index which was set before the current one.
 * @return	The previous choice index since the last valueChanged() call.
 */
int CAudioParameterChoice::GetLastIndex() const
{
	return m_lastIndex[1];
}

/**
 * Called at every CController::timerCallback() call. 
 * Counts down the number of timer ticks that are considered as the duration of a "gesture", when 
 * modifying a parameter via OSC. This is relevant for Touch automation.
 */
void CAudioParameterChoice::Tick()
{
	const ScopedLock lock(m_mutex);

	if (m_ticksSinceLastChange <= GESTURE_LENGTH_IN_TICKS)
	{
		// Don't let counter grow unnecessarily into infinity, 
		// we only need to know if maximum gesture length has been passed.
		m_ticksSinceLastChange++;

		// Once the tick counter passes the length of a gesture, 
		// we consider the gesture ended.
		if (m_ticksSinceLastChange > GESTURE_LENGTH_IN_TICKS)
			endChangeGesture();
	}
}

/**
 * Pass a parameter change to the host.
 * Will also trigger the start of a gesture, if not already in the middle of one.
 * @param newValue	The new choice index as a float, from 0.0f, to N-1, where N is the number of choices.
 */
void CAudioParameterChoice::SetParameterValue(float newValue)
{
	const ScopedLock lock(m_mutex);

	int newChoice = static_cast<int>(newValue);
	
	// AudioParameterChoice::getIndex() maps the internal 0.0f - 1.0f value to the 0 to N-1 range.
	if (getIndex() != newChoice)
	{
		// If user ist'n dragging a GUI control and already in the middle of a gesture, 
		// signal the start of a gesture now.
		if (m_ticksSinceLastChange > GESTURE_LENGTH_IN_TICKS)
			beginChangeGesture();

		// Change taking place so reset tick counter.
		m_ticksSinceLastChange = 0;

		// Pass the parameter value change to base class.
		// NOTE: Need to map to 0.0f to 1.0f range again.
		float maxValue = static_cast<float>(choices.size() - 1);
		setValueNotifyingHost(newChoice / maxValue);
	}
}


} // namespace dbaudio
