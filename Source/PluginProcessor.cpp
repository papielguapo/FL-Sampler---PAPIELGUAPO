#include "PluginProcessor.h"
#include "PluginEditor.h"

FLSamplerAudioProcessor::FLSamplerAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "FLSamplerParams", createParameterLayout())
{
    formatManager.registerBasicFormats();

    const char* precomputeIds[] = {
        "removeDc", "normalize", "reverse", "reversePol", "fadeStereo", "swapStereo",
        "smpStart", "length", "fadeIn", "fadeOut", "crossfade", "trim",
        "boost", "clip", "eq", "rmMix", "rmFreq", "cut", "res", "reverb", "reverbB", "stereoDelay", "pogo"
    };
    for (auto* id : precomputeIds)
        apvts.addParameterListener(id, this);
}

FLSamplerAudioProcessor::~FLSamplerAudioProcessor()
{
    const char* precomputeIds[] = {
        "removeDc", "normalize", "reverse", "reversePol", "fadeStereo", "swapStereo",
        "smpStart", "length", "fadeIn", "fadeOut", "crossfade", "trim",
        "boost", "clip", "eq", "rmMix", "rmFreq", "cut", "res", "reverb", "reverbB", "stereoDelay", "pogo"
    };
    for (auto* id : precomputeIds)
        apvts.removeParameterListener(id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout FLSamplerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master Header Bar Controls
    params.push_back(std::make_unique<juce::AudioParameterBool>("masterOn", "Channel Enabled", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterPan", "Channel Pan",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterVol", "Channel Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.78f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterPitch", "Channel Pitch",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("pitchRange", "Pitch Knob Range", 1, 48, 2));

    // Content Panel
    params.push_back(std::make_unique<juce::AudioParameterBool>("resample", "Resample", true));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("declickMode", "Declicking Mode",
        juce::StringArray{
            "Out only (no bleeding)",
            "Transient (no bleeding)",
            "Transient (bleeding)",
            "Generic (bleeding)",
            "Smooth (bleeding)",
            "Crossfade (bleeding)"
        }, 0));

    // Playback Panel
    params.push_back(std::make_unique<juce::AudioParameterFloat>("startOffset", "Sample Start Offset",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("loopStart", "Loop Start",
        juce::NormalisableRange<float>(0.0f, 0.99f, 0.0001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("loopEnd", "Loop End",
        juce::NormalisableRange<float>(0.01f, 1.0f, 0.0001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("useLoopPoints", "Use Loop Points", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("pingPongLoop", "Ping Pong Loop", false));

    // Time Stretching Panel
    params.push_back(std::make_unique<juce::AudioParameterFloat>("stretchPitch", "Time Stretch Pitch",
        juce::NormalisableRange<float>(-1200.0f, 1200.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("stretchMul", "Time Stretch Multiplier",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.001f, 0.5f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("stretchTime", "Time Stretch Time",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("stretchMode", "Time Stretch Mode",
        juce::StringArray{
            "Resample",
            "e3 generic",
            "e3 mono",
            "Stretch",
            "Stretch pro",
            "Slice stretch",
            "Slice map",
            "Auto",
            "e2 generic",
            "e2 transient",
            "e2 mono",
            "e2 speech"
        }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("formantShift", "Formant Shift",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    // Precomputed Effects Panel (Standard - Wrench tab)
    params.push_back(std::make_unique<juce::AudioParameterBool>("removeDc", "Remove DC Offset", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("normalize", "Normalize", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("reverse", "Reverse", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("reversePol", "Reverse Polarity", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("fadeStereo", "Fade Stereo", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("swapStereo", "Swap Stereo", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("smpStart", "Precomputed Sample Start",
        juce::NormalisableRange<float>(0.0f, 0.99f, 0.0001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("length", "Precomputed Sample Length",
        juce::NormalisableRange<float>(0.01f, 1.0f, 0.0001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fadeIn", "Quick Fade In",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fadeOut", "Quick Fade Out",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("crossfade", "Loop Crossfade",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("trim", "Trim Silence Threshold",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));

    // Precomputed Effects Panel (Legacy - Bulb tab)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("boost", "Precomputed Boost",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("clip", "Clip Boost", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eq", "Precomputed EQ",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("rmMix", "Ring Modulation Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("rmFreq", "Ring Modulation Frequency",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("cut", "Precomputed Cutoff",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("res", "Precomputed Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverb", "Precomputed Reverb",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("reverbB", "Precomputed Reverb Mode B", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("stereoDelay", "Stereo Delay",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pogo", "Pogo Pitch Bend",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));

    // Tab 2: Envelope / Instrument Settings (InsSheet)
    params.push_back(std::make_unique<juce::AudioParameterBool>("volEnvOn", "Volume Envelope Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envDelay", "Envelope Delay",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envAttack", "Envelope Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.02f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envHold", "Envelope Hold",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envDecay", "Envelope Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envSustain", "Envelope Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.80f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envRelease", "Envelope Release",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.20f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfoAmt", "Pitch LFO Amount",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfoSpeed", "LFO Speed",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.50f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("filterModX", "Filter Cutoff (Mod X)",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filterModY", "Filter Resonance (Mod Y)",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("filterType", "Filter Type",
        juce::StringArray{ "Fast LP", "LP", "BP", "HP", "BS", "SVF LPx2" }, 0));

    // Tab 3: Miscellaneous Functions (FuncSheet)
    params.push_back(std::make_unique<juce::AudioParameterInt>("rootNote", "Root Note", 0, 127, 60));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fineTune", "Fine Tune (cents)",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("enableMainPitch", "Enable Main Pitch", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("porta", "Portamento", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("mono", "Monophonic", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("portaTime", "Portamento Slide Time",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.12f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("maxPoly", "Max Polyphony", 1, 32, 16));
    params.push_back(std::make_unique<juce::AudioParameterBool>("cutSelf", "Cut Self", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("gateTime", "Gate Time",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("arpDir", "Arpeggiator Direction",
        juce::StringArray{ "Off", "Up", "Down", "Up-Down (Bounce)", "Random" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("arpTime", "Arpeggiator Time",
        juce::NormalisableRange<float>(0.03f, 0.50f, 0.001f), 0.125f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("arpRange", "Arpeggiator Range (Octaves)", 1, 4, 1));

    params.push_back(std::make_unique<juce::AudioParameterInt>("echoCount", "Echo Count", 0, 10, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("echoFeed", "Echo Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("echoTime", "Echo Time",
        juce::NormalisableRange<float>(0.02f, 1.0f, 0.001f), 0.20f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("echoPingPong", "Echo Ping Pong", false));

    return { params.begin(), params.end() };
}

void FLSamplerAudioProcessor::parameterChanged(const juce::String&, float)
{
    needsPrecompute.store(true, std::memory_order_release);
}

fls::PrecomputedSettings FLSamplerAudioProcessor::gatherPrecomputedSettings() const
{
    fls::PrecomputedSettings s;
    s.removeDcOffset  = apvts.getRawParameterValue("removeDc")->load() > 0.5f;
    s.normalize       = apvts.getRawParameterValue("normalize")->load() > 0.5f;
    s.reverse         = apvts.getRawParameterValue("reverse")->load() > 0.5f;
    s.reversePolarity = apvts.getRawParameterValue("reversePol")->load() > 0.5f;
    s.fadeStereo      = apvts.getRawParameterValue("fadeStereo")->load() > 0.5f;
    s.swapStereo      = apvts.getRawParameterValue("swapStereo")->load() > 0.5f;

    s.smpStart        = apvts.getRawParameterValue("smpStart")->load();
    s.length          = apvts.getRawParameterValue("length")->load();
    s.fadeIn          = apvts.getRawParameterValue("fadeIn")->load();
    s.fadeOut         = apvts.getRawParameterValue("fadeOut")->load();
    s.crossfade       = apvts.getRawParameterValue("crossfade")->load();
    s.trim            = apvts.getRawParameterValue("trim")->load();

    s.boost           = apvts.getRawParameterValue("boost")->load();
    s.clip            = apvts.getRawParameterValue("clip")->load() > 0.5f;
    s.eq              = apvts.getRawParameterValue("eq")->load();
    s.rmMix           = apvts.getRawParameterValue("rmMix")->load();
    s.rmFreq          = apvts.getRawParameterValue("rmFreq")->load();
    s.cut             = apvts.getRawParameterValue("cut")->load();
    s.res             = apvts.getRawParameterValue("res")->load();
    s.reverb          = apvts.getRawParameterValue("reverb")->load();
    s.reverbB         = apvts.getRawParameterValue("reverbB")->load() > 0.5f;
    s.stereoDelay     = apvts.getRawParameterValue("stereoDelay")->load();
    s.pogo            = apvts.getRawParameterValue("pogo")->load();
    return s;
}

fls::SamplerRuntimeParams FLSamplerAudioProcessor::gatherRuntimeParams() const
{
    fls::SamplerRuntimeParams p;
    p.masterOn        = apvts.getRawParameterValue("masterOn")->load() > 0.5f;
    p.masterPan       = apvts.getRawParameterValue("masterPan")->load();
    p.masterVol       = apvts.getRawParameterValue("masterVol")->load();
    p.masterPitch     = apvts.getRawParameterValue("masterPitch")->load();
    p.pitchRange      = (int)apvts.getRawParameterValue("pitchRange")->load();
    p.chokeLinkTrack  = mixerTrack.load(std::memory_order_relaxed);

    p.resampleOn      = apvts.getRawParameterValue("resample")->load() > 0.5f;
    p.declickMode     = (int)apvts.getRawParameterValue("declickMode")->load();

    p.startOffset     = apvts.getRawParameterValue("startOffset")->load();
    p.loopStart       = apvts.getRawParameterValue("loopStart")->load();
    p.loopEnd         = apvts.getRawParameterValue("loopEnd")->load();
    p.useLoopPoints   = apvts.getRawParameterValue("useLoopPoints")->load() > 0.5f;
    p.pingPongLoop    = apvts.getRawParameterValue("pingPongLoop")->load() > 0.5f;

    p.stretchPitch    = apvts.getRawParameterValue("stretchPitch")->load();
    p.stretchMul      = apvts.getRawParameterValue("stretchMul")->load();
    p.stretchTime     = apvts.getRawParameterValue("stretchTime")->load();
    p.stretchMode     = (int)apvts.getRawParameterValue("stretchMode")->load();
    p.formantShift    = apvts.getRawParameterValue("formantShift")->load();
    p.hostBpm         = currentHostBpm.load(std::memory_order_relaxed);
    p.embeddedLoopBpm = detectedSampleBpm.load(std::memory_order_relaxed);

    p.targetEnvs = targetEnvs;
    p.targetLfos = targetLfos;

    p.targetEnvs[1].enabled  = apvts.getRawParameterValue("volEnvOn")->load() > 0.5f;
    p.targetEnvs[1].delay    = apvts.getRawParameterValue("envDelay")->load();
    p.targetEnvs[1].attack   = apvts.getRawParameterValue("envAttack")->load();
    p.targetEnvs[1].hold     = apvts.getRawParameterValue("envHold")->load();
    p.targetEnvs[1].decay    = apvts.getRawParameterValue("envDecay")->load();
    p.targetEnvs[1].sustain  = apvts.getRawParameterValue("envSustain")->load();
    p.targetEnvs[1].release  = apvts.getRawParameterValue("envRelease")->load();

    p.targetLfos[4].delay    = lfoDelayAmt.load(std::memory_order_relaxed);
    p.targetLfos[4].attack   = lfoAttackAmt.load(std::memory_order_relaxed);
    p.targetLfos[4].amount   = apvts.getRawParameterValue("lfoAmt")->load();
    p.targetLfos[4].speed    = apvts.getRawParameterValue("lfoSpeed")->load();
    p.targetLfos[4].shape    = lfoShapeIdx.load(std::memory_order_relaxed);

    p.filterModX      = apvts.getRawParameterValue("filterModX")->load();
    p.filterModY      = apvts.getRawParameterValue("filterModY")->load();
    p.filterType      = (int)apvts.getRawParameterValue("filterType")->load();

    p.rootNote        = (int)apvts.getRawParameterValue("rootNote")->load();
    p.fineTuneCents   = apvts.getRawParameterValue("fineTune")->load();
    p.enableMainPitch = apvts.getRawParameterValue("enableMainPitch")->load() > 0.5f;
    p.addToKey        = addToKey.load(std::memory_order_relaxed);
    p.keyRegionLow    = keyRegionLow.load(std::memory_order_relaxed);
    p.keyRegionHigh   = keyRegionHigh.load(std::memory_order_relaxed);
    p.porta           = apvts.getRawParameterValue("porta")->load() > 0.5f;
    p.mono            = apvts.getRawParameterValue("mono")->load() > 0.5f;
    p.portaTime       = apvts.getRawParameterValue("portaTime")->load();
    p.fullPorta       = fullPorta.load(std::memory_order_relaxed);
    p.maxPoly         = (int)apvts.getRawParameterValue("maxPoly")->load();
    p.cutSelf         = apvts.getRawParameterValue("cutSelf")->load() > 0.5f;
    p.cutGroup        = cutGroup.load(std::memory_order_relaxed);
    p.cutByGroup      = cutByGroup.load(std::memory_order_relaxed);
    p.gateTime        = apvts.getRawParameterValue("gateTime")->load();
    p.shiftTime       = shiftTimeAmt.load(std::memory_order_relaxed);
    p.swingMul        = swingMulAmt.load(std::memory_order_relaxed);

    p.panOffset       = lvlPanOffset.load(std::memory_order_relaxed);
    p.volOffset       = lvlVolOffset.load(std::memory_order_relaxed);
    p.modXOffset      = lvlModXOffset.load(std::memory_order_relaxed);
    p.modYOffset      = lvlModYOffset.load(std::memory_order_relaxed);

    p.trackingKeyMode = trackingKeyMode.load(std::memory_order_relaxed);
    p.trackPan        = trackPanAmt.load(std::memory_order_relaxed);
    p.trackModX       = trackModXAmt.load(std::memory_order_relaxed);
    p.trackModY       = trackModYAmt.load(std::memory_order_relaxed);
    p.trackMidNote    = trackMidNote.load(std::memory_order_relaxed);

    p.arpDir          = (int)apvts.getRawParameterValue("arpDir")->load();
    p.arpTime         = apvts.getRawParameterValue("arpTime")->load();
    p.arpRange        = (int)apvts.getRawParameterValue("arpRange")->load();
    p.arpRepeat       = arpRepeatCount.load(std::memory_order_relaxed);
    p.arpSlide        = arpSlideOn.load(std::memory_order_relaxed);
    p.arpChord        = arpChordMode.load(std::memory_order_relaxed);

    p.echoCount       = (int)apvts.getRawParameterValue("echoCount")->load();
    p.echoFeed        = apvts.getRawParameterValue("echoFeed")->load();
    p.echoTime        = apvts.getRawParameterValue("echoTime")->load();
    p.echoPingPong    = apvts.getRawParameterValue("echoPingPong")->load() > 0.5f;
    p.echoPan         = echoPanAmt.load(std::memory_order_relaxed);
    p.echoModX        = echoModXAmt.load(std::memory_order_relaxed);
    p.echoModY        = echoModYAmt.load(std::memory_order_relaxed);
    p.echoPitch       = echoPitchAmt.load(std::memory_order_relaxed);
    p.echoFatMode     = echoFatModeOn.load(std::memory_order_relaxed);

    p.pitchWheelNorm  = midiPitchWheel.load(std::memory_order_relaxed);
    p.modWheelNorm    = midiModWheel.load(std::memory_order_relaxed);
    p.aftertouchNorm  = midiAftertouch.load(std::memory_order_relaxed);

    p.slicePositions  = &sliceMarkers;

    return p;
}

void FLSamplerAudioProcessor::rebuildPrecomputedBuffer()
{
    const juce::ScopedLock sl(sampleLock);
    if (rawSampleBuffer.getNumSamples() <= 0)
    {
        processedSampleBuffer.setSize(2, 0);
        waveformRevision.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    double loopStartNorm = apvts.getRawParameterValue("loopStart")->load();
    double loopEndNorm   = apvts.getRawParameterValue("loopEnd")->load();
    auto settings = gatherPrecomputedSettings();
    fls::PrecomputedEffects::process(rawSampleBuffer, processedSampleBuffer, loadedSampleRate, settings, loopStartNorm, loopEndNorm);
    waveformRevision.fetch_add(1, std::memory_order_relaxed);
}

bool FLSamplerAudioProcessor::loadSampleFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    const int numSamples = (int)juce::jmin<juce::int64>(reader->lengthInSamples, 48000 * 180); // up to 3 mins
    const int numCh = juce::jlimit(1, 2, (int)reader->numChannels);

    juce::AudioBuffer<float> temp(numCh, numSamples);
    reader->read(&temp, 0, numSamples, 0, true, numCh > 1);

    // Check for embedded WAV/AIFF loop points (smpl chunk) & root MIDI note just like FL Studio ("Load regions")
    bool foundMetadataRoot = false;
    if (loadRegions.load(std::memory_order_relaxed))
    {
        const auto& meta = reader->metadataValues;
        const int numLoops = meta.getValue("NumSampleLoops", "0").getIntValue();
        if (numLoops > 0 && numSamples > 1)
        {
            const int lStart = meta.getValue("Loop0Start", "-1").getIntValue();
            const int lEnd   = meta.getValue("Loop0End", "-1").getIntValue();
            if (lStart >= 0 && lEnd > lStart && lEnd <= numSamples)
            {
                const float normStart = juce::jlimit(0.0f, 0.99f, (float)lStart / (float)(numSamples - 1));
                const float normEnd   = juce::jlimit(normStart + 0.01f, 1.0f, (float)lEnd / (float)(numSamples - 1));
                if (auto* pS = apvts.getParameter("loopStart"))
                    pS->setValueNotifyingHost(pS->convertTo0to1(normStart));
                if (auto* pE = apvts.getParameter("loopEnd"))
                    pE->setValueNotifyingHost(pE->convertTo0to1(normEnd));
                if (auto* pOn = apvts.getParameter("useLoopPoints"))
                    pOn->setValueNotifyingHost(1.0f);
            }
        }
        const int midiUnity = meta.getValue("MidiUnityNote", "-1").getIntValue();
        if (midiUnity >= 12 && midiUnity <= 108)
        {
            foundMetadataRoot = true;
            detectedSampleRoot.store(midiUnity, std::memory_order_relaxed);
            if (auto* pRoot = apvts.getParameter("rootNote"))
                pRoot->setValueNotifyingHost(pRoot->convertTo0to1((float)midiUnity));
        }
    }

    const double sr = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    if (!foundMetadataRoot)
    {
        int autoRoot = fls::PrecomputedEffects::detectFundamentalMidiNote(temp, sr);
        detectedSampleRoot.store(autoRoot, std::memory_order_relaxed);
    }

    std::vector<float> detectedSlices = fls::PrecomputedEffects::detectTransientSlices(temp, sr);

    // Estimate loop BPM from duration if between 0.8s and 16s
    const double durSec = (double)numSamples / sr;
    if (durSec >= 1.2 && durSec <= 16.0)
    {
        double estBpm = (4.0 * 60.0) / durSec;
        while (estBpm < 70.0)  estBpm *= 2.0;
        while (estBpm > 180.0) estBpm *= 0.5;
        detectedSampleBpm.store(estBpm, std::memory_order_relaxed);
    }
    else
    {
        detectedSampleBpm.store(0.0, std::memory_order_relaxed);
    }

    {
        const juce::ScopedLock sl(sampleLock);
        rawSampleBuffer.makeCopyOf(temp);
        loadedSampleRate = sr;
        currentSampleFile = file;
        sliceMarkers = std::move(detectedSlices);
        recentSampleFiles.removeString(file.getFullPathName());
        recentSampleFiles.insert(0, file.getFullPathName());
        while (recentSampleFiles.size() > 10)
            recentSampleFiles.remove(recentSampleFiles.size() - 1);
        sampleLoaded.store(true, std::memory_order_relaxed);
        rebuildPrecomputedBuffer();
    }
    return true;
}

void FLSamplerAudioProcessor::clearSample()
{
    const juce::ScopedLock sl(sampleLock);
    samplerEngine.resetAllVoices();
    rawSampleBuffer.setSize(2, 0);
    processedSampleBuffer.setSize(2, 0);
    sliceMarkers.clear();
    currentSampleFile = juce::File();
    sampleLoaded.store(false, std::memory_order_relaxed);
    waveformRevision.fetch_add(1, std::memory_order_relaxed);
}

void FLSamplerAudioProcessor::addOrMoveSliceMarker(int existingIdx, float normPos, float& outActualPos)
{
    const juce::ScopedLock sl(sampleLock);
    float snapped = snapNormalizedToZeroCrossing(juce::jlimit(0.0f, 0.999f, normPos));
    outActualPos = snapped;
    loadSliceMarkers.store(true, std::memory_order_relaxed);
    if (existingIdx >= 0 && existingIdx < (int)sliceMarkers.size())
    {
        sliceMarkers[(size_t)existingIdx] = snapped;
    }
    else
    {
        sliceMarkers.push_back(snapped);
    }
    std::sort(sliceMarkers.begin(), sliceMarkers.end());
    waveformRevision.fetch_add(1, std::memory_order_relaxed);
}

void FLSamplerAudioProcessor::removeSliceMarker(int index)
{
    const juce::ScopedLock sl(sampleLock);
    if (index >= 0 && index < (int)sliceMarkers.size() && sliceMarkers.size() > 1)
    {
        sliceMarkers.erase(sliceMarkers.begin() + index);
        waveformRevision.fetch_add(1, std::memory_order_relaxed);
    }
}

void FLSamplerAudioProcessor::clearSliceMarkers()
{
    const juce::ScopedLock sl(sampleLock);
    sliceMarkers.clear();
    sliceMarkers.push_back(0.0f);
    waveformRevision.fetch_add(1, std::memory_order_relaxed);
}

bool FLSamplerAudioProcessor::stepFolderSample(int delta)
{
    juce::File cur = getLoadedFile();
    juce::File dir = cur.existsAsFile() ? cur.getParentDirectory()
                                        : juce::File::getSpecialLocation(juce::File::userMusicDirectory);
    if (!dir.isDirectory())
        return false;

    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.wav;*.flac;*.mp3;*.ogg;*.aiff;*.aif");
    if (files.isEmpty())
        return false;

    files.sort();
    int idx = files.indexOf(cur);
    if (idx < 0)
        idx = 0;
    else
        idx = (idx + delta + files.size()) % files.size();

    return loadSampleFromFile(files[idx]);
}

void FLSamplerAudioProcessor::triggerPreviewNote(int midiNote, float, float startNorm)
{
    pendingNoteStartPos.store(startNorm, std::memory_order_relaxed);
    pendingNoteOn.store(juce::jlimit(0, 127, midiNote), std::memory_order_relaxed);
}

void FLSamplerAudioProcessor::stopPreviewNote(int midiNote)
{
    pendingNoteOff.store(juce::jlimit(0, 127, midiNote), std::memory_order_relaxed);
}

float FLSamplerAudioProcessor::snapNormalizedToZeroCrossing(float normPos) const
{
    if (!zeroCrossSnap.load(std::memory_order_relaxed))
        return normPos;
    const juce::ScopedLock sl(sampleLock);
    return fls::PrecomputedEffects::snapToZeroCrossing(processedSampleBuffer, normPos);
}

juce::File FLSamplerAudioProcessor::exportProcessedSampleToTempWav() const
{
    const juce::ScopedLock sl(sampleLock);
    if (processedSampleBuffer.getNumSamples() <= 1)
        return {};

    juce::String baseName = currentSampleFile.existsAsFile()
        ? currentSampleFile.getFileNameWithoutExtension() + "_FLSampler.wav"
        : "FLSampler_Export.wav";
    juce::File outFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(baseName);
    outFile.deleteFile();

    juce::WavAudioFormat wavFormat;
    if (auto stream = std::unique_ptr<juce::FileOutputStream>(outFile.createOutputStream()))
    {
        if (auto writer = std::unique_ptr<juce::AudioFormatWriter>(
                wavFormat.createWriterFor(stream.get(), loadedSampleRate,
                                          (unsigned int)processedSampleBuffer.getNumChannels(), 24, {}, 0)))
        {
            stream.release();
            writer->writeFromAudioSampleBuffer(processedSampleBuffer, 0, processedSampleBuffer.getNumSamples());
            return outFile;
        }
    }
    return {};
}

juce::StringArray FLSamplerAudioProcessor::getRecentSamplePaths() const
{
    const juce::ScopedLock sl(sampleLock);
    return recentSampleFiles;
}

juce::String FLSamplerAudioProcessor::getLoadedFileName() const
{
    const juce::ScopedLock sl(sampleLock);
    if (!sampleLoaded.load(std::memory_order_relaxed) || !currentSampleFile.existsAsFile())
        return "(none)";
    return currentSampleFile.getFileName();
}

juce::File FLSamplerAudioProcessor::getLoadedFile() const
{
    const juce::ScopedLock sl(sampleLock);
    return currentSampleFile;
}

void FLSamplerAudioProcessor::getProcessedWaveformCopy(juce::AudioBuffer<float>& outBuf) const
{
    const juce::ScopedLock sl(sampleLock);
    outBuf.makeCopyOf(processedSampleBuffer);
}

void FLSamplerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    samplerEngine.prepare(sampleRate, samplesPerBlock);
}

void FLSamplerAudioProcessor::releaseResources()
{
    samplerEngine.resetAllVoices();
}

bool FLSamplerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::stereo() || mainOut == juce::AudioChannelSet::mono();
}

void FLSamplerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm(); bpm.hasValue() && *bpm > 20.0)
                currentHostBpm.store(*bpm, std::memory_order_relaxed);
        }
    }

    if (needsPrecompute.exchange(false, std::memory_order_acq_rel))
        rebuildPrecomputedBuffer();

    const juce::ScopedTryLock stl(sampleLock);
    if (!stl.isLocked() || processedSampleBuffer.getNumSamples() <= 1)
        return;

    auto runtimeParams = gatherRuntimeParams();

    // Check UI preview triggers
    int uiNoteOn = pendingNoteOn.exchange(-1, std::memory_order_relaxed);
    if (uiNoteOn >= 0)
    {
        float startPos = pendingNoteStartPos.exchange(-1.0f, std::memory_order_relaxed);
        samplerEngine.noteOn(uiNoteOn, 0.88f, processedSampleBuffer, runtimeParams, startPos);
    }

    int uiNoteOff = pendingNoteOff.exchange(-1, std::memory_order_relaxed);
    if (uiNoteOff >= 0)
        samplerEngine.noteOff(uiNoteOff, runtimeParams);

    // Process incoming MIDI messages (Piano Roll / Keyboard / Controllers / MPE Slide Notes)
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            samplerEngine.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), processedSampleBuffer, runtimeParams, -1.0f, false, msg.getChannel());
        }
        else if (msg.isNoteOff())
        {
            samplerEngine.noteOff(msg.getNoteNumber(), runtimeParams);
        }
        else if (msg.isPitchWheel())
        {
            float pw = ((float)msg.getPitchWheelValue() - 8192.0f) / 8192.0f;
            if (msg.getChannel() > 1)
            {
                samplerEngine.setChannelPitchBend(msg.getChannel(), juce::jlimit(-1.0f, 1.0f, pw), runtimeParams.pitchRange);
            }
            else
            {
                midiPitchWheel.store(juce::jlimit(-1.0f, 1.0f, pw), std::memory_order_relaxed);
                runtimeParams.pitchWheelNorm = pw;
            }
        }
        else if (msg.isController() && msg.getControllerNumber() == 1)
        {
            float mw = (float)msg.getControllerValue() / 127.0f;
            midiModWheel.store(mw, std::memory_order_relaxed);
            runtimeParams.modWheelNorm = mw;
        }
        else if (msg.isChannelPressure() || msg.isAftertouch())
        {
            float at = msg.isChannelPressure()
                ? (float)msg.getChannelPressureValue() / 127.0f
                : (float)msg.getAfterTouchValue() / 127.0f;
            midiAftertouch.store(at, std::memory_order_relaxed);
            runtimeParams.aftertouchNorm = at;
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            samplerEngine.resetAllVoices();
        }
    }

    samplerEngine.renderBlock(buffer, processedSampleBuffer, loadedSampleRate, runtimeParams);
}

void FLSamplerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("sampleFilePath", getLoadedFile().getFullPathName(), nullptr);
    state.setProperty("keepOnDisk", keepOnDisk.load(), nullptr);
    state.setProperty("loadRegions", loadRegions.load(), nullptr);
    state.setProperty("loadSliceMarkers", loadSliceMarkers.load(), nullptr);
    state.setProperty("zeroCrossSnap", zeroCrossSnap.load(), nullptr);
    state.setProperty("mixerTrack", mixerTrack.load(), nullptr);
    state.setProperty("cutGroup", cutGroup.load(), nullptr);
    state.setProperty("cutByGroup", cutByGroup.load(), nullptr);
    state.setProperty("keyRegionLow", keyRegionLow.load(), nullptr);
    state.setProperty("keyRegionHigh", keyRegionHigh.load(), nullptr);
    state.setProperty("addToKey", addToKey.load(), nullptr);
    state.setProperty("fullPorta", fullPorta.load(), nullptr);
    state.setProperty("shiftTimeAmt", shiftTimeAmt.load(), nullptr);
    state.setProperty("swingMulAmt", swingMulAmt.load(), nullptr);
    state.setProperty("trackingKeyMode", trackingKeyMode.load(), nullptr);
    state.setProperty("lvlPanOffset", lvlPanOffset.load(), nullptr);
    state.setProperty("lvlVolOffset", lvlVolOffset.load(), nullptr);
    state.setProperty("lvlModXOffset", lvlModXOffset.load(), nullptr);
    state.setProperty("lvlModYOffset", lvlModYOffset.load(), nullptr);
    state.setProperty("trackPanAmt", trackPanAmt.load(), nullptr);
    state.setProperty("trackModXAmt", trackModXAmt.load(), nullptr);
    state.setProperty("trackModYAmt", trackModYAmt.load(), nullptr);
    state.setProperty("trackMidNote", trackMidNote.load(), nullptr);
    state.setProperty("lfoDelayAmt", lfoDelayAmt.load(), nullptr);
    state.setProperty("lfoAttackAmt", lfoAttackAmt.load(), nullptr);
    state.setProperty("lfoShapeIdx", lfoShapeIdx.load(), nullptr);
    state.setProperty("arpRepeatCount", arpRepeatCount.load(), nullptr);
    state.setProperty("arpSlideOn", arpSlideOn.load(), nullptr);
    state.setProperty("arpChordMode", arpChordMode.load(), nullptr);
    state.setProperty("echoPanAmt", echoPanAmt.load(), nullptr);
    state.setProperty("echoModXAmt", echoModXAmt.load(), nullptr);
    state.setProperty("echoModYAmt", echoModYAmt.load(), nullptr);
    state.setProperty("echoPitchAmt", echoPitchAmt.load(), nullptr);
    state.setProperty("echoFatModeOn", echoFatModeOn.load(), nullptr);
    state.setProperty("skinMode", fls::FLColors::skinMode, nullptr);
    state.setProperty("isAbletonSkin", fls::FLColors::isAbletonSkin, nullptr);

    // Serialize all 5 AHDSR Envelopes & 5 LFO Modulators
    for (int i = 0; i < 5; ++i)
    {
        const juce::String pfx = "env" + juce::String(i) + "_";
        state.setProperty(pfx + "en", targetEnvs[(size_t)i].enabled, nullptr);
        state.setProperty(pfx + "del", targetEnvs[(size_t)i].delay, nullptr);
        state.setProperty(pfx + "att", targetEnvs[(size_t)i].attack, nullptr);
        state.setProperty(pfx + "hld", targetEnvs[(size_t)i].hold, nullptr);
        state.setProperty(pfx + "dec", targetEnvs[(size_t)i].decay, nullptr);
        state.setProperty(pfx + "sus", targetEnvs[(size_t)i].sustain, nullptr);
        state.setProperty(pfx + "rel", targetEnvs[(size_t)i].release, nullptr);
        state.setProperty(pfx + "amt", targetEnvs[(size_t)i].amount, nullptr);
        state.setProperty(pfx + "tA", targetEnvs[(size_t)i].attTension, nullptr);
        state.setProperty(pfx + "tD", targetEnvs[(size_t)i].decTension, nullptr);
        state.setProperty(pfx + "tR", targetEnvs[(size_t)i].relTension, nullptr);
        state.setProperty(pfx + "ts", targetEnvs[(size_t)i].tempoSync, nullptr);

        const juce::String lpfx = "lfo" + juce::String(i) + "_";
        state.setProperty(lpfx + "del", targetLfos[(size_t)i].delay, nullptr);
        state.setProperty(lpfx + "att", targetLfos[(size_t)i].attack, nullptr);
        state.setProperty(lpfx + "amt", targetLfos[(size_t)i].amount, nullptr);
        state.setProperty(lpfx + "spd", targetLfos[(size_t)i].speed, nullptr);
        state.setProperty(lpfx + "shp", targetLfos[(size_t)i].shape, nullptr);
        state.setProperty(lpfx + "ts",  targetLfos[(size_t)i].tempoSync, nullptr);
        state.setProperty(lpfx + "gl",  targetLfos[(size_t)i].globalPhase, nullptr);
    }

    // Serialize custom slice markers and recent files
    {
        const juce::ScopedLock sl(sampleLock);
        juce::StringArray sliceStrs;
        for (float s : sliceMarkers)
            sliceStrs.add(juce::String(s, 5));
        state.setProperty("sliceMarkersCsv", sliceStrs.joinIntoString(","), nullptr);
        state.setProperty("recentFilesPipe", recentSampleFiles.joinIntoString("|"), nullptr);

        // Portable WAV Sample Embedding when Keep on Disk is OFF and sample <= 15 sec
        if (!keepOnDisk.load() && rawSampleBuffer.getNumSamples() > 0 && rawSampleBuffer.getNumSamples() <= (int)(loadedSampleRate * 15.0))
        {
            juce::MemoryBlock wavBlock;
            {
                auto outStream = std::make_unique<juce::MemoryOutputStream>(wavBlock, false);
                juce::WavAudioFormat wavFmt;
                if (auto writer = std::unique_ptr<juce::AudioFormatWriter>(
                        wavFmt.createWriterFor(outStream.get(), loadedSampleRate,
                                               (unsigned int)rawSampleBuffer.getNumChannels(), 16, {}, 0)))
                {
                    outStream.release();
                    writer->writeFromAudioSampleBuffer(rawSampleBuffer, 0, rawSampleBuffer.getNumSamples());
                }
            }
            if (wavBlock.getSize() > 0)
                state.setProperty("embeddedWavB64", wavBlock.toBase64Encoding(), nullptr);
        }
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FLSamplerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto vt = juce::ValueTree::fromXml(*xmlState);
        apvts.replaceState(vt);

        keepOnDisk.store((bool)vt.getProperty("keepOnDisk", false));
        loadRegions.store((bool)vt.getProperty("loadRegions", true));
        loadSliceMarkers.store((bool)vt.getProperty("loadSliceMarkers", false));
        zeroCrossSnap.store((bool)vt.getProperty("zeroCrossSnap", true));
        mixerTrack.store((int)vt.getProperty("mixerTrack", 0));
        cutGroup.store((int)vt.getProperty("cutGroup", 0));
        cutByGroup.store((int)vt.getProperty("cutByGroup", 0));
        keyRegionLow.store((int)vt.getProperty("keyRegionLow", 0));
        keyRegionHigh.store((int)vt.getProperty("keyRegionHigh", 127));
        addToKey.store((bool)vt.getProperty("addToKey", false));
        fullPorta.store((bool)vt.getProperty("fullPorta", false));
        shiftTimeAmt.store((float)vt.getProperty("shiftTimeAmt", 0.0f));
        swingMulAmt.store((float)vt.getProperty("swingMulAmt", 0.0f));
        trackingKeyMode.store((bool)vt.getProperty("trackingKeyMode", true));
        lvlPanOffset.store((float)vt.getProperty("lvlPanOffset", 0.0f));
        lvlVolOffset.store((float)vt.getProperty("lvlVolOffset", 0.78f));
        lvlModXOffset.store((float)vt.getProperty("lvlModXOffset", 0.0f));
        lvlModYOffset.store((float)vt.getProperty("lvlModYOffset", 0.0f));
        trackPanAmt.store((float)vt.getProperty("trackPanAmt", 0.0f));
        trackModXAmt.store((float)vt.getProperty("trackModXAmt", 0.0f));
        trackModYAmt.store((float)vt.getProperty("trackModYAmt", 0.0f));
        trackMidNote.store((int)vt.getProperty("trackMidNote", 60));
        lfoDelayAmt.store((float)vt.getProperty("lfoDelayAmt", 0.0f));
        lfoAttackAmt.store((float)vt.getProperty("lfoAttackAmt", 0.35f));
        lfoShapeIdx.store((int)vt.getProperty("lfoShapeIdx", 0));
        arpRepeatCount.store((int)vt.getProperty("arpRepeatCount", 1));
        arpSlideOn.store((bool)vt.getProperty("arpSlideOn", false));
        arpChordMode.store((int)vt.getProperty("arpChordMode", 0));
        echoPanAmt.store((float)vt.getProperty("echoPanAmt", 0.0f));
        echoModXAmt.store((float)vt.getProperty("echoModXAmt", 0.65f));
        echoModYAmt.store((float)vt.getProperty("echoModYAmt", 0.25f));
        echoPitchAmt.store((float)vt.getProperty("echoPitchAmt", 0.0f));
        echoFatModeOn.store((bool)vt.getProperty("echoFatModeOn", false));

        if (vt.hasProperty("skinMode"))
            fls::FLColors::setSkinMode((int)vt.getProperty("skinMode", 0));
        else
            fls::FLColors::setSkinMode((bool)vt.getProperty("isAbletonSkin", false) ? 1 : 0);

        for (int i = 0; i < 5; ++i)
        {
            const juce::String pfx = "env" + juce::String(i) + "_";
            if (vt.hasProperty(pfx + "en"))
            {
                targetEnvs[(size_t)i].enabled    = (bool)vt.getProperty(pfx + "en");
                targetEnvs[(size_t)i].delay      = (float)vt.getProperty(pfx + "del");
                targetEnvs[(size_t)i].attack     = (float)vt.getProperty(pfx + "att");
                targetEnvs[(size_t)i].hold       = (float)vt.getProperty(pfx + "hld");
                targetEnvs[(size_t)i].decay      = (float)vt.getProperty(pfx + "dec");
                targetEnvs[(size_t)i].sustain    = (float)vt.getProperty(pfx + "sus");
                targetEnvs[(size_t)i].release    = (float)vt.getProperty(pfx + "rel");
                targetEnvs[(size_t)i].amount     = (float)vt.getProperty(pfx + "amt");
                targetEnvs[(size_t)i].attTension = (float)vt.getProperty(pfx + "tA");
                targetEnvs[(size_t)i].decTension = (float)vt.getProperty(pfx + "tD");
                targetEnvs[(size_t)i].relTension = (float)vt.getProperty(pfx + "tR");
                targetEnvs[(size_t)i].tempoSync  = (bool)vt.getProperty(pfx + "ts");
            }
            const juce::String lpfx = "lfo" + juce::String(i) + "_";
            if (vt.hasProperty(lpfx + "spd"))
            {
                targetLfos[(size_t)i].delay       = (float)vt.getProperty(lpfx + "del");
                targetLfos[(size_t)i].attack      = (float)vt.getProperty(lpfx + "att");
                targetLfos[(size_t)i].amount      = (float)vt.getProperty(lpfx + "amt");
                targetLfos[(size_t)i].speed       = (float)vt.getProperty(lpfx + "spd");
                targetLfos[(size_t)i].shape       = (int)vt.getProperty(lpfx + "shp");
                targetLfos[(size_t)i].tempoSync   = (bool)vt.getProperty(lpfx + "ts");
                targetLfos[(size_t)i].globalPhase = (bool)vt.getProperty(lpfx + "gl");
            }
        }

        juce::String recentPipe = vt.getProperty("recentFilesPipe", "").toString();
        if (recentPipe.isNotEmpty())
        {
            recentSampleFiles.clear();
            recentSampleFiles.addTokens(recentPipe, "|", "");
        }

        juce::String path = vt.getProperty("sampleFilePath", "").toString();
        bool loadedFromDisk = false;
        if (path.isNotEmpty())
            loadedFromDisk = loadSampleFromFile(juce::File(path));

        if (!loadedFromDisk && vt.hasProperty("embeddedWavB64"))
        {
            juce::MemoryBlock wavBlock;
            if (wavBlock.fromBase64Encoding(vt.getProperty("embeddedWavB64").toString()))
            {
                juce::WavAudioFormat wavFmt;
                std::unique_ptr<juce::AudioFormatReader> reader(
                    wavFmt.createReaderFor(new juce::MemoryInputStream(wavBlock, false), true));
                if (reader != nullptr && reader->lengthInSamples > 0)
                {
                    const int numSmp = (int)reader->lengthInSamples;
                    const int numCh = juce::jlimit(1, 2, (int)reader->numChannels);
                    juce::AudioBuffer<float> temp(numCh, numSmp);
                    reader->read(&temp, 0, numSmp, 0, true, numCh > 1);
                    const juce::ScopedLock sl(sampleLock);
                    rawSampleBuffer.makeCopyOf(temp);
                    loadedSampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
                    sampleLoaded.store(true, std::memory_order_relaxed);
                    rebuildPrecomputedBuffer();
                }
            }
        }

        juce::String slicesCsv = vt.getProperty("sliceMarkersCsv", "").toString();
        if (slicesCsv.isNotEmpty())
        {
            const juce::ScopedLock sl(sampleLock);
            juce::StringArray toks;
            toks.addTokens(slicesCsv, ",", "");
            if (toks.size() > 0)
            {
                sliceMarkers.clear();
                for (const auto& t : toks)
                    sliceMarkers.push_back(juce::jlimit(0.0f, 1.0f, t.getFloatValue()));
                waveformRevision.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
}

juce::AudioProcessorEditor* FLSamplerAudioProcessor::createEditor()
{
    return new FLSamplerAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FLSamplerAudioProcessor();
}
