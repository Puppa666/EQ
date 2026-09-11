#pragma once

#include <JuceHeader.h>
#include "SpectralAnalyzer.h"
#include "AutoBalancingEQ.h"


class BalancEQProcessor  : public juce::AudioProcessor
{
public:
    BalancEQProcessor();
    ~BalancEQProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void triggerAnalysis();

    bool hasAnalysisResult() const { return analysisReady; }
    const BandCorrection& getBandCorrection(int band) const { return corrections[band]; }
    const std::vector<float>& getSpectrumDb() const { return displaySpectrumDb; }
    const std::vector<float>& getRefDb() const { return displayRefDb; }
    const std::vector<float>& getCorrectionCurveDb() const { return displayCorrectionDb; }
    const std::vector<float>& getFreqs() const { return displayFreqs; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

private:
    static constexpr int kAccumulatorSize = kFFTSize * 8;
    juce::AudioBuffer<float> accumulator;
    int accumulatorWritePos = 0;
    bool accumulatorFull = false;
    juce::SpinLock accumulatorLock;

    SpectralAnalyzer analyzer;
    AutoBalancingEQ eq;

    BandCorrection corrections[kNumBands] = {};
    std::vector<float> displaySpectrumDb;
    std::vector<float> displayRefDb;
    std::vector<float> displayCorrectionDb;
    std::vector<float> displayFreqs;
    bool analysisReady = false;
    bool bypassed = false;

    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* hpFreqParam = nullptr;
    std::atomic<float>* bandGains[kNumBands] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BalancEQProcessor)
};
