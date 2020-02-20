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
 * Version management class
 */
class CVersion
{
public:

#pragma pack(push,1)
	typedef union version_t
	{
		struct
		{
			// for the little endian architectures you have to specify the
			// bitfields from the LSB to the MSB
			juce::uint32  minor : 16;	//< Minor version number.
			juce::uint32  major : 16;	//< Major version number.
		} v_struct;
		juce::uint32 all;				//< All bits as 32bit integer value. 
	} version_t;
#pragma pack(pop)

	CVersion();
	CVersion(juce::uint16 major, juce::uint16 minor);
	CVersion(juce::uint32);
	CVersion(String);
	CVersion(const CVersion &r);
	virtual ~CVersion();

	CVersion& operator=(const CVersion& r);
	bool operator==(const CVersion& r);
	bool operator!=(const CVersion& r);
	bool operator<(const CVersion& r);
	bool operator<=(const CVersion& r);
	bool operator>(const CVersion& r);
	bool operator>=(const CVersion& r);

	bool IsValid() const;
	String ToString() const;
	juce::uint32 ToInt() const;

protected:
	/**
	 * Major and minor version struct.
	 */
	version_t	m_version;
};


} // namespace dbaudio
