/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ZliMagFXDistortionAudioProcessor::ZliMagFXDistortionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), highShelfCutFilter(juce::dsp::IIR::Coefficients<float>::makeHighShelf(44100, 5000, 0.5, 0.05)),
                        highShelfBoostFilter(juce::dsp::IIR::Coefficients<float>::makeHighShelf(44100, 5000, 0.5, 20))
#endif
{
}

ZliMagFXDistortionAudioProcessor::~ZliMagFXDistortionAudioProcessor()
{
}

//==============================================================================
const juce::String ZliMagFXDistortionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ZliMagFXDistortionAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ZliMagFXDistortionAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ZliMagFXDistortionAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ZliMagFXDistortionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ZliMagFXDistortionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ZliMagFXDistortionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ZliMagFXDistortionAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ZliMagFXDistortionAudioProcessor::getProgramName (int index)
{
    return {};
}

void ZliMagFXDistortionAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ZliMagFXDistortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
//    lowPassFilter = juce::dsp::IIR::Filter<float> {}
            
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
 
    highShelfCutFilter.prepare(spec);
    highShelfCutFilter.reset();
    
    highShelfBoostFilter.prepare(spec);
    highShelfBoostFilter.reset();
}

void ZliMagFXDistortionAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ZliMagFXDistortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ZliMagFXDistortionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    juce::dsp::AudioBlock<float> block { buffer };
    juce::dsp::ProcessContextReplacing<float> processContext {block};

    highShelfBoostFilter.process(processContext);
    
    auto gainParam = parametersTree.getRawParameterValue("Gain")->load();

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        for(auto sample = 0; sample < buffer.getNumSamples(); sample++) {
            auto gainedSampleValue = channelData[sample] * gainParam;
            channelData[sample] = juce::dsp::FastMathApproximations::tanh(gainedSampleValue);
        }
    }
    
    highShelfCutFilter.process(processContext);
}

//==============================================================================
bool ZliMagFXDistortionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ZliMagFXDistortionAudioProcessor::createEditor()
{
//    return new ZliMagFXDistortionAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void ZliMagFXDistortionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ZliMagFXDistortionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZliMagFXDistortionAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout ZliMagFXDistortionAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    juce::NormalisableRange<float> gainRange {0.0, 10000.0, 0.5, 1.0};
    juce::ParameterID gainId {"Gain", 1};
    layout.add(std::make_unique<juce::AudioParameterFloat>(gainId, "Gain", gainRange, 0));
    
    return layout;
}
