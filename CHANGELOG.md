# Changelog

All notable changes to VST3 Player Host are documented in this file.

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
