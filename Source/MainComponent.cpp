// Copyright (C) 2026 CVA Labs. SPDX-License-Identifier: AGPL-3.0-only
#include "MainComponent.h"
#include <BinaryData.h>
#include <deque>
#if JUCE_WINDOWS
 #include <windows.h>
#endif

namespace
{
juce::String vst3ContainerPath(const juce::String& path)
{
    const auto marker = path.indexOfIgnoreCase(".vst3");
    return marker >= 0 ? path.substring(0, marker + 5) : path;
}
}

void MainComponent::InsertDragButton::mouseDrag(const juce::MouseEvent& event)
{
    juce::TextButton::mouseDrag(event);
    if (slot < 0 || event.getDistanceFromDragStart() < 6) return;
    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this);
        container != nullptr && !container->isDragAndDropActive())
        container->startDragging("insert:" + juce::String(slot), this, juce::ScaledImage(),
                                 false, nullptr, &event.source);
}

void MainComponent::PluginMenuLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button, const juce::Colour&, bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.5f);
    const auto text = button.getButtonText();
    const bool primary = text.containsIgnoreCase("load") || text == "Play"
        || text.containsIgnoreCase("save") || text.containsIgnoreCase("select vst")
        || text.containsIgnoreCase("scan") || text == "EDIT";
    auto top = primary ? juce::Colour(0xff29483e) : juce::Colour(0xff252a2b);
    auto bottom = primary ? juce::Colour(0xff142a25) : juce::Colour(0xff111516);
    if (highlighted) { top = top.brighter(0.13f); bottom = bottom.brighter(0.08f); }
    if (down) { top = top.darker(0.15f); bottom = bottom.darker(0.12f); }
    g.setColour(juce::Colours::black.withAlpha(0.65f));
    g.fillRoundedRectangle(r.translated(0.0f, 3.0f), 6.0f);
    g.setGradientFill(juce::ColourGradient(top, 0, r.getY(), bottom, 0, r.getBottom(), false));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(primary ? juce::Colour(0xff7d9d8c) : juce::Colour(0xff626869));
    g.drawRoundedRectangle(r, 6.0f, 1.2f);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawHorizontalLine(juce::roundToInt(r.getY() + 2.0f), r.getX() + 5.0f, r.getRight() - 5.0f);
}

void MainComponent::PluginMenuLookAndFeel::drawButtonText(
    juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.setColour(button.isEnabled() ? juce::Colour(0xfff0f1ed) : juce::Colour(0xff8b8d89));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(10, 4),
                     juce::Justification::centred, 1);
}

void MainComponent::PluginMenuLookAndFeel::drawToggleButton(
    juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool)
{
    const float boxSize = juce::jmin(28.0f, button.getHeight() * 0.58f);
    const float margin = juce::jmax(2.0f, button.getHeight() * 0.08f);
    auto box = juce::Rectangle<float>(margin, (button.getHeight() - boxSize) * 0.5f, boxSize, boxSize);
    g.setColour(juce::Colour(0xff090c0d)); g.fillRoundedRectangle(box, 4.0f);
    g.setColour(highlighted ? juce::Colour(0xff95ad9f) : juce::Colour(0xff747b79));
    g.drawRoundedRectangle(box, 4.0f, 1.5f);
    if (button.getToggleState())
    {
        g.setColour(juce::Colour(0xff77a755));
        g.fillRoundedRectangle(box.reduced(5.0f), 2.0f);
    }
    g.setColour(button.isEnabled() ? juce::Colour(0xffeeeeea) : juce::Colour(0xff777a77));
    g.setFont(juce::FontOptions(juce::jmax(7.0f, button.getHeight() * 0.36f)).withName("Bahnschrift"));
    g.drawText(button.getButtonText(), button.getLocalBounds().withTrimmedLeft(juce::roundToInt(boxSize + margin * 3.0f)),
               juce::Justification::centredLeft);
}

void MainComponent::PluginMenuLookAndFeel::drawComboBox(
    juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0, 0, (float) width, (float) height).reduced(1.5f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff252a2b), 0, 0,
                                           juce::Colour(0xff111516), 0, (float) height, false));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(juce::Colour(0xff626869)); g.drawRoundedRectangle(r, 6.0f, 1.2f);
    juce::Path arrow; const float cx = width - 24.0f, cy = height * 0.5f;
    arrow.startNewSubPath(cx - 7, cy - 4); arrow.lineTo(cx, cy + 4); arrow.lineTo(cx + 7, cy - 4);
    g.setColour(box.isEnabled() ? juce::Colour(0xffc5cac6) : juce::Colour(0xff666966));
    g.strokePath(arrow, juce::PathStrokeType(2.0f));
}

void MainComponent::PluginMenuLookAndFeel::drawLinearSlider(
    juce::Graphics& g, int x, int y, int width, int height, float pos, float, float,
    juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal) return;
    const float cy = y + height * 0.5f;
    const float designControlHeight = (float) slider.getProperties().getWithDefault("designHeight", height);
    const float controlScale = designControlHeight > 0.0f ? slider.getHeight() / designControlHeight : 1.0f;
    const float trackHeight = juce::jlimit(2.0f, 6.0f, 5.0f * controlScale);
    const float thumbDiameter = juce::jlimit(6.0f, 22.0f, 20.0f * controlScale);
    g.setColour(juce::Colour(0xff080b0c));
    g.fillRoundedRectangle((float) x, cy - trackHeight * 0.5f, (float) width,
                           trackHeight, trackHeight * 0.5f);
    g.setColour(juce::Colour(0xff385d50));
    g.fillRoundedRectangle((float) x, cy - trackHeight * 0.34f, juce::jmax(0.0f, pos - x),
                           trackHeight * 0.68f, trackHeight * 0.34f);
    const float thumbX = pos - thumbDiameter * 0.5f;
    const float thumbY = cy - thumbDiameter * 0.5f;
    g.setColour(juce::Colour(0xff8aa394)); g.fillEllipse(thumbX, thumbY, thumbDiameter, thumbDiameter);
    g.setColour(juce::Colour(0xff17221e));
    g.drawEllipse(thumbX, thumbY, thumbDiameter, thumbDiameter,
                  juce::jmax(0.7f, thumbDiameter * 0.068f));
}
class MainComponent::StereoMeter final : public juce::Component
{
public:
    explicit StereoMeter(juce::String meterName) : name(std::move(meterName)) {}

    void setLevels(float newLeft, float newRight, bool isOverloaded)
    {
        left = juce::jmax(newLeft, left * 0.84f);
        right = juce::jmax(newRight, right * 0.84f);
        overload = isOverloaded;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        const float meterScale = getWidth() / 120.0f;
        g.setFont(juce::FontOptions(juce::jmax(7.0f, 13.0f * meterScale), juce::Font::bold)
                      .withName("Bahnschrift"));
        g.drawText(name, area.removeFromTop(22), juce::Justification::centred);

        auto overloadArea = area.removeFromTop(32).reduced(3);
        g.setColour(overload ? juce::Colour(0xffff3b30) : juce::Colour(0xff3b3f47));
        g.fillRoundedRectangle(overloadArea.toFloat(), 4.0f);
        g.setColour(overload ? juce::Colours::white : juce::Colours::grey);
        g.drawText("OL", overloadArea, juce::Justification::centred);

        const int halfWidth = area.getWidth() / 2;
        drawBar(g, area.removeFromLeft(halfWidth).reduced(3), left, "L");
        drawBar(g, area.reduced(3), right, "R");
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        if (onReset) onReset();
    }

