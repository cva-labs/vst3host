// Copyright (C) 2026 CVA Labs. SPDX-License-Identifier: AGPL-3.0-only
#include <JuceHeader.h>
#include "MainComponent.h"

class VST3PlayerHostApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "VST3 Player Host"; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise(const juce::String&) override
    {
        const auto arguments = getCommandLineParameterArray();
        if (arguments.size() >= 3 && arguments[0] == "--scan-vst3")
        {
            scanPlugins({ arguments[1] }, juce::File(arguments[2]));
            juce::MessageManager::callAsync([this] { quit(); });
            return;
        }
        if (arguments.size() >= 3 && arguments[0] == "--scan-vst3-batch")
        {
            juce::StringArray identifiers;
            for (int i = 2; i < arguments.size(); ++i) identifiers.add(arguments[i]);
            scanPlugins(identifiers, juce::File(arguments[1]));
            juce::MessageManager::callAsync([this] { quit(); });
            return;
        }
        if (arguments.size() >= 3 && arguments[0] == "--test-audio")
        {
            AudioEngine engine;
            juce::String error;
            const bool loaded = engine.loadAudioFile(juce::File(arguments[1]), error);
            juce::File(arguments[2]).replaceWithText(loaded
                ? "OK length=" + juce::String(engine.getLength(), 3)
                : "ERROR " + error);
            juce::MessageManager::callAsync([this] { quit(); });
            return;
        }
        window = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }

private:
    static void scanPlugins(const juce::StringArray& identifiers, const juce::File& outputFile)
    {
        juce::VST3PluginFormat format;
        juce::Array<juce::var> result;

        for (const auto& identifier : identifiers)
        {
            juce::OwnedArray<juce::PluginDescription> descriptions;
            format.findAllTypesForFile(descriptions, identifier);
            for (const auto* description : descriptions)
            {
                if (description == nullptr) continue;
                auto item = std::make_unique<juce::DynamicObject>();
                item->setProperty("descriptionXml", description->createXml()->toString());
                item->setProperty("name", description->name);
                item->setProperty("manufacturer", description->manufacturerName);
                item->setProperty("path", description->fileOrIdentifier);
                item->setProperty("category", description->category);
                result.add(juce::var(item.release()));
            }
        }

        outputFile.replaceWithText(juce::JSON::toString(juce::var(result), false));
    }

    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name, juce::Colour(0xff15171b), allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, true);
            setResizeLimits(750, 500, 1536, 1200);
            if (auto* boundsConstrainer = getConstrainer())
                boundsConstrainer->setFixedAspectRatio(1536.0 / 1024.0);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    };

    std::unique_ptr<MainWindow> window;
};

START_JUCE_APPLICATION(VST3PlayerHostApplication)
