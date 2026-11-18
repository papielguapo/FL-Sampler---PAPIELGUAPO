#include "PluginEditor.h"
#include "BinaryData.h"

using namespace fls;

// Target colors for InsSheet sub-tabs matching the 5 user screenshots:
// 0=Panning (Cyan), 1=Volume (Green), 2=Mod X (Yellow-Lime), 3=Mod Y (Orange), 4=Pitch (Red-Pink)
static constexpr juce::uint32 kInsTargetColors[5] = {
    0xff42d6ff, // Panning
    0xff47e685, // Volume
    0xffd7e838, // Mod X
    0xffff9c2b, // Mod Y
    0xffff5268  // Pitch
};

FLSamplerAudioProcessorEditor::FLSamplerAudioProcessorEditor(FLSamplerAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      wavScope(p)
{
    setLookAndFeel(&flLookAndFeel);
    privateSkinBg = juce::ImageCache::getFromMemory(BinaryData::Skin_jpg, BinaryData::Skin_jpgSize);

    // Make contentCanvas pass mouse clicks to the editor when clicking custom-painted regions (tabs, keyboard, etc.)
    contentCanvas.setInterceptsMouseClicks(false, true);
    addAndMakeVisible(contentCanvas);

    // ==================== TOP HEADER BAR ====================
    contentCanvas.addAndMakeVisible(masterOnBtn);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "masterOn", masterOnBtn));

    setupKnob(masterPanKnob,   "Channel panning", 0.0,  true,  false, false);
    setupKnob(masterVolKnob,   "Channel volume",  0.78, false, true,  false);
    setupKnob(masterPitchKnob, "Channel pitch",   0.0,  true,  false, false);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "masterPan", masterPanKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "masterVol", masterVolKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "masterPitch", masterPitchKnob));
    masterPanKnob.setDefaultResetValue(0.0);
    masterVolKnob.setDefaultResetValue(0.78);
    masterPitchKnob.setDefaultResetValue(0.0);

    contentCanvas.addAndMakeVisible(pitchRangeBox);
    pitchRangeBox.setValue((int)audioProcessor.apvts.getRawParameterValue("pitchRange")->load());
    pitchRangeBox.onValueChanged = [this](int v)
    {
        if (auto* param = audioProcessor.apvts.getParameter("pitchRange"))
            param->setValueNotifyingHost(param->convertTo0to1((float)v));
        setHintText("Pitch knob range: " + juce::String(v) + " semitones");
    };

    contentCanvas.addAndMakeVisible(trackBox);
    trackBox.setValue(audioProcessor.mixerTrack.load());
    trackBox.onValueChanged = [this](int v)
    {
        audioProcessor.mixerTrack.store(v, std::memory_order_relaxed);
        setHintText("Choke Link Group (TRACK): " + (v == 0 ? juce::String("Off (---)") : ("Group #" + juce::String(v) + " (cuts linked instances)")));
    };

    // ==================== TAB 1: SAMPLE SETTINGS ====================
    contentCanvas.addAndMakeVisible(keepOnDiskBtn);
    keepOnDiskBtn.setToggleState(audioProcessor.keepOnDisk.load(), juce::dontSendNotification);
    keepOnDiskBtn.onClick = [this]() { audioProcessor.keepOnDisk.store(keepOnDiskBtn.getToggleState()); };

    contentCanvas.addAndMakeVisible(resampleBtn);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "resample", resampleBtn));

    contentCanvas.addAndMakeVisible(loadRegionsBtn);
    loadRegionsBtn.setToggleState(audioProcessor.loadRegions.load(), juce::dontSendNotification);
    loadRegionsBtn.onClick = [this]() { audioProcessor.loadRegions.store(loadRegionsBtn.getToggleState()); };

    contentCanvas.addAndMakeVisible(loadSlicesBtn);
    loadSlicesBtn.setToggleState(audioProcessor.loadSliceMarkers.load(), juce::dontSendNotification);
    loadSlicesBtn.onClick = [this]() { audioProcessor.loadSliceMarkers.store(loadSlicesBtn.getToggleState()); };

    declickCombo.setName("Declicking mode");
    declickCombo.onHintChanged = [this](const juce::String& h) { setHintText(h); };
    contentCanvas.addAndMakeVisible(declickCombo);
    declickCombo.addItemList({
        "Out only (no bleeding)",
        "Transient (no bleeding)",
        "Transient (bleeding)",
        "Generic (bleeding)",
        "Smooth (bleeding)",
        "Crossfade (bleeding)"
    }, 1);
    comboAttachments.push_back(std::make_unique<ComboAttachment>(audioProcessor.apvts, "declickMode", declickCombo));

    // Playback Panel
    setupKnob(startOffsetKnob, "Sample start offset", 0.0);
    setupKnob(loopStartKnob,   "Loop start",          0.0);
    setupKnob(loopEndKnob,     "Loop end",            1.0);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "startOffset", startOffsetKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "loopStart", loopStartKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "loopEnd", loopEndKnob));
    startOffsetKnob.setDefaultResetValue(0.0);
    loopStartKnob.setDefaultResetValue(0.0);
    loopEndKnob.setDefaultResetValue(1.0);

    contentCanvas.addAndMakeVisible(useLoopPointsBtn);
    contentCanvas.addAndMakeVisible(pingPongLoopBtn);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "useLoopPoints", useLoopPointsBtn));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "pingPongLoop", pingPongLoopBtn));

    // Time Stretching Panel
    setupKnob(stretchPitchKnob, "Time stretch pitch",      0.0, true);
    setupKnob(stretchMulKnob,   "Time stretch multiplier", 1.0);
    setupKnob(stretchTimeKnob,  "Time stretch time",       0.0);
    setupKnob(formantShiftKnob, "Formant shift (semitones)", 0.0, true, false, true);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "stretchPitch", stretchPitchKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "stretchMul", stretchMulKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "stretchTime", stretchTimeKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "formantShift", formantShiftKnob));
    stretchPitchKnob.setDefaultResetValue(0.0);
    stretchMulKnob.setDefaultResetValue(1.0);
    stretchTimeKnob.setDefaultResetValue(0.0);
    formantShiftKnob.setDefaultResetValue(0.0);

    stretchModeCombo.setName("Time stretch mode");
    stretchModeCombo.onHintChanged = [this](const juce::String& h) { setHintText(h); };
    contentCanvas.addAndMakeVisible(stretchModeCombo);
    stretchModeCombo.addItemList({
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
    }, 1);
    comboAttachments.push_back(std::make_unique<ComboAttachment>(audioProcessor.apvts, "stretchMode", stretchModeCombo));

    // Precomputed Effects (Standard - Wrench sub-tab)
    contentCanvas.addAndMakeVisible(removeDcBtn);
    contentCanvas.addAndMakeVisible(normalizeBtn);
    contentCanvas.addAndMakeVisible(reverseBtn);
    contentCanvas.addAndMakeVisible(reversePolBtn);
    contentCanvas.addAndMakeVisible(fadeStereoBtn);
    contentCanvas.addAndMakeVisible(swapStereoBtn);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "removeDc", removeDcBtn));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "normalize", normalizeBtn));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "reverse", reverseBtn));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "reversePol", reversePolBtn));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "fadeStereo", fadeStereoBtn));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "swapStereo", swapStereoBtn));

    setupKnob(smpStartKnob,  "Sample start (precomputed)", 0.0);
    setupKnob(lengthKnob,    "Sample length (precomputed)", 1.0, false, false, true); // Orange ring on LENGTH matching screenshot 1
    setupKnob(fadeInKnob,    "Quick fade in",  0.0);
    setupKnob(fadeOutKnob,   "Quick fade out", 0.0);
    setupKnob(crossfadeKnob, "Loop crossfade", 0.0);
    setupKnob(trimKnob,      "Trim threshold", 0.0);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "smpStart", smpStartKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "length", lengthKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "fadeIn", fadeInKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "fadeOut", fadeOutKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "crossfade", crossfadeKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "trim", trimKnob));
    smpStartKnob.setDefaultResetValue(0.0);
    lengthKnob.setDefaultResetValue(1.0);
    fadeInKnob.setDefaultResetValue(0.0);
    fadeOutKnob.setDefaultResetValue(0.0);
    crossfadeKnob.setDefaultResetValue(0.0);
    trimKnob.setDefaultResetValue(0.0);

    // Precomputed Effects (Legacy - Bulb sub-tab matching screenshot 2)
    setupKnob(boostKnob,       "Precomputed boost",      0.0);
    setupKnob(eqKnob,          "Precomputed EQ",         0.0, true);
    setupKnob(rmMixKnob,       "Ring modulation mix",    0.0);
    setupKnob(rmFreqKnob,      "Ring modulation freq",   0.5, false, false, true); // Orange arc on RM FREQ
    setupKnob(cutKnob,         "Precomputed cutoff",     1.0, false, false, true); // Full orange arc on CUT
    setupKnob(resKnob,         "Precomputed resonance",  0.0);
    setupKnob(reverbKnob,      "Precomputed reverb",     0.0);
    setupKnob(stereoDelayKnob, "Precomputed stereo delay", 0.0, true);
    setupKnob(pogoKnob,        "Precomputed pogo bend",  0.0, true);
    contentCanvas.addAndMakeVisible(clipBtn);

    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "boost", boostKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "eq", eqKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "rmMix", rmMixKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "rmFreq", rmFreqKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "cut", cutKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "res", resKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "reverb", reverbKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "stereoDelay", stereoDelayKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "pogo", pogoKnob));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "clip", clipBtn));
    boostKnob.setDefaultResetValue(0.0);
    eqKnob.setDefaultResetValue(0.0);
    rmMixKnob.setDefaultResetValue(0.0);
    rmFreqKnob.setDefaultResetValue(0.5);
    cutKnob.setDefaultResetValue(1.0);
    resKnob.setDefaultResetValue(0.0);
    reverbKnob.setDefaultResetValue(0.0);
    stereoDelayKnob.setDefaultResetValue(0.0);
    pogoKnob.setDefaultResetValue(0.0);

    wavScope.onSampleDropped = [this]() { repaint(); };
    wavScope.onHintChanged   = [this](const juce::String& h) { setHintText(h); };
    contentCanvas.addAndMakeVisible(wavScope);

    // ==================== TAB 2: ENVELOPE / INSTRUMENT SETTINGS ====================
    contentCanvas.addAndMakeVisible(volEnvOnBtn);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "volEnvOn", volEnvOnBtn));

    setupKnob(envDelayKnob,   "Envelope delay",   0.0);
    setupKnob(envAttackKnob,  "Envelope attack",  0.02);
    setupKnob(envHoldKnob,    "Envelope hold",    0.0);
    setupKnob(envDecayKnob,   "Envelope decay",   0.35);
    setupKnob(envSustainKnob, "Envelope sustain", 0.80);
    setupKnob(envReleaseKnob, "Envelope release", 0.20);
    setupKnob(envAmountKnob,  "Envelope amount",  0.0, true);
    envAmountKnob.setRange(-1.0, 1.0, 0.01);
    envAmountKnob.setValue(0.0);
    envAmountKnob.setDefaultResetValue(0.0);
    envAmountKnob.onValueChange = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].amount = (float)envAmountKnob.getValue(); };

    setupKnob(envAttTensionKnob, "Attack tension",  0.0,   true);
    envAttTensionKnob.setRange(-1.0, 1.0, 0.01);
    envAttTensionKnob.setValue(0.0);
    envAttTensionKnob.setDefaultResetValue(0.0);
    envAttTensionKnob.onValueChange = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].attTension = (float)envAttTensionKnob.getValue(); repaint(); };
    setupKnob(envDecTensionKnob, "Decay tension",  -0.35,  true);
    envDecTensionKnob.setRange(-1.0, 1.0, 0.01);
    envDecTensionKnob.setValue(-0.35);
    envDecTensionKnob.setDefaultResetValue(-0.35);
    envDecTensionKnob.onValueChange = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].decTension = (float)envDecTensionKnob.getValue(); repaint(); };
    setupKnob(envRelTensionKnob, "Release tension", -0.45, true);
    envRelTensionKnob.setRange(-1.0, 1.0, 0.01);
    envRelTensionKnob.setValue(-0.45);
    envRelTensionKnob.setDefaultResetValue(-0.45);
    envRelTensionKnob.onValueChange = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].relTension = (float)envRelTensionKnob.getValue(); repaint(); };

    contentCanvas.addAndMakeVisible(envTempoBtn);
    envTempoBtn.onClick = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].tempoSync = envTempoBtn.getToggleState(); };

    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "envDelay", envDelayKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "envAttack", envAttackKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "envHold", envHoldKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "envDecay", envDecayKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "envSustain", envSustainKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "envRelease", envReleaseKnob));
    envDelayKnob.setDefaultResetValue(0.0);
    envAttackKnob.setDefaultResetValue(0.02);
    envHoldKnob.setDefaultResetValue(0.0);
    envDecayKnob.setDefaultResetValue(0.35);
    envSustainKnob.setDefaultResetValue(0.80);
    envReleaseKnob.setDefaultResetValue(0.20);
    envDelayKnob.onValueChange   = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].delay   = (float)envDelayKnob.getValue();   repaint(); };
    envAttackKnob.onValueChange  = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].attack  = (float)envAttackKnob.getValue();  repaint(); };
    envHoldKnob.onValueChange    = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].hold    = (float)envHoldKnob.getValue();    repaint(); };
    envDecayKnob.onValueChange   = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].decay   = (float)envDecayKnob.getValue();   repaint(); };
    envSustainKnob.onValueChange = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].sustain = (float)envSustainKnob.getValue(); repaint(); };
    envReleaseKnob.onValueChange = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].release = (float)envReleaseKnob.getValue(); repaint(); };
    volEnvOnBtn.onClick          = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].enabled = volEnvOnBtn.getToggleState();     repaint(); };

    setupKnob(lfoDelayKnob, "LFO delay",  0.0);
    lfoDelayKnob.setRange(0.0, 1.0, 0.01);
    lfoDelayKnob.setValue(audioProcessor.lfoDelayAmt.load());
    lfoDelayKnob.setDefaultResetValue(0.0);
    lfoDelayKnob.onValueChange = [this]() {
        float v = (float)lfoDelayKnob.getValue();
        audioProcessor.lfoDelayAmt.store(v, std::memory_order_relaxed);
        audioProcessor.targetLfos[(size_t)currentInsTarget].delay = v;
        repaint();
    };
    setupKnob(lfoAttKnob,   "LFO attack", 0.35);
    lfoAttKnob.setRange(0.0, 1.0, 0.01);
    lfoAttKnob.setValue(audioProcessor.lfoAttackAmt.load());
    lfoAttKnob.setDefaultResetValue(0.35);
    lfoAttKnob.onValueChange = [this]() {
        float v = (float)lfoAttKnob.getValue();
        audioProcessor.lfoAttackAmt.store(v, std::memory_order_relaxed);
        audioProcessor.targetLfos[(size_t)currentInsTarget].attack = v;
        repaint();
    };
    setupKnob(lfoAmtKnob,   "LFO amount", 0.0, true);
    setupKnob(lfoSpeedKnob, "LFO speed",  0.5);
    contentCanvas.addAndMakeVisible(lfoTempoBtn);
    contentCanvas.addAndMakeVisible(lfoGlobalBtn);
    lfoTempoBtn.onClick  = [this]() { audioProcessor.targetLfos[(size_t)currentInsTarget].tempoSync   = lfoTempoBtn.getToggleState(); };
    lfoGlobalBtn.onClick = [this]() { audioProcessor.targetLfos[(size_t)currentInsTarget].globalPhase = lfoGlobalBtn.getToggleState(); };

    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "lfoAmt", lfoAmtKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "lfoSpeed", lfoSpeedKnob));
    lfoAmtKnob.setDefaultResetValue(0.0);
    lfoSpeedKnob.setDefaultResetValue(0.5);
    lfoAmtKnob.onValueChange   = [this]() { audioProcessor.targetLfos[(size_t)currentInsTarget].amount = (float)lfoAmtKnob.getValue(); repaint(); };
    lfoSpeedKnob.onValueChange = [this]()
    {
        float v = (float)lfoSpeedKnob.getValue();
        audioProcessor.targetLfos[(size_t)currentInsTarget].speed = v;
        if (lfoTempoBtn.getToggleState())
            setHintText("LFO speed (Sync): " + juce::String(fls::SamplerEngine::getMusicalDivisionName(v)));
        repaint();
    };

    setupKnob(filterModXKnob, "Filter cutoff (Mod X)",    1.0, false, false, false);
    setupKnob(filterModYKnob, "Filter resonance (Mod Y)", 0.0, false, false, false);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "filterModX", filterModXKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "filterModY", filterModYKnob));
    filterModXKnob.setDefaultResetValue(1.0);
    filterModYKnob.setDefaultResetValue(0.0);

    filterTypeCombo.setName("Filter type");
    filterTypeCombo.onHintChanged = [this](const juce::String& h) { setHintText(h); };
    contentCanvas.addAndMakeVisible(filterTypeCombo);
    filterTypeCombo.addItemList({ "Fast LP", "LP", "BP", "HP", "BS", "SVF LPx2" }, 1);
    comboAttachments.push_back(std::make_unique<ComboAttachment>(audioProcessor.apvts, "filterType", filterTypeCombo));

    // Shared Bottom Root Note & Keyboard Controls (Tab 2 & Tab 3)
    contentCanvas.addAndMakeVisible(enableMainPitchBtn);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "enableMainPitch", enableMainPitchBtn));
    contentCanvas.addAndMakeVisible(addToKeyBtn);
    addToKeyBtn.setToggleState(audioProcessor.addToKey.load(), juce::dontSendNotification);
    addToKeyBtn.onClick = [this]() { audioProcessor.addToKey.store(addToKeyBtn.getToggleState(), std::memory_order_relaxed); };

    fineTuneSlider.setName("Fine tune (cents)");
    fineTuneSlider.onHintChanged = [this](const juce::String& h) { setHintText(h); };
    fineTuneSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    fineTuneSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fineTuneSlider.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    fineTuneSlider.getProperties().set("flBipolar", true);
    contentCanvas.addAndMakeVisible(fineTuneSlider);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "fineTune", fineTuneSlider));
    fineTuneSlider.setDefaultResetValue(0.0);

    // ==================== TAB 3: MISCELLANEOUS FUNCTIONS ====================
    // 1. Levels adjustment
    setupKnob(lvlPanKnob,  "Panning multiplier", 0.0,  true);
    lvlPanKnob.setRange(-1.0, 1.0, 0.01);
    lvlPanKnob.setValue(audioProcessor.lvlPanOffset.load());
    lvlPanKnob.setDefaultResetValue(0.0);
    lvlPanKnob.onValueChange = [this]() { audioProcessor.lvlPanOffset.store((float)lvlPanKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(lvlVolKnob,  "Volume multiplier",  0.78, false, true);
    lvlVolKnob.setRange(0.0, 1.0, 0.01);
    lvlVolKnob.setValue(audioProcessor.lvlVolOffset.load());
    lvlVolKnob.setDefaultResetValue(0.78);
    lvlVolKnob.onValueChange = [this]() { audioProcessor.lvlVolOffset.store((float)lvlVolKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(lvlModXKnob, "Mod X offset",       0.0,  true);
    lvlModXKnob.setRange(-1.0, 1.0, 0.01);
    lvlModXKnob.setValue(audioProcessor.lvlModXOffset.load());
    lvlModXKnob.setDefaultResetValue(0.0);
    lvlModXKnob.onValueChange = [this]() { audioProcessor.lvlModXOffset.store((float)lvlModXKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(lvlModYKnob, "Mod Y offset",       0.0,  true);
    lvlModYKnob.setRange(-1.0, 1.0, 0.01);
    lvlModYKnob.setValue(audioProcessor.lvlModYOffset.load());
    lvlModYKnob.setDefaultResetValue(0.0);
    lvlModYKnob.onValueChange = [this]() { audioProcessor.lvlModYOffset.store((float)lvlModYKnob.getValue(), std::memory_order_relaxed); };

    // 2. Polyphony
    contentCanvas.addAndMakeVisible(maxPolyBox);
    {
        const int curMaxPoly = (int)audioProcessor.apvts.getRawParameterValue("maxPoly")->load();
        maxPolyBox.setValue(curMaxPoly == 16 ? 0 : curMaxPoly);
    }
    maxPolyBox.onValueChanged = [this](int v)
    {
        if (auto* param = audioProcessor.apvts.getParameter("maxPoly"))
            param->setValueNotifyingHost(param->convertTo0to1((float)(v == 0 ? 16 : v)));
        setHintText("Max polyphony: " + (v == 0 ? juce::String("--- (16)") : juce::String(v)));
    };
    setupKnob(portaTimeKnob, "Portamento slide time", 0.12);
    contentCanvas.addAndMakeVisible(portaBtn);
    contentCanvas.addAndMakeVisible(monoBtn);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "portaTime", portaTimeKnob));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "porta", portaBtn));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "mono", monoBtn));
    portaTimeKnob.setDefaultResetValue(0.12);

    // 3. Time
    setupKnob(gateTimeKnob, "Gate time", 1.0, false, false, true);
    setupKnob(shiftKnob,    "Time shift", 0.0);
    shiftKnob.setRange(0.0, 1.0, 0.01);
    shiftKnob.setValue(audioProcessor.shiftTimeAmt.load());
    shiftKnob.setDefaultResetValue(0.0);
    shiftKnob.onValueChange = [this]() { audioProcessor.shiftTimeAmt.store((float)shiftKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(swingKnob,    "Swing multiplier", 0.0, false, false, true);
    swingKnob.setRange(0.0, 1.0, 0.01);
    swingKnob.setValue(audioProcessor.swingMulAmt.load());
    swingKnob.setDefaultResetValue(0.0);
    swingKnob.onValueChange = [this]() { audioProcessor.swingMulAmt.store((float)swingKnob.getValue(), std::memory_order_relaxed); };
    contentCanvas.addAndMakeVisible(fullPortaBtn);
    fullPortaBtn.setToggleState(audioProcessor.fullPorta.load(), juce::dontSendNotification);
    fullPortaBtn.onClick = [this]() { audioProcessor.fullPorta.store(fullPortaBtn.getToggleState(), std::memory_order_relaxed); };
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "gateTime", gateTimeKnob));
    gateTimeKnob.setDefaultResetValue(1.0);

    // 4. Tracking
    setupKnob(trackPanKnob,  "Tracking pan",   0.0, true);
    trackPanKnob.setRange(-1.0, 1.0, 0.01);
    trackPanKnob.setValue(audioProcessor.trackPanAmt.load());
    trackPanKnob.setDefaultResetValue(0.0);
    trackPanKnob.onValueChange = [this]() { audioProcessor.trackPanAmt.store((float)trackPanKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(trackModXKnob, "Tracking Mod X", 0.0, true);
    trackModXKnob.setRange(-1.0, 1.0, 0.01);
    trackModXKnob.setValue(audioProcessor.trackModXAmt.load());
    trackModXKnob.setDefaultResetValue(0.0);
    trackModXKnob.onValueChange = [this]() { audioProcessor.trackModXAmt.store((float)trackModXKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(trackModYKnob, "Tracking Mod Y", 0.0, true);
    trackModYKnob.setRange(-1.0, 1.0, 0.01);
    trackModYKnob.setValue(audioProcessor.trackModYAmt.load());
    trackModYKnob.setDefaultResetValue(0.0);
    trackModYKnob.onValueChange = [this]() { audioProcessor.trackModYAmt.store((float)trackModYKnob.getValue(), std::memory_order_relaxed); };

    trackMidSlider.setName("Tracking middle value");
    trackMidSlider.onHintChanged = [this](const juce::String& h) { setHintText(h); };
    trackMidSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    trackMidSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    trackMidSlider.setRange(0.0, 127.0, 1.0);
    trackMidSlider.setValue(audioProcessor.trackMidNote.load());
    trackMidSlider.setDefaultResetValue(60.0);
    trackMidSlider.onValueChange = [this]() { audioProcessor.trackMidNote.store((int)trackMidSlider.getValue(), std::memory_order_relaxed); };
    trackMidSlider.getProperties().set("flOrangeRing", true);
    contentCanvas.addAndMakeVisible(trackMidSlider);

    // 5. Group
    contentCanvas.addAndMakeVisible(cutGroupBox);
    cutGroupBox.setValue(audioProcessor.cutGroup.load());
    cutGroupBox.onValueChanged = [this](int v)
    {
        audioProcessor.cutGroup.store(v, std::memory_order_relaxed);
        setHintText("Cut group: " + (v == 0 ? juce::String("---") : juce::String(v)));
    };
    contentCanvas.addAndMakeVisible(cutByBox);
    cutByBox.setValue(audioProcessor.cutByGroup.load());
    cutByBox.onValueChanged = [this](int v)
    {
        audioProcessor.cutByGroup.store(v, std::memory_order_relaxed);
        setHintText("Cut by group: " + (v == 0 ? juce::String("---") : juce::String(v)));
    };
    contentCanvas.addAndMakeVisible(cutSelfPushBtn);
    cutSelfPushBtn.setClickingTogglesState(true);
    cutSelfPushBtn.setColour(juce::TextButton::buttonColourId, FLColors::recessedBg());
    cutSelfPushBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4a5462));
    cutSelfPushBtn.setColour(juce::TextButton::textColourOffId, FLColors::textBright());
    cutSelfPushBtn.setColour(juce::TextButton::textColourOnId, FLColors::greenNeon());
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "cutSelf", cutSelfPushBtn));

    // 6. Arpeggiator
    setupKnob(arpTimeKnob, "Arpeggiator time", 0.125, false, false, true);
    setupKnob(arpGateKnob, "Arpeggiator gate", 1.0,   false, false, true);
    arpGateKnob.setRange(0.0, 1.0, 0.01);
    arpGateKnob.setValue(1.0);
    arpGateKnob.setDefaultResetValue(1.0);
    contentCanvas.addAndMakeVisible(arpRangeBox);
    arpRangeBox.setValue((int)audioProcessor.apvts.getRawParameterValue("arpRange")->load());
    arpRangeBox.onValueChanged = [this](int v)
    {
        if (auto* param = audioProcessor.apvts.getParameter("arpRange"))
            param->setValueNotifyingHost(param->convertTo0to1((float)v));
        setHintText("Arpeggiator range: " + juce::String(v) + " oct");
    };
    contentCanvas.addAndMakeVisible(arpRepeatBox);
    arpRepeatBox.onValueChanged = [this](int v)
    {
        audioProcessor.arpRepeatCount.store(juce::jmax(1, v == 0 ? 1 : v), std::memory_order_relaxed);
        setHintText("Arpeggiator repeat: " + (v == 0 ? juce::String("--- (1)") : juce::String(v)));
    };
    contentCanvas.addAndMakeVisible(arpSlideBtn);
    arpSlideBtn.setToggleState(audioProcessor.arpSlideOn.load(), juce::dontSendNotification);
    arpSlideBtn.onClick = [this]() { audioProcessor.arpSlideOn.store(arpSlideBtn.getToggleState(), std::memory_order_relaxed); };
    arpChordCombo.setName("Arpeggiator chord");
    arpChordCombo.onHintChanged = [this](const juce::String& h) { setHintText(h); };
    contentCanvas.addAndMakeVisible(arpChordCombo);
    arpChordCombo.addItemList({
        "Auto",
        "Auto (sustain)",
        "(none)",
        "Major",
        "Minor",
        "Sus2",
        "Sus4"
    }, 1);
    arpChordCombo.setSelectedId(2, juce::dontSendNotification);
    arpChordCombo.onChange = [this]()
    {
        audioProcessor.arpChordMode.store(arpChordCombo.getSelectedId() - 1, std::memory_order_relaxed);
    };
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "arpTime", arpTimeKnob));
    arpTimeKnob.setDefaultResetValue(0.125);
    arpTimeKnob.onValueChange = [this]()
    {
        float norm = (float)arpTimeKnob.valueToProportionOfLength(arpTimeKnob.getValue());
        setHintText("Arpeggiator time: " + juce::String(fls::SamplerEngine::getMusicalDivisionName(1.0f - norm)));
    };

    // 7. Echo delay / fat mode
    setupKnob(echoFeedKnob,  "Echo feedback", 0.45);
    setupKnob(echoPanKnob,   "Echo panning",  0.0, true);
    echoPanKnob.setRange(-1.0, 1.0, 0.01);
    echoPanKnob.setValue(audioProcessor.echoPanAmt.load());
    echoPanKnob.setDefaultResetValue(0.0);
    echoPanKnob.onValueChange = [this]() { audioProcessor.echoPanAmt.store((float)echoPanKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(echoModXKnob,  "Echo cutoff",   0.65, false, true);
    echoModXKnob.setRange(0.0, 1.0, 0.01);
    echoModXKnob.setValue(audioProcessor.echoModXAmt.load());
    echoModXKnob.setDefaultResetValue(0.65);
    echoModXKnob.onValueChange = [this]() { audioProcessor.echoModXAmt.store((float)echoModXKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(echoModYKnob,  "Echo resonance", 0.25, false, false, true);
    echoModYKnob.setRange(0.0, 1.0, 0.01);
    echoModYKnob.setValue(audioProcessor.echoModYAmt.load());
    echoModYKnob.setDefaultResetValue(0.25);
    echoModYKnob.onValueChange = [this]() { audioProcessor.echoModYAmt.store((float)echoModYKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(echoPitchKnob, "Echo pitch",    0.0, true);
    echoPitchKnob.setRange(-12.0, 12.0, 0.1);
    echoPitchKnob.setValue(audioProcessor.echoPitchAmt.load());
    echoPitchKnob.setDefaultResetValue(0.0);
    echoPitchKnob.onValueChange = [this]() { audioProcessor.echoPitchAmt.store((float)echoPitchKnob.getValue(), std::memory_order_relaxed); };
    setupKnob(echoTimeKnob,  "Echo time",     0.20, false, false, true);

    contentCanvas.addAndMakeVisible(echoCountBox);
    echoCountBox.setValue((int)audioProcessor.apvts.getRawParameterValue("echoCount")->load());
    echoCountBox.onValueChanged = [this](int v)
    {
        if (auto* param = audioProcessor.apvts.getParameter("echoCount"))
            param->setValueNotifyingHost(param->convertTo0to1((float)v));
        setHintText("Echoes: " + (v == 0 ? juce::String("Off") : juce::String(v)));
    };
    contentCanvas.addAndMakeVisible(echoPingPongBtn);
    contentCanvas.addAndMakeVisible(echoFatModeBtn);
    echoFatModeBtn.setToggleState(audioProcessor.echoFatModeOn.load(), juce::dontSendNotification);
    echoFatModeBtn.onClick = [this]() { audioProcessor.echoFatModeOn.store(echoFatModeBtn.getToggleState(), std::memory_order_relaxed); };
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "echoFeed", echoFeedKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "echoTime", echoTimeKnob));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "echoPingPong", echoPingPongBtn));
    echoFeedKnob.setDefaultResetValue(0.45);
    echoTimeKnob.setDefaultResetValue(0.20);
    echoTimeKnob.onValueChange = [this]()
    {
        float norm = (float)echoTimeKnob.valueToProportionOfLength(echoTimeKnob.getValue());
        setHintText("Echo delay time: " + juce::String(fls::SamplerEngine::getMusicalDivisionName(1.0f - norm)));
    };

    // Make window freely resizable with fixed aspect ratio (75% .. 250% scale of 526x456)
    setResizable(true, true);
    setResizeLimits(394, 342, 1315, 1140);
    getConstrainer()->setFixedAspectRatio((double)kDesignW / (double)kDesignH);

    // Exact FL Studio 2026 Sampler base dimensions
    setSize(kDesignW, kDesignH);
    updateInsTarget(1);
    updateActiveSheet(0);
    startTimerHz(30);
}

FLSamplerAudioProcessorEditor::~FLSamplerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void FLSamplerAudioProcessorEditor::setHintText(const juce::String& text)
{
    if (currentHintText != text)
    {
        currentHintText = text;
        repaint();
    }
}

void FLSamplerAudioProcessorEditor::setupKnob(fls::FLSlider& s, const juce::String& hintName, double defaultVal,
                                              bool bipolar, bool greenRing, bool orangeRing)
{
    s.setName(hintName);
    s.setDefaultResetValue(defaultVal);
    s.onHintChanged = [this](const juce::String& h) { setHintText(h); };
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.getProperties().set("flBipolar", bipolar);
    s.getProperties().set("flGreenRing", greenRing);
    s.getProperties().set("flOrangeRing", orangeRing);
    s.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    contentCanvas.addAndMakeVisible(s);
}

void FLSamplerAudioProcessorEditor::updateInsTarget(int targetIdx)
{
    currentInsTarget = juce::jlimit(0, 4, targetIdx);
    const int ringHex = (int)kInsTargetColors[currentInsTarget];

    juce::Slider* envKnobs[] = {
        &envDelayKnob, &envAttackKnob, &envHoldKnob, &envDecayKnob, &envSustainKnob, &envReleaseKnob,
        &envAmountKnob, &envAttTensionKnob, &envDecTensionKnob, &envRelTensionKnob,
        &lfoDelayKnob, &lfoAttKnob, &lfoAmtKnob, &lfoSpeedKnob
    };
    for (auto* k : envKnobs)
        k->getProperties().set("flRingColour", ringHex);

    const auto& te = audioProcessor.targetEnvs[(size_t)currentInsTarget];
    volEnvOnBtn.setToggleState(te.enabled, juce::dontSendNotification);
    envDelayKnob.setValue(te.delay, juce::dontSendNotification);
    envAttackKnob.setValue(te.attack, juce::dontSendNotification);
    envHoldKnob.setValue(te.hold, juce::dontSendNotification);
    envDecayKnob.setValue(te.decay, juce::dontSendNotification);
    envSustainKnob.setValue(te.sustain, juce::dontSendNotification);
    envReleaseKnob.setValue(te.release, juce::dontSendNotification);
    envAmountKnob.setValue(te.amount, juce::dontSendNotification);
    envAttTensionKnob.setValue(te.attTension, juce::dontSendNotification);
    envDecTensionKnob.setValue(te.decTension, juce::dontSendNotification);
    envRelTensionKnob.setValue(te.relTension, juce::dontSendNotification);
    envTempoBtn.setToggleState(te.tempoSync, juce::dontSendNotification);

    const auto& tl = audioProcessor.targetLfos[(size_t)currentInsTarget];
    currentLfoShape = tl.shape;
    lfoDelayKnob.setValue(tl.delay, juce::dontSendNotification);
    lfoAttKnob.setValue(tl.attack, juce::dontSendNotification);
    lfoAmtKnob.setValue(tl.amount, juce::dontSendNotification);
    lfoSpeedKnob.setValue(tl.speed, juce::dontSendNotification);
    lfoTempoBtn.setToggleState(tl.tempoSync, juce::dontSendNotification);
    lfoGlobalBtn.setToggleState(tl.globalPhase, juce::dontSendNotification);

    if (currentSheet == 1)
        updateActiveSheet(1);
    repaint();
}

void FLSamplerAudioProcessorEditor::updateActiveSheet(int sheetIdx)
{
    currentSheet = juce::jlimit(0, 2, sheetIdx);
    const bool isSmp  = (currentSheet == 0);
    const bool isIns  = (currentSheet == 1);
    const bool isFunc = (currentSheet == 2);

    keepOnDiskBtn.setVisible(isSmp);
    resampleBtn.setVisible(isSmp);
    loadRegionsBtn.setVisible(isSmp);
    loadSlicesBtn.setVisible(isSmp);
    declickCombo.setVisible(isSmp);

    startOffsetKnob.setVisible(isSmp);
    loopStartKnob.setVisible(isSmp);
    loopEndKnob.setVisible(isSmp);
    useLoopPointsBtn.setVisible(isSmp);
    pingPongLoopBtn.setVisible(isSmp);

    stretchPitchKnob.setVisible(isSmp);
    stretchMulKnob.setVisible(isSmp);
    stretchTimeKnob.setVisible(isSmp);
    formantShiftKnob.setVisible(isSmp);
    stretchModeCombo.setVisible(isSmp);

    wavScope.setVisible(isSmp);
    updatePrecomputedSubTab(legacyPrecomputedTab);

    // Tab 2: InsSheet (Note: Panning target [0] hides the Envelope panel, matching Screenshot 4!)
    const bool showEnvPanel = isIns && (currentInsTarget != 0);
    const bool showEnvAmt   = showEnvPanel && (currentInsTarget >= 2);
    volEnvOnBtn.setVisible(showEnvPanel);
    envDelayKnob.setVisible(showEnvPanel);
    envAttackKnob.setVisible(showEnvPanel);
    envHoldKnob.setVisible(showEnvPanel);
    envDecayKnob.setVisible(showEnvPanel);
    envSustainKnob.setVisible(showEnvPanel);
    envReleaseKnob.setVisible(showEnvPanel);
    envAttTensionKnob.setVisible(showEnvPanel);
    envDecTensionKnob.setVisible(showEnvPanel);
    envRelTensionKnob.setVisible(showEnvPanel);
    envTempoBtn.setVisible(showEnvPanel);
    envAmountKnob.setVisible(showEnvAmt);

    lfoDelayKnob.setVisible(isIns);
    lfoAttKnob.setVisible(isIns);
    lfoAmtKnob.setVisible(isIns);
    lfoSpeedKnob.setVisible(isIns);
    lfoTempoBtn.setVisible(isIns);
    lfoGlobalBtn.setVisible(isIns);

    filterModXKnob.setVisible(isIns);
    filterModYKnob.setVisible(isIns);
    filterTypeCombo.setVisible(isIns);

    // Shared Bottom Keyboard Panel (Visible in both Tab 2 & Tab 3!)
    const bool showKeyb = (isIns || isFunc);
    enableMainPitchBtn.setVisible(showKeyb);
    addToKeyBtn.setVisible(showKeyb);
    fineTuneSlider.setVisible(showKeyb);

    // Tab 3: FuncSheet
    lvlPanKnob.setVisible(isFunc);
    lvlVolKnob.setVisible(isFunc);
    lvlModXKnob.setVisible(isFunc);
    lvlModYKnob.setVisible(isFunc);

    maxPolyBox.setVisible(isFunc);
    portaTimeKnob.setVisible(isFunc);
    portaBtn.setVisible(isFunc);
    monoBtn.setVisible(isFunc);

    gateTimeKnob.setVisible(isFunc);
    shiftKnob.setVisible(isFunc);
    swingKnob.setVisible(isFunc);
    fullPortaBtn.setVisible(isFunc);

    trackPanKnob.setVisible(isFunc);
    trackModXKnob.setVisible(isFunc);
    trackModYKnob.setVisible(isFunc);
    trackMidSlider.setVisible(isFunc);

    cutGroupBox.setVisible(isFunc);
    cutByBox.setVisible(isFunc);
    cutSelfPushBtn.setVisible(isFunc);

    arpTimeKnob.setVisible(isFunc);
    arpGateKnob.setVisible(isFunc);
    arpRangeBox.setVisible(isFunc);
    arpRepeatBox.setVisible(isFunc);
    arpSlideBtn.setVisible(isFunc);
    arpChordCombo.setVisible(isFunc);

    echoFeedKnob.setVisible(isFunc);
    echoPanKnob.setVisible(isFunc);
    echoModXKnob.setVisible(isFunc);
    echoModYKnob.setVisible(isFunc);
    echoPitchKnob.setVisible(isFunc);
    echoTimeKnob.setVisible(isFunc);
    echoCountBox.setVisible(isFunc);
    echoPingPongBtn.setVisible(isFunc);
    echoFatModeBtn.setVisible(isFunc);

    repaint();
}

void FLSamplerAudioProcessorEditor::updatePrecomputedSubTab(bool showLegacy)
{
    legacyPrecomputedTab = showLegacy;
    const bool isSmp = (currentSheet == 0);
    const bool stdVis = isSmp && !legacyPrecomputedTab;
    const bool legVis = isSmp && legacyPrecomputedTab;

    removeDcBtn.setVisible(stdVis);
    normalizeBtn.setVisible(stdVis);
    reverseBtn.setVisible(stdVis);
    reversePolBtn.setVisible(stdVis);
    fadeStereoBtn.setVisible(stdVis);
    swapStereoBtn.setVisible(stdVis);
    smpStartKnob.setVisible(stdVis);
    lengthKnob.setVisible(stdVis);
    fadeInKnob.setVisible(stdVis);
    fadeOutKnob.setVisible(stdVis);
    crossfadeKnob.setVisible(stdVis);
    trimKnob.setVisible(stdVis);

    boostKnob.setVisible(legVis);
    clipBtn.setVisible(legVis);
    eqKnob.setVisible(legVis);
    rmMixKnob.setVisible(legVis);
    rmFreqKnob.setVisible(legVis);
    cutKnob.setVisible(legVis);
    resKnob.setVisible(legVis);
    reverbKnob.setVisible(legVis);
    stereoDelayKnob.setVisible(legVis);
    pogoKnob.setVisible(legVis);

    repaint();
}

void FLSamplerAudioProcessorEditor::openSampleFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Sample Audio File",
        audioProcessor.getLoadedFile().existsAsFile()
            ? audioProcessor.getLoadedFile().getParentDirectory()
            : juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.flac;*.mp3;*.ogg;*.aiff;*.aif");

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f.existsAsFile())
            {
                audioProcessor.loadSampleFromFile(f);
                wavScope.refreshIfNeeded();
                repaint();
            }
        });
}

