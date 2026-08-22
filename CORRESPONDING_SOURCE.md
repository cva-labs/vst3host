# Corresponding Source Manifest

This repository contains the CVA Labs source code and build scripts for VST3 Player Host.

Release builds use these exact third-party revisions:

- JUCE 8.0.10: `3af3ce009f6a02f6fa651008fffb5b41743a9fab`
- Steinberg ASIO SDK: `496a0765b8bb9c26f764f22f9a9712a937177db2`

The release asset named `VST3-Player-Host-v1.1.0-Complete-Corresponding-Source.zip` contains:

- The complete VST3 Player Host source and build scripts.
- The complete JUCE source at the revision above.
- The complete ASIO SDK source at the revision above.
- Licence texts and this manifest.

The included dependencies can be used without network downloads by configuring CMake with:

```text
-DFETCHCONTENT_SOURCE_DIR_JUCE=<archive>/ThirdParty/JUCE
-DFETCHCONTENT_SOURCE_DIR_ASIOSDK=<archive>/ThirdParty/ASIOSDK
```

The preferred form for modification is the uncompressed source tree contained in that archive.