    std::function<void()> onReset;

private:
    static void drawBar(juce::Graphics& g, juce::Rectangle<int> area, float level, const char* channel)
    {
        auto label = area.removeFromBottom(18);
        g.setColour(juce::Colours::grey);
        g.drawText(channel, label, juce::Justification::centred);
        const float db = juce::Decibels::gainToDecibels(level, -60.0f);
        const float proportion = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
        g.setColour(juce::Colour(0xff080b0c));
        g.fillRoundedRectangle(area.toFloat(), 3.0f);
        constexpr int segments = 24;
        const float gap = 3.0f;
        const float segmentHeight = (area.getHeight() - gap * (segments + 1)) / segments;
        for (int i = 0; i < segments; ++i)
        {
            const float p = (i + 1.0f) / segments;
            const float yy = area.getBottom() - gap - (i + 1) * segmentHeight - i * gap;
            auto colour = p > 0.90f ? juce::Colour(0xffd4493f)
                         : p > 0.72f ? juce::Colour(0xffc8aa3e) : juce::Colour(0xff70a744);
            g.setColour(p <= proportion ? colour : colour.withAlpha(0.12f));
            g.fillRect((float) area.getX() + 5.0f, yy,
                       (float) area.getWidth() - 10.0f, segmentHeight);
        }
    }

    juce::String name;
    float left = 0.0f, right = 0.0f;
    bool overload = false;
};

class MainComponent::PluginScanThread final : public juce::Thread
{
public:
    PluginScanThread(MainComponent& component, bool scanOnlyNew)
        : Thread("VST3 scanner"), owner(&component), newOnly(scanOnlyNew) {}

    void run() override
    {
        juce::VST3PluginFormat format;
        auto identifiers = format.searchPathsForPlugins(format.getDefaultLocationsToSearch(), true, false);
        if (newOnly)
        {
            const auto knownContainers = juce::StringArray::fromLines(
                MainComponent::getPluginInventoryFile().loadFileAsString());
            juce::StringArray changed;
            for (const auto& identifier : identifiers)
            {
                const auto container = vst3ContainerPath(identifier).toLowerCase();
                if (!knownContainers.contains(container)) changed.add(identifier);
            }
            identifiers = std::move(changed);
        }
        std::vector<juce::PluginDescription> results;
        const auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);

        struct ScanJob
        {
            juce::ChildProcess child;
            juce::File output;
            uint32_t startTime = 0;
            juce::StringArray identifiers;
        };
        std::vector<std::unique_ptr<ScanJob>> active;
        std::deque<juce::StringArray> pending;
        for (int start = 0; start < identifiers.size(); start += 12)
        {
            juce::StringArray batch;
            for (int i = start; i < juce::jmin(start + 12, identifiers.size()); ++i) batch.add(identifiers[i]);
            pending.push_back(std::move(batch));
        }

        const auto collectResult = [&results](const ScanJob& job)
        {
            if (job.child.getExitCode() != 0 || !job.output.existsAsFile()) return;
            const auto parsed = juce::JSON::parse(job.output);
            if (auto* plugins = parsed.getArray())
                for (const auto& plugin : *plugins)
                {
                    juce::PluginDescription description;
                    const auto xml = juce::parseXML(plugin["descriptionXml"].toString());
                    if (xml != nullptr && description.loadFromXml(*xml))
                        results.push_back(std::move(description));
                }
        };

        while (!pending.empty() || !active.empty())
        {
            if (threadShouldExit())
            {
                for (auto& job : active)
                    if (job->child.isRunning()) job->child.kill();
                return;
            }

            while (active.size() < 4 && !pending.empty())
            {
                auto job = std::make_unique<ScanJob>();
                job->identifiers = std::move(pending.front());
                pending.pop_front();
                job->output = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("vst3-scan-" + juce::Uuid().toString() + ".json");
                juce::StringArray arguments { executable.getFullPathName(), "--scan-vst3-batch",
                                              job->output.getFullPathName() };
                arguments.addArray(job->identifiers);
                job->startTime = juce::Time::getMillisecondCounter();
                if (job->child.start(arguments, 0)) active.push_back(std::move(job));
            }

            for (int i = static_cast<int>(active.size()) - 1; i >= 0; --i)
            {
                auto& job = *active[static_cast<size_t>(i)];
                const bool isWavesShell = job.identifiers.size() == 1
                    && job.identifiers[0].containsIgnoreCase("WaveShell");
                const uint32_t timeoutMs = job.identifiers.size() > 1 ? 4000u
                                         : (isWavesShell ? 300000u : 15000u);
                const bool timedOut = juce::Time::getMillisecondCounter() - job.startTime > timeoutMs;
                if (!job.child.isRunning() || timedOut)
                {
                    if (job.child.isRunning()) job.child.kill();
                    job.child.waitForProcessToFinish(500);
                    const bool succeeded = !timedOut && job.child.getExitCode() == 0 && job.output.existsAsFile();
                    if (succeeded)
                        collectResult(job);
                    else if (job.identifiers.size() > 1)
                        for (const auto& identifier : job.identifiers)
                            pending.push_back({ identifier });
                    job.output.deleteFile();
                    active.erase(active.begin() + i);
                }
            }
            wait(20);
        }

        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b)
        {
            const int companyOrder = a.manufacturerName.compareIgnoreCase(b.manufacturerName);
            return companyOrder == 0 ? a.name.compareIgnoreCase(b.name) < 0 : companyOrder < 0;
        });

        juce::MessageManager::callAsync([safeOwner = owner, results = std::move(results), merge = newOnly]() mutable
        {
            if (safeOwner != nullptr) safeOwner->finishPluginScan(std::move(results), merge);
        });
    }

private:
    juce::Component::SafePointer<MainComponent> owner;
    bool newOnly = false;
};

class MainComponent::PluginFolderCheckThread final : public juce::Thread
{
public:
    explicit PluginFolderCheckThread(MainComponent& component)
        : Thread("VST3 folder check"), owner(&component) {}

    void run() override
    {
        juce::VST3PluginFormat format;
        const auto identifiers = format.searchPathsForPlugins(format.getDefaultLocationsToSearch(), true, false);
        const auto inventoryFile = MainComponent::getPluginInventoryFile();
        int changedCount = 0;

        if (!inventoryFile.existsAsFile())
        {
            juce::StringArray containers;
            for (const auto& identifier : identifiers)
                containers.addIfNotAlreadyThere(vst3ContainerPath(identifier).toLowerCase());
            containers.sort(true);
            if (inventoryFile.getParentDirectory().createDirectory().wasOk())
                inventoryFile.replaceWithText(containers.joinIntoString("\n"));
        }
        else
        {
            const auto known = juce::StringArray::fromLines(inventoryFile.loadFileAsString());
            for (const auto& identifier : identifiers)
                if (!known.contains(vst3ContainerPath(identifier).toLowerCase())) ++changedCount;
        }

        juce::MessageManager::callAsync([safeOwner = owner, changedCount]
        {
            if (safeOwner == nullptr) return;
            safeOwner->scanNewPluginsButton.setButtonText(changedCount == 0
                ? "No new VST3" : "Rescan new (" + juce::String(changedCount) + ")");
            safeOwner->scanNewPluginsButton.setEnabled(changedCount > 0);
        });
    }

private:
    juce::Component::SafePointer<MainComponent> owner;
};

class MainComponent::PluginWindow final : public juce::DocumentWindow
{
public:
    explicit PluginWindow(juce::AudioProcessor& processor)
        : DocumentWindow(processor.getName(), juce::Colours::black, closeButton)
    {
        auto* editor = processor.createEditorIfNeeded();
        if (editor == nullptr)
            editor = new juce::GenericAudioProcessorEditor(processor);
        setContentOwned(editor, true);
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        centreWithSize(juce::jmax(420, editor->getWidth()), juce::jmax(260, editor->getHeight()));
        setVisible(true);
    }

