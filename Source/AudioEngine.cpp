// Copyright (C) 2026 CVA Labs. SPDX-License-Identifier: AGPL-3.0-only
#include "AudioEngine.h"

#if JUCE_WINDOWS
 #include <mfapi.h>
 #include <mfidl.h>
 #include <mfreadwrite.h>
 #include <wrl/client.h>
#endif

namespace
{
#if JUCE_WINDOWS
class MediaFoundationAudioReader final : public juce::AudioFormatReader
{
public:
    explicit MediaFoundationAudioReader(const juce::File& source)
        : juce::AudioFormatReader(nullptr, "Windows Media Foundation")
    {
        const auto comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool uninitialiseCom = SUCCEEDED(comResult);

        if (FAILED(MFStartup(MF_VERSION)))
        {
            if (uninitialiseCom) CoUninitialize();
            return;
        }

        Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader;
        auto pathToOpen = source;
        std::unique_ptr<juce::TemporaryFile> correctedExtension;

        // Media Foundation sometimes chooses its byte-stream handler from the
        // extension. Give a disguised MP4/AAC file a truthful temporary suffix.
        juce::FileInputStream headerStream(source);
        char header[12] {};
        const bool isIsoMedia = headerStream.openedOk()
                             && headerStream.read(header, sizeof(header)) == sizeof(header)
                             && std::memcmp(header + 4, "ftyp", 4) == 0;
        if (isIsoMedia && !source.hasFileExtension("m4a;mp4;aac"))
        {
            correctedExtension = std::make_unique<juce::TemporaryFile>(".m4a");
            if (source.copyFileTo(correctedExtension->getFile()))
                pathToOpen = correctedExtension->getFile();
        }

        HRESULT result = MFCreateSourceReaderFromURL(pathToOpen.getFullPathName().toWideCharPointer(),
                                                     nullptr, sourceReader.GetAddressOf());
        if (SUCCEEDED(result))
        {
            Microsoft::WRL::ComPtr<IMFMediaType> requestedType;
            result = MFCreateMediaType(requestedType.GetAddressOf());
            if (SUCCEEDED(result)) result = requestedType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            if (SUCCEEDED(result)) result = requestedType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
            if (SUCCEEDED(result))
                result = sourceReader->SetCurrentMediaType((DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                                           nullptr, requestedType.Get());
        }

        Microsoft::WRL::ComPtr<IMFMediaType> actualType;
        if (SUCCEEDED(result))
            result = sourceReader->GetCurrentMediaType((DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                                       actualType.GetAddressOf());

        UINT32 rate = 0, channels = 0, bitDepth = 0;
        if (SUCCEEDED(result)) result = MFGetAttributeUINT32(actualType.Get(), MF_MT_AUDIO_SAMPLES_PER_SECOND, 0) != 0 ? S_OK : E_FAIL;
        if (SUCCEEDED(result)) rate = MFGetAttributeUINT32(actualType.Get(), MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
        if (SUCCEEDED(result)) channels = MFGetAttributeUINT32(actualType.Get(), MF_MT_AUDIO_NUM_CHANNELS, 0);
        if (SUCCEEDED(result)) bitDepth = MFGetAttributeUINT32(actualType.Get(), MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        if (rate == 0 || channels == 0 || bitDepth != 16) result = E_FAIL;

        while (SUCCEEDED(result))
        {
            DWORD flags = 0;
            Microsoft::WRL::ComPtr<IMFSample> sample;
            result = sourceReader->ReadSample((DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr,
                                              &flags, nullptr, sample.GetAddressOf());
            if (FAILED(result) || (flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) break;
            if (sample == nullptr) continue;

            Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
            result = sample->ConvertToContiguousBuffer(buffer.GetAddressOf());
            BYTE* bytes = nullptr;
            DWORD byteCount = 0;
            if (SUCCEEDED(result)) result = buffer->Lock(&bytes, nullptr, &byteCount);
            if (SUCCEEDED(result))
            {
                const auto oldSize = pcm.size();
                pcm.resize(oldSize + byteCount / sizeof(int16_t));
                std::memcpy(pcm.data() + oldSize, bytes, byteCount);
                buffer->Unlock();
            }
        }

        MFShutdown();
        if (uninitialiseCom) CoUninitialize();

        if (rate > 0 && channels > 0 && !pcm.empty())
        {
            sampleRate = rate;
            numChannels = channels;
            bitsPerSample = 16;
            usesFloatingPointData = false;
            lengthInSamples = (juce::int64) (pcm.size() / channels);
        }
    }

    bool isValid() const { return sampleRate > 0 && numChannels > 0 && lengthInSamples > 0; }

    bool readSamples(int* const* destinations, int destinationChannels, int destinationOffset,
                     juce::int64 startSample, int samplesToRead) override
    {
        clearSamplesBeyondAvailableLength(destinations, destinationChannels, destinationOffset,
                                          startSample, samplesToRead, lengthInSamples);
        const int available = (int) juce::jlimit<juce::int64>(0, samplesToRead,
                                                              lengthInSamples - startSample);
        for (int channel = 0; channel < destinationChannels; ++channel)
        {
            auto* destination = destinations[channel];
            if (destination == nullptr) continue;
            destination += destinationOffset;
            if (channel >= (int) numChannels)
            {
                std::fill(destination, destination + available, 0);
                continue;
            }
            for (int sample = 0; sample < available; ++sample)
                destination[sample] = (int) ((juce::uint32) pcm[(size_t) (startSample + sample) * numChannels
                                                               + (size_t) channel] << 16);
        }
        return true;
    }

private:
    std::vector<int16_t> pcm;
};

std::unique_ptr<juce::AudioFormatReader> createMediaFoundationReader(const juce::File& file)
{
    auto reader = std::make_unique<MediaFoundationAudioReader>(file);
    return reader->isValid() ? std::unique_ptr<juce::AudioFormatReader>(reader.release()) : nullptr;
}
#endif

void writePluginDiagnostic(const juce::String& message)
{
    const auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("CVA Labs").getChildFile("VST3 Player Host")
        .getChildFile("plugin-diagnostic.log");
    file.getParentDirectory().createDirectory();
    file.appendText(juce::Time::getCurrentTime().toString(true, true, true, true)
                    + " | " + message + "\r\n", false, false, "UTF-8");
}
}

juce::Optional<juce::AudioPlayHead::PositionInfo> AudioEngine::HostPlayHead::getPosition() const
{
    juce::AudioPlayHead::PositionInfo position;
    const double bpm = juce::jmax(1.0, owner.hostTempoBpm);
    const double ppq = owner.hostPositionSeconds * bpm / 60.0;
    const double quartersPerBar = owner.hostTimeSigNumerator * 4.0
                                  / juce::jmax(1, owner.hostTimeSigDenominator);
    position.setTimeInSeconds(owner.hostPositionSeconds);
    position.setTimeInSamples((int64_t) std::llround(owner.hostPositionSeconds
                                                     * owner.currentSampleRate));
    position.setBpm(bpm);
    juce::AudioPlayHead::TimeSignature timeSignature;
    timeSignature.numerator = owner.hostTimeSigNumerator;
    timeSignature.denominator = owner.hostTimeSigDenominator;
    position.setTimeSignature(timeSignature);
    position.setPpqPosition(ppq);
    position.setPpqPositionOfLastBarStart(std::floor(ppq / quartersPerBar) * quartersPerBar);
    position.setBarCount((int64_t) std::floor(ppq / quartersPerBar));
    position.setIsPlaying(owner.hostIsPlaying);
    position.setIsRecording(false);
    position.setIsLooping(owner.midiLooping && owner.midiFileLoaded);
    if (owner.midiLooping && owner.midiFileLoaded)
    {
        juce::AudioPlayHead::LoopPoints loopPoints;
        loopPoints.ppqStart = 0.0;
        loopPoints.ppqEnd = owner.midiLength * bpm / 60.0;
        position.setLoopPoints(loopPoints);
    }
    return position;
}

AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();
    pluginFormats.addDefaultFormats();
}

AudioEngine::~AudioEngine()
{
    transport.setSource(nullptr);
    releaseResources();
}

void AudioEngine::prepareToPlay(int blockSize, double sampleRate)
{
    const juce::ScopedLock lock(processLock);
    currentBlockSize = blockSize;
    currentSampleRate = sampleRate;
    midiCollector.reset(sampleRate);
    transport.prepareToPlay(blockSize, sampleRate);
    workBuffer.setSize(2, blockSize);

    for (auto& insert : inserts)
        if (insert.plugin != nullptr)
            insert.plugin->prepareToPlay(sampleRate, blockSize);
}

void AudioEngine::releaseResources()
{
    const juce::ScopedLock lock(processLock);
    transport.releaseResources();
    for (auto& insert : inserts)
        if (insert.plugin != nullptr)
            insert.plugin->releaseResources();
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    const juce::ScopedLock lock(processLock);
    if (!useLiveInput.load())
    {
        info.clearActiveBufferRegion();
        transport.getNextAudioBlock(info);
    }
    else if (useMonoInput.load() && info.buffer->getNumChannels() >= 2)
    {
        auto* left = info.buffer->getWritePointer(0, info.startSample);
        auto* right = info.buffer->getWritePointer(1, info.startSample);

        for (int sample = 0; sample < info.numSamples; ++sample)
        {
            const float mono = 0.5f * (left[sample] + right[sample]);
            left[sample] = mono;
            right[sample] = mono;
        }
    }

    for (int channel = 0; channel < juce::jmin(2, info.buffer->getNumChannels()); ++channel)
    {
        const float peak = info.buffer->getMagnitude(channel, info.startSample, info.numSamples);
        updatePeak(inputPeaks[channel], peak);
        if (peak >= 1.0f) inputOverload.store(true);
    }
    midi.clear();
    midiCollector.removeNextBlockOfMessages(midi, info.numSamples);
    if (midiKeyboardState != nullptr)
        midiKeyboardState->processNextMidiBuffer(midi, 0, info.numSamples, true);

    bool hasHeldLiveNote = false;
    if (midiKeyboardState != nullptr)
        for (int note = 0; note < 128 && !hasHeldLiveNote; ++note)
            hasHeldLiveNote = midiKeyboardState->isNoteOnForChannels(0xffff, note);

    if (midiFileLoaded && midiPlaying)
    {
        int blockOffset = 0;
        int samplesRemaining = info.numSamples;

        while (samplesRemaining > 0 && midiPlaying)
        {
            const double secondsToEnd = juce::jmax(0.0, midiLength - midiPosition);
            const int samplesToEnd = juce::jmax(1, (int) std::ceil(secondsToEnd * currentSampleRate));
            const int segmentSamples = juce::jmin(samplesRemaining, samplesToEnd);
            const bool reachesEnd = segmentSamples >= samplesToEnd;
            const double segmentEnd = reachesEnd
                                        ? midiLength
                                        : midiPosition + segmentSamples / currentSampleRate;

            while (nextMidiEvent < midiSequence.getNumEvents())
            {
                const auto* event = midiSequence.getEventPointer(nextMidiEvent);
                const double eventTime = event->message.getTimeStamp();
                // Include events exactly on the loop boundary (commonly the final note-off).
                if (eventTime > segmentEnd || (!reachesEnd && eventTime >= segmentEnd)) break;
                if (eventTime >= midiPosition)
                {
                    // A note held on the hardware/on-screen keyboard owns its Note Off.
                    // Do not let an overlapping MIDI-file note of the same pitch cut it short.
                    if (event->message.isNoteOff()
                        && midiKeyboardState != nullptr
                        && midiKeyboardState->isNoteOnForChannels(0xffff,
                                                                  event->message.getNoteNumber()))
                    {
                        ++nextMidiEvent;
                        continue;
                    }
                    const int eventOffset = blockOffset
                        + juce::jlimit(0, segmentSamples - 1,
                                      juce::roundToInt((eventTime - midiPosition) * currentSampleRate));
                    midi.addEvent(event->message, eventOffset);
                }
                ++nextMidiEvent;
            }

            midiPosition = segmentEnd;
            blockOffset += segmentSamples;
            samplesRemaining -= segmentSamples;

            if (!reachesEnd) continue;

            if (midiLooping && midiLength > 0.0)
            {
                midiPosition = 0.0;
                nextMidiEvent = 0;
            }
            else
            {
                midiPlaying = false;
                midiPosition = midiLength;
            }
        }
    }

    hostIsPlaying = midiPlaying || transport.isPlaying() || hasHeldLiveNote;
    if (midiFileLoaded)
        hostPositionSeconds = midiPosition;
    else if (readerSource != nullptr)
        hostPositionSeconds = transport.getCurrentPosition();
    else if (hostIsPlaying)
        hostPositionSeconds += info.numSamples / currentSampleRate;

    for (auto& insert : inserts)
    {
        if (insert.plugin == nullptr || insert.bypassed)
            continue;

        // Plugins are allowed to consume or rewrite their MIDI buffer. Give every insert
        // an independent copy so an earlier plugin cannot remove a held note for a later one.
        juce::MidiBuffer pluginMidi(midi);
        const int pluginChannels = juce::jmax(2,
                                              insert.plugin->getTotalNumInputChannels(),
                                              insert.plugin->getTotalNumOutputChannels());
        workBuffer.setSize(pluginChannels, info.numSamples, false, false, true);
        workBuffer.clear();

        const int channelsToCopy = juce::jmin(2, info.buffer->getNumChannels());
        for (int channel = 0; channel < channelsToCopy; ++channel)
            workBuffer.copyFrom(channel, 0, *info.buffer, channel,
                                info.startSample, info.numSamples);

        if (insert.firstProcessPending)
            writePluginDiagnostic(insert.plugin->getName() + " | first process begin | in="
                                  + juce::String(insert.plugin->getTotalNumInputChannels())
                                  + " out=" + juce::String(insert.plugin->getTotalNumOutputChannels())
                                  + " buffer=" + juce::String(pluginChannels));
        insert.plugin->processBlock(workBuffer, pluginMidi);
        if (insert.firstProcessPending)
        {
            writePluginDiagnostic(insert.plugin->getName() + " | first process completed");
            insert.firstProcessPending = false;
        }

        for (int channel = 0; channel < channelsToCopy; ++channel)
            info.buffer->copyFrom(channel, info.startSample, workBuffer,
                                  channel, 0, info.numSamples);
    }


    for (int channel = 0; channel < juce::jmin(2, info.buffer->getNumChannels()); ++channel)
    {
        const float peak = info.buffer->getMagnitude(channel, info.startSample, info.numSamples);
        updatePeak(outputPeaks[channel], peak);
        if (peak >= 1.0f) outputOverload.store(true);
    }
}

bool AudioEngine::loadAudioFile(const juce::File& file, juce::String& error)
{
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));

#if JUCE_USE_MP3AUDIOFORMAT
    // Retry MP3 files with the decoder directly. This also handles files whose
    // metadata/header prevents AudioFormatManager from identifying the format.
    if (reader == nullptr && file.hasFileExtension("mp3"))
    {
        juce::MP3AudioFormat mp3Format;
        reader.reset(mp3Format.createReaderFor(new juce::FileInputStream(file), true));
    }
#endif

#if JUCE_WINDOWS
    if (reader == nullptr)
        reader = createMediaFoundationReader(file);
#endif

    if (reader == nullptr)
    {
        error = file.existsAsFile()
                    ? "Could not decode this audio file. It may be damaged or not contain valid MP3/WAV audio."
                    : "The selected audio file no longer exists or cannot be accessed.";
        return false;
    }

    auto nextSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    const juce::ScopedLock lock(processLock);
    transport.stop();
    transport.setSource(nullptr);
    readerSource = std::move(nextSource);
    currentAudioFile = file;
    midiFileLoaded = false;
    currentMidiFile = juce::File();
    hostTempoBpm = 120.0;
    hostTimeSigNumerator = 4;
    hostTimeSigDenominator = 4;
    transport.setSource(readerSource.get(), 0, nullptr, readerSource->getAudioFormatReader()->sampleRate);
    transport.setPosition(0.0);
    return true;
}

bool AudioEngine::loadMidiFile(const juce::File& file, juce::String& error)
{
    juce::FileInputStream input(file);
    juce::MidiFile parsed;
    if (!input.openedOk() || !parsed.readFrom(input))
    {
        error = "Could not read this MIDI file. It may be damaged or invalid.";
        return false;
    }
    double musicalEndTicks = 0.0;
    double initialTempoBpm = 120.0;
    int initialTimeSigNumerator = 4, initialTimeSigDenominator = 4;
    struct TimeSignature { double tick; int numerator; int denominator; };
    std::vector<TimeSignature> signatures { { 0.0, 4, 4 } };
    for (int track = 0; track < parsed.getNumTracks(); ++track)
    {
        if (const auto* sequence = parsed.getTrack(track))
        {
            for (int eventIndex = 0; eventIndex < sequence->getNumEvents(); ++eventIndex)
            {
                const auto& message = sequence->getEventPointer(eventIndex)->message;
                if (message.getChannel() > 0)
                    musicalEndTicks = juce::jmax(musicalEndTicks, message.getTimeStamp());
                if (message.isTimeSignatureMetaEvent())
                {
                    int numerator = 4, denominator = 4;
                    message.getTimeSignatureInfo(numerator, denominator);
                    if (numerator > 0 && denominator > 0)
                    {
                        signatures.push_back({ message.getTimeStamp(), numerator, denominator });
                        if (message.getTimeStamp() <= 0.0)
                        {
                            initialTimeSigNumerator = numerator;
                            initialTimeSigDenominator = denominator;
                        }
                    }
                }
                if (message.isTempoMetaEvent() && message.getTimeStamp() <= 0.0)
                    initialTempoBpm = 60.0 / message.getTempoSecondsPerQuarterNote();
            }
        }
    }

    double loopEndTicks = musicalEndTicks;
    const int ticksPerQuarter = parsed.getTimeFormat();
    if (ticksPerQuarter > 0)
    {
        std::sort(signatures.begin(), signatures.end(),
                  [](const auto& a, const auto& b) { return a.tick < b.tick; });
        auto activeSignature = signatures.front();
        for (const auto& signature : signatures)
            if (signature.tick <= musicalEndTicks) activeSignature = signature;

        const double ticksPerBar = ticksPerQuarter * 4.0 * activeSignature.numerator
                                   / activeSignature.denominator;
        const double barsFromSignature = (musicalEndTicks - activeSignature.tick) / ticksPerBar;
        const double completeBars = juce::jmax(1.0, std::ceil(barsFromSignature - 1.0e-9));
        loopEndTicks = activeSignature.tick + completeBars * ticksPerBar;
    }

    // Convert the calculated bar boundary with the MIDI file's complete tempo map.
    const int originalTrackCount = parsed.getNumTracks();
    juce::MidiMessageSequence loopBoundaryTrack;
    auto boundary = juce::MidiMessage::textMetaEvent(0, "CVA_LOOP_END");
    boundary.setTimeStamp(loopEndTicks);
    loopBoundaryTrack.addEvent(boundary);
    parsed.addTrack(loopBoundaryTrack);
    parsed.convertTimestampTicksToSeconds();

    double calculatedLoopLength = 0.0;
    if (const auto* boundaryTrack = parsed.getTrack(originalTrackCount))
        if (boundaryTrack->getNumEvents() > 0)
            calculatedLoopLength = boundaryTrack->getEventPointer(0)->message.getTimeStamp();

    juce::MidiMessageSequence combined;
    for (int track = 0; track < originalTrackCount; ++track)
        if (const auto* sequence = parsed.getTrack(track)) combined.addSequence(*sequence, 0.0);
    combined.updateMatchedPairs();

    const juce::ScopedLock lock(processLock);
    transport.stop();
    transport.setSource(nullptr);
    readerSource.reset();
    currentAudioFile = juce::File();
    midiSequence = std::move(combined);
    midiLength = juce::jmax(midiSequence.getEndTime(), calculatedLoopLength);
    hostTempoBpm = initialTempoBpm;
    hostTimeSigNumerator = initialTimeSigNumerator;
    hostTimeSigDenominator = initialTimeSigDenominator;
    midiPosition = 0.0;
    nextMidiEvent = 0;
    midiPlaying = false;
    midiFileLoaded = true;
    currentMidiFile = file;
    return true;
}

juce::File AudioEngine::getLoadedAudioFile() const
{
    const juce::ScopedLock lock(processLock);
    return currentAudioFile;
}

void AudioEngine::play()                         { if (midiFileLoaded) midiPlaying = true; else transport.start(); }
void AudioEngine::pause()                        { midiPlaying = false; transport.stop(); }
void AudioEngine::stop()                         { midiPlaying = false; midiPosition = 0.0; nextMidiEvent = 0; transport.stop(); transport.setPosition(0.0); }
void AudioEngine::setPosition(double seconds)    { if (midiFileLoaded) { midiPosition = juce::jlimit(0.0, midiLength, seconds); nextMidiEvent = midiSequence.getNextIndexAtTime(midiPosition); } else transport.setPosition(juce::jlimit(0.0, getLength(), seconds)); }
double AudioEngine::getPosition() const           { return midiFileLoaded ? midiPosition : transport.getCurrentPosition(); }
double AudioEngine::getLength() const             { return midiFileLoaded ? midiLength : transport.getLengthInSeconds(); }
bool AudioEngine::isPlaying() const               { return midiFileLoaded ? midiPlaying : transport.isPlaying(); }
void AudioEngine::setGain(float gain)             { transport.setGain(gain); }
void AudioEngine::setLooping(bool shouldLoop)
{
    const juce::ScopedLock lock(processLock);
    if (readerSource != nullptr)
        readerSource->setLooping(shouldLoop);
    midiLooping = shouldLoop;
}

bool AudioEngine::isLooping() const
{
    const juce::ScopedLock lock(processLock);
    return midiFileLoaded ? midiLooping : (readerSource != nullptr && readerSource->isLooping());
}

void AudioEngine::setLiveInput(bool shouldUseLiveInput)
{
    useLiveInput.store(shouldUseLiveInput);
    if (shouldUseLiveInput)
        transport.stop();
}

bool AudioEngine::isUsingLiveInput() const { return useLiveInput.load(); }

void AudioEngine::setMonoInput(bool shouldUseMonoInput) { useMonoInput.store(shouldUseMonoInput); }
bool AudioEngine::isUsingMonoInput() const { return useMonoInput.load(); }

void AudioEngine::updatePeak(std::atomic<float>& destination, float value)
{
    auto previous = destination.load();
    while (value > previous && !destination.compare_exchange_weak(previous, value)) {}
}

float AudioEngine::consumeInputPeak(int channel)
{
    return juce::isPositiveAndBelow(channel, 2) ? inputPeaks[channel].exchange(0.0f) : 0.0f;
}

float AudioEngine::consumeOutputPeak(int channel)
{
    return juce::isPositiveAndBelow(channel, 2) ? outputPeaks[channel].exchange(0.0f) : 0.0f;
}

bool AudioEngine::hasInputOverload() const { return inputOverload.load(); }
bool AudioEngine::hasOutputOverload() const { return outputOverload.load(); }

void AudioEngine::clearMeterOverloads()
{
    inputOverload.store(false);
    outputOverload.store(false);
}

void AudioEngine::loadPlugin(int slot, const juce::File& file,
                             std::function<void(juce::String)> completion)
{
    if (!juce::isPositiveAndBelow(slot, numInserts))
        return;

    juce::OwnedArray<juce::PluginDescription> descriptions;
    for (auto* format : pluginFormats.getFormats())
        format->findAllTypesForFile(descriptions, file.getFullPathName());

    if (descriptions.isEmpty())
    {
        completion("Δεν βρέθηκε έγκυρο VST3 plugin σε αυτό το αρχείο.");
        return;
    }

    loadPlugin(slot, *descriptions.getFirst(), std::move(completion));
}

void AudioEngine::loadPlugin(int slot, const juce::PluginDescription& description,
                             std::function<void(juce::String)> completion)
{
    if (!juce::isPositiveAndBelow(slot, numInserts))
        return;

    const bool isInstrument = description.isInstrument;
    writePluginDiagnostic(description.name + " | async create requested");
    pluginFormats.createPluginInstanceAsync(
        description, currentSampleRate, currentBlockSize,
        [this, slot, isInstrument, completion = std::move(completion)]
        (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
        {
            if (instance == nullptr)
            {
                completion(error.isNotEmpty() ? error : "Αποτυχία φόρτωσης plugin.");
                return;
            }

            writePluginDiagnostic(instance->getName() + " | instance created");
            instance->setPlayHead(&hostPlayHead);
            auto layout = instance->getBusesLayout();
            for (auto& bus : layout.inputBuses) bus = juce::AudioChannelSet::disabled();
            for (auto& bus : layout.outputBuses) bus = juce::AudioChannelSet::disabled();
            if (!isInstrument && !layout.inputBuses.isEmpty())
                layout.inputBuses.getReference(0) = juce::AudioChannelSet::stereo();
            if (!layout.outputBuses.isEmpty())
                layout.outputBuses.getReference(0) = juce::AudioChannelSet::stereo();

            if (isInstrument)
            {
                // Multi-output instruments such as Groove Agent must retain their native
                // output-bus topology. Some accept a forced stereo layout but then crash
                // internally on their first processing call.
                writePluginDiagnostic(instance->getName() + " | retaining native instrument buses | in="
                                      + juce::String(instance->getTotalNumInputChannels())
                                      + " out=" + juce::String(instance->getTotalNumOutputChannels()));
                instance->setRateAndBufferSizeDetails(currentSampleRate, currentBlockSize);
            }
            else
            {
                writePluginDiagnostic(instance->getName() + " | setting stereo effect bus layout");
                if (!instance->setBusesLayout(layout))
                    instance->setPlayConfigDetails(2, 2, currentSampleRate, currentBlockSize);
                else
                    instance->setRateAndBufferSizeDetails(currentSampleRate, currentBlockSize);
            }
            writePluginDiagnostic(instance->getName() + " | prepare begin");
            instance->prepareToPlay(currentSampleRate, currentBlockSize);
            writePluginDiagnostic(instance->getName() + " | prepare completed | in="
                                  + juce::String(instance->getTotalNumInputChannels())
                                  + " out=" + juce::String(instance->getTotalNumOutputChannels()));
            std::unique_ptr<juce::AudioPluginInstance> retiredPlugin;
            {
                const juce::ScopedLock lock(processLock);
                retiredPlugin = std::move(inserts[slot].plugin);
                inserts[slot].plugin = std::move(instance);
                inserts[slot].bypassed = false;
                inserts[slot].firstProcessPending = true;
            }

            // Never call into or destroy a hosted plugin while holding processLock.
            // A VST3 teardown may wait for its audio-side work to finish; holding the
            // lock here would prevent the audio callback from reaching that point.
            if (retiredPlugin != nullptr)
            {
                retiredPlugin->setPlayHead(nullptr);
                retiredPlugin->suspendProcessing(true);
                retiredPlugin->releaseResources();
                retiredPlugin.reset();
            }
            completion({});
        });
}

void AudioEngine::clearPlugin(int slot)
{
    if (!juce::isPositiveAndBelow(slot, numInserts)) return;

    writePluginDiagnostic("CLEAR slot " + juce::String(slot + 1) + " | engine begin");
    std::unique_ptr<juce::AudioPluginInstance> retiredPlugin;
    {
        writePluginDiagnostic("CLEAR slot " + juce::String(slot + 1) + " | waiting processLock");
        const juce::ScopedLock lock(processLock);
        writePluginDiagnostic("CLEAR slot " + juce::String(slot + 1) + " | processLock acquired");
        retiredPlugin = std::move(inserts[slot].plugin);
        inserts[slot].bypassed = false;
        inserts[slot].firstProcessPending = false;
    }
    writePluginDiagnostic("CLEAR slot " + juce::String(slot + 1) + " | processLock released");

    if (retiredPlugin != nullptr)
    {
        writePluginDiagnostic("CLEAR | setPlayHead null begin");
        retiredPlugin->setPlayHead(nullptr);
        writePluginDiagnostic("CLEAR | setPlayHead null done; suspend begin");
        retiredPlugin->suspendProcessing(true);
        writePluginDiagnostic("CLEAR | suspend done; releaseResources begin");
        retiredPlugin->releaseResources();
        writePluginDiagnostic("CLEAR | releaseResources done; retain until host shutdown begin");
        retiredPlugins.push_back(std::move(retiredPlugin));
        writePluginDiagnostic("CLEAR | retained until host shutdown");
    }
    writePluginDiagnostic("CLEAR slot " + juce::String(slot + 1) + " | engine complete");
}

void AudioEngine::setPluginBypassed(int slot, bool bypassed)
{
    if (juce::isPositiveAndBelow(slot, numInserts))
    {
        const juce::ScopedLock lock(processLock);
        inserts[slot].bypassed = bypassed;
    }
}

bool AudioEngine::isPluginBypassed(int slot) const
{
    const juce::ScopedLock lock(processLock);
    return juce::isPositiveAndBelow(slot, numInserts) && inserts[slot].bypassed;
}

juce::String AudioEngine::getPluginName(int slot) const
{
    const juce::ScopedLock lock(processLock);
    if (!juce::isPositiveAndBelow(slot, numInserts) || inserts[slot].plugin == nullptr) return "Empty";
    return inserts[slot].plugin->getName();
}

juce::AudioProcessor* AudioEngine::getPlugin(int slot) const
{
    const juce::ScopedLock lock(processLock);
    return juce::isPositiveAndBelow(slot, numInserts) ? inserts[slot].plugin.get() : nullptr;
}

juce::var AudioEngine::createRackPreset() const
{
    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty("format", "VST3PlayerHostRack");
    root->setProperty("version", 1);

    juce::Array<juce::var> slots;
    const juce::ScopedLock lock(processLock);

    for (const auto& insert : inserts)
    {
        auto slot = std::make_unique<juce::DynamicObject>();
        if (insert.plugin != nullptr)
        {
            const auto description = insert.plugin->getPluginDescription();
            juce::MemoryBlock state;
            insert.plugin->getStateInformation(state);
            slot->setProperty("path", description.fileOrIdentifier);
            slot->setProperty("name", insert.plugin->getName());
            slot->setProperty("state", state.toBase64Encoding());
            slot->setProperty("bypassed", insert.bypassed);
        }
        else
        {
            slot->setProperty("path", "");
            slot->setProperty("name", "");
            slot->setProperty("state", "");
            slot->setProperty("bypassed", false);
        }
        slots.add(juce::var(slot.release()));
    }

    root->setProperty("slots", juce::var(slots));
    return juce::var(root.release());
}

bool AudioEngine::restorePluginState(int slot, const juce::String& base64State, bool bypassed)
{
    if (!juce::isPositiveAndBelow(slot, numInserts)) return false;
    const juce::ScopedLock lock(processLock);
    auto* plugin = inserts[slot].plugin.get();
    if (plugin == nullptr) return false;

    juce::MemoryBlock state;
    if (base64State.isNotEmpty() && !state.fromBase64Encoding(base64State)) return false;
    if (state.getSize() > 0)
        plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    inserts[slot].bypassed = bypassed;
    return true;
}

void AudioEngine::swapInserts(int firstSlot, int secondSlot)
{
    if (!juce::isPositiveAndBelow(firstSlot, numInserts)
        || !juce::isPositiveAndBelow(secondSlot, numInserts)
        || firstSlot == secondSlot)
        return;

    const juce::ScopedLock lock(processLock);
    std::swap(inserts[firstSlot], inserts[secondSlot]);
}