void FLSamplerAudioProcessorEditor::timerCallback()
{
    wavScope.refreshIfNeeded();
    if (currentSheet == 1)
        repaint(6, 80, getWidth() - 12, 220);
}

// Draws an FL Studio 2026 panel (or Ableton Live 12 flat device rack / GrowthTune Private card when active)
static void drawFLPanel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, int titleIndent = 8)
{
    auto rf = r.toFloat();
    if (FLColors::skinMode == 1)
    {
        g.setColour(FLColors::panelBg());
        g.fillRect(rf);
        g.setColour(FLColors::panelBorder());
        g.drawRect(rf, 1.0f);

        if (title.isNotEmpty())
        {
            g.setColour(juce::Colour(0xff2c2c2c));
            g.fillRect((float)r.getX() + 1.0f, (float)r.getY() + 1.0f, (float)r.getWidth() - 2.0f, 17.0f);
            g.setColour(FLColors::panelBorder());
            g.drawHorizontalLine(r.getY() + 18, (float)r.getX() + 1.0f, (float)r.getRight() - 1.0f);

            g.setColour(FLColors::textBright());
            g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            g.drawText(title, r.getX() + titleIndent, r.getY() + 2, r.getWidth() - titleIndent - 4, 16,
                       juce::Justification::centredLeft, false);
        }
        return;
    }
    if (FLColors::skinMode == 2)
    {
        // Translucent dark-glass card so VOCESFX Skin 1 background shines through cleanly!
        g.setColour(juce::Colour(0xc8120c18));
        g.fillRoundedRectangle(rf, 6.0f);
        g.setColour(juce::Colour(0xffec4899).withAlpha(0.38f));
        g.drawRoundedRectangle(rf.reduced(0.5f), 6.0f, 1.1f);

        if (title.isNotEmpty())
        {
            juce::ColourGradient titleGrad(juce::Colour(0xffec4899).withAlpha(0.18f), (float)r.getX(), (float)r.getY(),
                                           juce::Colour(0xffa855f7).withAlpha(0.03f), (float)r.getRight(), (float)r.getY(), false);
            g.setGradientFill(titleGrad);
            g.fillRoundedRectangle((float)r.getX() + 1.0f, (float)r.getY() + 1.0f, (float)r.getWidth() - 2.0f, 18.0f, 5.0f);
            g.setColour(juce::Colour(0xff2d243b));
            g.drawHorizontalLine(r.getY() + 19, (float)r.getX() + 2.0f, (float)r.getRight() - 2.0f);

            g.setColour(FLColors::textBright());
            g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            g.drawText(title, r.getX() + titleIndent, r.getY() + 2, r.getWidth() - titleIndent - 4, 16,
                       juce::Justification::centredLeft, false);
        }
        return;
    }
    if (FLColors::skinMode == 3)
    {
        g.setColour(FLColors::panelBg());
        g.fillRoundedRectangle(rf, 4.0f);
        g.setColour(FLColors::panelBorder());
        g.drawRoundedRectangle(rf.reduced(0.5f), 4.0f, 1.0f);

        if (title.isNotEmpty())
        {
            g.setColour(juce::Colour(0xff141418));
            g.fillRoundedRectangle((float)r.getX() + 1.0f, (float)r.getY() + 1.0f, (float)r.getWidth() - 2.0f, 18.0f, 3.0f);
            g.setColour(juce::Colour(0xff27272a));
            g.drawHorizontalLine(r.getY() + 19, (float)r.getX() + 1.0f, (float)r.getRight() - 1.0f);

            g.setColour(FLColors::textBright());
            g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            g.drawText(title, r.getX() + titleIndent, r.getY() + 2, r.getWidth() - titleIndent - 4, 16,
                       juce::Justification::centredLeft, false);
        }
        return;
    }

    g.setColour(FLColors::panelBg());
    g.fillRoundedRectangle(rf, 2.0f);
    g.setColour(FLColors::panelBorder());
    g.drawRoundedRectangle(rf.reduced(0.5f), 2.0f, 1.0f);

    if (title.isNotEmpty())
    {
        // Angled chamfer line after title
        int titleW = (int)title.length() * 7 + titleIndent + 14;
        g.setColour(juce::Colour(0xff262b32));
        g.drawLine((float)(r.getX() + titleW), (float)r.getY(),
                   (float)(r.getX() + titleW + 10), (float)(r.getY() + 18), 1.0f);
        g.drawLine((float)(r.getX() + titleW + 10), (float)(r.getY() + 18),
                   (float)r.getRight() - 1.0f, (float)(r.getY() + 18), 1.0f);

        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(12.5f)));
        g.drawText(title, r.getX() + titleIndent, r.getY() + 2, r.getWidth() - titleIndent - 4, 16,
                   juce::Justification::centredLeft, false);
    }
}

