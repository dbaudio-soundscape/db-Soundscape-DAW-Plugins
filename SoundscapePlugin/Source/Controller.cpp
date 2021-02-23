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


#include "Controller.h"
#include "Overview.h"
#include "PluginProcessor.h"


namespace dbaudio
{


static constexpr int OSC_INTERVAL_MIN = 20;		//< Minimum supported OSC messaging rate in milliseconds
static constexpr int OSC_INTERVAL_MAX = 5000;	//< Maximum supported OSC messaging rate in milliseconds
static constexpr int OSC_INTERVAL_DEF = 50;		//< Default OSC messaging rate in milliseconds

static const String OSC_DEFAULT_IP("127.0.0.1");	//< Default IP Address

static constexpr int RX_PORT_DS100 = 50010;		//< UDP port which the DS100 is listening to for OSC
static constexpr int RX_PORT_HOST = 50011;		//< UDP port to which the DS100 will send OSC replies

static constexpr int KEEPALIVE_TIMEOUT = 5000;	//< Milliseconds without response after which we consider plugin "Offline"
static constexpr int KEEPALIVE_INTERVAL = 1500;	//< Interval at which keepalive (ping) messages are sent, in milliseconds
static constexpr int MAX_HEARTBEAT_COUNT = 0xFFFF;	//< No point counting beyond this number.


/**
 * Pre-defined OSC command and response strings
 */
static const String kOscDelimiterString("/");
static const String kOscCommandString_ping("/ping");
static const String kOscCommandString_source_position_xy("/dbaudio1/coordinatemapping/source_position_xy/%d/%d");
static const String kOscCommandString_reverbsendgain("/dbaudio1/matrixinput/reverbsendgain/%d");
static const String kOscCommandString_source_spread("/dbaudio1/positioning/source_spread/%d");
static const String kOscCommandString_source_delaymode("/dbaudio1/positioning/source_delaymode/%d");
static const String kOscResponseString_pong("/pong");
static const String kOscResponseString_source_position_xy("/dbaudio1/coordinatemapping/source_position_xy");
static const String kOscResponseString_reverbsendgain("/dbaudio1/matrixinput/reverbsendgain");
static const String kOscResponseString_source_spread("/dbaudio1/positioning/source_spread");
static const String kOscResponseString_source_delaymode("/dbaudio1/positioning/source_delaymode");




/*
===============================================================================
 Class CController
===============================================================================
*/

/**
 * The one and only instance of CController.
 */
CController* CController::m_singleton = nullptr;

/**
 * Constructs an CController object.
 * There can be only one instance of this class, see m_singleton. This is so that network traffic
 * is managed from a central point and only one UDP port is opened for all OSC communication.
 */
CController::CController()
{
	jassert(!m_singleton);	// only one instnce allowed!!
	m_singleton = this;

	m_ipAddress = String("");
	m_oscMsgRate = 0;

	// Clear all changed flags initially
	for (int cs = 0; cs < DCS_Max; cs++)
		m_parametersChanged[cs] = DCT_None;

	// CController derives from OSCReceiver::Listener
	m_oscReceiver.addListener(this); 

	// Default OSC server settings. These might become overwritten 
	// by setStateInformation()
	SetRate(DCS_Osc, OSC_INTERVAL_DEF);
	SetIpAddress(DCS_Osc, OSC_DEFAULT_IP);
}

/**
 * Destroys the CController.
 */
CController::~CController()
{
	stopTimer();
	DisconnectOsc();

	// Destroy overView window and overView Manager
	COverviewManager* ovrMgr = COverviewManager::GetInstance();
	if (ovrMgr)
		ovrMgr->CloseOverview(true);

	const ScopedLock lock(m_mutex);
	m_processors.clearQuick();

	m_singleton = nullptr;
}

/**
 * Returns the one and only instance of CController. If it doesn't exist yet, it is created.
 * @return The CController singleton object.
 * @sa m_singleton, CController
 */
CController* CController::GetInstance()
{
	if (m_singleton == nullptr)
	{
		m_singleton = new CController();
	}
	return m_singleton;
}

/**
 * Method which will be called every time a parameter or property has been changed.
 * @param changeSource	The application module which is causing the property change.
 * @param changeTypes	Defines which parameter or property has been changed.
 */
void CController::SetParameterChanged(DataChangeSource changeSource, DataChangeTypes changeTypes)
{
	const ScopedLock lock(m_mutex);

	// Set the specified change flag for all DataChangeSources, except for the one causing the change.
	for (int cs = 0; cs < DCS_Max; cs++)
	{
		m_parametersChanged[cs] |= changeTypes;
	}

	// Forward the change to all plugin instances. This is needed, for example, so that all plugin's
	// GUIs update an IP address change.
	for (int i = 0; i < m_processors.size(); ++i)
	{
		m_processors[i]->SetParameterChanged(changeSource, changeTypes);
	}
}

/**
 * Get the state of the desired flag (or flags) for the desired change source.
 * @param changeSource	The application module querying the change flag.
 * @param change		The desired parameter (or parameters).
 * @return	True if any of the given parameters has changed it's value 
 *			since the last time PopParameterChanged() was called.
 */
bool CController::GetParameterChanged(DataChangeSource changeSource, DataChangeTypes change)
{
	const ScopedLock lock(m_mutex);
	return ((m_parametersChanged[changeSource] & change) != 0);
}

/**
 * Reset the state of the desired flag (or flags) for the desired change source.
 * Will return the state of the flag before the resetting.
 * @param changeSource	The application module querying the change flag.
 * @param change		The desired parameter (or parameters).
 * @return	The state of the flag before the resetting.
 */
bool CController::PopParameterChanged(DataChangeSource changeSource, DataChangeTypes change)
{
	const ScopedLock lock(m_mutex);
	bool ret((m_parametersChanged[changeSource] & change) != 0);
	m_parametersChanged[changeSource] &= ~change; // Reset flag.
	return ret;
}

/**
 * Register a plugin instance to the local list of processors. 
 * @param p		Pointer to newly crated plugin processor object.
 * @return		The PluginId of the newly added Plug-in.
 */
PluginId CController::AddProcessor(CPlugin* p)
{
	const ScopedLock lock(m_mutex);

	// Get the highest Input number of all current Plugins.
	SourceId currentMaxSourceId = 0;
	for (int i = 0; i < m_processors.size(); ++i)
	{
		if (m_processors[i]->GetSourceId() > currentMaxSourceId)
			currentMaxSourceId = m_processors[i]->GetSourceId();
	}

	m_processors.add(p);
	SetParameterChanged(DCS_Osc, DCT_NumPlugins);

	// Set the new Plugin's InputID to the next in sequence.
	p->SetSourceId(DCS_Osc, currentMaxSourceId + 1);

	PluginId newPluginId = static_cast<PluginId>(m_processors.size() - 1);
#ifdef DB_SHOW_DEBUG
	p->PushDebugMessage(String::formatted("++ CController::AddProcessor: pId=%d ++", newPluginId));
#endif

	return newPluginId;
}

/**
 * Remove a plugin instance from the local list of processors.
 * @param p		Pointer to plugin processor object which should be removed.
 */
void CController::RemoveProcessor(CPlugin* p)
{
	if (m_processors.size() > 1)
	{
		int idx = m_processors.indexOf(p);
		jassert(idx >= 0); // Tried to remove inexistent plugin object.
		if (idx >= 0)
		{
			const ScopedLock lock(m_mutex);
			m_processors.remove(idx);

			SetParameterChanged(DCS_Osc, DCT_NumPlugins);
		}
	}

	// If last plugin instance is being removed, delete CController singleton.
	else if (m_processors.size() == 1)
	{
		{ // Scope for lock.
			const ScopedLock lock(m_mutex);
			m_processors.clearQuick();
		}
	
		delete this;
	}
}

/**
 * Number of registered plugin instances.
 * @return	Number of registered plugin instances.
 */
int CController::GetProcessorCount() const
{
	const ScopedLock lock(m_mutex);
	return m_processors.size();
}

/**
 * Get a pointer to a specified processor.
 * @param idx	The index of the desired processor.
 * @return	The pointer to the desired processor.
 */
CPlugin* CController::GetProcessor(PluginId idx) const
{
	const ScopedLock lock(m_mutex);
	if ((idx >= 0) && (idx < m_processors.size()))
		return m_processors[idx];

	jassertfalse; // Index out of range!
	return nullptr;
}

/**
 * Getter function for the IP address to which m_oscSender and m_oscReceiver are connected.
 * @return	Current IP address.
 */
String CController::GetIpAddress() const
{
	return m_ipAddress;
}

/**
 * Static methiod which returns the default IP address.
 * @return	IP address to use as default.
 */
String CController::GetDefaultIpAddress()
{
	return OSC_DEFAULT_IP;
}

/**
 * Setter function for the IP address to which m_oscSender and m_oscReceiver are connected.
 * NOTE: changing ip address will disconnect m_oscSender and m_oscReceiver.
 * @param changeSource	The application module which is causing the property change.
 * @param ipAddress		New IP address.
 */
void CController::SetIpAddress(DataChangeSource changeSource, String ipAddress)
{
	if (m_ipAddress != ipAddress)
	{
		const ScopedLock lock(m_mutex);

		m_ipAddress = ipAddress;

		// Start "offline" after changing IP address
		m_heartBeatsRx = MAX_HEARTBEAT_COUNT;
		m_heartBeatsTx = 0;

		// Signal the change to all plugins. 
		SetParameterChanged(changeSource, (DCT_IPAddress | DCT_Online));

		ReconnectOsc();
	}
}

/**
 * Getter function for the OSC communication state.
 * @return		True if a valid OSC message was received and successfully processed recently.
 *				False if no response was received for longer than the timeout threshold.
 */
bool CController::GetOnline() const
{
	return ((m_heartBeatsRx * m_oscMsgRate) < KEEPALIVE_TIMEOUT);
}

/**
 * Getter for the rate at which OSC messages are being sent out.
 * @return	Messaging rate, in milliseconds.
 */
int CController::GetRate() const
{
	return m_oscMsgRate;
}

/**
 * Setter for the rate at which OSC messages are being sent out.
 * @param changeSource	The application module which is causing the property change.
 * @param rate	New messaging rate, in milliseconds.
 */
void CController::SetRate(DataChangeSource changeSource, int rate)
{
	if (rate != m_oscMsgRate)
	{
		const ScopedLock lock(m_mutex);

		// Clip rate to the allowed range.
		rate = jmin(OSC_INTERVAL_MAX, jmax(OSC_INTERVAL_MIN, rate));

		m_oscMsgRate = rate;

		// Signal the change to all plugins.
		SetParameterChanged(changeSource, DCT_MessageRate);

		// Reset timer to the new interval.
		startTimer(m_oscMsgRate);
	}
}

/**
 * Static methiod which returns the allowed minimum and maximum OSC message rates.
 * @return	Returns a std::pair<int, int> where the first number is the minimum supported rate, 
 *			and the second number is the maximum.
 */
std::pair<int, int> CController::GetSupportedRateRange()
{
	return std::pair<int, int>(OSC_INTERVAL_MIN, OSC_INTERVAL_MAX);
}

/**
 * Method to initialize IP address and polling rate.
 * @param changeSource	The application module which is causing the property change.
 * @param ipAddress		New IP address.
 * @param rate			New messaging rate, in milliseconds.
 */
void CController::InitGlobalSettings(DataChangeSource changeSource, String ipAddress, int rate)
{
	SetIpAddress(changeSource, ipAddress);
	SetRate(changeSource, rate);
}

/**
 * Called when the OSCReceiver receives a new OSC message, since CController inherits from OSCReceiver::Listener.
 * It forwards the message to all registered plugin objects.
 * @param message	The received OSC message.
 */
void CController::oscMessageReceived(const OSCMessage &message)
{
	const ScopedLock lock(m_mutex);
	jassert(m_processors.size() > 0);

	if (m_processors.size() > 0)
	{
		int i;
		bool resetHeartbeat = false;

		// Check if the incoming message is a response to a sent "ping".
		String addressString = message.getAddressPattern().toString();
		if (addressString.startsWith(kOscResponseString_pong))
			resetHeartbeat = true;

		// Check if the incoming message contains parameters.
		else if (message.size() > 0)
		{
			// Parse the Source ID
			SourceId sourceId = (addressString.fromLastOccurrenceOf(kOscDelimiterString, false, true)).getIntValue();
			jassert(sourceId > 0);
			if (sourceId > 0)
			{
				AutomationParameterIndex pIdx = ParamIdx_MaxIndex;
				DataChangeTypes change = DCT_None;
				int mappingId = 0;

				// Determine which parameter was changed depending on the incoming message's address pattern.
				if (addressString.startsWith(kOscResponseString_source_position_xy))
				{
					// Parse the Mapping ID
					addressString = addressString.upToLastOccurrenceOf(kOscDelimiterString, false, true);
					mappingId = (addressString.fromLastOccurrenceOf(kOscDelimiterString, false, true)).getIntValue();
					jassert(mappingId > 0);

					pIdx = ParamIdx_X;
					change = DCT_SourcePosition;
				}
				else if (addressString.startsWith(kOscResponseString_reverbsendgain))
				{
					pIdx = ParamIdx_ReverbSendGain;
					change = DCT_ReverbSendGain;
				}
				else if (addressString.startsWith(kOscResponseString_source_spread))
				{
					pIdx = ParamIdx_SourceSpread;
					change = DCT_SourceSpread;
				}
				else if (addressString.startsWith(kOscResponseString_source_delaymode))
				{
					pIdx = ParamIdx_DelayMode;
					change = DCT_DelayMode;
				}

				// Continue if the message's address pattern was recognized 
				if (change != DCT_None)
				{
					// Check all plugin instances to see if any of them want the new coordinates.
					for (i = 0; i < m_processors.size(); ++i)
					{
						// Check for matching Input number.
						CPlugin* plugin = m_processors[i];
						if (sourceId == plugin->GetSourceId())
						{
							// Check if a SET command was recently sent out and might currently be on transit to the device.
							// If so, ignore the incoming message so that our local data does not jump back to a now outdated value.
							bool ignoreResponse = plugin->IsParamInTransit(change);
							ComsMode mode = plugin->GetComsMode();

							// Only pass on new positions to plugins that are in RX mode.
							// Also, ignore all incoming messages for properties which this plugin wants to send a set command.
							if (!ignoreResponse && ((mode & (CM_Rx | CM_PollOnce)) != 0) && (plugin->GetParameterChanged(DCS_Osc, change) == false))
							{
								// Special handling for X/Y position, since message contains two parameters and MappingID needs to match too.
								if (pIdx == ParamIdx_X)
								{
									if (mappingId == plugin->GetMappingId())
									{
										// Set the plugin's new position.
										plugin->SetParameterValue(DCS_Osc, ParamIdx_X, message[0].getFloat32());
										plugin->SetParameterValue(DCS_Osc, ParamIdx_Y, message[1].getFloat32());

										// A request was sent to the DS100 by the CController because this plugin was in CM_PollOnce mode.
										// Since the response was now processed, set the plugin back into it's original mode.
										if ((mode & CM_PollOnce) == CM_PollOnce)
										{
											mode &= ~CM_PollOnce;
											plugin->SetComsMode(DCS_Osc, mode);
										}
									}
								}

								// All other automation parameters.
								else 
								{
									// DelayMode is an integer.
									float newValue;
									if ((pIdx == ParamIdx_DelayMode) && message[0].isInt32())
										newValue = static_cast<float>(message[0].getInt32());
									else
										newValue = message[0].getFloat32();

									plugin->SetParameterValue(DCS_Osc, pIdx, newValue);
								}
							}
						}
					}

					// Since pIdx was set, we know the received OSC message has valid format.
					// -> Signal to reset the number of heartbeats since last response.
					resetHeartbeat = true;
				}
			}
		}

		// A valid OSC message was received and successfully processed
		// -> reset the number of heartbeats since last response.
		if (resetHeartbeat)
		{
			bool wasOnline = GetOnline();
			m_heartBeatsRx = 0;

			// If previous state was "Offline", force all plugins to
			// update their GUI, since we are now Online.
			if (!wasOnline)
			{
				SetParameterChanged(DCS_Osc, DCT_Online);
			}
		}
	}
}

/**
 * Send a OSCMessage out to the connected ip address.
 * @param message	The OSC message to be sent.
 */
bool CController::SendOSCMessage(OSCMessage message)
{
	bool ret = (m_oscSender.send(message));
	if (ret)
		m_heartBeatsTx = 0;
	return ret;
}

/**
 * Disconnect the OSCSender from it's host.
 */
void CController::DisconnectOsc()
{
	m_oscSender.disconnect();
	m_oscReceiver.disconnect();
}

/**
 * Disconnect and re-connect the OSCSender to a host specified by the current ip settings.
 */
void CController::ReconnectOsc()
{
	DisconnectOsc();

	// Connect both sender and receiver  
	bool ok = m_oscSender.connect(m_ipAddress, RX_PORT_DS100);
	jassert(ok);

	ok = m_oscReceiver.connect(RX_PORT_HOST);
	jassert(ok);
}

/**
 * Timer callback function, which will be called at regular intervals to
 * send out OSC messages.
 * Reimplemented from base class Timer.
 */
void CController::timerCallback()
{
	const ScopedLock lock(m_mutex);
	if (m_processors.size() > 0)
	{
		// Check that we don't flood the line with pings, only send them in small intervals.
		bool sendKeepAlive = (((m_heartBeatsRx * m_oscMsgRate) > KEEPALIVE_INTERVAL) ||
								((m_heartBeatsTx * m_oscMsgRate) > KEEPALIVE_INTERVAL));

		int i;
		CPlugin* pro = nullptr;
		ComsMode mode;
		String messageString;

		for (i = 0; i < m_processors.size(); ++i)
		{
			pro = m_processors[i];

			// If the OscBypass parameter has changed since the last interval, 
			// update the OSC Rx/Tx mode of each Plugin accordingly.
			bool oscBypassed = pro->GetBypass();
			if (pro->PopParameterChanged(DCS_Osc, DCT_Bypass))
			{
				if (oscBypassed)
					pro->SetComsMode(DCS_Osc, CM_Off);
				else
					pro->RestoreComsMode(DCS_Osc);

				// Changing ComsMode also sets the changed flag for Bypass. 
				// Clear it so we don't come in here again unnecessarily.
				pro->PopParameterChanged(DCS_Osc, DCT_Bypass);
			}
			mode = pro->GetComsMode();

			// Signal every timer tick to each plugin instance. 
			// This is used to trigger gestures for touch automation.
			pro->Tick();

			// If plugin is in Bypass, we can skip all of the stuff below.
			if (!oscBypassed)
			{
				bool msgSent;
				DataChangeTypes paramSetsInTransit = DCT_None;

				// Iterate through all automation parameters.
				for (int pIdx = ParamIdx_X; pIdx < ParamIdx_MaxIndex; ++pIdx)
				{
					msgSent = false;

					switch (pIdx)
					{
						case ParamIdx_X:
						{
							// SET command is only sent out while in CM_Tx mode, provided that
							// this parameter has been changed since the last timer tick.
							if (((mode & CM_Tx) == CM_Tx) && pro->GetParameterChanged(DCS_Osc, DCT_SourcePosition))
							{
								messageString = String::formatted(kOscCommandString_source_position_xy, pro->GetMappingId(), pro->GetSourceId());
								msgSent = SendOSCMessage(OSCMessage(messageString, pro->GetParameterValue(ParamIdx_X), pro->GetParameterValue(ParamIdx_Y)));
								paramSetsInTransit |= DCT_SourcePosition;
							}

							// GET command for x/y coordinates is only sent out while in CM_Rx or CM_PollOnce mode,
							// provided that we didn't already send a SET command. Get command is just the OSC address pattern without parameters.
							if ((!msgSent) && ((mode & (CM_Rx | CM_PollOnce)) != 0))
							{
								messageString = String::formatted(kOscCommandString_source_position_xy, pro->GetMappingId(), pro->GetSourceId());
								msgSent = SendOSCMessage(OSCMessage(messageString));
							}
						}
						break;

						case ParamIdx_Y:
							// Changes to ParamIdx_Y are handled together with ParamIdx_X, so skip it.
							continue;
							break;

						case ParamIdx_ReverbSendGain:
						{
							// SET command is only sent out while in CM_Tx mode, provided that
							// this parameter has been changed since the last timer tick.
							if (((mode & CM_Tx) == CM_Tx) && pro->GetParameterChanged(DCS_Osc, DCT_ReverbSendGain))
							{
								messageString = String::formatted(kOscCommandString_reverbsendgain, pro->GetSourceId());
								msgSent = SendOSCMessage(OSCMessage(messageString, pro->GetParameterValue(ParamIdx_ReverbSendGain)));
								paramSetsInTransit |= DCT_ReverbSendGain;
							}

							// GET command is only sent out while in CM_Rx mode, provided that we 
							// didn't already send a SET command. Get command is just the OSC address pattern without parameters.
							if ((!msgSent) && ((mode & CM_Rx) == CM_Rx))
							{
								messageString = String::formatted(kOscCommandString_reverbsendgain, pro->GetSourceId());
								msgSent = SendOSCMessage(OSCMessage(messageString));
							}
						}
						break;

						case ParamIdx_SourceSpread:
						{
							// SET command is only sent out while in CM_Tx mode, provided that
							// this parameter has been changed since the last timer tick.
							if (((mode & CM_Tx) == CM_Tx) && pro->GetParameterChanged(DCS_Osc, DCT_SourceSpread))
							{
								messageString = String::formatted(kOscCommandString_source_spread, pro->GetSourceId());
								msgSent = SendOSCMessage(OSCMessage(messageString, pro->GetParameterValue(ParamIdx_SourceSpread)));
								paramSetsInTransit |= DCT_SourceSpread;
							}

							// GET command is only sent out while in CM_Rx mode, provided that we 
							// didn't already send a SET command. Get command is just the OSC address pattern without parameters.
							if ((!msgSent) && ((mode & CM_Rx) == CM_Rx))
							{
								messageString = String::formatted(kOscCommandString_source_spread, pro->GetSourceId());
								msgSent = SendOSCMessage(OSCMessage(messageString));
							}
						}
						break;

						case ParamIdx_DelayMode:
						{
							// SET command is only sent out while in CM_Tx mode, provided that
							// this parameter has been changed since the last timer tick.
							if (((mode & CM_Tx) == CM_Tx) && pro->GetParameterChanged(DCS_Osc, DCT_DelayMode))
							{
								messageString = String::formatted(kOscCommandString_source_delaymode, pro->GetSourceId());
								msgSent = SendOSCMessage(OSCMessage(messageString, static_cast<int>(pro->GetParameterValue(ParamIdx_DelayMode))));
								paramSetsInTransit |= DCT_DelayMode;
							}

							// GET command is only sent out while in CM_Rx mode, provided that we 
							// didn't already send a SET command. Get command is just the OSC address pattern without parameters.
							if ((!msgSent) && ((mode & CM_Rx) == CM_Rx))
							{
								messageString = String::formatted(kOscCommandString_source_delaymode, pro->GetSourceId());
								msgSent = SendOSCMessage(OSCMessage(messageString));
							}
						}
						break;

						case ParamIdx_Bypass:
							// Nothing to do, this is not a parameter which will arrive per OSC.
							continue;
							break;

						default:
							jassertfalse;
							break;
					}

					if (msgSent)
					{
						// Since we are expecting at least one response from the DS100, 
						// we can use that as heartbeat, no need to send an extra ping.
						sendKeepAlive = false;
					}
				}

				// Flag the parameters for which we just sent a SET command out.
				pro->SetParamInTransit(paramSetsInTransit);
			}

			// All changed parameters were sent out, so we can reset their flags now.
			pro->PopParameterChanged(DCS_Osc, DCT_AutomationParameters);
		}
		
		if (sendKeepAlive)
		{
			// If we aren't expecting any responses from the DS100, we need to at least send a "ping"
			// so that we can use the "pong" to check our connection status. 
			// See handling of "pong" in oscMessageReceived()
			OSCMessage oscMessage(kOscCommandString_ping);
			SendOSCMessage(oscMessage);
		}

		bool wasOnline = GetOnline();
		if (m_heartBeatsRx < MAX_HEARTBEAT_COUNT)
			m_heartBeatsRx++;
		if (m_heartBeatsTx < MAX_HEARTBEAT_COUNT)
			m_heartBeatsTx++;

		// If we have just crossed the treshold, force all plugins to update their
		// GUI, since we are now Offline.
		if (wasOnline && (GetOnline() == false))
			SetParameterChanged(DCS_Osc, DCT_Online);
	}
}


} // namespace dbaudio