    void closeButtonPressed() override { setVisible(false); }
};

MainComponent::MainComponent()
{
    setOpaque(true);
    setLookAndFeel(&pluginMenuLookAndFeel);
    logoImage = juce::ImageCache::getFromMemory(BinaryData::cvalabslogo_png,
                                                 BinaryData::cvalabslogo_pngSize);

    inputMeter = std::make_unique<StereoMeter>("INPUT");
    outputMeter = std::make_unique<StereoMeter>("OUTPUT");
    inputMeter->onReset = [this] { engine.clearMeterOverloads(); };
    outputMeter->onReset = [this] { engine.clearMeterOverloads(); };
    addAndMakeVisible(*inputMeter);
    addAndMakeVisible(*outputMeter);
    constexpr double designWidth = 1536.0, designHeight = 1024.0;
    auto available = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.reduced(36);
    const double initialScale = 0.8 * juce::jmin(1.0, available.getWidth() / designWidth,
                                                 available.getHeight() / designHeight);
    setSize(juce::roundToInt(designWidth * initialScale),
            juce::roundToInt(designHeight * initialScale));

    const std::array<juce::Component*, 25> mainControls {
        &openButton, &playButton, &stopButton, &fileLabel,
        &timeLabel, &positionSlider, &gainSlider, &gainLabel, &repeatButton,
        &liveInputButton, &monoInputButton, &audioSettingsButton,
        &savePresetButton, &deletePresetButton, &presetBox, &scanPluginsButton, &scanNewPluginsButton,
        &oversamplingBox, &bufferSizeBox, &midiInputBox, &autoSavePresetButton, &confirmOnLoadButton,
        &muteButton, &logoLink, &keyboardButton
    };
    for (auto* c : mainControls)
        addAndMakeVisible(c);

    fileLabel.setText("Drop audio/MIDI here", juce::dontSendNotification);
    fileLabel.setJustificationType(juce::Justification::centred);
    fileLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe5e6e2));
    fileLabel.setFont(juce::FontOptions(19.0f).withName("Bahnschrift"));
    timeLabel.setJustificationType(juce::Justification::centredRight);
    timeLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff0f1ed));
    timeLabel.setFont(juce::FontOptions(19.0f, juce::Font::bold).withName("Bahnschrift"));
    gainLabel.setText("Output", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centredRight);
    gainLabel.setColour(juce::Label::textColourId, juce::Colour(0xffeeeeea));
    gainLabel.setFont(juce::FontOptions(18.0f).withName("Bahnschrift"));

    positionSlider.setRange(0.0, 1.0, 0.001);
    positionSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    positionSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    positionSlider.getProperties().set("designHeight", 34);
    positionSlider.onDragStart = [this] { draggingPosition = true; };
    positionSlider.onDragEnd = [this] { draggingPosition = false; engine.setPosition(positionSlider.getValue()); };

    gainSlider.setRange(-60.0, 6.0, 0.1);
    gainSlider.getProperties().set("designHeight", 52);
    gainSlider.setValue(0.0);
    gainSlider.setTextValueSuffix(" dB");
    gainSlider.onValueChange = [this]
    {
        if (!muteButton.getToggleState())
            engine.setGain(juce::Decibels::decibelsToGain((float) gainSlider.getValue()));
    };

    openButton.onClick = [this] { chooseAudioFile(); };
    playButton.onClick = [this] { engine.isPlaying() ? engine.pause() : engine.play(); };
    stopButton.onClick = [this] { engine.stop(); };
    repeatButton.onClick = [this] { engine.setLooping(repeatButton.getToggleState()); };
    liveInputButton.onClick = [this]
    {
        const bool live = liveInputButton.getToggleState();
        engine.setLiveInput(live);
        monoInputButton.setEnabled(live);
        playButton.setEnabled(!live);
        stopButton.setEnabled(!live);
        positionSlider.setEnabled(!live);
    };
    monoInputButton.setEnabled(false);
    monoInputButton.onClick = [this]
    {
        engine.setMonoInput(monoInputButton.getToggleState());
    };
    audioSettingsButton.onClick = [this] { showAudioSettings(); };
    muteButton.setClickingTogglesState(true);
    muteButton.onClick = [this]
    {
        engine.setGain(muteButton.getToggleState() ? 0.0f
            : juce::Decibels::decibelsToGain((float) gainSlider.getValue()));
    };
    keyboardButton.setClickingTogglesState(true);
    keyboardButton.onClick = [this]
    {
        keyboardVisible = keyboardButton.getToggleState();
        midiKeyboard.setVisible(keyboardVisible);
        const int oldHeight = getHeight();
        const int logicalHeight = keyboardVisible ? 1195 : 1024;
        const int targetHeight = juce::roundToInt(getWidth() * logicalHeight / 1536.0);
        if (auto* window = findParentComponentOfClass<juce::ResizableWindow>())
        {
            if (auto* boundsConstrainer = window->getConstrainer())
                boundsConstrainer->setFixedAspectRatio(1536.0 / logicalHeight);
            window->setSize(window->getWidth(), window->getHeight() + targetHeight - oldHeight);
        }
        resized();
        repaint();
    };
    scanPluginsButton.onClick = [this] { startPluginScan(false); };
    scanNewPluginsButton.onClick = [this] { startPluginScan(true); };
    savePresetButton.onClick = [this] { saveRackPreset(); };
    deletePresetButton.onClick = [this] { deleteRackPreset(); };
    presetBox.setTextWhenNothingSelected("Select preset...");
    oversamplingBox.addItemList({ "1x", "2x", "4x", "8x" }, 1);
    oversamplingBox.setSelectedId(2, juce::dontSendNotification);
    bufferSizeBox.addItemList({ "64 samples", "128 samples", "256 samples", "512 samples", "1024 samples" }, 1);
    bufferSizeBox.setSelectedId(4, juce::dontSendNotification);
    midiInputBox.addItem("None", 1);
    for (const auto& midiDevice : juce::MidiInput::getAvailableDevices())
        midiInputBox.addItem(midiDevice.name, midiInputBox.getNumItems() + 1);
    midiInputBox.setSelectedId(1, juce::dontSendNotification);
    autoSavePresetButton.setToggleState(true, juce::dontSendNotification);
    confirmOnLoadButton.setToggleState(true, juce::dontSendNotification);
    bufferSizeBox.setVisible(false); midiInputBox.setVisible(false);
    autoSavePresetButton.setVisible(false); confirmOnLoadButton.setVisible(false);
    deletePresetButton.setEnabled(false);
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        deletePresetButton.setEnabled(index >= 0);
        const auto library = readPresetLibrary();
        auto* presets = library["presets"].getArray();
        if (presets != nullptr && juce::isPositiveAndBelow(index, presets->size()))
            loadRackPreset(presets->getReference(index)["rack"]);
    };
    reloadPresetLibrary();

    for (int i = 0; i < AudioEngine::numInserts; ++i)
    {
        insertButtons[i].setInsertSlot(i);
        insertButtons[i].setButtonText("Empty");
        pluginSelectors[i].setButtonText("Select VST3...");
        pluginSelectors[i].setLookAndFeel(&pluginMenuLookAndFeel);
        pluginSelectors[i].setEnabled(false);
        bypassButtons[i].setButtonText("Bypass");
        clearButtons[i].setButtonText("Clear");
        addAndMakeVisible(insertButtons[i]); addAndMakeVisible(pluginSelectors[i]);
        addAndMakeVisible(bypassButtons[i]); addAndMakeVisible(clearButtons[i]);
        insertButtons[i].onClick = [this, i] { showPluginEditor(i); };
        pluginSelectors[i].onClick = [this, i] { showPluginMenu(i); };
        bypassButtons[i].onClick = [this, i] { engine.setPluginBypassed(i, bypassButtons[i].getToggleState()); };
        clearButtons[i].onClick = [safeThis = juce::Component::SafePointer<MainComponent> (this), i]
        {
            juce::MessageManager::callAsync ([safeThis, i]
            {
                if (safeThis == nullptr) return;
                safeThis->clearInsert(i);
            });
        };
        refreshInsert(i);
    }

    engine.setMidiKeyboardState(&midiKeyboardState);
    addAndMakeVisible(midiKeyboard);
    midiKeyboard.setVisible(false);
    midiKeyboard.setAvailableRange(24, 108);
    midiKeyboard.setKeyWidth(18.0f);

    std::unique_ptr<juce::XmlElement> savedDeviceState;
    const auto audioSettingsFile = getAudioSettingsFile();
    if (audioSettingsFile.existsAsFile())
        savedDeviceState = juce::XmlDocument::parse(audioSettingsFile);
    setAudioChannels(2, 2);
    if (savedDeviceState != nullptr)
        deviceManager.initialise(2, 2, savedDeviceState.get(), true);
    syncMidiInputs();
    startTimerHz(30);
    const bool hadPluginCache = loadPluginScanCache();
    if (!hadPluginCache) scanPluginsButton.setButtonText("Scan VST3");
    logoLink.setTooltip("CVA Labs on Patreon");
    scanNewPluginsButton.setButtonText("Checking VST3...");
    scanNewPluginsButton.setEnabled(false);
    pluginFolderCheckThread = std::make_unique<PluginFolderCheckThread>(*this);
    pluginFolderCheckThread->startThread();
}

