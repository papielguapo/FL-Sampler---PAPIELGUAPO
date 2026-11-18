#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "DSP/PrecomputedEffects.h"
#include "DSP/SamplerEngine.h"
#include <mutex>

class FLSamplerAudioProcessor : public juce::AudioProcessor,
                                private juce::AudioProcessorValueTreeState::Listener
{
public:
    FLSamplerAudioProcessor();
    ~FLSamplerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Sample management
    bool loadSampleFromFile(const juce::File& file);
    void clearSample();
    bool stepFolderSample(int delta);
    void triggerPreviewNote(int midiNote = 60, float velocity = 0.85f, float startNorm = -1.0f);
    void stopPreviewNote(int midiNote = 60);
    float snapNormalizedToZeroCrossing(float normPos) const;
    juce::File exportProcessedSampleToTempWav() const;

    juce::String getLoadedFileName() const;
    juce::File   getLoadedFile() const;
    juce::StringArray getRecentSamplePaths() const;
    bool         hasSampleLoaded() const noexcept { return sampleLoaded.load(std::memory_order_relaxed); }
    int          getDetectedSampleRootNote() const noexcept { return detectedSampleRoot.load(std::memory_order_relaxed); }
    double       getDetectedSampleBpm() const noexcept { return detectedSampleBpm.load(std::memory_order_relaxed); }
    const std::vector<float>& getSliceMarkers() const noexcept { return sliceMarkers; }

    // Thread-safe copy of processed sample buffer for WAVScope waveform display
    void getProcessedWaveformCopy(juce::AudioBuffer<float>& outBuf) const;
    float getActivePlayheadNorm() const noexcept { return samplerEngine.getActivePlayheadNorm(); }
    float getActiveVoiceAgeSec() const noexcept { return samplerEngine.getActiveVoiceAgeSec(); }
    uint32_t getWaveformRevision() const noexcept { return waveformRevision.load(std::memory_order_relaxed); }

    void rebuildPrecomputedBuffer();

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Non-APVTS quick state for Root note / Track / Keep on disk / Load regions / Slice markers
    std::atomic<bool>  keepOnDisk       { false };
    std::atomic<bool>  loadRegions      { true };
    std::atomic<bool>  loadSliceMarkers { false };
    std::atomic<bool>  zeroCrossSnap    { true };
    std::atomic<int>   mixerTrack       { 0 }; // 0 = "---", 1..125 = Cross-Instance Choke Link Group
    std::atomic<int>   cutGroup         { 0 }; // 0 = "---", 1..99  = Cut Group
    std::atomic<int>   cutByGroup       { 0 }; // 0 = "---", 1..99  = Cut By Group
    std::atomic<int>   keyRegionLow     { 0 };
    std::atomic<int>   keyRegionHigh    { 127 };
    std::atomic<bool>  addToKey         { false };
    std::atomic<bool>  fullPorta        { false };
    std::atomic<float> shiftTimeAmt     { 0.0f };
    std::atomic<float> swingMulAmt      { 0.0f };
    std::atomic<bool>  trackingKeyMode  { true };
    std::atomic<float> lvlPanOffset     { 0.0f };
    std::atomic<float> lvlVolOffset     { 0.78f };
    std::atomic<float> lvlModXOffset    { 0.0f };
    std::atomic<float> lvlModYOffset    { 0.0f };
    std::atomic<float> trackPanAmt      { 0.0f };
    std::atomic<float> trackModXAmt     { 0.0f };
    std::atomic<float> trackModYAmt     { 0.0f };
    std::atomic<int>   trackMidNote     { 60 };
    std::atomic<float> lfoDelayAmt      { 0.0f };
    std::atomic<float> lfoAttackAmt     { 0.35f };
    std::atomic<int>   lfoShapeIdx      { 0 };
    std::atomic<int>   arpRepeatCount   { 1 };
    std::atomic<bool>  arpSlideOn       { false };
    std::atomic<int>   arpChordMode     { 0 };
    std::atomic<float> echoPanAmt       { 0.0f };
    std::atomic<float> echoModXAmt      { 0.65f };
    std::atomic<float> echoModYAmt      { 0.25f };
    std::atomic<float> echoPitchAmt     { 0.0f };
    std::atomic<bool>  echoFatModeOn    { false };

    // 5-Target AHDSR & LFO Banks (0=Pan, 1=Vol, 2=ModX, 3=ModY, 4=Pitch)
    std::array<fls::AHDSREnvelope, 5> targetEnvs {{
        { false, 0.0f, 0.05f, 0.0f, 0.30f, 0.0f, 0.20f, 0.0f, 0.0f, -0.35f, -0.45f, false }, // Pan
        { false, 0.0f, 0.02f, 0.0f, 0.35f, 0.8f, 0.20f, 1.0f, 0.0f, -0.35f, -0.45f, false }, // Vol
        { false, 0.0f, 0.05f, 0.0f, 0.30f, 0.0f, 0.20f, 0.0f, 0.0f, -0.35f, -0.45f, false }, // Mod X
        { false, 0.0f, 0.05f, 0.0f, 0.30f, 0.0f, 0.20f, 0.0f, 0.0f, -0.35f, -0.45f, false }, // Mod Y
        { false, 0.0f, 0.02f, 0.0f, 0.20f, 0.0f, 0.15f, 0.0f, 0.0f, -0.35f, -0.45f, false }  // Pitch
    }};
    std::array<fls::LFOModulator, 5> targetLfos {};
    juce::StringArray recentSampleFiles;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    fls::PrecomputedSettings gatherPrecomputedSettings() const;
    fls::SamplerRuntimeParams gatherRuntimeParams() const;

    juce::AudioFormatManager formatManager;
    mutable juce::CriticalSection sampleLock;
    juce::AudioBuffer<float> rawSampleBuffer;
    juce::AudioBuffer<float> processedSampleBuffer;
    double loadedSampleRate = 44100.0;
    juce::File currentSampleFile;
    std::vector<float> sliceMarkers;
    std::atomic<int>    detectedSampleRoot { 60 };
    std::atomic<double> detectedSampleBpm  { 0.0 };
    std::atomic<double> currentHostBpm     { 120.0 };
    std::atomic<float>  midiPitchWheel     { 0.0f };
    std::atomic<float>  midiModWheel       { 0.0f };
    std::atomic<float>  midiAftertouch     { 0.0f };
    std::atomic<bool> sampleLoaded { false };
    std::atomic<bool> needsPrecompute { false };
    std::atomic<uint32_t> waveformRevision { 1 };

    // Pending preview notes from UI click on waveform or mini-keyboard
    std::atomic<int>   pendingNoteOn       { -1 };
    std::atomic<float> pendingNoteStartPos { -1.0f };
    std::atomic<int>   pendingNoteOff      { -1 };

    fls::SamplerEngine samplerEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLSamplerAudioProcessor)
};
