#pragma once

#include <JuceHeader.h>
#include "AudioEngine.h"
#include <vector>

class MainComponent final : public juce::AudioAppComponent,
                            public juce::FileDragAndDropTarget,
                            private juce::MidiInputCallback,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int, double) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
    void releaseResources() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;

private:
    class PluginWindow;
    class StereoMeter;
    class PluginScanThread;
    class PluginFolderCheckThread;
    class PluginMenuLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        juce::Font getPopupMenuFont() override { return juce::FontOptions(14.0f).withName("Bahnschrift"); }
        juce::Font getTextButtonFont(juce::TextButton&, int height) override
        { return juce::FontOptions(juce::jlimit(7.0f, 20.0f, height * 0.38f)).withName("Bahnschrift"); }
        juce::Font getComboBoxFont(juce::ComboBox& box) override
        { return juce::FontOptions(juce::jmax(7.0f, box.getHeight() * 0.34f)).withName("Bahnschrift"); }
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                                  bool highlighted, bool down) override;
        void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
        void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int,
                          juce::ComboBox&) override;
        void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                              juce::Slider::SliderStyle, juce::Slider&) override;
    };
    void timerCallback() override;
    void chooseAudioFile();
    void openAudioFile(const juce::File&);
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;
    void syncMidiInputs();
    void saveAudioSettings() const;
    static juce::File getAudioSettingsFile();
    void showPluginMenu(int slot);
    void loadSelectedPlugin(int slot, int pluginIndex);
    void startPluginScan(bool newOnly = false);
    void checkForNewPluginContainers();
    void finishPluginScan(std::vector<juce::PluginDescription>, bool mergeWithExisting = false,
                          bool updateInventory = true);
    bool loadPluginScanCache();
    void savePluginScanCache() const;
    static juce::File getPluginScanCacheFile();
    static juce::File getPluginInventoryFile();
    void saveCurrentPluginInventory() const;
    void showPluginEditor(int slot);
    void showAudioSettings();
    void saveRackPreset();
    void deleteRackPreset();
    void loadRackPreset(const juce::var& rack);
    juce::var createFullPreset() const;
    void restoreSourceFromPreset(const juce::var& rack);
    void reloadPresetLibrary();
    juce::var readPresetLibrary() const;
    bool writePresetLibrary(const juce::var&) const;
    static juce::File getPresetLibraryFile();
    void refreshInsert(int slot);
    static juce::String timeText(double seconds);

    AudioEngine engine;
    juce::TextButton openButton { "Load audio" }, playButton { "Play" }, stopButton { "Stop" };
    juce::ToggleButton repeatButton { "Repeat" };
    juce::ToggleButton liveInputButton { "Live input" };
    juce::ToggleButton monoInputButton { "Mono" };
    juce::TextButton audioSettingsButton { "Audio settings" };
    juce::TextButton scanPluginsButton { "Rescan VST3" };
    juce::TextButton muteButton { "MUTE" };
    juce::TextButton keyboardButton { "Keyboard" };
    juce::HyperlinkButton logoLink { "", juce::URL("https://patreon.com/cvalabs") };
    juce::TextButton scanNewPluginsButton { "Rescan new only" };
    juce::TextButton savePresetButton { "Save preset" };
    juce::TextButton deletePresetButton { "Delete" };
    juce::ComboBox presetBox;
    juce::ComboBox oversamplingBox, bufferSizeBox, midiInputBox;
    juce::ToggleButton autoSavePresetButton { "AUTO SAVE PRESET" };
    juce::ToggleButton confirmOnLoadButton { "CONFIRM ON LOAD" };
    juce::Label fileLabel, timeLabel, gainLabel;
    juce::Slider positionSlider, gainSlider;
    juce::MidiKeyboardState midiKeyboardState;
    juce::MidiKeyboardComponent midiKeyboard { midiKeyboardState,
                                                juce::MidiKeyboardComponent::horizontalKeyboard };
    juce::TextButton insertButtons[AudioEngine::numInserts];
    juce::TextButton pluginSelectors[AudioEngine::numInserts];
    juce::ToggleButton bypassButtons[AudioEngine::numInserts];
    juce::TextButton clearButtons[AudioEngine::numInserts];
    std::unique_ptr<StereoMeter> inputMeter, outputMeter;
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<juce::AlertWindow> presetNameDialog;
    std::unique_ptr<juce::AlertWindow> presetDeleteDialog;
    std::unique_ptr<PluginWindow> pluginWindows[AudioEngine::numInserts];
    std::unique_ptr<PluginScanThread> pluginScanThread;
    std::unique_ptr<PluginFolderCheckThread> pluginFolderCheckThread;
    std::vector<juce::PluginDescription> scannedPlugins;
    PluginMenuLookAndFeel pluginMenuLookAndFeel;
    juce::Image logoImage;
    bool pluginSlotLoading[AudioEngine::numInserts] {};
    bool draggingPosition = false;
    float cpuLoadPercent = 0.0f;
    float ramLoadPercent = 0.0f;
    bool keyboardVisible = false;
    juce::StringArray registeredMidiInputs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