MainComponent::~MainComponent()
{
    stopTimer();
    saveAudioSettings();
    for (const auto& identifier : registeredMidiInputs)
        deviceManager.removeMidiInputDeviceCallback(identifier, this);
    if (pluginScanThread != nullptr)
    {
        pluginScanThread->signalThreadShouldExit();
        pluginScanThread->stopThread(3000);
        pluginScanThread.reset();
    }
    if (pluginFolderCheckThread != nullptr)
    {
        pluginFolderCheckThread->signalThreadShouldExit();
        pluginFolderCheckThread->stopThread(3000);
        pluginFolderCheckThread.reset();
    }
    for (auto& window : pluginWindows)
        window.reset();
    for (auto& selector : pluginSelectors)
        selector.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    shutdownAudio();
}

void MainComponent::prepareToPlay(int blockSize, double sampleRate) { engine.prepareToPlay(blockSize, sampleRate); }
void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& info) { engine.getNextAudioBlock(info); }
void MainComponent::releaseResources() { engine.releaseResources(); }

void MainComponent::paint(juce::Graphics& g)
{
    constexpr float designWidth = 1536.0f;
    const float designHeight = keyboardVisible ? 1195.0f : 1024.0f;
    const float scale = juce::jmin(getWidth() / designWidth, getHeight() / designHeight);
    const float offsetX = (getWidth() - designWidth * scale) * 0.5f;
    const float offsetY = (getHeight() - designHeight * scale) * 0.5f;
    g.addTransform(juce::AffineTransform::translation(offsetX, offsetY).scaled(scale));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff222728), 0, 0,
                                           juce::Colour(0xff0d1213), designWidth,
                                           designHeight, false));
    g.fillRect(0.0f, 0.0f, designWidth, designHeight);
    for (int y = 2; y < (int) designHeight; y += 4)
    {
        g.setColour((y % 8 == 2 ? juce::Colours::white : juce::Colours::black).withAlpha(0.018f));
        g.drawHorizontalLine(y, 0.0f, designWidth);
    }
    g.setColour(juce::Colour(0xff525858));
    g.drawRoundedRectangle(juce::Rectangle<float>(0, 0, designWidth, designHeight).reduced(12.0f),
                           9.0f, 1.2f);
    if (logoImage.isValid())
        g.drawImageWithin(logoImage, 48, 54, 390, 50, juce::RectanglePlacement::centred);
    g.setColour(juce::Colour(0xff555b5a).withAlpha(0.65f));
    g.drawHorizontalLine(128, 28.0f, designWidth - 28.0f);
    g.setColour(juce::Colour(0xfff2f2ee));
    g.setFont(juce::FontOptions(34.0f, juce::Font::bold).withName("Bahnschrift"));
    g.drawText("VST3 PLAYER HOST", 520, 48, 500, 64, juce::Justification::centred);
    g.setColour(juce::Colour(0xff65e887));
    g.drawRoundedRectangle(1217.0f, 62.0f, 163.0f, 48.0f, 6.0f, 1.5f);
    g.setFont(juce::FontOptions(17.0f, juce::Font::bold).withName("Bahnschrift"));
    g.drawEllipse(1236.0f, 75.0f, 21.0f, 21.0f, 1.6f);
    juce::Path freeCheck;
    freeCheck.startNewSubPath(1241.0f, 85.0f);
    freeCheck.lineTo(1245.0f, 89.0f);
    freeCheck.lineTo(1253.0f, 80.0f);
    g.strokePath(freeCheck, juce::PathStrokeType(1.8f));
    g.drawText("FREE", 1260, 62, 104, 48, juce::Justification::centred);
    g.setColour(juce::Colour(0xffd8dcda));
    g.setFont(juce::FontOptions(17.0f).withName("Bahnschrift"));
    g.drawText("v" + juce::String(ProjectInfo::versionString), 1395, 58, 100, 56,
               juce::Justification::centred);

    const auto panel = [&](juce::Rectangle<float> r)
    {
        g.setColour(juce::Colour(0xff090d0e).withAlpha(0.62f)); g.fillRoundedRectangle(r, 8.0f);
        g.setColour(juce::Colour(0xff4c5352)); g.drawRoundedRectangle(r, 8.0f, 1.0f);
    };
    panel({ 44, 154, 1127, 288 });
    panel({ 44, 454, 1127, 326 });
    panel({ 44, 792, 1127, 182 });
    panel({ 1187, 154, 303, 818 });
    if (keyboardVisible) panel({ 44, 990, 1127, 180 });

    g.setColour(juce::Colour(0xffeeeeea));
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold).withName("Bahnschrift"));

    g.setColour(juce::Colour(0xffeeeeea));
    g.setFont(juce::FontOptions(17.0f, juce::Font::bold).withName("Bahnschrift"));
    for (int i = 0; i < AudioEngine::numInserts; ++i)
    {
        const int y = 508 + i * 63;
        g.drawText("INSERT " + juce::String(i + 1), 72, y, 115, 42,
                   juce::Justification::centredLeft);
        if (i > 0)
        {
            g.setColour(juce::Colour(0xff343a39));
            g.drawHorizontalLine(y - 8, 68.0f, 1145.0f);
            g.setColour(juce::Colour(0xffeeeeea));
        }
    }

    if (juce::isPositiveAndBelow(dragTargetSlot, AudioEngine::numInserts))
    {
        const int y = 503 + dragTargetSlot * 63;
        g.setColour(juce::Colour(0xff65e887));
        g.drawRoundedRectangle(64.0f, (float) y, 1085.0f, 55.0f, 6.0f, 2.0f);
    }

    g.setFont(juce::FontOptions(15.0f).withName("Bahnschrift"));
    g.drawText("OVERSAMPLING", 76, 850, 180, 24, juce::Justification::centredLeft);
    g.drawText("CPU LOAD", 76, 894, 120, 24, juce::Justification::centredLeft);
    g.drawText("RAM LOAD", 76, 934, 120, 24, juce::Justification::centredLeft);

    const auto drawSystemLoad = [&](int y, float percent)
    {
        constexpr int segments = 12;
        for (int i = 0; i < segments; ++i)
        {
            const bool lit = (i + 1) * (100.0f / segments) <= percent;
            g.setColour(lit ? juce::Colour(0xff58c758) : juce::Colour(0xff202524));
            g.fillRect(201 + i * 13, y, 10, 22);
            g.setColour(juce::Colour(0xff080b0c)); g.drawRect(201 + i * 13, y, 10, 22, 1);
        }
        g.setColour(juce::Colour(0xff65e887));
        g.drawText(juce::String(juce::roundToInt(percent)) + "%", 370, y - 2, 70, 26,
                   juce::Justification::centredLeft);
    };
    drawSystemLoad(895, cpuLoadPercent); drawSystemLoad(935, ramLoadPercent);

    const float dash[] { 7.0f, 5.0f };
    juce::Path dropFrame; dropFrame.addRoundedRectangle(263.0f, 250.0f, 365.0f, 54.0f, 6.0f);
    juce::Path dashedFrame;
    juce::PathStrokeType(1.2f).createDashedStroke(dashedFrame, dropFrame, dash, 2);
    g.setColour(juce::Colour(0xff858b88));
    g.fillPath(dashedFrame);

}

