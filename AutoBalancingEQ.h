#pragma once

#include <JuceHeader.h>
#include "SpectralAnalyzer.h"


class AutoBalancingEQ
{
public:
    AutoBalancingEQ()
    {
        for (int b = 0; b < kNumBands; ++b)
        {
            bandFilters[b] = std::make_unique<juce::dsp::IIR::Filter<float>>();
        }
    }

    ~AutoBalancingEQ() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        for (int b = 0; b < kNumBands; ++b)
            bandFilters[b]->prepare(spec);
    }

    void reset()
    {
        for (int b = 0; b < kNumBands; ++b)
            bandFilters[b]->reset();
    }

    void resetCorrections()
    {
        for (int b = 0; b < kNumBands; ++b)
            applyBandGain(b, 0.0f);
    }

    void applyCorrections(const BandCorrection* corrections)
    {
        for (int b = 0; b < kNumBands; ++b)
        {
            currentCorrections[b] = corrections[b];
            applyBandGain(b, corrections[b].gainDb);
        }
    }

    float processSample(float input)
    {
        float output = input;
        for (int b = 0; b < kNumBands; ++b)
            output = bandFilters[b]->processSample(output);
        return output;
    }

    const BandCorrection& getCorrection(int band) const
    {
        jassert(band >= 0 && band < kNumBands);
        return currentCorrections[band];
    }

    double getSampleRate() const { return sampleRate; }

private:
    void applyBandGain(int band, float gainDb)
    {
        if (sampleRate <= 0.0) return;

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                  sampleRate,
                  kBandCenterHz[band],
                  kBandQ[band],
                  gainDb);
        *bandFilters[band]->coefficients = *coeffs;
    }

    std::array<std::unique_ptr<juce::dsp::IIR::Filter<float>>, kNumBands> bandFilters;
    BandCorrection currentCorrections[kNumBands] = {};
    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoBalancingEQ)
};
