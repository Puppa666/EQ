#pragma once

#include <JuceHeader.h>

inline constexpr int kFFTOrder = 13;
inline constexpr int kFFTSize = 1 << kFFTOrder;
inline constexpr float kReferenceSlope = -4.5f;
inline constexpr int kNumBands = 6;
inline constexpr float kMaxCorrectionDb = 12.0f;
inline constexpr float kRefFrequency = 1000.0f;

inline constexpr float kBandCenterHz[kNumBands] = {
    60.0f, 200.0f, 800.0f, 2500.0f, 8000.0f, 15000.0f
};

inline constexpr float kBandQ[kNumBands] = {
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.7f
};

inline constexpr const char* kBandLabels[kNumBands] = {
    "Sub", "Low", "Low-Mid", "Mid", "Hi-Mid", "High"
};

struct BandCorrection {
    float centerHz = 0.0f;
    float q = 1.0f;
    float gainDb = 0.0f;
    float gainLinear = 1.0f;
};


class SpectralAnalyzer
{
public:
    SpectralAnalyzer()
    {
        windowBuffer.resize(kFFTSize);
        for (int i = 0; i < kFFTSize; ++i)
            windowBuffer[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (float)(kFFTSize - 1)));
    }

    ~SpectralAnalyzer() = default;

    static void computeReferenceCurve(const float* freqs, float* refDb, int numBins, float sampleRate)
    {
        for (int i = 0; i < numBins; ++i)
        {
            float f = freqs[i];
            if (f > 0.0f)
                refDb[i] = kReferenceSlope * std::log2(f / kRefFrequency);
            else
                refDb[i] = 0.0f;
        }
    }

    void analyzeSpectrum(const float* channelData, int numSamples,
                         float* spectrumDb, int numBins, float /*sampleRate*/)
    {
        juce::dsp::FFT fft(kFFTOrder);

        int hopSize = kFFTSize / 2;
        int numFrames = juce::jmax(1, (numSamples - kFFTSize) / hopSize + 1);
        std::vector<float> accumSpectrum(numBins, 0.0f);
        int validFrames = 0;

        for (int frame = 0; frame < numFrames; ++frame)
        {
            int start = frame * hopSize;
            int avail = juce::jmin(kFFTSize, numSamples - start);
            if (avail < hopSize) break;

            std::vector<float> windowed(kFFTSize, 0.0f);
            for (int i = 0; i < avail; ++i)
                windowed[i] = channelData[start + i] * windowBuffer[i];

            std::vector<float> fftData(kFFTSize * 2, 0.0f);
            for (int i = 0; i < kFFTSize; ++i)
                fftData[i * 2] = windowed[i];

            fft.performRealOnlyForwardTransform(fftData.data(), true);

            for (int i = 0; i < numBins; ++i)
            {
                float re = fftData[i * 2];
                float im = fftData[i * 2 + 1];
                accumSpectrum[i] += std::sqrt(re * re + im * im);
            }
            validFrames++;
        }

        float invFrames = 1.0f / (float)juce::jmax(1, validFrames);
        for (int i = 0; i < numBins; ++i)
        {
            float avgMag = accumSpectrum[i] * invFrames;
            spectrumDb[i] = 20.0f * std::log10(avgMag + 1e-10f);
        }
    }

    void computeBandCorrections(const float* spectrumDb, const float* refDb,
                                 const float* freqs, int numBins,
                                 BandCorrection* corrections, float /*sampleRate*/)
    {
        for (int b = 0; b < kNumBands; ++b)
        {
            float fc = kBandCenterHz[b];
            float fLow = fc / 2.0f;
            float fHigh = fc * 2.0f;

            float sumDiff = 0.0f;
            int count = 0;

            for (int i = 0; i < numBins; ++i)
            {
                if (freqs[i] >= fLow && freqs[i] <= fHigh)
                {
                    sumDiff += (spectrumDb[i] - refDb[i]);
                    count++;
                }
            }

            float avgDiff = (count > 0) ? (sumDiff / (float)count) : 0.0f;
            float correction = -avgDiff;
            correction = juce::jlimit(-kMaxCorrectionDb, kMaxCorrectionDb, correction);

            corrections[b].centerHz = fc;
            corrections[b].q = kBandQ[b];
            corrections[b].gainDb = correction;
            corrections[b].gainLinear = std::pow(10.0f, correction / 20.0f);
        }
    }

    void computeCorrectionCurve(const BandCorrection* corrections,
                                  float* curveDb, const float* freqs, int numBins)
    {
        for (int i = 0; i < numBins; ++i)
            curveDb[i] = 0.0f;

        for (int b = 0; b < kNumBands; ++b)
        {
            float fc = corrections[b].centerHz;
            float gain = corrections[b].gainDb;

            for (int i = 0; i < numBins; ++i)
            {
                float f = freqs[i];
                if (f <= 0.0f) continue;
                float logRatio = std::log2(f / fc);
                float bell = gain * std::exp(-0.5f * (logRatio * 2.5f) * (logRatio * 2.5f));
                curveDb[i] += bell;
            }
        }
    }

private:
    std::vector<float> windowBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralAnalyzer)
};