static void drawSmallCapLabel(juce::Graphics& g, const juce::Slider& s, const juce::String& text, int yOffset = 0)
{
    if (!s.isVisible()) return;
    auto b = s.getBounds();
    g.setColour(FLColors::textDim());
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.drawText(text, b.getX() - 18, b.getBottom() - 1 + yOffset, b.getWidth() + 36, 12,
               juce::Justification::centred, false);
}

void FLSamplerAudioProcessorEditor::drawRootNoteAndKeyboardPanel(juce::Graphics& g)
{
    static const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int rootNote = (int)audioProcessor.apvts.getRawParameterValue("rootNote")->load();
    const juce::String rootName = juce::String(noteNames[rootNote % 12]) + juce::String(rootNote / 12);

    drawFLPanel(g, { 6, 304, 514, 146 }, "Root note: " + rootName);

    // "Reset" button
    g.setColour(FLColors::recessedBg());
    g.fillRoundedRectangle(resetRootBtnRect.toFloat(), 2.5f);
    g.setColour(juce::Colour(0xff191c21));
    g.drawRoundedRectangle(resetRootBtnRect.toFloat(), 2.5f, 1.0f);
    g.setColour(FLColors::textBright());
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("Reset", resetRootBtnRect, juce::Justification::centred, false);

    // "Fine tune" label
    g.setColour(FLColors::textBright());
    g.drawText("Fine tune", 258, 330, 56, 18, juce::Justification::centredLeft, false);

    // Multi-touch hand icon on far right
    {
        float hx = 502.0f, hy = 338.0f;
        g.setColour(FLColors::textBright());
        g.fillRoundedRectangle(hx - 4.0f, hy - 3.0f, 8.0f, 7.0f, 2.0f);
        g.drawLine(hx - 6.0f, hy - 5.0f, hx - 1.0f, hy - 1.0f, 2.0f);
    }

    // Piano Keyboard C2..C8 (MIDI 24..96 = 43 white keys)
    g.setColour(juce::Colour(0xffd2d8e0));
    g.fillRect(bottomKeybRect);

    const int startMidi = 24; // C2
    const int endMidi   = 96; // C8

    // Count white keys (43 white keys from C2 to C8)
    auto isBlackKey = [](int m) {
        int pc = m % 12;
        return (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
    };

    const int numWhiteKeys = 43;
    const float whiteW = (float)bottomKeybRect.getWidth() / (float)numWhiteKeys;
    const float kbX = (float)bottomKeybRect.getX();
    const float kbY = (float)bottomKeybRect.getY();
    const float kbH = (float)bottomKeybRect.getHeight();

    // First pass: draw white keys
    int wIdx = 0;
    for (int m = startMidi; m <= endMidi; ++m)
    {
        if (isBlackKey(m)) continue;
        juce::Rectangle<float> wr(kbX + wIdx * whiteW, kbY, whiteW, kbH);

        int oct = (m / 12);
        bool altOctave = (oct % 2 == 1);
        if (m == rootNote)
            g.setColour(FLColors::isAbletonSkin ? juce::Colour(0xff00d2ff) : juce::Colour(0xff74a9e8));
        else if (m == previewHeldMidiNote)
            g.setColour(FLColors::greenNeon());
        else
            g.setColour(altOctave ? juce::Colour(0xffc9d0d8) : juce::Colour(0xffdde3ea));

        g.fillRect(wr);
        g.setColour(juce::Colour(0xff8e96a0));
        g.drawRect(wr, 0.5f);

        if (m % 12 == 0)
        {
            g.setColour(juce::Colour(0xff1e2228));
            g.setFont(juce::Font(juce::FontOptions(10.0f)));
            g.drawText("C" + juce::String(oct), (int)wr.getX() + 1, (int)kbY + 1, 20, 12,
                       juce::Justification::centredLeft, false);
        }
        ++wIdx;
    }

    // Top header strip above black keys
    g.setColour(juce::Colour(0xff68707a).withAlpha(0.35f));
    g.fillRect(kbX, kbY, (float)bottomKeybRect.getWidth(), 13.0f);

    // Second pass: draw 3D black keys
    wIdx = 0;
    for (int m = startMidi; m <= endMidi; ++m)
    {
        if (!isBlackKey(m))
        {
            ++wIdx;
            continue;
        }
        float bx = kbX + ((float)wIdx - 0.35f) * whiteW;
        float bw = whiteW * 0.70f;
        float bh = kbH * 0.62f;
        juce::Rectangle<float> br(bx, kbY + 13.0f, bw, bh - 13.0f);

        g.setColour(m == rootNote ? (FLColors::isAbletonSkin ? juce::Colour(0xff009ec2) : juce::Colour(0xff4c88d4))
                                  : (m == previewHeldMidiNote ? FLColors::greenNeon() : juce::Colour(0xff23272e)));
        g.fillRect(br);
        g.setColour(juce::Colour(0xff525a65));
        g.drawRect(br.reduced(1.2f, 1.2f), 0.7f);
    }

    // Bottom scrollbar bar under keyboard + Key Region Bar (Left-drag in scrollbar or Shift+Right-click keys)
    juce::Rectangle<int> scrollBar(bottomKeybRect.getX(), bottomKeybRect.getBottom(), bottomKeybRect.getWidth(), 12);
    g.setColour(juce::Colour(0xff22262c));
    g.fillRect(scrollBar);

    const int regLo = juce::jlimit(24, 96, audioProcessor.keyRegionLow.load());
    const int regHi = juce::jlimit(regLo, 96, audioProcessor.keyRegionHigh.load());
    float rx0 = kbX + ((float)(regLo - 24) / 72.0f) * (float)bottomKeybRect.getWidth();
    float rx1 = kbX + ((float)(regHi - 24 + 1) / 72.0f) * (float)bottomKeybRect.getWidth();
    g.setColour(FLColors::orangeRing().withAlpha(0.85f));
    g.fillRoundedRectangle(rx0, (float)scrollBar.getY() + 2.0f, juce::jmax(6.0f, rx1 - rx0), 8.0f, 2.0f);
}

void FLSamplerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(FLColors::windowBg());

    // Scale all vector drawing uniformly when window is resized!
    const float scaleX = (float)getWidth()  / (float)kDesignW;
    const float scaleY = (float)getHeight() / (float)kDesignH;
    juce::Graphics::ScopedSaveState sss(g);
    g.addTransform(juce::AffineTransform::scale(scaleX, scaleY));

    // Draw VOCESFX Skin 1 background image when PRIVATE skin (skinMode == 2) is active!
    if (FLColors::isPrivateSkin && privateSkinBg.isValid())
    {
        g.drawImage(privateSkinBg, juce::Rectangle<float>(0.0f, 0.0f, (float)kDesignW, (float)kDesignH),
                    juce::RectanglePlacement::stretchToFit);
        g.setColour(juce::Colours::black.withAlpha(0.32f));
        g.fillRect(0, 0, kDesignW, kDesignH);
    }

    // 1. Top Header Bar (y: 0..42)
    g.setColour(FLColors::isPrivateSkin ? juce::Colour(0xd8140d1c) : FLColors::headerBg());
    g.fillRect(0, 0, kDesignW, 42);
    g.setColour(FLColors::panelBorder());
    g.drawHorizontalLine(42, 0.0f, (float)kDesignW);

    // Highlight outer border in FL Neon Green when dragging an audio file anywhere over the plugin
    if (isWindowDragHover)
    {
        g.setColour(FLColors::greenNeon());
        g.drawRect(0, 0, kDesignW, kDesignH, 2);
    }

    // Magnifying Glass (Lupa) icon on the left, with Skin Switcher Button ("SKIN: FL" | "SKIN: LIVE" | "PRIVATE" | "SKIN: BLACK") on its right!
    {
        const float lupaX = 135.0f;
        const float lupaY = 13.0f;
        g.setColour(FLColors::isPrivateSkin ? juce::Colour(0xff06b6d4) : FLColors::greenNeon());
        g.drawEllipse(lupaX, lupaY, 10.5f, 10.5f, 1.6f);
        g.drawLine(lupaX + 8.8f, lupaY + 8.8f, lupaX + 13.8f, lupaY + 13.8f, 2.0f);

        auto sr = skinToggleBtnRect.toFloat();
        g.setColour(FLColors::recessedBg());
        g.fillRoundedRectangle(sr, FLColors::skinMode == 1 ? 0.0f : 3.0f);
        g.setColour(FLColors::skinMode > 0 ? FLColors::greenNeon() : FLColors::panelBorder());
        g.drawRoundedRectangle(sr.reduced(0.5f), FLColors::skinMode == 1 ? 0.0f : 3.0f, 1.0f);

        g.setColour(FLColors::skinMode > 0 ? FLColors::greenNeon() : FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
        const char* skinNames[4] = { "SKIN: FL", "SKIN: LIVE", "PRIVATE", "SKIN: BLACK" };
        g.drawText(skinNames[juce::jlimit(0, 3, FLColors::skinMode)], skinToggleBtnRect, juce::Justification::centred, false);

        // Draw PAPIELGUAPO vector signature ONLY in the PRIVATE skin (omitted in all other skins!)
        if (FLColors::isPrivateSkin && currentHintText.isEmpty())
        {
            FLColors::drawPapielguapoSignature(g, (float)skinToggleBtnRect.getRight() + 6.0f, 11.0f, 18.0f,
                juce::Colour(0xffec4899));
        }
    }

    // FL Hint Bar readout in header bar when active
    if (currentHintText.isNotEmpty())
    {
        g.setColour(FLColors::recessedBg().withAlpha(0.95f));
        g.fillRoundedRectangle(212.0f, 9.0f, 104.0f, 24.0f, FLColors::skinMode == 1 ? 0.0f : 3.0f);
        g.setColour(FLColors::textDim());
        g.setFont(juce::Font(juce::FontOptions(9.5f)));
        g.drawText(currentHintText, 215, 9, 98, 24, juce::Justification::centredLeft, true);
    }

    // 3 Left Sheet Tab Icons (Waveform, Envelope, Wrench)
    // Tab 0: Waveform icon
    {
        g.setColour(currentSheet == 0 ? FLColors::recessedBg() : juce::Colours::transparentBlack);
        g.fillRect(tabRectWave);

        juce::Path waveIcon;
        float cx = (float)tabRectWave.getCentreX();
        float cy = (float)tabRectWave.getCentreY();
        waveIcon.startNewSubPath(cx - 11.0f, cy);
        waveIcon.lineTo(cx - 6.0f, cy);
        waveIcon.lineTo(cx - 3.5f, cy - 8.5f);
        waveIcon.lineTo(cx - 0.5f, cy + 8.5f);
        waveIcon.lineTo(cx + 2.5f, cy - 5.5f);
        waveIcon.lineTo(cx + 5.0f, cy + 5.5f);
        waveIcon.lineTo(cx + 7.5f, cy);
        waveIcon.lineTo(cx + 11.0f, cy);
        g.setColour(currentSheet == 0 ? FLColors::greenNeon() : juce::Colours::white);
        g.strokePath(waveIcon, juce::PathStrokeType(2.0f));
    }

    // Tab 1: Envelope / Instrument icon
    {
        g.setColour(currentSheet == 1 ? FLColors::recessedBg() : juce::Colours::transparentBlack);
        g.fillRect(tabRectEnv);

        juce::Path envIcon;
        float x0 = (float)tabRectEnv.getX() + 12.0f;
        float y0 = (float)tabRectEnv.getBottom() - 10.0f;
        envIcon.startNewSubPath(x0, y0);
        envIcon.lineTo(x0 + 5.0f, y0 - 15.0f);
        envIcon.lineTo(x0 + 11.0f, y0 - 4.0f);
        envIcon.lineTo(x0 + 18.0f, y0);
        g.setColour(currentSheet == 1 ? FLColors::greenNeon() : juce::Colours::white);
        g.strokePath(envIcon, juce::PathStrokeType(1.6f));
        g.drawEllipse(x0 - 2.0f, y0 - 2.0f, 4.0f, 4.0f, 1.4f);
        g.drawEllipse(x0 + 3.0f, y0 - 17.0f, 4.0f, 4.0f, 1.4f);
        g.drawEllipse(x0 + 16.0f, y0 - 2.0f, 4.0f, 4.0f, 1.4f);
    }

    // Tab 2: Wrench (Misc functions) icon
    {
        g.setColour(currentSheet == 2 ? FLColors::recessedBg() : juce::Colours::transparentBlack);
        g.fillRect(tabRectFunc);

        float cx = (float)tabRectFunc.getCentreX();
        float cy = (float)tabRectFunc.getCentreY();
        g.setColour(currentSheet == 2 ? FLColors::greenNeon() : juce::Colours::white);
        g.drawLine(cx - 7.0f, cy + 7.0f, cx + 3.0f, cy - 3.0f, 2.5f);
        g.drawEllipse(cx + 0.5f, cy - 8.5f, 8.0f, 8.0f, 1.8f);
    }

    // Master Header Labels (ON, PAN, VOL, PITCH, RANGE, TRACK)
    g.setColour(FLColors::textBright());
    g.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
    g.drawText("ON",    masterOnBtn.getX() - 6, 30, 24, 11, juce::Justification::centred);
    g.drawText("PAN",   masterPanKnob.getX() - 4, 30, masterPanKnob.getWidth() + 8, 11, juce::Justification::centred);
    g.drawText("VOL",   masterVolKnob.getX() - 4, 30, masterVolKnob.getWidth() + 8, 11, juce::Justification::centred);
    g.drawText("PITCH", masterPitchKnob.getX() - 6, 30, masterPitchKnob.getWidth() + 12, 11, juce::Justification::centred);
    g.drawText("RANGE", pitchRangeBox.getX() - 6, 30, pitchRangeBox.getWidth() + 12, 11, juce::Justification::centred);
    g.drawText("TRACK", trackBox.getX() - 4, 30, trackBox.getWidth() + 8, 11, juce::Justification::centred);

    if (currentSheet == 0)
    {
        // ==================== SHEET 0: SAMPLE SETTINGS ====================
        // File Panel (x=6, y=48, w=252, h=42)
        drawFLPanel(g, { 6, 48, 252, 42 }, "");
        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(12.5f)));
        g.drawText("File", 16, 58, 28, 20, juce::Justification::centredLeft, false);

        g.setColour(FLColors::textDim());
        g.drawText(audioProcessor.getLoadedFileName(), fileNameBoxRect, juce::Justification::centredLeft, true);

        auto drawIconBtn = [&](juce::Rectangle<int> r, int iconType)
        {
            g.setColour(FLColors::recessedBg());
            g.fillRoundedRectangle(r.toFloat(), 2.0f);
            g.setColour(juce::Colour(0xff1b1e24));
            g.drawRoundedRectangle(r.toFloat(), 2.0f, 1.0f);
            g.setColour(FLColors::textBright());

            float cx = (float)r.getCentreX();
            float cy = (float)r.getCentreY();
            if (iconType == 0) // Open Folder
            {
                g.drawRect((int)cx - 6, (int)cy - 3, 12, 8, 1);
                g.drawLine(cx - 6.0f, cy - 5.0f, cx - 1.0f, cy - 5.0f, 1.5f);
            }
            else if (iconType == 1) // Magnifying Glass (Lupa) - Step / Locate Sample
            {
                g.drawEllipse(cx - 5.0f, cy - 5.0f, 7.5f, 7.5f, 1.4f);
                g.drawLine(cx + 1.2f, cy + 1.2f, cx + 5.0f, cy + 5.0f, 1.8f);
            }
            else // X Remove
            {
                g.drawLine(cx - 4.0f, cy - 4.0f, cx + 4.0f, cy + 4.0f, 1.5f);
                g.drawLine(cx - 4.0f, cy + 4.0f, cx + 4.0f, cy - 4.0f, 1.5f);
            }
        };
        drawIconBtn(openFileBtnRect, 0);
        drawIconBtn(removeFileBtnRect, 2);
        drawIconBtn(locateFileBtnRect, 1);

        // PAPIELGUAPO signature to the right of the Lupa button in the File bar ONLY in PRIVATE skin!
        if (FLColors::isPrivateSkin)
        {
            FLColors::drawPapielguapoSignature(g, (float)locateFileBtnRect.getRight() + 4.0f, (float)locateFileBtnRect.getY() + 5.0f, 14.0f,
                juce::Colour(0xffec4899));
        }

        // Content Panel (x=6, y=94, w=252, h=106)
        drawFLPanel(g, { 6, 94, 252, 106 }, "Content");
        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText("Declicking mode", 16, 170, 96, 20, juce::Justification::centredLeft, false);

        // Playback Panel (x=6, y=204, w=252, h=86)
        drawFLPanel(g, { 6, 204, 252, 86 }, "Playback");
        drawSmallCapLabel(g, startOffsetKnob, "START OFFSET");
        drawSmallCapLabel(g, loopStartKnob,   "START");
        drawSmallCapLabel(g, loopEndKnob,     "END");

        // Loop cycle icon between START and END
        {
            float lx = (float)(loopStartKnob.getRight() + loopEndKnob.getX()) * 0.5f;
            float ly = (float)loopStartKnob.getBottom() + 4.0f;
            g.setColour(FLColors::textBright());
            g.drawEllipse(lx - 3.5f, ly - 3.5f, 7.0f, 7.0f, 1.3f);
        }

        // Time Stretching Panel (x=264, y=48, w=256, h=86)
        drawFLPanel(g, { 264, 48, 256, 86 }, "Time stretching");
        drawSmallCapLabel(g, stretchPitchKnob, "PITCH");
        drawSmallCapLabel(g, stretchMulKnob,   "MUL");
        drawSmallCapLabel(g, stretchTimeKnob,  "TIME");
        drawSmallCapLabel(g, formantShiftKnob, "FMT");
        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(11.5f)));
        g.drawText("Mode", 422, 64, 38, 16, juce::Justification::centredLeft, false);

        // Precomputed Effects Panel (x=264, y=138, w=256, h=152)
        drawFLPanel(g, { 264, 138, 256, 152 }, "Precomputed effects", 56);

        // Wrench & Bulb sub-tab icons on the left of "Precomputed effects" title
        g.setColour(!legacyPrecomputedTab ? juce::Colours::white : FLColors::textDim());
        g.drawLine((float)fxSubTabWrench.getX() + 4.0f, (float)fxSubTabWrench.getBottom() - 4.0f,
                   (float)fxSubTabWrench.getRight() - 4.0f, (float)fxSubTabWrench.getY() + 4.0f, 2.0f);

        g.setColour(legacyPrecomputedTab ? FLColors::orangeRing() : FLColors::textDim());
        g.drawEllipse((float)fxSubTabBulb.getCentreX() - 4.0f, (float)fxSubTabBulb.getY() + 2.0f, 8.0f, 9.0f, 1.5f);
        g.drawLine((float)fxSubTabBulb.getCentreX() - 2.0f, (float)fxSubTabBulb.getBottom() - 3.0f,
                   (float)fxSubTabBulb.getCentreX() + 2.0f, (float)fxSubTabBulb.getBottom() - 3.0f, 1.5f);

        if (!legacyPrecomputedTab)
        {
            drawSmallCapLabel(g, smpStartKnob,  "SMP START");
            drawSmallCapLabel(g, lengthKnob,    "LENGTH");
            drawSmallCapLabel(g, fadeInKnob,    "IN");
            drawSmallCapLabel(g, fadeOutKnob,   "OUT");
            drawSmallCapLabel(g, crossfadeKnob, "CROSSFADE");
            drawSmallCapLabel(g, trimKnob,      "TRIM");
        }
        else
        {
            // Exact Legacy Precomputed Effects layout from Screenshot 1!
            drawSmallCapLabel(g, boostKnob,       "BOOST");
            drawSmallCapLabel(g, eqKnob,          "EQ");
            drawSmallCapLabel(g, rmMixKnob,       "RM MIX");
            drawSmallCapLabel(g, rmFreqKnob,      "RM FREQ");
            drawSmallCapLabel(g, cutKnob,         "CUT");
            drawSmallCapLabel(g, resKnob,         "RES");
            drawSmallCapLabel(g, reverbKnob,      "REVERB");
            drawSmallCapLabel(g, stereoDelayKnob, "STEREO DELAY");
            drawSmallCapLabel(g, pogoKnob,        "POGO");

            // Vertical A / B toggle switch next to REVERB knob
            const bool revB = audioProcessor.apvts.getRawParameterValue("reverbB")->load() > 0.5f;
            g.setColour(FLColors::recessedBg());
            g.fillRoundedRectangle(reverbSwitchRect.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff181b20));
            g.drawRoundedRectangle(reverbSwitchRect.toFloat(), 6.0f, 1.0f);
            float knobY = revB ? (float)reverbSwitchRect.getBottom() - 11.0f : (float)reverbSwitchRect.getY() + 1.0f;
            g.setColour(juce::Colour(0xff9aa3ad));
            g.fillEllipse((float)reverbSwitchRect.getX() + 1.0f, knobY, 10.0f, 10.0f);

            g.setColour(FLColors::textBright());
            g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
            g.drawText("A", reverbSwitchRect.getRight() + 3, reverbSwitchRect.getY() - 1, 12, 12, juce::Justification::centredLeft);
            g.drawText("B", reverbSwitchRect.getRight() + 3, reverbSwitchRect.getBottom() - 11, 12, 12, juce::Justification::centredLeft);
        }
    }
    else if (currentSheet == 1)
    {
        // ==================== SHEET 1: ENVELOPE / INSTRUMENT SETTINGS ====================
        // Top Sub-tab Bar: Panning | Volume | Mod X | Mod Y | Pitch
        const char* targetNames[5] = { "Panning", "Volume", "Mod X", "Mod Y", "Pitch" };
        for (int i = 0; i < 5; ++i)
        {
            if (currentInsTarget == i)
            {
                g.setColour(juce::Colour(0xff23272e));
                g.fillRoundedRectangle(insTargetRects[i].toFloat(), 2.0f);
                g.setColour(juce::Colour(kInsTargetColors[i]));
            }
            else
            {
                g.setColour(FLColors::textBright());
            }
            g.setFont(juce::Font(juce::FontOptions(13.0f)));
            g.drawText(targetNames[i], insTargetRects[i], juce::Justification::centred, false);
        }

        const juce::Colour activeCol(kInsTargetColors[currentInsTarget]);

        // Left Envelope Panel (hidden when Panning target is selected, matching Screenshot 3!)
        if (currentInsTarget != 0)
        {
            drawFLPanel(g, { 6, 82, 268, 216 }, "");

            // Dark recessed envelope graph box
            juce::Rectangle<int> envGraph(14, 106, 252, 86);
            g.setColour(FLColors::recessedBg());
            g.fillRect(envGraph);

            // Draw AHDSR curve with tension & circular nodes in target color
            float d   = (float)envDelayKnob.getValue() * 35.0f;
            float a   = (float)envAttackKnob.getValue() * 55.0f;
            float h   = (float)envHoldKnob.getValue() * 40.0f;
            float dec = (float)envDecayKnob.getValue() * 55.0f;
            float sus = (float)envSustainKnob.getValue();
            float rel = (float)envReleaseKnob.getValue() * 50.0f;

            float x0 = (float)envGraph.getX() + 4.0f;
            float yBot = (float)envGraph.getBottom() - 4.0f;
            float yTop = (float)envGraph.getY() + 4.0f;

            // When Envelope is disabled (Mod X/Y/Pitch screenshots), show flat line with nodes at bottom
            const bool isEnvActive = volEnvOnBtn.getToggleState() || (currentInsTarget == 1);
            float yPeak = isEnvActive ? yTop : yBot;
            float ySus  = isEnvActive ? (yBot - sus * (yBot - yTop)) : yBot;

            float x1 = x0 + d;
            float x2 = x1 + juce::jmax(18.0f, a);
            float x3 = x2 + juce::jmax(18.0f, h);
            float x4 = x3 + juce::jmax(18.0f, dec);
            float x5 = x4 + juce::jmax(18.0f, rel);

            const float attTens = (float)envAttTensionKnob.getValue();
            const float decTens = (float)envDecTensionKnob.getValue();
            const float relTens = (float)envRelTensionKnob.getValue();

            juce::Path p;
            p.startNewSubPath(x0, yBot);
            p.lineTo(x1, yBot);
            p.quadraticTo((x1 + x2) * 0.5f, (yBot + yPeak) * 0.5f - attTens * 22.0f, x2, yPeak);
            p.lineTo(x3, yPeak);
            p.quadraticTo((x3 + x4) * 0.5f, (yPeak + ySus) * 0.5f - decTens * 22.0f, x4, ySus);
            p.quadraticTo((x4 + x5) * 0.5f, (ySus + yBot) * 0.5f - relTens * 22.0f, x5, yBot);

            g.setColour(activeCol);
            g.strokePath(p, juce::PathStrokeType(1.8f));

            // Live Envelope Playhead Dot when a voice is active (using real voice age in seconds!)
            const float voiceAgeSec = audioProcessor.getActiveVoiceAgeSec();
            if (isEnvActive && voiceAgeSec >= 0.0f)
            {
                const float totEnvSec = juce::jmax(0.08f, (float)(envDelayKnob.getValue() * 2.0 + envAttackKnob.getValue() * 3.0 + envHoldKnob.getValue() * 2.0 + envDecayKnob.getValue() * 4.0 + 0.35));
                const float envNorm = juce::jlimit(0.0f, 1.0f, voiceAgeSec / totEnvSec);
                const float cursorX = juce::jlimit((float)envGraph.getX(), (float)envGraph.getRight(),
                                                   x0 + envNorm * (x5 - x0));
                g.setColour(activeCol.withAlpha(0.65f));
                g.drawVerticalLine((int)cursorX, (float)envGraph.getY(), (float)envGraph.getBottom());
            }

            auto drawNode = [&](float nx, float ny)
            {
                g.setColour(FLColors::recessedBg());
                g.fillEllipse(nx - 3.0f, ny - 3.0f, 6.0f, 6.0f);
                g.setColour(activeCol);
                g.drawEllipse(nx - 3.0f, ny - 3.0f, 6.0f, 6.0f, 1.5f);
            };
            drawNode(x1, yBot);
            drawNode(x2, yPeak);
            drawNode(x3, yPeak);
            drawNode((x3 + x4) * 0.5f, (yPeak + ySus) * 0.5f - decTens * 11.0f);
            drawNode(x5, yBot);

            drawSmallCapLabel(g, envDelayKnob,   "DELAY");
            drawSmallCapLabel(g, envAttackKnob,  "ATT");
            drawSmallCapLabel(g, envHoldKnob,    "HOLD");
            drawSmallCapLabel(g, envDecayKnob,   "DEC");
            drawSmallCapLabel(g, envSustainKnob, "SUS");
            drawSmallCapLabel(g, envReleaseKnob, "REL");
            if (envAmountKnob.isVisible())
                drawSmallCapLabel(g, envAmountKnob, "AMT");

            g.setColour(FLColors::textDim());
            g.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
            g.drawText("TENSION", envAttTensionKnob.getRight() + 2, envAttTensionKnob.getY() + 4, 46, 12, juce::Justification::centred);
            g.drawText("TENSION", envDecTensionKnob.getRight() + 2, envDecTensionKnob.getY() + 4, 46, 12, juce::Justification::centred);
        }

        // Middle LFO Panel (x=280, y=82, w=138, h=216)
        drawFLPanel(g, { 280, 82, 138, 216 }, "LFO", 66);

        // 3 LFO Shape Icons (Sine, Triangle, Pulse) on the left of "LFO" title
        for (int s = 0; s < 3; ++s)
        {
            auto r = lfoShapeRects[s].toFloat();
            g.setColour(currentLfoShape == s ? activeCol : FLColors::textBright());
            juce::Path sp;
            if (s == 0) // Sine
            {
                sp.startNewSubPath(r.getX() + 2, r.getCentreY());
                sp.cubicTo(r.getX() + 5, r.getY() + 1, r.getX() + 7, r.getY() + 1, r.getCentreX(), r.getCentreY());
                sp.cubicTo(r.getRight() - 5, r.getBottom() - 1, r.getRight() - 3, r.getBottom() - 1, r.getRight() - 2, r.getCentreY());
            }
            else if (s == 1) // Triangle
            {
                sp.startNewSubPath(r.getX() + 2, r.getCentreY() + 3);
                sp.lineTo(r.getX() + 6, r.getY() + 3);
                sp.lineTo(r.getRight() - 5, r.getBottom() - 3);
                sp.lineTo(r.getRight() - 2, r.getCentreY());
            }
            else // Pulse
            {
                sp.startNewSubPath(r.getX() + 2, r.getBottom() - 3);
                sp.lineTo(r.getX() + 2, r.getY() + 3);
                sp.lineTo(r.getCentreX(), r.getY() + 3);
                sp.lineTo(r.getCentreX(), r.getBottom() - 3);
                sp.lineTo(r.getRight() - 2, r.getBottom() - 3);
                sp.lineTo(r.getRight() - 2, r.getY() + 3);
            }
            g.strokePath(sp, juce::PathStrokeType(1.4f));
        }

        // LFO Preview Graph Box
        juce::Rectangle<int> lfoGraph(288, 106, 122, 86);
        g.setColour(FLColors::recessedBg());
        g.fillRect(lfoGraph);

        float lfoDel = (float)lfoDelayKnob.getValue() * 0.35f;
        float lfoAtt = juce::jmax(0.02f, (float)lfoAttKnob.getValue() * 0.45f);
        float lfoAmt = (float)lfoAmtKnob.getValue();
        float lfoSpd = 1.0f + (float)lfoSpeedKnob.getValue() * 5.0f;
        juce::Path lfoPath;
        for (int x = 0; x < lfoGraph.getWidth(); ++x)
        {
            float t = (float)x / (float)lfoGraph.getWidth();
            float env = 0.0f;
            if (t >= lfoDel)
                env = juce::jmin(1.0f, (t - lfoDel) / lfoAtt);
            float ph = std::fmod(juce::jmax(0.0f, t - lfoDel) * lfoSpd, 1.0f);
            float rawWave = 0.0f;
            if (currentLfoShape == 0) // Sine
                rawWave = std::sin(ph * juce::MathConstants<float>::twoPi);
            else if (currentLfoShape == 1) // Triangle
                rawWave = (ph < 0.25f) ? (ph * 4.0f) : ((ph < 0.75f) ? (2.0f - ph * 4.0f) : (ph * 4.0f - 4.0f));
            else // Pulse
                rawWave = (ph < 0.5f) ? 1.0f : -1.0f;

            float v = rawWave * lfoAmt * env;
            float py = (float)lfoGraph.getCentreY() - v * (lfoGraph.getHeight() * 0.40f);
            if (x == 0) lfoPath.startNewSubPath((float)(lfoGraph.getX() + x), py);
            else        lfoPath.lineTo((float)(lfoGraph.getX() + x), py);
        }
        g.setColour(activeCol);
        g.strokePath(lfoPath, juce::PathStrokeType(1.6f));

        if (audioProcessor.getActiveVoiceAgeSec() >= 0.0f)
        {
            const float lfoPhase = std::fmod(audioProcessor.getActiveVoiceAgeSec() * 0.5f, 1.0f);
            const int lx = lfoGraph.getX() + (int)(lfoPhase * (float)lfoGraph.getWidth());
            g.setColour(activeCol.withAlpha(0.65f));
            g.drawVerticalLine(lx, (float)lfoGraph.getY(), (float)lfoGraph.getBottom());
        }

        drawSmallCapLabel(g, lfoDelayKnob, "DELAY");
        drawSmallCapLabel(g, lfoAttKnob,   "ATT");
        drawSmallCapLabel(g, lfoAmtKnob,   "AMT");
        drawSmallCapLabel(g, lfoSpeedKnob, "SPEED");

        // Right Filter Panel (x=424, y=82, w=96, h=216)
        drawFLPanel(g, { 424, 82, 96, 216 }, "Filter");
        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText("MOD X", 460, 96, 52, 14, juce::Justification::centred);
        g.drawText("MOD Y", 428, 232, 52, 14, juce::Justification::centred);

        drawRootNoteAndKeyboardPanel(g);
    }
    else if (currentSheet == 2)
    {
        // ==================== SHEET 2: MISCELLANEOUS FUNCTIONS (Screenshot 6) ====================
        // Row 1: Levels adjustment | Polyphony | Time | Vol Key Tracking
        drawFLPanel(g, { 6, 48, 168, 120 }, "Levels adjustment");
        drawSmallCapLabel(g, lvlPanKnob,  "PAN");
        drawSmallCapLabel(g, lvlVolKnob,  "VOL");
        drawSmallCapLabel(g, lvlModXKnob, "MOD X");
        drawSmallCapLabel(g, lvlModYKnob, "MOD Y");

        drawFLPanel(g, { 180, 48, 114, 120 }, "Polyphony");
        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText("Max", 188, 84, 30, 18, juce::Justification::centredLeft);
        drawSmallCapLabel(g, portaTimeKnob, "SLIDE");

        drawFLPanel(g, { 300, 48, 102, 120 }, "Time");
        drawSmallCapLabel(g, gateTimeKnob, "GATE");
        drawSmallCapLabel(g, shiftKnob,    "SHIFT");
        drawSmallCapLabel(g, swingKnob,    "SWING");

        drawFLPanel(g, { 408, 48, 112, 120 }, "");
        // Vol | Key | Tracking header tabs
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.setColour(!trackingKeyMode ? FLColors::orangeRing() : FLColors::textBright());
        g.drawText("Vol", trackVolTabRect, juce::Justification::centred);
        g.setColour(trackingKeyMode ? FLColors::orangeRing() : FLColors::textBright());
        g.drawText("Key", trackKeyTabRect, juce::Justification::centred);
        g.setColour(FLColors::textBright());
        g.drawText("Tracking", 462, 50, 56, 16, juce::Justification::centredLeft);

        drawSmallCapLabel(g, trackPanKnob,  "PAN");
        drawSmallCapLabel(g, trackModXKnob, "MOD X");
        drawSmallCapLabel(g, trackModYKnob, "MOD Y");
        g.setColour(FLColors::textDim());
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText("Mid", 414, 138, 28, 16, juce::Justification::centredLeft);

        // Row 2: Group | Arpeggiator | Echo delay / fat mode
        drawFLPanel(g, { 6, 174, 66, 124 }, "Group");
        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText("Cut", 10, 206, 26, 18, juce::Justification::centredLeft);
        g.drawText("By",  10, 236, 26, 18, juce::Justification::centredLeft);

        drawFLPanel(g, { 78, 174, 216, 124 }, "Arpeggiator", 112);

        // Draw 6 Arpeggiator Direction Icons (X, Up, Down, Up-Down, Sticky, ?) in title bar!
        const int curArpDir = (int)audioProcessor.apvts.getRawParameterValue("arpDir")->load();
        const char* arpGlyphs[6] = { "X", "^", "v", "<>", "=", "?" };
        for (int i = 0; i < 6; ++i)
        {
            g.setColour(curArpDir == i ? FLColors::orangeRing() : FLColors::textBright());
            g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
            g.drawText(arpGlyphs[i], arpDirIconRects[i], juce::Justification::centred);
        }

        drawSmallCapLabel(g, arpTimeKnob, "TIME");
        drawSmallCapLabel(g, arpGateKnob, "GATE");
        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText("Range",  152, 212, 38, 18, juce::Justification::centredLeft);
        g.drawText("Repeat", 224, 212, 42, 18, juce::Justification::centredLeft);
        g.drawText("Chord",  152, 266, 38, 18, juce::Justification::centredLeft);

        drawFLPanel(g, { 300, 174, 220, 124 }, "Echo delay / fat mode");
        drawSmallCapLabel(g, echoFeedKnob,  "FEED");
        drawSmallCapLabel(g, echoPanKnob,   "PAN");
        drawSmallCapLabel(g, echoModXKnob,  "MOD X");
        drawSmallCapLabel(g, echoModYKnob,  "MOD Y");
        drawSmallCapLabel(g, echoPitchKnob, "PITCH");
        drawSmallCapLabel(g, echoTimeKnob,  "TIME");
        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText("Echoes", 312, 264, 44, 18, juce::Justification::centredLeft);

        drawRootNoteAndKeyboardPanel(g);
    }
}

