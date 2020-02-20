# d&b Soundscape Plug-in

Copyright (C) 2017-2020, d&b audiotechnik GmbH & Co. KG

By downloading software from this site, you agree to the terms and conditions described in the [End-User License Agreement](../EULA.md). If you do not agree with these terms and conditions, do not download the software.

## What is the d&b Soundscape Plug-in?

The d&b Soundscape Plug-in enables a Digital Audio Workstation (DAW), AVID S6L Console, or any other Plug-in host to control the basic En-Scene and En-Space parameters of any desired sound object on the DS100 platform using the OSC protocol. It is designed to work with d&b Soundscape and the d&b DS100.

For more information about the d&b Soundscape system, go to www.dbaudio.com and www.dbsoundscape.com.

## Building the Plug-in

The d&b Soundscape Plug-in was developed using [JUCE](https://github.com/WeAreROLI/JUCE), a cross-platform C++ application framework. Download and install JUCE v5.4.5 and use the Projucer tool to open the provided SoundscapePlugin.jucer configuration file. This file defines export configurations for Microsoft Visual Studio 2017 and Apple Xcode.

### VST3 SDK

To compile the Soundscape Plug-in as VST3, please go to steinberg.net to obtain the required VST3 SDK. Make sure you enter the path to the VST3 SDK correctly in the Projucer's settings.

### AAX SDK

To compile the Soundscape Plug-in as AAX, please go to developer.avid.com to obtain the required AAX SDK. Make sure you enter the path to the AAX SDK correctly in the Projucer's settings.

### AU SDK

The Core Audio SDK required to compile the Soundscape Plug-in as an Audio Units Plug-in (AU) is included in Xcode, which means no manual download is required.

### Windows

To build the Soundscape Plug-in on Windows, first install Visual Studio 2017. Open the SoundscapePlugin.jucer file from this repository using JUCE's Projucer tool. In Projucer, select the exporter target "Visual Studio 2017" and click on "Save and open in IDE". This generates or updates the required build files and opens Visual Studio 2017, in which you can build the Plug-in.

## macOS

To build the Soundscape Plug-in on macOS, first install Xcode. Open the SoundscapePlugin.jucer file from this repository using JUCE's Projucer tool. In Projucer, select the exporter target "Xcode" and click on "Save and open in IDE". This generates or updates the required build files and opens Xcode, in which you can build the Plug-in.
