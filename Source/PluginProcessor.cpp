#include "PluginProcessor.h"
#include "PluginEditor.h"

ALLHaasAudioProcessor::ALLHaasAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    props(),
    apvts(*this, nullptr, "params", param::createParameterLayout()),
    params
    {
    	apvts.getParameter(param::toID(param::PID::DistanceLM)),
        apvts.getParameter(param::toID(param::PID::CutoffLM)),
        apvts.getParameter(param::toID(param::PID::FeedbackLM)),
        apvts.getParameter(param::toID(param::PID::DistanceRS)),
        apvts.getParameter(param::toID(param::PID::CutoffRS)),
        apvts.getParameter(param::toID(param::PID::FeedbackRS)),
        apvts.getParameter(param::toID(param::PID::StereoConfig)),
        apvts.getParameter(param::toID(param::PID::Mono))
    },
    allHaas(),
    oscilloscope()
#endif
{
    juce::PropertiesFile::Options options;
    options.applicationName = JucePlugin_Name;
    options.filenameSuffix = ".settings";
    options.folderName = JucePlugin_Manufacturer + juce::File::getSeparatorString() + JucePlugin_Name;
    options.osxLibrarySubFolder = "Application Support";
    options.commonToAllUsers = false;
    options.ignoreCaseOfKeyNames = true;
    options.doNotSave = false;
    options.millisecondsBeforeSaving = 100;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    props.setStorageParameters(options);
}

ALLHaasAudioProcessor::~ALLHaasAudioProcessor()
{
}

const juce::String ALLHaasAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ALLHaasAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ALLHaasAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ALLHaasAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ALLHaasAudioProcessor::getTailLengthSeconds() const
{
    return 0.;
}

int ALLHaasAudioProcessor::getNumPrograms()
{
    return 1;
}

int ALLHaasAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ALLHaasAudioProcessor::setCurrentProgram (int)
{
}

const juce::String ALLHaasAudioProcessor::getProgramName (int)
{
    return {};
}

void ALLHaasAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void ALLHaasAudioProcessor::prepareToPlay(double sampleRate, int maxBlockSize)
{
    allHaas.prepare(sampleRate, maxBlockSize);
}

void ALLHaasAudioProcessor::releaseResources()
{

}

bool ALLHaasAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto stereo = juce::AudioChannelSet::stereo();
    return mainIn == mainOut && mainOut == stereo;
}

void ALLHaasAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    {
        auto totalNumInputChannels = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear(i, 0, numSamples);
    }
    if (numSamples == 0)
        return;
    auto samples = buffer.getArrayOfWritePointers();

    const auto& cutoffLeftParam = *params[static_cast<int>(PID::CutoffLM)];
    const auto& cutoffRightParam = *params[static_cast<int>(PID::CutoffRS)];

    const auto& feedbackLeftParam = *params[static_cast<int>(PID::FeedbackLM)];
    const auto& feedbackRightParam = *params[static_cast<int>(PID::FeedbackRS)];

    const auto& cutoffRange = cutoffLeftParam.getNormalisableRange();
    const auto& feedbackRange = feedbackLeftParam.getNormalisableRange();

    const auto cutoffLeft = static_cast<double>(cutoffRange.convertFrom0to1(cutoffLeftParam.getValue()));
    const auto cutoffRight = static_cast<double>(cutoffRange.convertFrom0to1(cutoffRightParam.getValue()));
    const auto feedbackLeftHz = static_cast<double>(feedbackRange.convertFrom0to1(feedbackLeftParam.getValue()));
    const auto feedbackRightHz = static_cast<double>(feedbackRange.convertFrom0to1(feedbackRightParam.getValue()));

    const auto& distanceLeftParam = *params[static_cast<int>(PID::DistanceLM)];
    const auto& distanceRightParam = *params[static_cast<int>(PID::DistanceRS)];
    const auto& distanceRange = distanceLeftParam.getNormalisableRange();
    const auto numFiltersLeft = static_cast<int>(std::round(distanceRange.convertFrom0to1(distanceLeftParam.getValue())));
    const auto numFiltersRight = static_cast<int>(std::round(distanceRange.convertFrom0to1(distanceRightParam.getValue())));

    const auto isMidSide = params[static_cast<int>(PID::StereoConfig)]->getValue() > .5f;
    if(isMidSide)
        for(auto s = 0; s < numSamples; ++s)
		{
			const auto mid = (samples[0][s] + samples[1][s]) * .5f;
			const auto side = (samples[0][s] - samples[1][s]) * .5f;
			samples[0][s] = mid;
			samples[1][s] = side;
		}

    allHaas
    (
        samples,
        cutoffLeft, cutoffRight,
        feedbackLeftHz, feedbackRightHz,
        numFiltersLeft, numFiltersRight,
        numSamples
    );

    if (isMidSide)
        for (auto s = 0; s < numSamples; ++s)
		{
			const auto mid = samples[0][s];
			const auto side = samples[1][s];
			samples[0][s] = mid + side;
			samples[1][s] = mid - side;
		}

    const auto& monoParam = *params[static_cast<int>(PID::Mono)];
    if (monoParam.getValue() > .5f)
        for (auto s = 0; s < numSamples; ++s)
        {
            const auto mid = (samples[0][s] + samples[1][s]) * .5f;
            samples[0][s] = samples[1][s] = mid;
        }

    oscilloscope(samples, numSamples);
}

bool ALLHaasAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ALLHaasAudioProcessor::createEditor()
{
    return new ALLHaasAudioProcessorEditor (*this);
}

void ALLHaasAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ALLHaasAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ALLHaasAudioProcessor();
}