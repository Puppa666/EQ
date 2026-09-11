#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BalancEQProcessor::BalancEQProcessor()
    : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    bypassParam = apvts.getRawParameterValue("bypass");
    hpFreqParam = apvts.getRawParameterValue("hp_freq");

    for (int b = 0; b < kNumBands; ++b)
    {
        juce::String paramId = "band_" + juce::String(b) + "_gain";
        bandGains[b] = apvts.getRawParameterValue(paramId);
    }

    int numBins = kFFTSize / 2 + 1;
    displaySpectrumDb.resize(numBins, -100.0f);
    displayRefDb.resize(numBins, 0.0f);
    displayCorrectionDb.resize(numBins, 0.0f);
    displayFreqs.resize(numBins, 0.0f);

    for (int i = 0; i < numBins; ++i)
        displayFreqs[i] = static_cast<float>(i) * 44100.0f / (float)kFFTSize;
}

BalancEQProcessor::~BalancEQProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout BalancEQProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "hp_freq", "High-Pass Freq",
        juce::StringArray({"Off", "20 Hz", "30 Hz", "40 Hz", "60 Hz", "80 Hz"}),
        1));

    for (int b = 0; b < kNumBands; ++b)
    {
        juce::String paramId = "band_" + juce::String(b) + "_gain";
        juce::String paramName = juce::String(kBandLabels[b]) + " Gain";
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramId, paramName, -12.0f, 12.0f, 0.0f));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
void BalancEQProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    eq.prepare(spec);

    accumulator.setSize(1, kAccumulatorSize);
    accumulator.clear();
    accumulatorWritePos = 0;
    accumulatorFull = false;

    int numBins = kFFTSize / 2 + 1;
    for (int i = 0; i < numBins; ++i)
        displayFreqs[i] = static_cast<float>(i) * static_cast<float>(sampleRate) / (float)kFFTSize;

    SpectralAnalyzer::computeReferenceCurve(
        displayFreqs.data(), displayRefDb.data(), numBins, static_cast<float>(sampleRate));
}

void BalancEQProcessor::releaseResources() {}

bool BalancEQProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

//==============================================================================
void BalancEQProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (bypassParam != nullptr && bypassParam->load() > 0.5f)
        return;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Accumulate mono signal for analysis
    {
        juce::SpinLock::ScopedLockType lock(accumulatorLock);
        const float* ch0 = buffer.getReadPointer(0);
        float* acc = accumulator.getWritePointer(0);

        for (int s = 0; s < numSamples; ++s)
        {
            float sample = ch0[s];
            if (numChannels > 1)
            {
                const float* ch1 = buffer.getReadPointer(1);
                sample = (sample + ch1[s]) * 0.5f;
            }
            acc[accumulatorWritePos] = sample;
            accumulatorWritePos = (accumulatorWritePos + 1) % kAccumulatorSize;
            if (accumulatorWritePos == 0)
                accumulatorFull = true;
        }
    }

    if (analysisReady)
    {
        // Process through EQ
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            for (int s = 0; s < numSamples; ++s)
                channelData[s] = eq.processSample(channelData[s]);
        }
    }
}

//==============================================================================
void BalancEQProcessor::triggerAnalysis()
{
    if (!accumulatorFull)
        return;

    juce::SpinLock::ScopedLockType lock(accumulatorLock);

    float sr = static_cast<float>(getSampleRate());
    int numBins = kFFTSize / 2 + 1;

    std::vector<float> spectrumDb(numBins, 0.0f);
    analyzer.analyzeSpectrum(accumulator.getReadPointer(0), kAccumulatorSize,
                              spectrumDb.data(), numBins, sr);

    BandCorrection newCorrections[kNumBands];
    analyzer.computeBandCorrections(spectrumDb.data(), displayRefDb.data(),
                                     displayFreqs.data(), numBins,
                                     newCorrections, sr);

    for (int b = 0; b < kNumBands; ++b)
    {
        corrections[b] = newCorrections[b];
        if (bandGains[b] != nullptr)
            bandGains[b]->store(newCorrections[b].gainDb);
    }

    displaySpectrumDb = spectrumDb;
    analyzer.computeCorrectionCurve(newCorrections, displayCorrectionDb.data(),
                                      displayFreqs.data(), numBins);

    eq.applyCorrections(newCorrections);
    analysisReady = true;

    accumulator.clear();
    accumulatorWritePos = 0;
    accumulatorFull = false;
}

//==============================================================================
juce::AudioProcessorEditor* BalancEQProcessor::createEditor()
{
    return new BalancEQEditor (*this);
}

bool BalancEQProcessor::hasEditor() const { return true; }

//==============================================================================
const juce::String BalancEQProcessor::getName() const { return "BalancEQ"; }
bool BalancEQProcessor::acceptsMidi() const { return false; }
bool BalancEQProcessor::producesMidi() const { return false; }
double BalancEQProcessor::getTailLengthSeconds() const { return 0.0; }

int BalancEQProcessor::getNumPrograms() { return 1; }
int BalancEQProcessor::getCurrentProgram() { return 0; }
void BalancEQProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String BalancEQProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void BalancEQProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

//==============================================================================
void BalancEQProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BalancEQProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// This function is called by JUCE to create plugin instances
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BalancEQProcessor();
}