void FLSamplerAudioProcessorEditor::resized()
{
    const float scaleX = (float)getWidth()  / (float)kDesignW;
    const float scaleY = (float)getHeight() / (float)kDesignH;
    contentCanvas.setBounds(0, 0, kDesignW, kDesignH);
    contentCanvas.setTransform(juce::AffineTransform::scale(scaleX, scaleY));

    // Top Header Bar (y: 0..42)
    tabRectWave       = { 4,   2, 42, 38 };
    tabRectEnv        = { 46,  2, 42, 38 };
    tabRectFunc       = { 88,  2, 42, 38 };
    skinToggleBtnRect = { 152, 9, 56, 24 }; // Placed immediately to the right of the Lupa icon (x=135..149)!

    masterOnBtn.setBounds(320, 10, 16, 16);
    masterPanKnob.setBounds(346, 2, 28, 28);
    masterVolKnob.setBounds(376, 2, 28, 28);
    masterPitchKnob.setBounds(412, 2, 28, 28);
    pitchRangeBox.setBounds(438, 5, 28, 22);
    trackBox.setBounds(484, 5, 36, 22);

    // ==================== SHEET 0: SAMPLE SETTINGS ====================
    fileNameBoxRect   = { 44,  58, 102, 22 };
    openFileBtnRect   = { 148, 56, 20, 24 };
    removeFileBtnRect = { 170, 56, 18, 24 };
    locateFileBtnRect = { 190, 56, 20, 24 }; // Magnifying glass (Lupa) with space to its right for PAPIELGUAPO signature!

    // Content Panel (x=6, y=94, w=252, h=106)
    keepOnDiskBtn.setBounds(14, 118, 112, 18);
    loadRegionsBtn.setBounds(136, 118, 112, 18);
    resampleBtn.setBounds(14, 142, 112, 18);
    loadSlicesBtn.setBounds(136, 142, 118, 18);
    declickCombo.setBounds(112, 168, 138, 24);

    // Playback Panel (x=6, y=204, w=252, h=86)
    startOffsetKnob.setBounds(24, 232, 36, 36);
    loopStartKnob.setBounds(76, 232, 36, 36);
    loopEndKnob.setBounds(116, 232, 36, 36);
    useLoopPointsBtn.setBounds(162, 234, 92, 18);
    pingPongLoopBtn.setBounds(162, 258, 92, 18);

    // Time Stretching Panel (x=264, y=48, w=256, h=86)
    stretchPitchKnob.setBounds(272, 76, 34, 34);
    stretchMulKnob.setBounds(308, 76, 34, 34);
    stretchTimeKnob.setBounds(344, 76, 34, 34);
    formantShiftKnob.setBounds(380, 76, 34, 34);
    stretchModeCombo.setBounds(420, 82, 92, 24);

    // Precomputed Effects Panel (x=264, y=138, w=256, h=152)
    fxSubTabWrench = { 270, 141, 20, 18 };
    fxSubTabBulb   = { 292, 141, 20, 18 };

    removeDcBtn.setBounds(272, 166, 118, 18);
    reversePolBtn.setBounds(396, 166, 118, 18);
    normalizeBtn.setBounds(272, 188, 118, 18);
    fadeStereoBtn.setBounds(396, 188, 118, 18);
    reverseBtn.setBounds(272, 210, 118, 18);
    swapStereoBtn.setBounds(396, 210, 118, 18);

    smpStartKnob.setBounds(274,  236, 34, 34);
    lengthKnob.setBounds(314,    236, 34, 34);
    fadeInKnob.setBounds(354,    236, 34, 34);
    fadeOutKnob.setBounds(394,   236, 34, 34);
    crossfadeKnob.setBounds(434, 236, 34, 34);
    trimKnob.setBounds(474,      236, 34, 34);

    // Legacy Precomputed Knobs (Bulb Sub-tab matching Screenshot 1)
    boostKnob.setBounds(276,       172, 34, 34);
    eqKnob.setBounds(316,          172, 34, 34);
    rmMixKnob.setBounds(362,       172, 34, 34);
    rmFreqKnob.setBounds(398,      172, 34, 34);
    cutKnob.setBounds(444,         172, 34, 34);
    resKnob.setBounds(480,         172, 34, 34);
    clipBtn.setBounds(274,         218, 60, 16);

    reverbSwitchRect = { 280, 242, 12, 24 };
    reverbKnob.setBounds(312,      236, 34, 34);
    stereoDelayKnob.setBounds(362, 236, 34, 34);
    pogoKnob.setBounds(480,        236, 34, 34);

    // Bottom Waveform Scope (x=6, y=296, w=514, h=154)
    wavScope.setBounds(6, 296, 514, 154);

    // ==================== SHEET 1: ENVELOPE / INSTRUMENT SETTINGS ====================
    insTargetRects[0] = { 10,  52, 52, 22 };
    insTargetRects[1] = { 62,  52, 52, 22 };
    insTargetRects[2] = { 114, 52, 46, 22 };
    insTargetRects[3] = { 160, 52, 46, 22 };
    insTargetRects[4] = { 206, 52, 42, 22 };

    volEnvOnBtn.setBounds(12, 84, 90, 18);
    envDelayKnob.setBounds(16,   198, 32, 32);
    envAttackKnob.setBounds(50,  198, 32, 32);
    envHoldKnob.setBounds(84,    198, 32, 32);
    envDecayKnob.setBounds(118,  198, 32, 32);
    envSustainKnob.setBounds(152, 198, 32, 32);
    envReleaseKnob.setBounds(186, 198, 32, 32);
    envAmountKnob.setBounds(234,  198, 32, 32);

    envAttTensionKnob.setBounds(52,  248, 22, 22);
    envDecTensionKnob.setBounds(118, 248, 22, 22);
    envRelTensionKnob.setBounds(184, 248, 22, 22);
    envTempoBtn.setBounds(216,       252, 56, 18);

    lfoShapeRects[0] = { 286, 84, 16, 16 };
    lfoShapeRects[1] = { 306, 84, 16, 16 };
    lfoShapeRects[2] = { 326, 84, 16, 16 };

    lfoDelayKnob.setBounds(290, 198, 30, 30);
    lfoAttKnob.setBounds(322,   198, 30, 30);
    lfoAmtKnob.setBounds(354,   198, 30, 30);
    lfoSpeedKnob.setBounds(386, 198, 30, 30);
    lfoTempoBtn.setBounds(308,  252, 54, 18);
    lfoGlobalBtn.setBounds(364, 252, 54, 18);

    filterModXKnob.setBounds(438, 110, 56, 56);
    filterModYKnob.setBounds(458, 178, 56, 56);
    filterTypeCombo.setBounds(430, 252, 84, 24);

    // Shared Bottom Root Note & Keyboard Panel (x=6, y=304, w=514, h=146)
    resetRootBtnRect   = { 14, 328, 44, 22 };
    enableMainPitchBtn.setBounds(68, 330, 114, 18);
    addToKeyBtn.setBounds(184, 330, 72, 18);
    fineTuneSlider.setBounds(314, 328, 96, 22);
    bottomKeybRect     = { 10, 356, 506, 80 };

    // ==================== SHEET 2: MISCELLANEOUS FUNCTIONS ====================
    lvlPanKnob.setBounds(14,   76, 36, 36);
    lvlVolKnob.setBounds(54,   76, 36, 36);
    lvlModXKnob.setBounds(94,  76, 36, 36);
    lvlModYKnob.setBounds(134, 76, 36, 36);

    maxPolyBox.setBounds(216,    80, 30, 24);
    portaTimeKnob.setBounds(254, 76, 34, 34);
    portaBtn.setBounds(188,      136, 52, 18);
    monoBtn.setBounds(242,       136, 50, 18);

    gateTimeKnob.setBounds(308, 76, 30, 30);
    shiftKnob.setBounds(340,    76, 30, 30);
    swingKnob.setBounds(372,    76, 30, 30);
    fullPortaBtn.setBounds(308, 136, 84, 18);

    trackVolTabRect = { 414, 50, 24, 16 };
    trackKeyTabRect = { 438, 50, 24, 16 };
    trackPanKnob.setBounds(418,  76, 30, 30);
    trackModXKnob.setBounds(454, 76, 30, 30);
    trackModYKnob.setBounds(486, 76, 30, 30);
    trackMidSlider.setBounds(444, 136, 68, 20);

    cutGroupBox.setBounds(38,    204, 28, 22);
    cutByBox.setBounds(38,       234, 28, 22);
    cutSelfPushBtn.setBounds(12, 264, 54, 24);

    for (int i = 0; i < 6; ++i)
        arpDirIconRects[i] = { 82 + i * 16, 176, 15, 16 };

    arpTimeKnob.setBounds(86,   204, 30, 30);
    arpGateKnob.setBounds(118,  204, 30, 30);
    arpRangeBox.setBounds(190,  210, 26, 22);
    arpRepeatBox.setBounds(264, 210, 26, 22);
    arpSlideBtn.setBounds(86,   266, 56, 18);
    arpChordCombo.setBounds(192, 264, 96, 22);

    echoFeedKnob.setBounds(308,  204, 30, 30);
    echoPanKnob.setBounds(340,   204, 30, 30);
    echoModXKnob.setBounds(378,  204, 30, 30);
    echoModYKnob.setBounds(410,  204, 30, 30);
    echoPitchKnob.setBounds(448, 204, 30, 30);
    echoTimeKnob.setBounds(484,  204, 30, 30);

    echoCountBox.setBounds(354,    260, 28, 24);
    echoPingPongBtn.setBounds(444, 252, 72, 18);
    echoFatModeBtn.setBounds(444,  274, 72, 18);
}

void FLSamplerAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    const float scaleX = (float)getWidth()  / (float)kDesignW;
    const float scaleY = (float)getHeight() / (float)kDesignH;
    const juce::Point<int> pt((int)std::round(e.position.x / juce::jmax(0.1f, scaleX)),
                              (int)std::round(e.position.y / juce::jmax(0.1f, scaleY)));

    // Header Sheet Tabs & 4-Way Skin Toggle (FL -> LIVE -> PRIVATE -> BLACK)
    if (tabRectWave.contains(pt)) { updateActiveSheet(0); setHintText("Sample settings"); return; }
    if (tabRectEnv.contains(pt))  { updateActiveSheet(1); setHintText("Envelope / instrument settings"); return; }
    if (tabRectFunc.contains(pt)) { updateActiveSheet(2); setHintText("Miscellaneous functions"); return; }
    if (skinToggleBtnRect.contains(pt))
    {
        const int nextSkin = (FLColors::skinMode + 1) % 4;
        FLColors::setSkinMode(nextSkin);
        flLookAndFeel.refreshThemeColours();
        setHintText(nextSkin == 0 ? "Skin: FL Studio 2026 Look"
                  : (nextSkin == 1 ? "Skin: Ableton Live 12 Look"
                  : (nextSkin == 2 ? "Skin: PRIVATE (VOCESFX + GrowthTune)"
                                   : "Skin: BLACK (Pure Pitch-Black Stealth)")));
        contentCanvas.repaint();
        repaint();
        return;
    }

    if (currentSheet == 0)
    {
        if (openFileBtnRect.contains(pt) || fileNameBoxRect.contains(pt))
        {
            if (e.mods.isPopupMenu() && !audioProcessor.recentSampleFiles.isEmpty())
            {
                juce::PopupMenu m;
                m.addSectionHeader("Recent Sample Files");
                for (int i = 0; i < audioProcessor.recentSampleFiles.size(); ++i)
                    m.addItem(100 + i, juce::File(audioProcessor.recentSampleFiles[i]).getFileName());
                m.showMenuAsync(juce::PopupMenu::Options(), [this](int res)
                {
                    if (res >= 100)
                    {
                        int idx = res - 100;
                        if (idx >= 0 && idx < audioProcessor.recentSampleFiles.size())
                        {
                            juce::File f(audioProcessor.recentSampleFiles[idx]);
                            if (f.existsAsFile())
                            {
                                audioProcessor.loadSampleFromFile(f);
                                wavScope.refreshIfNeeded();
                                repaint();
                            }
                        }
                    }
                });
                return;
            }
            openSampleFileChooser();
            return;
        }
        if (locateFileBtnRect.contains(pt))
        {
            audioProcessor.stepFolderSample(e.mods.isRightButtonDown() ? -1 : 1);
            wavScope.refreshIfNeeded();
            setHintText("Loaded: " + audioProcessor.getLoadedFileName());
            repaint();
            return;
        }
        if (removeFileBtnRect.contains(pt))
        {
            audioProcessor.clearSample();
            wavScope.refreshIfNeeded();
            setHintText("Sample unloaded");
            repaint();
            return;
        }
        if (fxSubTabWrench.contains(pt))
        {
            updatePrecomputedSubTab(false);
            setHintText("Precomputed effects (standard)");
            return;
        }
        if (fxSubTabBulb.contains(pt))
        {
            updatePrecomputedSubTab(true);
            setHintText("Precomputed effects (legacy)");
            return;
        }
        if (legacyPrecomputedTab && reverbSwitchRect.contains(pt))
        {
            if (auto* param = audioProcessor.apvts.getParameter("reverbB"))
                param->setValueNotifyingHost(param->getValue() > 0.5f ? 0.0f : 1.0f);
            repaint();
            return;
        }
    }
    else if (currentSheet == 1)
    {
        for (int i = 0; i < 5; ++i)
        {
            if (insTargetRects[i].contains(pt))
            {
                updateInsTarget(i);
                return;
            }
        }
        for (int s = 0; s < 3; ++s)
        {
            if (lfoShapeRects[s].contains(pt))
            {
                currentLfoShape = s;
                audioProcessor.lfoShapeIdx.store(s, std::memory_order_relaxed);
                audioProcessor.targetLfos[(size_t)currentInsTarget].shape = s;
                setHintText(s == 0 ? "LFO shape: Sine" : (s == 1 ? "LFO shape: Triangle" : "LFO shape: Pulse"));
                repaint();
                return;
            }
        }

        // Interactive AHDSR Graph Node Dragging
        juce::Rectangle<int> envGraph(14, 106, 252, 86);
        if (currentInsTarget != 0 && envGraph.contains(pt))
        {
            float d   = (float)envDelayKnob.getValue() * 35.0f;
            float a   = (float)envAttackKnob.getValue() * 55.0f;
            float h   = (float)envHoldKnob.getValue() * 40.0f;
            float dec = (float)envDecayKnob.getValue() * 55.0f;
            float rel = (float)envReleaseKnob.getValue() * 50.0f;

            float x0 = (float)envGraph.getX() + 4.0f;
            float x1 = x0 + d;
            float x2 = x1 + juce::jmax(18.0f, a);
            float x3 = x2 + juce::jmax(18.0f, h);
            float x4 = x3 + juce::jmax(18.0f, dec);
            float x5 = x4 + juce::jmax(18.0f, rel);

            float xs[5] = { x1, x2, x3, x4, x5 };
            float bestDist = 999.0f;
            int bestNode = -1;
            for (int n = 0; n < 5; ++n)
            {
                float dist = std::abs((float)pt.x - xs[n]);
                if (dist < bestDist) { bestDist = dist; bestNode = n; }
            }
            if (bestDist < 18.0f)
            {
                draggingEnvNodeIdx = bestNode;
                if (!volEnvOnBtn.getToggleState())
                    volEnvOnBtn.setToggleState(true, juce::sendNotificationSync);
                return;
            }
        }
    }
    else if (currentSheet == 2)
    {
        if (trackVolTabRect.contains(pt))
        {
            trackingKeyMode = false;
            audioProcessor.trackingKeyMode.store(false, std::memory_order_relaxed);
            repaint();
            return;
        }
        if (trackKeyTabRect.contains(pt))
        {
            trackingKeyMode = true;
            audioProcessor.trackingKeyMode.store(true, std::memory_order_relaxed);
            repaint();
            return;
        }
        for (int i = 0; i < 6; ++i)
        {
            if (arpDirIconRects[i].contains(pt))
            {
                if (auto* param = audioProcessor.apvts.getParameter("arpDir"))
                    param->setValueNotifyingHost(param->convertTo0to1((float)juce::jmin(4, i)));
                repaint();
                return;
            }
        }
    }

    // Shared Root Note, Key Region Bar & Piano Keyboard (Sheets 1 & 2)
    if (currentSheet == 1 || currentSheet == 2)
    {
        if (resetRootBtnRect.contains(pt))
        {
            if (auto* param = audioProcessor.apvts.getParameter("rootNote"))
                param->setValueNotifyingHost(param->convertTo0to1(60.0f)); // Reset to C5
            audioProcessor.keyRegionLow.store(0, std::memory_order_relaxed);
            audioProcessor.keyRegionHigh.store(127, std::memory_order_relaxed);
            setHintText("Root note reset to C5 & Key Region reset to Full (C0..G10)");
            repaint();
            return;
        }

        juce::Rectangle<int> scrollBar(bottomKeybRect.getX(), bottomKeybRect.getBottom(), bottomKeybRect.getWidth(), 12);
        if (scrollBar.contains(pt))
        {
            float relX = juce::jlimit(0.0f, 1.0f, (float)(pt.x - scrollBar.getX()) / (float)scrollBar.getWidth());
            int midi = 24 + (int)std::round(relX * 72.0f);
            int curLo = audioProcessor.keyRegionLow.load();
            int curHi = audioProcessor.keyRegionHigh.load();
            draggingKeyRegionEdge = (std::abs(midi - curLo) <= std::abs(midi - curHi)) ? 0 : 1;
            if (draggingKeyRegionEdge == 0)
                audioProcessor.keyRegionLow.store(juce::jmin(midi, curHi), std::memory_order_relaxed);
            else
                audioProcessor.keyRegionHigh.store(juce::jmax(midi, curLo), std::memory_order_relaxed);
            repaint();
            return;
        }

        if (bottomKeybRect.contains(pt))
        {
            const int numWhiteKeys = 43;
            const float relX = (float)(pt.x - bottomKeybRect.getX()) / (float)bottomKeybRect.getWidth();
            const float relY = (float)(pt.y - bottomKeybRect.getY()) / (float)bottomKeybRect.getHeight();

            auto isBlackKey = [](int m) {
                int pc = m % 12;
                return (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
            };

            int midi = 60;
            bool hitBlack = false;
            // First check black keys if clicking in top 62% of keyboard
            if (relY <= 0.62f)
            {
                const float whiteW = (float)bottomKeybRect.getWidth() / (float)numWhiteKeys;
                const float kbX = (float)bottomKeybRect.getX();
                int wIdx = 0;
                for (int m = 24; m <= 96; ++m)
                {
                    if (!isBlackKey(m))
                    {
                        ++wIdx;
                        continue;
                    }
                    float bx = kbX + ((float)wIdx - 0.35f) * whiteW;
                    float bw = whiteW * 0.70f;
                    if (pt.x >= (int)bx && pt.x <= (int)(bx + bw))
                    {
                        midi = m;
                        hitBlack = true;
                        break;
                    }
                }
            }

            if (!hitBlack)
            {
                int wTarget = juce::jlimit(0, numWhiteKeys - 1, (int)(relX * numWhiteKeys));
                int wCount = 0;
                for (int m = 24; m <= 96; ++m)
                {
                    if (!isBlackKey(m))
                    {
                        if (wCount == wTarget)
                        {
                            midi = m;
                            break;
                        }
                        ++wCount;
                    }
                }
            }

            static const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            const juce::String noteStr = juce::String(noteNames[midi % 12]) + juce::String(midi / 12);

            if (e.mods.isShiftDown())
            {
                if (e.mods.isRightButtonDown())
                    audioProcessor.keyRegionHigh.store(juce::jmax(midi, audioProcessor.keyRegionLow.load()), std::memory_order_relaxed);
                else
                    audioProcessor.keyRegionLow.store(juce::jmin(midi, audioProcessor.keyRegionHigh.load()), std::memory_order_relaxed);
                setHintText("Key Region: " + noteStr);
                repaint();
            }
            else if (e.mods.isRightButtonDown())
            {
                if (auto* param = audioProcessor.apvts.getParameter("rootNote"))
                    param->setValueNotifyingHost(param->convertTo0to1((float)midi));
                setHintText("Root note set to " + noteStr);
                repaint();
            }
            else
            {
                previewHeldMidiNote = midi;
                audioProcessor.triggerPreviewNote(midi, 0.9f);
                setHintText("Preview note: " + noteStr);
                repaint();
            }
        }
    }
}

void FLSamplerAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    const float scaleX = (float)getWidth()  / (float)kDesignW;
    const float scaleY = (float)getHeight() / (float)kDesignH;
    const juce::Point<int> pt((int)std::round(e.position.x / juce::jmax(0.1f, scaleX)),
                              (int)std::round(e.position.y / juce::jmax(0.1f, scaleY)));

    if (draggingEnvNodeIdx >= 0 && currentSheet == 1)
    {
        juce::Rectangle<int> envGraph(14, 106, 252, 86);
        float normX = juce::jlimit(0.0f, 1.0f, (float)(pt.x - envGraph.getX()) / (float)envGraph.getWidth());
        float normY = juce::jlimit(0.0f, 1.0f, 1.0f - (float)(pt.y - envGraph.getY()) / (float)envGraph.getHeight());

        if (draggingEnvNodeIdx == 0)      envDelayKnob.setValue(normX, juce::sendNotificationSync);
        else if (draggingEnvNodeIdx == 1) envAttackKnob.setValue(normX, juce::sendNotificationSync);
        else if (draggingEnvNodeIdx == 2) envHoldKnob.setValue(normX, juce::sendNotificationSync);
        else if (draggingEnvNodeIdx == 3)
        {
            envDecayKnob.setValue(normX, juce::sendNotificationSync);
            envSustainKnob.setValue(normY, juce::sendNotificationSync);
        }
        else if (draggingEnvNodeIdx == 4) envReleaseKnob.setValue(normX, juce::sendNotificationSync);
        repaint();
        return;
    }

    if (draggingKeyRegionEdge >= 0)
    {
        float relX = juce::jlimit(0.0f, 1.0f, (float)(pt.x - bottomKeybRect.getX()) / (float)bottomKeybRect.getWidth());
        int midi = 24 + (int)std::round(relX * 72.0f);
        if (draggingKeyRegionEdge == 0)
            audioProcessor.keyRegionLow.store(juce::jmin(midi, audioProcessor.keyRegionHigh.load()), std::memory_order_relaxed);
        else
            audioProcessor.keyRegionHigh.store(juce::jmax(midi, audioProcessor.keyRegionLow.load()), std::memory_order_relaxed);
        repaint();
    }
}

void FLSamplerAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    draggingEnvNodeIdx = -1;
    draggingKeyRegionEdge = -1;
    if (previewHeldMidiNote >= 0)
    {
        audioProcessor.stopPreviewNote(previewHeldMidiNote);
        previewHeldMidiNote = -1;
        repaint();
    }
}

bool FLSamplerAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    return wavScope.isInterestedInFileDrag(files);
}

void FLSamplerAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    isWindowDragHover = true;
    wavScope.fileDragEnter(files, x, y);
    setHintText("Drop audio file to load sample...");
    repaint();
}

void FLSamplerAudioProcessorEditor::fileDragExit(const juce::StringArray& files)
{
    isWindowDragHover = false;
    wavScope.fileDragExit(files);
    repaint();
}

void FLSamplerAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    isWindowDragHover = false;
    wavScope.filesDropped(files, x, y);
    setHintText("Loaded: " + audioProcessor.getLoadedFileName());
    repaint();
}
