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
 * Class CAudioParameterFloat, a custom AudioParameterFloat which provides it's own implementation
 * of getNumSteps(), required for AAX. See this method's description for more info.
 *
 * This derivation supports automatic gesture management, which depends on the Tick() method 
 * being called regular intervals.
 */
class CAudioParameterFloat : public AudioParameterFloat
{
public:
	CAudioParameterFloat(	String parameterID, 
							String name, 
							float minValue, 
							float maxValue, 
							float stepSize,
							float defaultValue);

	~CAudioParameterFloat() override;

	void BeginGuiGesture();
	void EndGuiGesture();

	void SetParameterValue(float);
	float GetLastValue() const;
	void Tick();

protected:
	int getNumSteps() const override;
	void valueChanged(float newValue) override;

	/**
	 * Number of Tick() calls since the last value change. 
	 */
	int m_ticksSinceLastChange;

	/**
	 * True if user is currently dragging or turning a GUI control, and thus in the middle of a gesture.
	 */
	bool m_inGuiGesture;

	/**
	 * SetParameterValue() and Tick() may be called from 2 different threads, so make sure
	 * m_ticksSinceLastChange is handled in a tread-safe way.
	 */
	CriticalSection			m_mutex;

	/**
	 * Since AudioParameterFloat::setValue() is unfortunately private, we use this to remember
	 * the last two values in order to detect actual value changes in AudioProcessorParameter::Listener::parameterValueChanged().
	 * These values are normalized between 0.0f and 1.0f.
	 */
	float m_lastValue[2];

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CAudioParameterFloat)
};


/**
 * Class CAudioParameterChoice, a custom AudioParameterChoice.
 *
 * This derivation supports automatic gesture management, which depends on the Tick() method
 * being called regular intervals.
 */
class CAudioParameterChoice : public AudioParameterChoice
{
public:
	CAudioParameterChoice(	const String& parameterID,
							const String& name,
							const StringArray& choices,
							int defaultItemIndex,
							const String& label = String(),
							std::function<String(int index, int maximumStringLength)> stringFromIndex = nullptr,
							std::function<int(const String& text)> indexFromString = nullptr);

	~CAudioParameterChoice() override;

	void SetParameterValue(float);
	int GetLastIndex() const;
	void Tick();

protected:
	void valueChanged(int newValue) override;

	/**
	 * Number of Tick() calls since the last value change.
	 */
	int m_ticksSinceLastChange;

	/**
	 * SetParameterValue() and Tick() may be called from 2 different threads, so make sure
	 * m_ticksSinceLastChange is handled in a tread-safe way.
	 */
	CriticalSection			m_mutex;

	/**
	 * Since AudioParameterChoice::setValue() is unfortunately private, we use this to remember
	 * the last two values in order to detect actual value changes in AudioProcessorParameter::Listener::parameterValueChanged().
	 */
	int m_lastIndex[2];

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CAudioParameterChoice)
};


} // namespace dbaudio
