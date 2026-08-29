# Changelog

All notable changes to VST3 Player Host are documented in this file.

## [1.1.1] - 2026-08-29

### Fixed

- Clearing an insert now also resets its bypass state and immediately removes the bypass check mark.
- Clearing an insert now resets its pending first-process state.

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
