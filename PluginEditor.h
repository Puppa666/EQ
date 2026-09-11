#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


class SpectrumDisplay  : public juce::Component
{
public:
    SpectrumDisplay(BalancEQProcessor& proc)
        : processor(proc)
    {
        setOpaque(true);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff0d1117));

        auto bounds = getLocalBounds().toFloat().reduced(10, 10);
        auto plotArea = bounds.withTrimmedBottom(30).withTrimmedLeft(45).withTrimmedRight(15);

        int numBins = static_cast<int>(processor.getFreqs().size());
        if (numBins == 0)
        {
            g.setColour(juce::Colour(0xff555555));
            g.setFont(14.0f);
            g.drawFittedText("Press RESET to analyze", getLocalBounds(),
                            juce::Justification::centred, 1);
            return;
        }

        const auto& freqs = processor.getFreqs();
        const auto& spectrumDb = processor.getSpectrumDb();
        const auto& refDb = processor.getRefDb();
        const auto& corrDb = processor.getCorrectionCurveDb();

        float fMin = 20.0f, fMax = 20000.0f;
        float dbMin = -40.0f, dbMax = 10.0f;

        auto freqToX = [fMin, fMax, plotArea](float f) -> float {
            float logMin = std::log10(fMin);
            float logMax = std::log10(fMax);
            float t = (std::log10(juce::jmax(fMin, f)) - logMin) / (logMax - logMin);
            return plotArea.getX() + t * plotArea.getWidth();
        };

        auto dbToY = [dbMin, dbMax, plotArea](float db) -> float {
            float t = (db - dbMin) / (dbMax - dbMin);
            return plotArea.getBottom() - t * plotArea.getHeight();
        };

        // Grid
        g.setColour(juce::Colour::fromRGBA(0x30, 0x30, 0x60, 0x80));
        for (float f : {50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f})
            g.drawVerticalLine(static_cast<int>(freqToX(f)), static_cast<int>(plotArea.getY()),
                              static_cast<int>(plotArea.getBottom()));
        for (float db = -30.0f; db <= 0.0f; db += 10.0f)
            g.drawHorizontalLine(static_cast<int>(dbToY(db)), static_cast<int>(plotArea.getX()),
                                 static_cast<int>(plotArea.getRight()));

        if (!processor.hasAnalysisResult())
        {
            // Reference only
            juce::Path refPath;
            bool started = false;
            for (int i = 0; i < numBins; ++i)
            {
                if (freqs[i] < fMin || freqs[i] > fMax) continue;
                float x = freqToX(freqs[i]);
                float y = dbToY(juce::jlimit(dbMin, dbMax, refDb[i]));
                if (!started) { refPath.startNewSubPath(x, y); started = true; }
                else refPath.lineTo(x, y);
            }
            g.setColour(juce::Colour(0xffff9800));
            float dashLengths1[] = {4.0f, 4.0f};
            juce::Path dashedRefPath;
            juce::PathStrokeType(1.5f).createDashedStroke(dashedRefPath, refPath, dashLengths1, 2);
            g.strokePath(dashedRefPath, juce::PathStrokeType(1.5f));

            g.setColour(juce::Colour(0xff555555));
            g.setFont(14.0f);
            g.drawFittedText("Waiting for RESET...", getLocalBounds(), juce::Justification::centred, 1);
            return;
        }

        // Spectrum fill
        juce::Path specFill;
        bool started = false;
        for (int i = 0; i < numBins; ++i)
        {
            if (freqs[i] < fMin || freqs[i] > fMax) continue;
            float x = freqToX(freqs[i]);
            float y = dbToY(juce::jlimit(dbMin, dbMax, spectrumDb[i]));
            if (!started) { specFill.startNewSubPath(x, dbToY(dbMin)); specFill.lineTo(x, y); started = true; }
            else specFill.lineTo(x, y);
        }
        specFill.lineTo(freqToX(fMax), dbToY(dbMin));
        specFill.closeSubPath();
        g.setColour(juce::Colour::fromRGBA(0x40, 0x80, 0xf7, 0x30));
        g.fillPath(specFill);

        // Input spectrum line
        juce::Path specPath;
        started = false;
        for (int i = 0; i < numBins; ++i)
        {
            if (freqs[i] < fMin || freqs[i] > fMax) continue;
            float x = freqToX(freqs[i]);
            float y = dbToY(juce::jlimit(dbMin, dbMax, spectrumDb[i]));
            if (!started) { specPath.startNewSubPath(x, y); started = true; }
            else specPath.lineTo(x, y);
        }
        g.setColour(juce::Colour(0x44, 0xc3, 0xf7));
        g.strokePath(specPath, juce::PathStrokeType(1.2f));

        // Reference dashed
        juce::Path refPath;
        started = false;
        for (int i = 0; i < numBins; ++i)
        {
            if (freqs[i] < fMin || freqs[i] > fMax) continue;
            float x = freqToX(freqs[i]);
            float y = dbToY(juce::jlimit(dbMin, dbMax, refDb[i]));
            if (!started) { refPath.startNewSubPath(x, y); started = true; }
            else refPath.lineTo(x, y);
        }
        g.setColour(juce::Colour(0xffff9800));
        float dashLengths2[] = {4.0f, 4.0f};
        juce::Path dashedRefPath2;
        juce::PathStrokeType(1.5f).createDashedStroke(dashedRefPath2, refPath, dashLengths2, 2);
        g.strokePath(dashedRefPath2, juce::PathStrokeType(1.5f));

        // Corrected spectrum
        juce::Path corrPath;
        started = false;
        for (int i = 0; i < numBins; ++i)
        {
            if (freqs[i] < fMin || freqs[i] > fMax) continue;
            float x = freqToX(freqs[i]);
            float correctedDb = spectrumDb[i] + corrDb[i];
            float y = dbToY(juce::jlimit(dbMin, dbMax, correctedDb));
            if (!started) { corrPath.startNewSubPath(x, y); started = true; }
            else corrPath.lineTo(x, y);
        }
        g.setColour(juce::Colour(0x4c, 0xaf, 0x50));
        g.strokePath(corrPath, juce::PathStrokeType(2.0f));

        // EQ correction curve
        juce::Path eqPath;
        started = false;
        for (int i = 0; i < numBins; ++i)
        {
            if (freqs[i] < fMin || freqs[i] > fMax) continue;
            float x = freqToX(freqs[i]);
            float y = dbToY(juce::jlimit(dbMin, dbMax, corrDb[i]));
            if (!started) { eqPath.startNewSubPath(x, y); started = true; }
            else eqPath.lineTo(x, y);
        }
        g.setColour(juce::Colour::fromRGBA(0xff, 0xe9, 0x45, 0x60));
        g.strokePath(eqPath, juce::PathStrokeType(1.5f));

        // Band markers
        for (int b = 0; b < kNumBands; ++b)
        {
            float fc = kBandCenterHz[b];
            float gain = processor.getBandCorrection(b).gainDb;
            float x = freqToX(fc);
            juce::Colour colour;
            if (std::abs(gain) > 3.0f) colour = juce::Colour(0xffe94560);
            else if (std::abs(gain) > 1.0f) colour = juce::Colour(0xffff9800);
            else colour = juce::Colour(0xff4caf50);
            g.setColour(colour.withAlpha(0.3f));
            g.drawVerticalLine(static_cast<int>(x), static_cast<int>(plotArea.getY()),
                              static_cast<int>(plotArea.getBottom()));
            juce::String gainStr = (gain > 0 ? "+" : "") + juce::String(gain, 1) + "dB";
            g.setColour(colour);
            g.setFont(10.0f);
            g.drawFittedText(gainStr, static_cast<int>(x) - 25, static_cast<int>(plotArea.getY()) + 2, 50, 14,
                            juce::Justification::centred, 1);
        }

        // Frequency labels
        g.setColour(juce::Colour(0xffa0a0a0));
        g.setFont(9.0f);
        for (float f : {50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f})
        {
            float x = freqToX(f);
            juce::String label = (f >= 1000.0f) ? (juce::String(f / 1000.0f, 0) + "k") : juce::String(static_cast<int>(f));
            g.drawFittedText(label, static_cast<int>(x) - 12, static_cast<int>(plotArea.getBottom()) + 4, 24, 14,
                            juce::Justification::centred, 1);
        }
        for (float db = -30.0f; db <= 0.0f; db += 10.0f)
        {
            float y = dbToY(db);
            g.drawFittedText(juce::String(static_cast<int>(db)),
                            static_cast<int>(plotArea.getX()) - 35, static_cast<int>(y) - 7, 30, 14,
                            juce::Justification::right, 1);
        }
    }

