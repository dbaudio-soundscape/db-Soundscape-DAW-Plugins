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

#include "Common.h"


namespace dbaudio
{


/**
 * Forward declarations.
 */
class CAudioParameterFloat;
class CAudioParameterChoice;


/**
 * Class CPlugin, a derived AudioProcessor which can be wrapped as VST, AU, or AAX. 
 */
class CPlugin :
	public AudioProcessor,
	public AudioProcessorParameter::Listener
{
public:
	/**
	 * Target hosts or DAWs which require special handling of some kind.
	 */
	enum TargetHost
	{
		TargetHost_Unknown = 0,
		TargetHost_ProTools,
		TargetHost_S6L
	};

	CPlugin();
	~CPlugin() override;

	void InitializeSettings(SourceId sourceId, int mappingId, String ipAddress, int oscMsgRate, ComsMode newMode);

	SourceId GetSourceId() const;
	void SetSourceId(DataChangeSource changeSource, SourceId sourceId);

	int GetMappingId() const;
	void SetMappingId(DataChangeSource changeSource, int mappingId);

	String GetIpAddress() const;
	void SetIpAddress(DataChangeSource changeSource, String ipAddress);

	int GetMessageRate() const;
	void SetMessageRate(DataChangeSource changeSource, int oscMsgRate);

	ComsMode GetComsMode() const;
	void SetComsMode(DataChangeSource changeSource, ComsMode newMode);
	void RestoreComsMode(DataChangeSource changeSource);
	bool GetOnline() const;
	bool GetBypass() const;

	float GetParameterValue(AutomationParameterIndex paramIdx, bool normalized = false) const;
	void SetParameterValue(DataChangeSource changeSource, AutomationParameterIndex paramIdx, float newValue);

	bool GetParameterChanged(DataChangeSource changeSource, DataChangeTypes change);
	bool PopParameterChanged(DataChangeSource changeSource, DataChangeTypes change);
	void SetParameterChanged(DataChangeSource changeSource, DataChangeTypes changeTypes);

	void Tick();
	void SetParamInTransit(DataChangeTypes paramsChanged);
	bool IsParamInTransit(DataChangeTypes paramsChanged) const;

	void OnOverviewButtonClicked();

#ifdef DB_SHOW_DEBUG
	void PushDebugMessage(String message);
	String GetDebugMessages();
#endif

	bool IsTargetHostAvidConsole() const;
	TargetHost GetTargetHost() const;

	// Overriden functions of class AudioProcessor

	virtual void getStateInformation(MemoryBlock& destData) override;
	virtual void setStateInformation(const void* data, int sizeInBytes) override;
	virtual void updateTrackProperties(const TrackProperties& properties) override;
	virtual AudioProcessorParameter* getBypassParameter() const override;

	// Overriden functions of class AudioProcessorParameter::Listener

	virtual void parameterValueChanged(int parameterIndex, float newValue) override;
	virtual void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

	// Functions which need to be reimplemented from class AudioProcessor, but which 
	// aren't relevant for our use.

	bool acceptsMidi() const override;
	void changeProgramName(int index, const String& newName) override;
	AudioProcessorEditor* createEditor() override;
	int getCurrentProgram() override;
	int getNumPrograms() override;
	const String getProgramName(int index) override;
	const String getName() const override;
	double getTailLengthSeconds() const override;
	bool hasEditor() const override;
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void processBlock(AudioSampleBuffer&, MidiBuffer&) override;
	bool producesMidi() const override;
	void releaseResources() override;
	void setCurrentProgram(int index) override;
#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

protected:
	/**
	 * X coordinate in meters.
	 * NOTE: not using std::unique_ptr here, see addParameter().
	 */
	CAudioParameterFloat*		m_xPos;

	/**
	 * Y coordinate in meters.
	 */
	CAudioParameterFloat*		m_yPos;

	/**
	 * Matrix input En-Space gain.
	 */
	CAudioParameterFloat*		m_reverbSendGain;

	/**
	 * Sound object spread.
	 */
	CAudioParameterFloat*		m_sourceSpread;

	/**
	 * Sound object delay mode (Off, Tight, Full).
	 */
	CAudioParameterChoice*		m_delayMode;

	/**
	 * Bypass automation parameter: 1 for bypass, 0 for normal.
	 */
	CAudioParameterChoice*		m_bypassParam;

	/**
	 * Current OSC communication mode, sending and/or receiving.
	 */
	ComsMode					m_comsMode;

	/**
	 * Previous OSC communication mode, before going into Bypass.
	 */
	ComsMode					m_comsModeWhenNotBypassed;

	/*
	 * Coordinate mapping index (1 to 4).
	 */
	int							m_mappingId;

	/*
	 * SourceID, or matrix input number.
	 */
	SourceId					m_sourceId;

	/**
	 * Unique ID of this Plug-in instance. 
	 * This is also this Plug-in's index within the CController::m_processors array.
	 */
	PluginId					m_pluginId;

	/**
	 * Keep track of which automation parameters have changed recently. 
	 * The array has one entry for each application module (see enum DataChangeSource).
	 */
	DataChangeTypes				m_parametersChanged[DCS_Max];

	/**
	 * Flags used to indicate when a SET command for a parameter is currently out on the network.
	 * Until such a flag is cleared (in the Tick() method), calls to IsParamInTransit will return true.
	 * This mechanism is used to ensure that parameters aren't overwritten right after having been
	 * changed via the Gui or the host.
	 */
	DataChangeTypes				m_paramSetCommandsInTransit = DCT_None;

	/**
	 * Name of this Plug-in instance. Some hosts (i.e. VST3) which support updateTrackProperties(..) 
	 * or changeProgramName(..) will set this to the DAW track name (i.e. "Guitar", or "Vocals", etc).
	 * On other hosts this will remain empty.
	 */
	String						m_pluginDisplayName;

	/**
	 * Member used to ensure that property changes are registered to the correct source.
	 * See CPlugin::SetParameterValue().
	 */
	DataChangeSource			m_currentChangeSource = DCS_Host;

#ifdef DB_SHOW_DEBUG
	/**
	 * Temp buffer for debugging messages. 
	 */
	String						m_debugMessageBuffer;
#endif

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPlugin)
};


} // namespace dbaudio
