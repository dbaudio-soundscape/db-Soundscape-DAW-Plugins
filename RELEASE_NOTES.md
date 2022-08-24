# d&b Soundscape Plug-in: Release notes

Copyright (C) 2017-2022, d&b audiotechnik GmbH & Co. KG


## V2.8.4

### Features
* MacOS binaries are now notarized.

---

## V2.8.3

### Bugfixes
* Plugin input numbers no longer get changed when loading a second project on a new tab in the same DAW.

---

## V2.8.1

### Features
* When opening the multi-source surface, the selected mapping is now automatically set to the active Plug-In's mapping.

### Bugfixes
* The user's current IP address is no longer overwritten when loading showfiles or snapshots (Avid VENUE S6L) or recalling presets (DAW).
* Avid VENUE S6L: The user's preferred Plug-In window size no longer gets reset when switching channels.

---

## V2.8.0

### Features
* First open source release.

### Bugfixes
* On Pro Tools, the Input number will now correctly increase automatically as Plug-in instances are added to the project.

---

## V2.7.1

### Features
* Implemented multi-source surface for the overview window.

### Bugfixes
* Fixed GUI stability issues.

---

## V2.6.1

### Features
* Added support for Touch automation via OSC.
* Added Info/License window.

---

## V2.6.0

### Features

* Plug-in now also available as VST3.
* New automation parameter **«OSCBypass»**.
* Tx and Rx modes are no longer mutually exclusive.

### Bugfixes

* Setting delay mode to **«Tight»** in R1 no longer shows as **«Full»** on the Plug-in.
* On Pro Tools, the IP Address no longer resets when adding a new Plug-in instance to the project.

### Known issues

* Touch automation via OSC may not work properly on some hosts

---

## V2.3.0

* Initial release