private:
    BalancEQProcessor& processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};


class BalancEQEditor  : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    BalancEQEditor(BalancEQProcessor& p)
        : AudioProcessorEditor(&p), processor(p), spectrumDisplay(p)
    {
        setOpaque(true);
        setSize(600, 450);

        addAndMakeVisible(spectrumDisplay);

        resetButton.setButtonText("RESET");
        resetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe94560));
        resetButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        resetButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff6b6b));
        resetButton.onClick = [this] { resetButtonClicked(); };
        addAndMakeVisible(resetButton);

        statusLabel.setText("Load audio and press RESET", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
        addAndMakeVisible(statusLabel);

        for (int b = 0; b < kNumBands; ++b)
        {
            bandLabels[b].setText(juce::String(kBandLabels[b]), juce::dontSendNotification);
            bandLabels[b].setColour(juce::Label::textColourId, juce::Colour(0xffa0c4ff));
            bandLabels[b].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(bandLabels[b]);

            freqLabels[b].setText(juce::String(static_cast<int>(kBandCenterHz[b])) + " Hz",
                                  juce::dontSendNotification);
            freqLabels[b].setColour(juce::Label::textColourId, juce::Colour(0xff666666));
            freqLabels[b].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(freqLabels[b]);

            gainLabels[b].setText("0.0 dB", juce::dontSendNotification);
            gainLabels[b].setColour(juce::Label::textColourId, juce::Colour(0xffe94560));
            gainLabels[b].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(gainLabels[b]);
        }

        startTimerHz(4);
    }

    ~BalancEQEditor() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff1a1a2e));
        g.setColour(juce::Colour(0xffe94560));
        g.setFont(22.0f);
        g.drawFittedText("BalancEQ", 15, 8, 200, 28, juce::Justification::centredLeft, 1);
        g.setColour(juce::Colour(0xffa0c4ff));
        g.setFont(11.0f);
        g.drawFittedText("Intelligent Spectral Balancing EQ | -4.5 dB/oct",
                        150, 14, 350, 16, juce::Justification::centredLeft, 1);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 45);
        spectrumDisplay.setBounds(area.withTrimmedBottom(90));

        auto bottomBar = area.removeFromBottom(85);
        resetButton.setBounds(bottomBar.removeFromLeft(120).withTrimmedTop(10));
        statusLabel.setBounds(bottomBar.removeFromLeft(200).withTrimmedTop(14));

        int bandWidth = bottomBar.getWidth() / kNumBands;
        for (int b = 0; b < kNumBands; ++b)
        {
            auto bandRect = bottomBar.removeFromLeft(bandWidth);
            bandLabels[b].setBounds(bandRect.withTrimmedTop(2).withHeight(16));
            freqLabels[b].setBounds(bandRect.withTrimmedTop(18).withHeight(14));
            gainLabels[b].setBounds(bandRect.withTrimmedTop(32).withHeight(18));
        }
    }

    void timerCallback() override
    {
        if (processor.hasAnalysisResult())
        {
            for (int b = 0; b < kNumBands; ++b)
            {
                float gain = processor.getBandCorrection(b).gainDb;
                juce::String gainStr = (gain > 0 ? "+" : "") + juce::String(gain, 1) + " dB";
                gainLabels[b].setText(gainStr, juce::dontSendNotification);
                if (std::abs(gain) > 3.0f)
                    gainLabels[b].setColour(juce::Label::textColourId, juce::Colour(0xffe94560));
                else if (std::abs(gain) > 1.0f)
                    gainLabels[b].setColour(juce::Label::textColourId, juce::Colour(0xffff9800));
                else
                    gainLabels[b].setColour(juce::Label::textColourId, juce::Colour(0xff4caf50));
            }
        }
        spectrumDisplay.repaint();
    }

private:
    void resetButtonClicked()
    {
        statusLabel.setText("Analyzing...", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff9800));
        repaint();

        juce::Timer::callAfterDelay(100, [this]() {
            processor.triggerAnalysis();
            statusLabel.setText("Balanced!", juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4caf50));
            repaint();
        });
    }

    BalancEQProcessor& processor;
    SpectrumDisplay spectrumDisplay;
    juce::TextButton resetButton;
    juce::Label statusLabel;
    juce::Label bandLabels[kNumBands];
    juce::Label freqLabels[kNumBands];
    juce::Label gainLabels[kNumBands];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BalancEQEditor)
};