void MainComponent::resized()
{
    constexpr float designWidth = 1536.0f;
    const float designHeight = keyboardVisible ? 1195.0f : 1024.0f;
    const float scale = juce::jmin(getWidth() / designWidth, getHeight() / designHeight);
    const float offsetX = (getWidth() - designWidth * scale) * 0.5f;
    const float offsetY = (getHeight() - designHeight * scale) * 0.5f;
    const auto place = [scale, offsetX, offsetY](juce::Component& c, int x, int y, int w, int h)
    {
        c.setBounds(juce::roundToInt(offsetX + x * scale),
                    juce::roundToInt(offsetY + y * scale),
                    juce::roundToInt(w * scale), juce::roundToInt(h * scale));
    };

    place(presetBox, 70, 175, 615, 50);
    place(logoLink, 48, 54, 390, 50);
    place(savePresetButton, 715, 175, 210, 50); place(deletePresetButton, 950, 175, 190, 50);
    place(openButton, 70, 250, 172, 54); place(fileLabel, 263, 250, 365, 54);
    place(liveInputButton, 655, 252, 145, 48); place(monoInputButton, 818, 252, 115, 48);
    place(audioSettingsButton, 950, 250, 190, 54);
    place(positionSlider, 70, 330, 890, 34); place(timeLabel, 992, 326, 145, 40);
    place(playButton, 70, 377, 150, 48); place(stopButton, 238, 377, 140, 48);
    place(repeatButton, 442, 377, 130, 48);
    place(gainLabel, 850, 375, 85, 48); place(gainSlider, 935, 373, 205, 52);
    place(*inputMeter, 1205, 180, 119, 654); place(*outputMeter, 1349, 180, 119, 654);
    place(muteButton, 1242, 874, 175, 54);
    place(oversamplingBox, 247, 840, 168, 42);
    place(scanPluginsButton, 482, 870, 210, 50); place(scanNewPluginsButton, 710, 870, 210, 50);
    place(keyboardButton, 938, 870, 210, 50);
    if (keyboardVisible) place(midiKeyboard, 62, 1008, 1090, 144);

    fileLabel.setFont(juce::FontOptions(juce::jmax(7.0f, 19.0f * scale)).withName("Bahnschrift"));
    timeLabel.setFont(juce::FontOptions(juce::jmax(7.0f, 19.0f * scale), juce::Font::bold).withName("Bahnschrift"));
    gainLabel.setFont(juce::FontOptions(juce::jmax(7.0f, 18.0f * scale)).withName("Bahnschrift"));
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false,
                               juce::jmax(34, juce::roundToInt(82.0f * scale)),
                               juce::jmax(18, juce::roundToInt(40.0f * scale)));

    for (int i = 0; i < AudioEngine::numInserts; ++i)
    {
        const int y = 507 + i * 63;
        place(insertButtons[i], 185, y, 300, 46);
        place(pluginSelectors[i], 500, y, 285, 46);
        place(bypassButtons[i], 800, y, 145, 46);
        place(clearButtons[i], 960, y, 120, 46);
        bypassButtons[i].setVisible(true);
        clearButtons[i].setVisible(true);
    }
}

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& p : files)
        if (juce::File(p).hasFileExtension("wav;mp3;m4a;mp4;aac;mid;midi")) return true;
    return false;
}

void MainComponent::filesDropped(const juce::StringArray& files, int, int)
{
    if (!files.isEmpty()) openAudioFile(juce::File(files[0]));
}

void MainComponent::timerCallback()
{
    // Enumerating system MIDI devices can enter OS/driver code and must not run at
    // the 30 Hz UI-meter rate. Refresh roughly once every two seconds instead.
    if (++midiDeviceRefreshTicks >= 60)
    {
        midiDeviceRefreshTicks = 0;
        syncMidiInputs();
    }
    const auto length = engine.getLength();
    positionSlider.setRange(0.0, juce::jmax(0.001, length), 0.001);
    if (!draggingPosition) positionSlider.setValue(engine.getPosition(), juce::dontSendNotification);
    timeLabel.setText(timeText(engine.getPosition()) + " / " + timeText(length), juce::dontSendNotification);
    playButton.setButtonText(engine.isPlaying() ? "Pause" : "Play");
    inputMeter->setLevels(engine.consumeInputPeak(0), engine.consumeInputPeak(1), engine.hasInputOverload());
    outputMeter->setLevels(engine.consumeOutputPeak(0), engine.consumeOutputPeak(1), engine.hasOutputOverload());
    cpuLoadPercent = juce::jlimit(0.0f, 100.0f, (float) deviceManager.getCpuUsage() * 100.0f);
#if JUCE_WINDOWS
    MEMORYSTATUSEX memoryStatus {};
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (GlobalMemoryStatusEx(&memoryStatus))
        ramLoadPercent = (float) memoryStatus.dwMemoryLoad;
#endif
    repaint();
}

int MainComponent::insertSlotAt(juce::Point<int> position) const
{
    for (int slot = 0; slot < AudioEngine::numInserts; ++slot)
    {
        auto row = insertButtons[slot].getBounds()
                       .getUnion(clearButtons[slot].getBounds())
                       .expanded(12, 7);
        if (row.contains(position)) return slot;
    }
    return -1;
}

bool MainComponent::isInterestedInDragSource(const SourceDetails& details)
{
    return details.description.toString().startsWith("insert:");
}

void MainComponent::itemDragMove(const SourceDetails& details)
{
    const int nextTarget = insertSlotAt(details.localPosition);
    if (nextTarget != dragTargetSlot)
    {
        dragTargetSlot = nextTarget;
        repaint();
    }
}

void MainComponent::itemDragExit(const SourceDetails&)
{
    dragTargetSlot = -1;
    repaint();
}

void MainComponent::itemDropped(const SourceDetails& details)
{
    const int sourceSlot = details.description.toString().fromFirstOccurrenceOf("insert:", false, false).getIntValue();
    const int targetSlot = insertSlotAt(details.localPosition);
    dragTargetSlot = -1;

    if (!juce::isPositiveAndBelow(sourceSlot, AudioEngine::numInserts)
        || !juce::isPositiveAndBelow(targetSlot, AudioEngine::numInserts)
        || sourceSlot == targetSlot
        || engine.getPlugin(sourceSlot) == nullptr
        || pluginSlotLoading[sourceSlot] || pluginSlotLoading[targetSlot])
    {
        repaint();
        return;
    }

    // Editors point directly at their plugin instances, so close both before
    // moving the instances to their new insert positions.
    pluginWindows[sourceSlot].reset();
    pluginWindows[targetSlot].reset();
    engine.swapInserts(sourceSlot, targetSlot);
    refreshInsert(sourceSlot);
    refreshInsert(targetSlot);
    repaint();
}

