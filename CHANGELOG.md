# Changelog

All notable changes to VST3 Player Host are documented in this file.

## [1.3.0] - 2026-09-11

### Added

- Chained MIDI routing between inserts: outgoing MIDI events produced by a plug-in (for example a MIDI generator such as HandScaleUniverse) are now forwarded to every later insert slot, so a MIDI-only plug-in can drive a synthesiser loaded after it. Each insert still receives an independent copy of the host MIDI input plus the accumulated output of the earlier slots.
- Optional external MIDI output port: `AudioEngine::setMidiOutputDevice` opens a MIDI output device (hardware or virtual, such as loopMIDI) and streams plug-in-generated MIDI to it; pass an empty identifier to close it. Ownership is managed on the message thread and the audio thread reads the pointer atomically.
- First-process diagnostics now report how many MIDI events a plug-in emitted in its first processed block ("MIDI out events in first block: N"), making MIDI-out plug-ins easy to verify.

### Changed

- The insert chain now treats the events remaining in a plug-in's MIDI buffer after `processBlock` as the plug-in's MIDI output, matching the VST3 hosting semantics of the JUCE wrapper (the buffer contents are replaced by the plug-in's outgoing events).

## [1.2.1] - 2026-09-05

### Added

- Added an Autostart toggle to the right of Repeat. When enabled, loading any audio or MIDI file (drag and drop or file picker) or selecting a rack preset starts playback automatically.
- Rack presets now store the autostart preference and restore it on load; presets saved before this version keep the current setting.

## [1.2.0] - 2026-08-29

### Added

- Displayed the application version in the upper-right corner of the interface.

### Changed

- Reduced MIDI device enumeration frequency to keep the interface responsive.

### Fixed

- Fixed a host freeze when clearing an insert containing a VST3 plug-in with a native editor.
- Removed plug-ins from the real-time audio chain before running their shutdown sequence.
- Avoided destroying plug-ins while holding the audio processing lock.
- Made Clear Insert update its controls immediately and consistently from both the button and plug-in menu.
- Prevented quarantined VST3 editors from blocking final host process termination after Clear Insert.

## [1.1.0] - 2026-08-22

### Added

- Enabled native MP3 decoding in JUCE builds.
- Added Windows Media Foundation decoding for AAC, M4A and MP4 audio.
- Added content-based detection for MP4/AAC files incorrectly named with an `.mp3` extension.
- Added `.aac`, `.m4a` and `.mp4` support to the file picker and drag-and-drop target.
- Added drag-and-drop insert reordering. Dropping onto an occupied insert swaps both plug-ins while preserving their state and bypass settings.

### Fixed

- Replaced corrupted audio-file error text with readable diagnostic messages.
- Fixed failure to load valid MP3 files in builds where the JUCE MP3 decoder was not enabled.

## [1.0.0] - 2026-08-21

- Initial public release for Windows x64 and macOS Universal.
