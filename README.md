# VST3 Player Host

VST3 Player Host is a standalone audio and MIDI player for Windows and macOS, developed by CVA Labs with JUCE.

## Features

- WAV, MP3, AAC, M4A, MP4 audio, MID and MIDI file loading with drag and drop
- Play, pause, stop, seek and repeat
- Live audio input through ASIO on Windows and CoreAudio on macOS
- Hardware MIDI input and an on-screen MIDI keyboard
- Mono input option
- Four serial VST3 effect or instrument inserts
- Drag-and-drop insert reordering that preserves plug-in state and bypass settings
- Native plug-in editors, bypass and clear controls
- Manufacturer-grouped VST3 browser with instruments highlighted separately
- Full scan and scan-new-only modes with a persistent plug-in cache
- Named rack presets containing plug-ins, plug-in states, source settings and audio-device settings
- Stereo input/output meters with overload indicators
- Resizable interface with automatic display fitting

## Signal Flow

Audio playback or live input:

`Audio source → Insert 1 → Insert 2 → Insert 3 → Insert 4 → Output`

MIDI playback or live MIDI input:

`MIDI source → VST3 instrument → following inserts → Output`

## Presets and Settings

Windows data is stored under:

`%APPDATA%\CVA Labs\VST3 Player Host`

macOS data is stored in the corresponding JUCE application-data location under the current user account.

The application remembers the selected audio driver/device, sample rate, buffer size, channel configuration and enabled MIDI inputs.

## Build for Windows

Requirements:

- Windows 10 or later
- Visual Studio 2022 or newer with **Desktop development with C++**
- Git
- CMake 3.22 or newer

```powershell
.\build-windows.ps1
```

Expected output:

`build/VST3PlayerHost_artefacts/Release/VST3 Player Host.exe`

## Build for macOS Universal

Requirements:

- macOS 11 or later
- Xcode Command Line Tools
- Git
- CMake 3.22 or newer

```bash
chmod +x build-macos.sh
./build-macos.sh
```

The script produces `VST3-Player-Host-macOS-Universal.zip` for both Apple Silicon and Intel Macs.

## Automated Builds

GitHub Actions builds both platforms automatically after every push to `main`:

- **Build Windows x64** produces a ZIP containing `VST3 Player Host.exe`.
- **Build macOS Universal** produces an ad-hoc-signed Universal `.app` ZIP.

Open the repository's **Actions** page, select a successful run and download the artifact from the bottom of the run page.

The macOS artifact is not notarized. On first launch, macOS may require right-clicking the application and selecting **Open**.

## Licence

Copyright (C) 2026 CVA Labs.

VST3 Player Host is free software licensed under the **GNU Affero General Public License v3.0**. See [LICENSE](LICENSE).

The project uses JUCE 8 under AGPLv3 and the Steinberg ASIO SDK under GPLv3. GPLv3 and AGPLv3 code may be combined as expressly permitted by both licences; the combined application is distributed under AGPLv3. Distributions of the executable must be accompanied by access to the complete corresponding source code and build scripts.