void MainComponent::chooseAudioFile()
{
    chooser = std::make_unique<juce::FileChooser>("Choose an audio or MIDI file", juce::File{},
                                                  "*.wav;*.mp3;*.m4a;*.mp4;*.aac;*.mid;*.midi");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [this](const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) openAudioFile(fc.getResult()); });
}

void MainComponent::openAudioFile(const juce::File& file)
{
    juce::String error;
    const bool isMidi = file.hasFileExtension("mid;midi");
    if ((isMidi ? engine.loadMidiFile(file, error) : engine.loadAudioFile(file, error)))
    {
        engine.setLooping(repeatButton.getToggleState());
        fileLabel.setText(file.getFileName(), juce::dontSendNotification);
    }
    else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "File error", error);
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    engine.addIncomingMidiMessage(message);
}

void MainComponent::syncMidiInputs()
{
    juce::StringArray enabledNow;
    for (const auto& device : juce::MidiInput::getAvailableDevices())
    {
        if (!deviceManager.isMidiInputDeviceEnabled(device.identifier)) continue;
        enabledNow.add(device.identifier);
        if (!registeredMidiInputs.contains(device.identifier))
            deviceManager.addMidiInputDeviceCallback(device.identifier, this);
    }
    for (const auto& identifier : registeredMidiInputs)
        if (!enabledNow.contains(identifier))
            deviceManager.removeMidiInputDeviceCallback(identifier, this);
    registeredMidiInputs = enabledNow;
}

juce::File MainComponent::getAudioSettingsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("CVA Labs").getChildFile("VST3 Player Host")
        .getChildFile("audio-settings.xml");
}

void MainComponent::saveAudioSettings() const
{
    if (auto state = deviceManager.createStateXml())
    {
        const auto file = getAudioSettingsFile();
        file.getParentDirectory().createDirectory();
        state->writeTo(file);
    }
}

void MainComponent::showPluginMenu(int slot)
{
    if (scannedPlugins.empty() || pluginSlotLoading[slot]) return;

    juce::PopupMenu companies;
    constexpr int bypassMenuId = 1000001, clearMenuId = 1000002;
    if (engine.getPlugin(slot) != nullptr)
    {
        companies.addItem(bypassMenuId, engine.isPluginBypassed(slot) ? "Disable bypass" : "Bypass plugin");
        companies.addItem(clearMenuId, "Clear insert");
        companies.addSeparator();
    }
    size_t start = 0;
    while (start < scannedPlugins.size())
    {
        auto company = scannedPlugins[start].manufacturerName.trim();
        if (company.isEmpty()) company = "Unknown manufacturer";

        juce::PopupMenu plugins;
        size_t end = start;
        while (end < scannedPlugins.size())
        {
            auto candidate = scannedPlugins[end].manufacturerName.trim();
            if (candidate.isEmpty()) candidate = "Unknown manufacturer";
            if (candidate != company) break;
            juce::PopupMenu::Item item(scannedPlugins[end].name);
            item.itemID = static_cast<int>(end) + 1;
            if (scannedPlugins[end].isInstrument)
                item.colour = juce::Colour(0xff65e887);
            plugins.addItem(std::move(item));
            ++end;
        }
        companies.addSubMenu(company, plugins);
        start = end;
    }

    companies.setLookAndFeel(&pluginMenuLookAndFeel);
    const juce::Component::SafePointer<MainComponent> safeThis(this);
    companies.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&pluginSelectors[slot]),
                            [safeThis, slot, bypassMenuId, clearMenuId](int result)
                            {
                                if (safeThis == nullptr || result <= 0) return;
                                if (result == bypassMenuId)
                                {
                                    const bool bypassed = !safeThis->engine.isPluginBypassed(slot);
                                    safeThis->engine.setPluginBypassed(slot, bypassed);
                                    safeThis->bypassButtons[slot].setToggleState(bypassed, juce::dontSendNotification);
                                }
                                else if (result == clearMenuId)
                                {
                                    safeThis->clearInsert(slot);
                                }
                                else safeThis->loadSelectedPlugin(slot, result - 1);
                            });
}

void MainComponent::loadSelectedPlugin(int slot, int pluginIndex)
{
    if (!juce::isPositiveAndBelow(pluginIndex, static_cast<int>(scannedPlugins.size()))) return;
    const auto description = scannedPlugins[static_cast<size_t>(pluginIndex)];

    const juce::Component::SafePointer<MainComponent> safeThis(this);
    if (engine.getPlugin(slot) != nullptr)
        clearInsert(slot);
    insertButtons[slot].setButtonText("Loading...");
    pluginSlotLoading[slot] = true;
    insertButtons[slot].setEnabled(false);
    pluginSelectors[slot].setEnabled(false);
    bypassButtons[slot].setEnabled(false);
    clearButtons[slot].setEnabled(false);

    engine.loadPlugin(slot, description, [safeThis, slot](const juce::String& error)
    {
        if (safeThis == nullptr) return;
        safeThis->refreshInsert(slot);
        if (error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Plugin error", error);
    });
}

void MainComponent::startPluginScan(bool newOnly)
{
    if (pluginScanThread != nullptr)
    {
        pluginScanThread->signalThreadShouldExit();
        pluginScanThread->stopThread(3000);
        pluginScanThread.reset();
    }

    scanPluginsButton.setEnabled(false);
    scanNewPluginsButton.setEnabled(false);
    (newOnly ? scanNewPluginsButton : scanPluginsButton).setButtonText("Scanning...");
    if (!newOnly)
    {
        for (auto& selector : pluginSelectors)
        {
            selector.setButtonText("Scanning VST3...");
            selector.setEnabled(false);
        }
    }

    pluginScanThread = std::make_unique<PluginScanThread>(*this, newOnly);
    pluginScanThread->startThread();
}

void MainComponent::checkForNewPluginContainers()
{
    juce::VST3PluginFormat format;
    const auto identifiers = format.searchPathsForPlugins(format.getDefaultLocationsToSearch(), true, false);
    const auto inventoryFile = getPluginInventoryFile();
    if (!inventoryFile.existsAsFile())
    {
        saveCurrentPluginInventory();
        scanNewPluginsButton.setButtonText("No new VST3");
        scanNewPluginsButton.setEnabled(false);
        return;
    }

    const auto knownContainers = juce::StringArray::fromLines(inventoryFile.loadFileAsString());
    int changedCount = 0;

    for (const auto& identifier : identifiers)
    {
        const auto container = vst3ContainerPath(identifier).toLowerCase();
        if (!knownContainers.contains(container)) ++changedCount;
    }

    scanNewPluginsButton.setButtonText(changedCount == 0
        ? "No new VST3"
        : "Rescan new (" + juce::String(changedCount) + ")");
    scanNewPluginsButton.setEnabled(changedCount > 0);
}

void MainComponent::finishPluginScan(std::vector<juce::PluginDescription> results,
                                     bool mergeWithExisting, bool updateInventory)
{
    if (mergeWithExisting)
    {
        for (const auto& replacement : results)
            scannedPlugins.erase(std::remove_if(scannedPlugins.begin(), scannedPlugins.end(),
                                                [&](const auto& old)
                                                {
                                                    return old.fileOrIdentifier == replacement.fileOrIdentifier;
                                                }),
                                 scannedPlugins.end());
        scannedPlugins.insert(scannedPlugins.end(), std::make_move_iterator(results.begin()),
                              std::make_move_iterator(results.end()));
        std::sort(scannedPlugins.begin(), scannedPlugins.end(), [](const auto& a, const auto& b)
        {
            const int companyOrder = a.manufacturerName.compareIgnoreCase(b.manufacturerName);
            return companyOrder == 0 ? a.name.compareIgnoreCase(b.name) < 0 : companyOrder < 0;
        });
    }
    else scannedPlugins = std::move(results);
    for (auto& selector : pluginSelectors)
    {
        const int slot = static_cast<int>(&selector - &pluginSelectors[0]);
        selector.setButtonText("Select VST3...");
        selector.setEnabled(!scannedPlugins.empty() && !pluginSlotLoading[slot]);
    }

    scanPluginsButton.setButtonText("Rescan VST3");
    scanPluginsButton.setEnabled(true);
    savePluginScanCache();
    if (updateInventory)
    {
        saveCurrentPluginInventory();
        scanNewPluginsButton.setButtonText("No new VST3");
        scanNewPluginsButton.setEnabled(false);
    }
}

juce::File MainComponent::getPluginScanCacheFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("CVA Labs").getChildFile("VST3 Player Host").getChildFile("vst3-cache-instruments.json");
}

