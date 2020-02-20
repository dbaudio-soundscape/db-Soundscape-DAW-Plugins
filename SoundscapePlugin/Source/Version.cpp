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


#include "Version.h"


namespace dbaudio
{


/*
===============================================================================
 Class CVersion
===============================================================================
*/

/**
 * Class standard constructor.
 */
CVersion::CVersion()
{
	m_version.all = 0; // Default initialization (invalid version);
}

/**
 * Class constructor.
 * @param major		Major version number as uint16.
 * @param minor		Minor version number as uint16.
 */
CVersion::CVersion(juce::uint16 major, juce::uint16 minor)
{
	m_version.v_struct.major = major;
	m_version.v_struct.minor = minor;
}

/**
 * Class constructor.
 * @param source	A 32bit integer value containing the version's
 *					major (msb 2 bytes) and minor (lsb 2 bytes) numbers.
 */
CVersion::CVersion(juce::uint32 source)
{
	m_version.all = source;
}

/**
 * Class constructor.
 * @param source	A string containing the version's major (M) and minor (m) numbers
 *					in the form MMM.mmm or also MMM.mmm.xxx where the debug part of the 
 *					version string (xxx) is ignored.
 */
CVersion::CVersion(String source)
{
	m_version.all = 0; // Default initialization (invalid version);
	if (source.length() > 0)
	{
		m_version.v_struct.major = static_cast<juce::uint16>((source.upToFirstOccurrenceOf(".", false, true)).getIntValue());
		source = (source.fromFirstOccurrenceOf(".", false, true)).upToFirstOccurrenceOf(".", false, true);
		m_version.v_struct.minor = static_cast<juce::uint16>(source.getIntValue());
	}
}

/**
 * Class copy constructor.
 * @param source	CVersion object to copy from.
 */
CVersion::CVersion(const CVersion &source)
{
	*this = source;
}

/**
 * Class destructor.
 */
CVersion::~CVersion()
{
}

/**
 * Assignment operator.
 * @param r			CVersion object to copy from.
 * @return			Reference to this object.
 */
CVersion& CVersion::operator=(const CVersion& r)
{
	m_version.all = r.m_version.all;
	return *this;
}

/**
 * Is equal operator.
 * @param r			CVersion object to compare to.
 * @return			True if objects are equal.
 */
bool CVersion::operator==(const CVersion& r)
{
	return (m_version.all == r.m_version.all);
}

/**
 * Not equal operator.
 * @param r			CVersion object to compare to.
 * @return			True if objects are different.
 */
bool CVersion::operator!=(const CVersion& r)
{
	return (m_version.all != r.m_version.all);
}

/**
 * Less than operator.
 * @param r			CVersion object to compare to.
 * @return			True if this object is smaller than r.
 */
bool CVersion::operator<(const CVersion& r)
{
	if (m_version.v_struct.major == r.m_version.v_struct.major)
		return (m_version.v_struct.minor < r.m_version.v_struct.minor);
	return (m_version.v_struct.major < r.m_version.v_struct.major);
}

/**
 * Less or equal then operator.
 * @param r			CVersion object to compare to.
 * @return			True if this object is smaller or equal to r.
 */
bool CVersion::operator<=(const CVersion& r)
{
	return ((m_version.all == r.m_version.all) || (m_version.all < r.m_version.all));
}

/**
 * Larger than operator.
 * @param r			CVersion object to compare to.
 * @return			True if this object is larger than r.
 */
bool CVersion::operator>(const CVersion& r)
{
	if (m_version.v_struct.major == r.m_version.v_struct.major)
		return (m_version.v_struct.minor > r.m_version.v_struct.minor);
	return (m_version.v_struct.major > r.m_version.v_struct.major);
}

/**
 * Larger or equal then operator.
 * @param r			CVersion object to compare to.
 * @return			True if this object is larger or equal to r.
 */
bool CVersion::operator>=(const CVersion& r)
{
	return ((m_version.all == r.m_version.all) || (m_version.all > r.m_version.all));
}

/**
 * Check if this is a valid version number (non-zero).
 * @return			True if the version is non-zero.
 */
bool CVersion::IsValid() const
{
	return (m_version.all > 0);
}

/**
 * Convert to string.
 * @return	A string representing the version in the format [major].[minor]
 */
String CVersion::ToString() const
{
	return String::formatted("%d.%d", m_version.v_struct.major, m_version.v_struct.minor);
}

/**
 * Convert version to a 32bit integer.
 * @return	A 32bit integer containing the complete version number.
 *			Major version is in the msb 16 bits, and the minor version in the lsb 16 bits.
 */
juce::uint32 CVersion::ToInt() const
{
	return m_version.all;
}


} // namespace dbaudio
