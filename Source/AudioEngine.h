// Copyright (C) 2026 CVA Labs. SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <JuceHeader.h>
#include <vector>

class AudioEngine final : public juce::AudioSource
{
public:
    static constexpr int numInserts = 4;

    AudioEngine();
    ~AudioEngine() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;

    bool loadAudioFile(const juce::File&, juce::String& error);
    bool loadMidiFile(const juce::File&, juce::String& error);
    juce::File getLoadedAudioFile() const;
    void play();
    void pause();
    void stop();
    void setPosition(double seconds);
    double getPosition() const;
    double getLength() const;
    bool isPlaying() const;
    void setGain(float linearGain);
    void setLooping(bool shouldLoop);
    bool isLooping() const;
    void setLiveInput(bool shouldUseLiveInput);
    bool isUsingLiveInput() const;
    void setMonoInput(bool shouldUseMonoInput);
    bool isUsingMonoInput() const;
    float consumeInputPeak(int channel);
    float consumeOutputPeak(int channel);
    bool hasInputOverload() const;
    bool hasOutputOverload() const;
    void clearMeterOverloads();
    void setMidiKeyboardState(juce::MidiKeyboardState* state) { midiKeyboardState = state; }
    void addIncomingMidiMessage(const juce::MidiMessage& message) { midiCollector.addMessageToQueue(message); }

    void loadPlugin(int slot, const juce::File&, std::function<void(juce::String)> completion);
    void loadPlugin(int slot, const juce::PluginDescription&, std::function<void(juce::String)> completion);
    void clearPlugin(int slot);
    void setPluginBypassed(int slot, bool);
    bool isPluginBypassed(int slot) const;
    juce::String getPluginName(int slot) const;
    juce::AudioProcessor* getPlugin(int slot) const;
    juce::var createRackPreset() const;
    bool restorePluginState(int slot, const juce::String& base64State, bool bypassed);
    void swapInserts(int firstSlot, int secondSlot);

private:
    class HostPlayHead final : public juce::AudioPlayHead
    {
    public:
        explicit HostPlayHead(AudioEngine& engine) : owner(engine) {}
        juce::Optional<PositionInfo> getPosition() const override;
    private:
        AudioEngine& owner;
    };

    struct Insert
    {
        std::unique_ptr<juce::AudioPluginInstance> plugin;
        bool bypassed = false;
        bool firstProcessPending = false;
    };

    juce::AudioFormatManager formatManager;
    juce::AudioPluginFormatManager pluginFormats;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::File currentAudioFile;
    juce::AudioTransportSource transport;
    Insert inserts[numInserts];
    // Some VST3 editors install native message-loop hooks which are only safe to
    // tear down while the host itself is shutting down.  Cleared instances are
    // disconnected from audio immediately and retained until that point.
    std::vector<std::unique_ptr<juce::AudioPluginInstance>> retiredPlugins;
    mutable juce::CriticalSection processLock;
    juce::AudioBuffer<float> workBuffer;
    juce::MidiBuffer midi;
    juce::MidiMessageCollector midiCollector;
    HostPlayHead hostPlayHead { *this };
    juce::MidiKeyboardState* midiKeyboardState = nullptr;
    juce::MidiMessageSequence midiSequence;
    juce::File currentMidiFile;
    double midiPosition = 0.0, midiLength = 0.0;
    int nextMidiEvent = 0;
    bool midiFileLoaded = false, midiPlaying = false, midiLooping = false;
    double hostPositionSeconds = 0.0;
    double hostTempoBpm = 120.0;
    int hostTimeSigNumerator = 4, hostTimeSigDenominator = 4;
    bool hostIsPlaying = false;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    std::atomic<bool> useLiveInput { false };
    std::atomic<bool> useMonoInput { false };
    std::atomic<float> inputPeaks[2] {};
    std::atomic<float> outputPeaks[2] {};
    std::atomic<bool> inputOverload { false };
    std::atomic<bool> outputOverload { false };

    static void updatePeak(std::atomic<float>& destination, float value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