juce::File MainComponent::getPluginInventoryFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("CVA Labs").getChildFile("VST3 Player Host").getChildFile("vst3-inventory.txt");
}

void MainComponent::saveCurrentPluginInventory() const
{
    juce::VST3PluginFormat format;
    const auto identifiers = format.searchPathsForPlugins(format.getDefaultLocationsToSearch(), true, false);
    juce::StringArray containers;
    for (const auto& identifier : identifiers)
        containers.addIfNotAlreadyThere(vst3ContainerPath(identifier).toLowerCase());
    containers.sort(true);

    const auto file = getPluginInventoryFile();
    if (file.getParentDirectory().createDirectory().wasOk())
        file.replaceWithText(containers.joinIntoString("\n"));
}

bool MainComponent::loadPluginScanCache()
{
    const auto parsed = juce::JSON::parse(getPluginScanCacheFile());
    auto* plugins = parsed.getArray();
    if (plugins == nullptr || plugins->isEmpty()) return false;

    std::vector<juce::PluginDescription> cached;
    for (const auto& plugin : *plugins)
    {
        juce::PluginDescription description;
        const auto xml = juce::parseXML(plugin["descriptionXml"].toString());
        if (xml != nullptr) description.loadFromXml(*xml);
        if (juce::File(description.fileOrIdentifier).exists()) cached.push_back(std::move(description));
    }
    if (cached.empty()) return false;
    finishPluginScan(std::move(cached), false, false);
    return true;
}

void MainComponent::savePluginScanCache() const
{
    juce::Array<juce::var> plugins;
    for (const auto& description : scannedPlugins)
    {
        auto item = std::make_unique<juce::DynamicObject>();
        item->setProperty("descriptionXml", description.createXml()->toString());
        item->setProperty("name", description.name);
        item->setProperty("manufacturer", description.manufacturerName);
        item->setProperty("path", description.fileOrIdentifier);
        item->setProperty("category", description.category);
        plugins.add(juce::var(item.release()));
    }

    const auto file = getPluginScanCacheFile();
    if (file.getParentDirectory().createDirectory().wasOk())
        file.replaceWithText(juce::JSON::toString(juce::var(plugins), false));
}

void MainComponent::showPluginEditor(int slot)
{
    if (auto* plugin = engine.getPlugin(slot))
    {
        if (pluginWindows[slot] == nullptr) pluginWindows[slot] = std::make_unique<PluginWindow>(*plugin);
        else pluginWindows[slot]->setVisible(true);
        pluginWindows[slot]->toFront(true);
    }
}

void MainComponent::showAudioSettings()
{
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager, 0, 2, 0, 2, true, false, true, false);
    selector->setSize(520, 420);

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Audio / ASIO settings";
    options.dialogBackgroundColour = juce::Colour(0xff262a31);
    options.content.setOwned(selector.release());
    options.componentToCentreAround = this;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}

void MainComponent::saveRackPreset()
{
    presetNameDialog = std::make_unique<juce::AlertWindow>("Save preset",
                                                           "Enter a name for this rack preset:",
                                                           juce::MessageBoxIconType::NoIcon);
    const auto suggestedName = presetBox.getText().isNotEmpty() ? presetBox.getText() : "New preset";
    presetNameDialog->addTextEditor("name", suggestedName, "Preset name");
    presetNameDialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    presetNameDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    const juce::Component::SafePointer<MainComponent> safeThis(this);
    presetNameDialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [safeThis](int result)
        {
            if (safeThis == nullptr) return;
            if (result == 1)
            {
                const auto name = safeThis->presetNameDialog->getTextEditorContents("name").trim();
                if (name.isNotEmpty())
                {
                    auto library = safeThis->readPresetLibrary();
                    auto* presets = library["presets"].getArray();
                    bool replaced = false;
                    for (auto& item : *presets)
                    {
                        if (item["name"].toString().equalsIgnoreCase(name))
                        {
                            item.getDynamicObject()->setProperty("name", name);
                            item.getDynamicObject()->setProperty("rack", safeThis->createFullPreset());
                            replaced = true;
                            break;
                        }
                    }

                    if (!replaced)
                    {
                        auto entry = std::make_unique<juce::DynamicObject>();
                        entry->setProperty("name", name);
                        entry->setProperty("rack", safeThis->createFullPreset());
                        presets->add(juce::var(entry.release()));
                    }

                    if (safeThis->writePresetLibrary(library))
                    {
                        safeThis->reloadPresetLibrary();
                        if (auto* savedPresets = library["presets"].getArray())
                            for (int i = 0; i < savedPresets->size(); ++i)
                                if (savedPresets->getReference(i)["name"].toString() == name)
                                    safeThis->presetBox.setSelectedId(i + 1, juce::dontSendNotification);
                        safeThis->deletePresetButton.setEnabled(true);
                    }
                    else
                    {
                        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                                "Preset error", "Could not save the preset library.");
                    }
                }
            }
            safeThis->presetNameDialog.reset();
        }), false);
}

juce::var MainComponent::createFullPreset() const
{
    auto preset = engine.createRackPreset();
    auto* object = preset.getDynamicObject();
    object->setProperty("sourceMode", liveInputButton.getToggleState() ? "live" : "file");
    object->setProperty("audioFile", engine.getLoadedAudioFile().getFullPathName());
    object->setProperty("monoInput", monoInputButton.getToggleState());
    object->setProperty("repeat", repeatButton.getToggleState());
    object->setProperty("outputGainDb", gainSlider.getValue());
    if (auto state = deviceManager.createStateXml())
        object->setProperty("audioDeviceState", state->toString());
    return preset;
}

void MainComponent::deleteRackPreset()
{
    const int selectedIndex = presetBox.getSelectedId() - 1;
    auto library = readPresetLibrary();
    auto* presets = library["presets"].getArray();
    if (presets == nullptr || !juce::isPositiveAndBelow(selectedIndex, presets->size()))
        return;

    const auto name = presets->getReference(selectedIndex)["name"].toString();
    presetDeleteDialog = std::make_unique<juce::AlertWindow>("Delete preset",
        "Delete preset \"" + name + "\"? This cannot be undone.",
        juce::MessageBoxIconType::WarningIcon);
    presetDeleteDialog->addButton("Delete", 1);
    presetDeleteDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    const juce::Component::SafePointer<MainComponent> safeThis(this);
    presetDeleteDialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [safeThis, selectedIndex](int result)
        {
            if (safeThis == nullptr) return;
            if (result == 1)
            {
                auto currentLibrary = safeThis->readPresetLibrary();
                if (auto* currentPresets = currentLibrary["presets"].getArray();
                    currentPresets != nullptr && juce::isPositiveAndBelow(selectedIndex, currentPresets->size()))
                {
                    currentPresets->remove(selectedIndex);
                    if (safeThis->writePresetLibrary(currentLibrary))
                    {
                        safeThis->presetBox.setSelectedId(0, juce::dontSendNotification);
                        safeThis->deletePresetButton.setEnabled(false);
                        safeThis->reloadPresetLibrary();
                    }
                }
            }
            safeThis->presetDeleteDialog.reset();
        }), false);
}

juce::File MainComponent::getPresetLibraryFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("CVA Labs").getChildFile("VST3 Player Host").getChildFile("preset-library.json");
}

juce::var MainComponent::readPresetLibrary() const
{
    const auto file = getPresetLibraryFile();
    if (file.existsAsFile())
    {
        const auto parsed = juce::JSON::parse(file);
        if (parsed.isObject() && parsed["format"].toString() == "VST3PlayerHostPresetLibrary"
            && parsed["presets"].isArray())
            return parsed;
    }

    auto library = std::make_unique<juce::DynamicObject>();
    library->setProperty("format", "VST3PlayerHostPresetLibrary");
    library->setProperty("version", 1);
    library->setProperty("presets", juce::Array<juce::var>{});
    return juce::var(library.release());
}

bool MainComponent::writePresetLibrary(const juce::var& library) const
{
    const auto file = getPresetLibraryFile();
    if (file.getParentDirectory().createDirectory().failed()) return false;
    return file.replaceWithText(juce::JSON::toString(library, true));
}

void MainComponent::reloadPresetLibrary()
{
    const auto previousName = presetBox.getText();
    presetBox.clear(juce::dontSendNotification);
    const auto library = readPresetLibrary();
    if (auto* presets = library["presets"].getArray())
        for (int i = 0; i < presets->size(); ++i)
            presetBox.addItem(presets->getReference(i)["name"].toString(), i + 1);

    if (previousName.isNotEmpty())
        presetBox.setText(previousName, juce::dontSendNotification);
}

void MainComponent::loadRackPreset(const juce::var& preset)
{
    const juce::Component::SafePointer<MainComponent> safeThis(this);
    auto* slots = preset.isObject() ? preset["slots"].getArray() : nullptr;
    if (!preset.isObject() || preset["format"].toString() != "VST3PlayerHostRack"
        || slots == nullptr || slots->size() != AudioEngine::numInserts)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                "Preset error", "This is not a valid rack preset.");
        return;
    }

    restoreSourceFromPreset(preset);

    struct LoadStatus { int remaining = AudioEngine::numInserts; juce::String errors; };
    auto status = std::make_shared<LoadStatus>();
    savePresetButton.setEnabled(false);
    presetBox.setEnabled(false);

    const std::function<void(int, juce::String)> finish = [safeThis, status](int slot, juce::String error)
    {
        if (safeThis == nullptr) return;
        safeThis->insertButtons[slot].setEnabled(true);
        safeThis->refreshInsert(slot);
        if (error.isNotEmpty()) status->errors += "Insert " + juce::String(slot + 1) + ": " + error + "\n";
        if (--status->remaining == 0)
        {
            safeThis->savePresetButton.setEnabled(true);
            safeThis->presetBox.setEnabled(true);
            if (status->errors.isNotEmpty())
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                        "Preset loaded with errors", status->errors);
        }
    };

    for (int slot = 0; slot < AudioEngine::numInserts; ++slot)
    {
        pluginWindows[slot].reset();
        insertButtons[slot].setButtonText("Loading preset...");
        insertButtons[slot].setEnabled(false);
        safeThis->pluginSlotLoading[slot] = true;
        safeThis->pluginSelectors[slot].setEnabled(false);
        bypassButtons[slot].setEnabled(false);
        clearButtons[slot].setEnabled(false);

        const auto slotData = slots->getReference(slot);
        const auto path = slotData["path"].toString();
        if (path.isEmpty())
        {
            engine.clearPlugin(slot);
            finish(slot, {});
            continue;
        }

        engine.loadPlugin(slot, juce::File(path),
            [safeThis, status, finish, slot, slotData](const juce::String& error)
            {
                if (safeThis == nullptr) return;
                auto result = error;
                if (result.isEmpty()
                    && !safeThis->engine.restorePluginState(slot, slotData["state"].toString(),
                                                            static_cast<bool>(slotData["bypassed"])))
                    result = "The plugin loaded, but its saved state could not be restored.";
                finish(slot, result);
            });
    }
}

void MainComponent::restoreSourceFromPreset(const juce::var& preset)
{
    const auto deviceState = preset["audioDeviceState"].toString();
    if (deviceState.isNotEmpty())
    {
        if (auto xml = juce::parseXML(deviceState))
        {
            const auto error = deviceManager.initialise(2, 2, xml.get(), true);
            if (error.isNotEmpty())
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                        "Audio device warning", error);
        }
    }

    const bool repeat = static_cast<bool>(preset["repeat"]);
    const bool mono = static_cast<bool>(preset["monoInput"]);
    const bool live = preset["sourceMode"].toString() == "live";
    const double gainDb = preset.hasProperty("outputGainDb")
                            ? static_cast<double>(preset["outputGainDb"]) : 0.0;

    repeatButton.setToggleState(repeat, juce::dontSendNotification);
    engine.setLooping(repeat);
    monoInputButton.setToggleState(mono, juce::dontSendNotification);
    monoInputButton.setEnabled(live);
    engine.setMonoInput(mono);
    gainSlider.setValue(gainDb, juce::sendNotificationSync);

    liveInputButton.setToggleState(live, juce::dontSendNotification);
    engine.setLiveInput(live);
    playButton.setEnabled(!live);
    stopButton.setEnabled(!live);
    positionSlider.setEnabled(!live);

    if (!live)
    {
        const juce::File audioFile(preset["audioFile"].toString());
        if (audioFile.existsAsFile())
            openAudioFile(audioFile);
        else if (audioFile.getFullPathName().isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                "Audio file missing", "The preset audio file was not found:\n" + audioFile.getFullPathName());
    }
}

void MainComponent::refreshInsert(int slot)
{
    const auto name = engine.getPluginName(slot);
    const bool loaded = name != "Empty";
    insertButtons[slot].setButtonText(name);
    pluginSelectors[slot].setButtonText("Select VST3...");
    insertButtons[slot].setEnabled(loaded);
    pluginSlotLoading[slot] = false;
    pluginSelectors[slot].setEnabled(!scannedPlugins.empty());
    bypassButtons[slot].setEnabled(loaded); clearButtons[slot].setEnabled(loaded);
    bypassButtons[slot].setToggleState(engine.isPluginBypassed(slot), juce::dontSendNotification);
    repaint();
}

void MainComponent::clearInsert(int slot)
{
    if (!juce::isPositiveAndBelow(slot, AudioEngine::numInserts)) return;

    // Keep the native editor and its processor paired until application shutdown.
    // A few VST3 editors leave native message-loop work pending when destroyed from
    // their Clear button callback, which can otherwise freeze the host UI.
    if (pluginWindows[slot] != nullptr)
    {
        pluginWindows[slot]->setVisible(false);
        retiredPluginWindows.push_back(std::move(pluginWindows[slot]));
    }

    engine.clearPlugin(slot);
    refreshInsert(slot);
}

juce::String MainComponent::timeText(double seconds)
{
    const int total = juce::jmax(0, juce::roundToInt(seconds));
    return juce::String(total / 60).paddedLeft('0', 2) + ":" + juce::String(total % 60).paddedLeft('0', 2);
}
