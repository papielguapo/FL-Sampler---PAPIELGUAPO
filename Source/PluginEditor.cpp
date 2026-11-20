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
        audioProcessor.syncInstanceChokeRegistration();
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
    setupKnob(startOffsetKnob, "Sample start offset", 0.0, false, false, true);
    setupKnob(loopStartKnob,   "Loop start",          0.0, false, false, true);
    setupKnob(loopEndKnob,     "Loop end",            1.0, false, false, true);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "startOffset", startOffsetKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "loopStart", loopStartKnob));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "loopEnd", loopEndKnob));
    startOffsetKnob.setDefaultResetValue(0.0);
    loopStartKnob.setDefaultResetValue(0.0);
    loopEndKnob.setDefaultResetValue(1.0);
    loopStartKnob.onValueChange = [this]()
    {
        if (loopStartKnob.getValue() > 0.0005 && !useLoopPointsBtn.getToggleState())
        {
            if (auto* p = audioProcessor.apvts.getParameter("useLoopPoints"))
                p->setValueNotifyingHost(1.0f);
        }
    };
    loopEndKnob.onValueChange = [this]()
    {
        if (loopEndKnob.getValue() < 0.9995 && !useLoopPointsBtn.getToggleState())
        {
            if (auto* p = audioProcessor.apvts.getParameter("useLoopPoints"))
                p->setValueNotifyingHost(1.0f);
        }
    };

    useLoopPointsBtn.setButtonText("Use loop points");
    contentCanvas.addAndMakeVisible(useLoopPointsBtn);
    contentCanvas.addAndMakeVisible(pingPongLoopBtn);
    useLoopPointsBtn.setBounds(162, 234, 102, 18);
    pingPongLoopBtn.setBounds(162, 258, 102, 18);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "useLoopPoints", useLoopPointsBtn));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, "pingPongLoop", pingPongLoopBtn));

    // Time Stretching Panel
    setupKnob(stretchPitchKnob, "Time stretch pitch",      0.0, true);
    setupKnob(stretchMulKnob,   "Time stretch multiplier", 1.0);
    setupKnob(stretchTimeKnob,  "Time stretch time",       0.0);
    formantShiftKnob.setName("Formant shift (semitones)");
    formantShiftKnob.onHintChanged = [this](const juce::String& h) { setHintText(h); };
    formantShiftKnob.setSliderStyle(juce::Slider::LinearHorizontal);
    formantShiftKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    formantShiftKnob.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    formantShiftKnob.getProperties().set("flBipolar", true);
    formantShiftKnob.getProperties().set("flRedSlider", true);
    contentCanvas.addAndMakeVisible(formantShiftKnob);
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
    stretchModeCombo.onChange = [this]()
    {
        if (currentSheet == 0)
        {
            const bool isStretchPro = (stretchModeCombo.getSelectedItemIndex() == 4);
            formantShiftKnob.setVisible(isStretchPro);
            repaint();
        }
    };

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

    setupKnob(smpStartKnob,  "Sample start (precomputed)", 0.0, false, false, true);
    setupKnob(lengthKnob,    "Sample length (precomputed)", 1.0, false, false, true); // Orange ring on LENGTH matching screenshot 1
    setupKnob(fadeInKnob,    "Quick fade in",  0.0, false, false, true);
    setupKnob(fadeOutKnob,   "Quick fade out", 0.0, false, false, true);
    setupKnob(crossfadeKnob, "Loop crossfade", 0.0, false, false, true);
    setupKnob(trimKnob,      "Trim threshold", 0.0, false, false, true);
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

    setupKnob(envDelayKnob,   "Envelope delay",   0.0);
    setupKnob(envAttackKnob,  "Envelope attack",  0.0);
    setupKnob(envHoldKnob,    "Envelope hold",    0.28);
    setupKnob(envDecayKnob,   "Envelope decay",   0.0);
    setupKnob(envSustainKnob, "Envelope sustain", 0.0);
    setupKnob(envReleaseKnob, "Envelope release", 0.0);
    setupKnob(envAmountKnob,  "Envelope amount",  0.0, true);
    envDelayKnob.setRange(0.0, 1.0, 0.001);
    envAttackKnob.setRange(0.0, 1.0, 0.001);
    envHoldKnob.setRange(0.0, 1.0, 0.001);
    envDecayKnob.setRange(0.0, 1.0, 0.001);
    envSustainKnob.setRange(0.0, 1.0, 0.001);
    envReleaseKnob.setRange(0.0, 1.0, 0.001);
    envAmountKnob.setRange(-1.0, 1.0, 0.01);
    envAmountKnob.setValue(0.0);
    envAmountKnob.setDefaultResetValue(0.0);
    envAmountKnob.onValueChange = [this]()
    {
        const float v = (float)envAmountKnob.getValue();
        auto& te = audioProcessor.targetEnvs[(size_t)currentInsTarget];
        te.amount = v;
        if (currentInsTarget >= 2)
        {
            const bool shouldEnable = std::abs(v) > 0.005f;
            te.enabled = shouldEnable;
            volEnvOnBtn.setToggleState(shouldEnable, juce::dontSendNotification);
        }
        repaint();
    };

    setupKnob(envAttTensionKnob, "Attack tension",  0.0,   true);
    envAttTensionKnob.setRange(-1.0, 1.0, 0.01);
    envAttTensionKnob.setValue(0.0);
    envAttTensionKnob.setDefaultResetValue(0.0);
    envAttTensionKnob.onValueChange = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].attTension = (float)envAttTensionKnob.getValue(); repaint(); };
    setupKnob(envDecTensionKnob, "Decay tension",   0.0,   true);
    envDecTensionKnob.setRange(-1.0, 1.0, 0.01);
    envDecTensionKnob.setValue(0.0);
    envDecTensionKnob.setDefaultResetValue(0.0);
    envDecTensionKnob.onValueChange = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].decTension = (float)envDecTensionKnob.getValue(); repaint(); };
    setupKnob(envRelTensionKnob, "Release tension", -0.45, true);
    envRelTensionKnob.setRange(-1.0, 1.0, 0.01);
    envRelTensionKnob.setValue(-0.45);
    envRelTensionKnob.setDefaultResetValue(-0.45);
    envRelTensionKnob.onValueChange = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].relTension = (float)envRelTensionKnob.getValue(); repaint(); };

    contentCanvas.addAndMakeVisible(envTempoBtn);
    envTempoBtn.onClick = [this]() { audioProcessor.targetEnvs[(size_t)currentInsTarget].tempoSync = envTempoBtn.getToggleState(); };

    auto syncVolEnvParam = [this](const char* paramId, float normVal)
    {
        if (currentInsTarget == 1)
        {
            if (auto* p = audioProcessor.apvts.getParameter(paramId))
                p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normVal));
        }
    };

    envDelayKnob.onValueChange   = [this, syncVolEnvParam]() { float v = (float)envDelayKnob.getValue();   audioProcessor.targetEnvs[(size_t)currentInsTarget].delay   = v; syncVolEnvParam("envDelay",   v); repaint(); };
    envAttackKnob.onValueChange  = [this, syncVolEnvParam]() { float v = (float)envAttackKnob.getValue();  audioProcessor.targetEnvs[(size_t)currentInsTarget].attack  = v; syncVolEnvParam("envAttack",  v); repaint(); };
    envHoldKnob.onValueChange    = [this, syncVolEnvParam]() { float v = (float)envHoldKnob.getValue();    audioProcessor.targetEnvs[(size_t)currentInsTarget].hold    = v; syncVolEnvParam("envHold",    v); repaint(); };
    envDecayKnob.onValueChange   = [this, syncVolEnvParam]() { float v = (float)envDecayKnob.getValue();   audioProcessor.targetEnvs[(size_t)currentInsTarget].decay   = v; syncVolEnvParam("envDecay",   v); repaint(); };
    envSustainKnob.onValueChange = [this, syncVolEnvParam]() { float v = (float)envSustainKnob.getValue(); audioProcessor.targetEnvs[(size_t)currentInsTarget].sustain = v; syncVolEnvParam("envSustain", v); repaint(); };
    envReleaseKnob.onValueChange = [this, syncVolEnvParam]() { float v = (float)envReleaseKnob.getValue(); audioProcessor.targetEnvs[(size_t)currentInsTarget].release = v; syncVolEnvParam("envRelease", v); repaint(); };
    volEnvOnBtn.onClick          = [this, syncVolEnvParam]() { bool on = volEnvOnBtn.getToggleState();     audioProcessor.targetEnvs[(size_t)currentInsTarget].enabled = on; syncVolEnvParam("volEnvOn", on ? 1.0f : 0.0f); repaint(); };

    setupKnob(lfoDelayKnob, "LFO delay",  0.0);
    lfoDelayKnob.setRange(0.0, 1.0, 0.01);
    lfoDelayKnob.setValue(audioProcessor.lfoDelayAmt.load());
    lfoDelayKnob.setDefaultResetValue(0.0);
    lfoDelayKnob.onValueChange = [this]() {
        float v = (float)lfoDelayKnob.getValue();
        if (currentInsTarget == 4)
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
        if (currentInsTarget == 4)
            audioProcessor.lfoAttackAmt.store(v, std::memory_order_relaxed);
        audioProcessor.targetLfos[(size_t)currentInsTarget].attack = v;
        repaint();
    };
    setupKnob(lfoAmtKnob,   "LFO amount", 0.0, true);
    setupKnob(lfoSpeedKnob, "LFO speed",  0.5);
    lfoAmtKnob.setRange(-1.0, 1.0, 0.01);
    lfoSpeedKnob.setRange(0.0, 1.0, 0.001);
    contentCanvas.addAndMakeVisible(lfoTempoBtn);
    contentCanvas.addAndMakeVisible(lfoGlobalBtn);
    lfoTempoBtn.onClick  = [this]() { audioProcessor.targetLfos[(size_t)currentInsTarget].tempoSync   = lfoTempoBtn.getToggleState(); };
    lfoGlobalBtn.onClick = [this]() { audioProcessor.targetLfos[(size_t)currentInsTarget].globalPhase = lfoGlobalBtn.getToggleState(); };

    lfoAmtKnob.setDefaultResetValue(0.0);
    lfoSpeedKnob.setDefaultResetValue(0.5);
    lfoAmtKnob.onValueChange = [this]()
    {
        float v = (float)lfoAmtKnob.getValue();
        audioProcessor.targetLfos[(size_t)currentInsTarget].amount = v;
        if (currentInsTarget == 4)
        {
            if (auto* p = audioProcessor.apvts.getParameter("lfoAmt"))
                p->setValueNotifyingHost(p->convertTo0to1(v));
        }
        repaint();
    };
    lfoSpeedKnob.onValueChange = [this]()
    {
        float v = (float)lfoSpeedKnob.getValue();
        audioProcessor.targetLfos[(size_t)currentInsTarget].speed = v;
        if (currentInsTarget == 4)
        {
            if (auto* p = audioProcessor.apvts.getParameter("lfoSpeed"))
                p->setValueNotifyingHost(p->convertTo0to1(v));
        }
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
    trackingKeyMode = audioProcessor.trackingKeyMode.load(std::memory_order_relaxed);
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
        audioProcessor.syncInstanceChokeRegistration();
        setHintText("Cut group: " + (v == 0 ? juce::String("---") : juce::String(v)));
    };
    contentCanvas.addAndMakeVisible(cutByBox);
    cutByBox.setValue(audioProcessor.cutByGroup.load());
    cutByBox.onValueChanged = [this](int v)
    {
        audioProcessor.cutByGroup.store(v, std::memory_order_relaxed);
        audioProcessor.syncInstanceChokeRegistration();
        setHintText("Cut by group: " + (v == 0 ? juce::String("---") : juce::String(v)));
    };
    contentCanvas.addAndMakeVisible(cutSelfPushBtn);
    cutSelfPushBtn.setClickingTogglesState(false);
    cutSelfPushBtn.setColour(juce::TextButton::buttonColourId, FLColors::recessedBg());
    cutSelfPushBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff38404c));
    cutSelfPushBtn.setColour(juce::TextButton::textColourOffId, FLColors::textBright());
    cutSelfPushBtn.setColour(juce::TextButton::textColourOnId, FLColors::textBright());
    cutSelfPushBtn.onClick = [this]()
    {
        const int g = audioProcessor.allocateNextFreeSelfCutTrackGroup();
        cutGroupBox.setValue(g, false);
        cutByBox.setValue(g, false);
        trackBox.setValue(g, false);
        if (auto* p = audioProcessor.apvts.getParameter("cutSelf"))
            p->setValueNotifyingHost(1.0f);
        setHintText("Cut self assigned to Track " + juce::String(g) + " (Cut " + juce::String(g) + " - By " + juce::String(g) + ")");
        repaint();
    };

    // 6. Arpeggiator
    setupKnob(arpTimeKnob, "Arpeggiator time", 0.125, false, false, true);
    setupKnob(arpGateKnob, "Arpeggiator gate", 1.0,   false, false, true);
    arpGateKnob.setRange(0.0, 1.0, 0.01);
    arpGateKnob.setValue(audioProcessor.arpGateAmt.load());
    arpGateKnob.setDefaultResetValue(1.0);
    arpGateKnob.onValueChange = [this]() { audioProcessor.arpGateAmt.store((float)arpGateKnob.getValue(), std::memory_order_relaxed); };
    contentCanvas.addAndMakeVisible(arpRangeBox);
    arpRangeBox.setValue((int)audioProcessor.apvts.getRawParameterValue("arpRange")->load());
    arpRangeBox.onValueChanged = [this](int v)
    {
        if (auto* param = audioProcessor.apvts.getParameter("arpRange"))
            param->setValueNotifyingHost(param->convertTo0to1((float)v));
        setHintText("Arpeggiator range: " + juce::String(v) + " oct");
    };
    contentCanvas.addAndMakeVisible(arpRepeatBox);
    {
        const int curRep = audioProcessor.arpRepeatCount.load();
        arpRepeatBox.setValue(curRep <= 1 ? 0 : curRep);
    }
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
    arpChordCombo.setSelectedId(juce::jlimit(1, 7, audioProcessor.arpChordMode.load() + 1), juce::dontSendNotification);
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
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboAttachments.clear();
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

    if (currentInsTarget == 1)
    {
        auto& ve = audioProcessor.targetEnvs[1];
        ve.enabled = audioProcessor.apvts.getRawParameterValue("volEnvOn")->load() > 0.5f;
        ve.delay   = audioProcessor.apvts.getRawParameterValue("envDelay")->load();
        ve.attack  = audioProcessor.apvts.getRawParameterValue("envAttack")->load();
        ve.hold    = audioProcessor.apvts.getRawParameterValue("envHold")->load();
        ve.decay   = audioProcessor.apvts.getRawParameterValue("envDecay")->load();
        ve.sustain = audioProcessor.apvts.getRawParameterValue("envSustain")->load();
        ve.release = audioProcessor.apvts.getRawParameterValue("envRelease")->load();
    }

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

    if (currentInsTarget == 1)
    {
        envDelayKnob.setDefaultResetValue(0.0);
        envAttackKnob.setDefaultResetValue(0.0);
        envHoldKnob.setDefaultResetValue(1.0);
        envDecayKnob.setDefaultResetValue(0.0);
        envSustainKnob.setDefaultResetValue(0.0);
        envReleaseKnob.setDefaultResetValue(0.32);
        envDecTensionKnob.setDefaultResetValue(0.0);
        envRelTensionKnob.setDefaultResetValue(-0.45);
    }
    else
    {
        envDelayKnob.setDefaultResetValue(0.0);
        envAttackKnob.setDefaultResetValue(0.18);
        envHoldKnob.setDefaultResetValue(0.28);
        envDecayKnob.setDefaultResetValue(0.35);
        envSustainKnob.setDefaultResetValue(0.35);
        envReleaseKnob.setDefaultResetValue(0.25);
        envDecTensionKnob.setDefaultResetValue(0.0);
        envRelTensionKnob.setDefaultResetValue(0.0);
    }

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
    stretchModeCombo.setVisible(isSmp);
    formantShiftKnob.setVisible(isSmp && (int)audioProcessor.apvts.getRawParameterValue("stretchMode")->load() == 4);

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

void FLSamplerAudioProcessorEditor::applyEnvelopeValues(bool enabled, float del, float att, float hld, float dec, float sus, float rel,
                                                        float amt, float attT, float decT, float relT, bool tempoSync)
{
    auto& te = audioProcessor.targetEnvs[(size_t)currentInsTarget];
    te.enabled    = enabled;
    te.delay      = del;
    te.attack     = att;
    te.hold       = hld;
    te.decay      = dec;
    te.sustain    = sus;
    te.release    = rel;
    te.amount     = amt;
    te.attTension = attT;
    te.decTension = decT;
    te.relTension = relT;
    te.tempoSync  = tempoSync;

    if (currentInsTarget == 1)
    {
        auto setParam = [this](const char* id, float v)
        {
            if (auto* p = audioProcessor.apvts.getParameter(id))
                p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, v));
        };
        setParam("volEnvOn",   enabled ? 1.0f : 0.0f);
        setParam("envDelay",   del);
        setParam("envAttack",  att);
        setParam("envHold",    hld);
        setParam("envDecay",   dec);
        setParam("envSustain", sus);
        setParam("envRelease", rel);
    }

    volEnvOnBtn.setToggleState(enabled, juce::dontSendNotification);
    envDelayKnob.setValue(del, juce::dontSendNotification);
    envAttackKnob.setValue(att, juce::dontSendNotification);
    envHoldKnob.setValue(hld, juce::dontSendNotification);
    envDecayKnob.setValue(dec, juce::dontSendNotification);
    envSustainKnob.setValue(sus, juce::dontSendNotification);
    envReleaseKnob.setValue(rel, juce::dontSendNotification);
    envAmountKnob.setValue(amt, juce::dontSendNotification);
    envAttTensionKnob.setValue(attT, juce::dontSendNotification);
    envDecTensionKnob.setValue(decT, juce::dontSendNotification);
    envRelTensionKnob.setValue(relT, juce::dontSendNotification);
    envTempoBtn.setToggleState(tempoSync, juce::dontSendNotification);
    repaint();
}

void FLSamplerAudioProcessorEditor::showEnvelopePresetMenu()
{
    juce::File presetDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("FLSampler")
                               .getChildFile("EnvelopePresets");
    presetDir.createDirectory();

    juce::PopupMenu m;
    m.addSectionHeader("Envelope Presets");
    m.addItem(1, "808 / Square Hold (FL Default)");
    m.addItem(2, "Short Pluck");
    m.addItem(3, "Punchy Drum / Perc");
    m.addItem(4, "Full Sustain Gate");
    m.addItem(5, "Soft Pad / Swell");
    m.addItem(6, "Filter / Pitch Pluck (+Amount)");
    m.addItem(7, "Reset / Flat (Disabled)");
    m.addSeparator();
    m.addItem(10, "Save current envelope preset...");
    m.addItem(11, "Load envelope preset (.flenv)...");

    auto userFiles = presetDir.findChildFiles(juce::File::findFiles, false, "*.flenv");
    userFiles.sort();
    if (!userFiles.isEmpty())
    {
        m.addSeparator();
        m.addSectionHeader("Saved User Presets");
        for (int i = 0; i < userFiles.size(); ++i)
            m.addItem(100 + i, userFiles[i].getFileNameWithoutExtension());
    }

    juce::Component::SafePointer<FLSamplerAudioProcessorEditor> safeThis(this);
    m.showMenuAsync(juce::PopupMenu::Options(), [safeThis, presetDir, userFiles](int res)
    {
        if (safeThis == nullptr)
            return;
        auto* self = safeThis.getComponent();
        if (res == 1)
        {
            self->applyEnvelopeValues(true, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.32f, 1.0f, 0.0f, 0.0f, -0.45f, false);
            self->setHintText("Envelope preset: 808 / Square Hold");
        }
        else if (res == 2)
        {
            self->applyEnvelopeValues(true, 0.0f, 0.0f, 0.0f, 0.28f, 0.0f, 0.12f, 0.75f, 0.0f, -0.45f, -0.45f, false);
            self->setHintText("Envelope preset: Short Pluck");
        }
        else if (res == 3)
        {
            self->applyEnvelopeValues(true, 0.0f, 0.0f, 0.06f, 0.22f, 0.0f, 0.08f, 0.85f, 0.0f, -0.35f, -0.45f, false);
            self->setHintText("Envelope preset: Punchy Drum / Perc");
        }
        else if (res == 4)
        {
            self->applyEnvelopeValues(true, 0.0f, 0.0f, 0.0f, 0.10f, 1.0f, 0.04f, 1.0f, 0.0f, 0.0f, -0.45f, false);
            self->setHintText("Envelope preset: Full Sustain Gate");
        }
        else if (res == 5)
        {
            self->applyEnvelopeValues(true, 0.0f, 0.42f, 0.10f, 0.45f, 0.80f, 0.48f, 0.70f, 0.15f, -0.25f, -0.35f, false);
            self->setHintText("Envelope preset: Soft Pad / Swell");
        }
        else if (res == 6)
        {
            self->applyEnvelopeValues(true, 0.0f, 0.0f, 0.02f, 0.25f, 0.0f, 0.15f, 0.65f, 0.0f, -0.50f, -0.45f, false);
            self->setHintText("Envelope preset: Filter / Pitch Pluck");
        }
        else if (res == 7)
        {
            if (self->currentInsTarget == 1)
                self->applyEnvelopeValues(false, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.32f, 1.0f, 0.0f, 0.0f, -0.45f, false);
            else
                self->applyEnvelopeValues(false, 0.0f, 0.18f, 0.28f, 0.35f, 0.35f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, false);
            self->setHintText("Envelope reset to default");
        }
        else if (res == 10)
        {
            self->fileChooser = std::make_unique<juce::FileChooser>("Save Envelope Preset", presetDir, "*.flenv");
            self->fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                [safeThis](const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;
                    auto f = fc.getResult();
                    if (f != juce::File())
                    {
                        juce::File target = f.withFileExtension(".flenv");
                        const auto& te = safeThis->audioProcessor.targetEnvs[(size_t)safeThis->currentInsTarget];
                        juce::ValueTree vt("FLEnvelopePreset");
                        vt.setProperty("enabled",    te.enabled.load(), nullptr);
                        vt.setProperty("delay",      te.delay.load(), nullptr);
                        vt.setProperty("attack",     te.attack.load(), nullptr);
                        vt.setProperty("hold",       te.hold.load(), nullptr);
                        vt.setProperty("decay",      te.decay.load(), nullptr);
                        vt.setProperty("sustain",    te.sustain.load(), nullptr);
                        vt.setProperty("release",    te.release.load(), nullptr);
                        vt.setProperty("amount",     te.amount.load(), nullptr);
                        vt.setProperty("attTension", te.attTension.load(), nullptr);
                        vt.setProperty("decTension", te.decTension.load(), nullptr);
                        vt.setProperty("relTension", te.relTension.load(), nullptr);
                        vt.setProperty("tempoSync",  te.tempoSync.load(), nullptr);
                        if (auto xml = vt.createXml())
                        {
                            xml->writeTo(target);
                            safeThis->setHintText("Saved envelope preset: " + target.getFileNameWithoutExtension());
                        }
                    }
                });
        }
        else if (res == 11)
        {
            self->fileChooser = std::make_unique<juce::FileChooser>("Load Envelope Preset", presetDir, "*.flenv");
            self->fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis](const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;
                    auto f = fc.getResult();
                    if (f.existsAsFile())
                    {
                        if (auto xml = juce::XmlDocument::parse(f))
                        {
                            auto vt = juce::ValueTree::fromXml(*xml);
                            if (vt.isValid())
                            {
                                safeThis->applyEnvelopeValues(
                                    (bool)vt.getProperty("enabled", true),
                                    (float)vt.getProperty("delay", 0.0f),
                                    (float)vt.getProperty("attack", 0.0f),
                                    (float)vt.getProperty("hold", 0.28f),
                                    (float)vt.getProperty("decay", 0.0f),
                                    (float)vt.getProperty("sustain", 0.0f),
                                    (float)vt.getProperty("release", 0.0f),
                                    (float)vt.getProperty("amount", 1.0f),
                                    (float)vt.getProperty("attTension", 0.0f),
                                    (float)vt.getProperty("decTension", 0.0f),
                                    (float)vt.getProperty("relTension", -0.45f),
                                    (bool)vt.getProperty("tempoSync", false));
                                safeThis->setHintText("Loaded envelope preset: " + f.getFileNameWithoutExtension());
                            }
                        }
                    }
                });
        }
        else if (res >= 100 && res - 100 < userFiles.size())
        {
            juce::File f = userFiles[res - 100];
            if (auto xml = juce::XmlDocument::parse(f))
            {
                auto vt = juce::ValueTree::fromXml(*xml);
                if (vt.isValid())
                {
                    self->applyEnvelopeValues(
                        (bool)vt.getProperty("enabled", true),
                        (float)vt.getProperty("delay", 0.0f),
                        (float)vt.getProperty("attack", 0.0f),
                        (float)vt.getProperty("hold", 0.28f),
                        (float)vt.getProperty("decay", 0.0f),
                        (float)vt.getProperty("sustain", 0.0f),
                        (float)vt.getProperty("release", 0.0f),
                        (float)vt.getProperty("amount", 1.0f),
                        (float)vt.getProperty("attTension", 0.0f),
                        (float)vt.getProperty("decTension", 0.0f),
                        (float)vt.getProperty("relTension", -0.45f),
                        (bool)vt.getProperty("tempoSync", false));
                    self->setHintText("Loaded envelope preset: " + f.getFileNameWithoutExtension());
                }
            }
        }
    });
}

void FLSamplerAudioProcessorEditor::openSampleFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Sample Audio File",
        audioProcessor.getLoadedFile().existsAsFile()
            ? audioProcessor.getLoadedFile().getParentDirectory()
            : juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.flac;*.mp3;*.ogg;*.aiff;*.aif");

    juce::Component::SafePointer<FLSamplerAudioProcessorEditor> safeThis(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& fc)
        {
            if (safeThis == nullptr)
                return;
            auto f = fc.getResult();
            if (f.existsAsFile())
            {
                safeThis->audioProcessor.loadSampleFromFile(f);
                safeThis->wavScope.refreshIfNeeded();
                safeThis->repaint();
            }
        });
}

void FLSamplerAudioProcessorEditor::timerCallback()
{
    audioProcessor.flushPrecomputeIfNeeded();
    wavScope.refreshIfNeeded();

    const uint32_t curStateRev = audioProcessor.getStateRevision();
    if (curStateRev != lastSeenStateRev)
    {
        lastSeenStateRev = curStateRev;
        flLookAndFeel.refreshThemeColours();
        keepOnDiskBtn.setToggleState(audioProcessor.keepOnDisk.load(), juce::dontSendNotification);
        loadRegionsBtn.setToggleState(audioProcessor.loadRegions.load(), juce::dontSendNotification);
        loadSlicesBtn.setToggleState(audioProcessor.loadSliceMarkers.load(), juce::dontSendNotification);
        trackBox.setValue(audioProcessor.mixerTrack.load());
        cutGroupBox.setValue(audioProcessor.cutGroup.load());
        cutByBox.setValue(audioProcessor.cutByGroup.load());
        addToKeyBtn.setToggleState(audioProcessor.addToKey.load(), juce::dontSendNotification);
        fullPortaBtn.setToggleState(audioProcessor.fullPorta.load(), juce::dontSendNotification);
        shiftKnob.setValue(audioProcessor.shiftTimeAmt.load(), juce::dontSendNotification);
        swingKnob.setValue(audioProcessor.swingMulAmt.load(), juce::dontSendNotification);
        trackingKeyMode = audioProcessor.trackingKeyMode.load();
        lvlPanKnob.setValue(audioProcessor.lvlPanOffset.load(), juce::dontSendNotification);
        lvlVolKnob.setValue(audioProcessor.lvlVolOffset.load(), juce::dontSendNotification);
        lvlModXKnob.setValue(audioProcessor.lvlModXOffset.load(), juce::dontSendNotification);
        lvlModYKnob.setValue(audioProcessor.lvlModYOffset.load(), juce::dontSendNotification);
        trackPanKnob.setValue(audioProcessor.trackPanAmt.load(), juce::dontSendNotification);
        trackModXKnob.setValue(audioProcessor.trackModXAmt.load(), juce::dontSendNotification);
        trackModYKnob.setValue(audioProcessor.trackModYAmt.load(), juce::dontSendNotification);
        trackMidSlider.setValue(audioProcessor.trackMidNote.load(), juce::dontSendNotification);
        arpGateKnob.setValue(audioProcessor.arpGateAmt.load(), juce::dontSendNotification);
        const int curRep = audioProcessor.arpRepeatCount.load();
        arpRepeatBox.setValue(curRep <= 1 ? 0 : curRep);
        arpSlideBtn.setToggleState(audioProcessor.arpSlideOn.load(), juce::dontSendNotification);
        arpChordCombo.setSelectedId(juce::jlimit(1, 7, audioProcessor.arpChordMode.load() + 1), juce::dontSendNotification);
        echoPanKnob.setValue(audioProcessor.echoPanAmt.load(), juce::dontSendNotification);
        echoModXKnob.setValue(audioProcessor.echoModXAmt.load(), juce::dontSendNotification);
        echoModYKnob.setValue(audioProcessor.echoModYAmt.load(), juce::dontSendNotification);
        echoPitchKnob.setValue(audioProcessor.echoPitchAmt.load(), juce::dontSendNotification);
        echoFatModeBtn.setToggleState(audioProcessor.echoFatModeOn.load(), juce::dontSendNotification);
        updateInsTarget(currentInsTarget);
        contentCanvas.repaint();
        repaint();
    }

    const int curPoly = (int)audioProcessor.apvts.getRawParameterValue("maxPoly")->load();
    if (maxPolyBox.getValue() != curPoly)
        maxPolyBox.setValue(curPoly);
    const int curRange = (int)audioProcessor.apvts.getRawParameterValue("pitchRange")->load();
    if (pitchRangeBox.getValue() != curRange)
        pitchRangeBox.setValue(curRange);
    const int curArpRange = (int)audioProcessor.apvts.getRawParameterValue("arpRange")->load();
    if (arpRangeBox.getValue() != curArpRange)
        arpRangeBox.setValue(curArpRange);
    const int curEchoCount = (int)audioProcessor.apvts.getRawParameterValue("echoCount")->load();
    if (echoCountBox.getValue() != curEchoCount)
        echoCountBox.setValue(curEchoCount);

    if (currentSheet == 0)
    {
        const int modeIdx = (int)audioProcessor.apvts.getRawParameterValue("stretchMode")->load();
        const bool wantFormantVis = (modeIdx == 4);
        if (formantShiftKnob.isVisible() != wantFormantVis)
        {
            formantShiftKnob.setVisible(wantFormantVis);
            repaint();
        }
    }
    else if (currentSheet == 1)
    {
        repaint(6, 80, getWidth() - 12, 220);
    }
}

// Draws an FL Studio 2026 panel (or DAW / OS themed panel when active)
static void drawFLPanel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, int titleIndent = 8)
{
    auto rf = r.toFloat();
    if (FLColors::isWin98Skin)
    {
        // Classic Windows 98 raised 3D grey dialog box with etched group frame & navy header bar
        g.setColour(FLColors::panelBg());
        g.fillRect(r);
        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(r.getY(), (float)r.getX(), (float)r.getRight() - 1.0f);
        g.drawVerticalLine(r.getX(), (float)r.getY(), (float)r.getBottom() - 1.0f);
        g.setColour(juce::Colour(0xff808080));
        g.drawHorizontalLine(r.getBottom() - 2, (float)r.getX() + 1.0f, (float)r.getRight() - 1.0f);
        g.drawVerticalLine(r.getRight() - 2, (float)r.getY() + 1.0f, (float)r.getBottom() - 1.0f);
        g.setColour(juce::Colour(0xff404040));
        g.drawHorizontalLine(r.getBottom() - 1, (float)r.getX(), (float)r.getRight());
        g.drawVerticalLine(r.getRight() - 1, (float)r.getY(), (float)r.getBottom());

        if (title.isNotEmpty())
        {
            juce::ColourGradient titleG(juce::Colour(0xff000080), (float)r.getX() + 2.0f, (float)r.getY() + 2.0f,
                                        juce::Colour(0xff1084d0), (float)r.getRight() - 2.0f, (float)r.getY() + 2.0f, false);
            g.setGradientFill(titleG);
            g.fillRect(r.getX() + 2, r.getY() + 2, r.getWidth() - 4, 16);

            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
            g.drawText(title, r.getX() + titleIndent, r.getY() + 2, r.getWidth() - titleIndent - 4, 16,
                       juce::Justification::centredLeft, false);
        }
        return;
    }
    if (FLColors::isMacOSXSkin)
    {
        // Classic Mac OS X Aqua rounded card with subtle horizontal Aqua pinstripes & glossy header bar
        g.setColour(FLColors::panelBg());
        g.fillRoundedRectangle(rf, 6.0f);

        g.setColour(juce::Colour(0xffe2e5eb));
        for (int py = r.getY() + 20; py < r.getBottom() - 2; py += 4)
            g.drawHorizontalLine(py, (float)r.getX() + 2.0f, (float)r.getRight() - 2.0f);

        if (title.isNotEmpty())
        {
            juce::ColourGradient aquaHeader(juce::Colour(0xfff8fafd), (float)r.getX(), (float)r.getY(),
                                            juce::Colour(0xffd4dce8), (float)r.getX(), (float)r.getY() + 19.0f, false);
            g.setGradientFill(aquaHeader);
            g.fillRoundedRectangle((float)r.getX() + 1.0f, (float)r.getY() + 1.0f, (float)r.getWidth() - 2.0f, 18.0f, 5.0f);
            g.setColour(juce::Colour(0xffb5becb));
            g.drawHorizontalLine(r.getY() + 19, (float)r.getX() + 1.0f, (float)r.getRight() - 1.0f);

            g.setColour(FLColors::textBright());
            g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            g.drawText(title, r.getX() + titleIndent, r.getY() + 2, r.getWidth() - titleIndent - 4, 16,
                       juce::Justification::centredLeft, false);
        }

        g.setColour(FLColors::panelBorder());
        g.drawRoundedRectangle(rf.reduced(0.5f), 6.0f, 1.0f);
        return;
    }
    if (FLColors::isWinampSkin)
    {
        // Classic Winamp 2.x panel: dark navy-slate chassis with 3D metallic double bevel & gold/bronze header bar
        g.setColour(FLColors::panelBg());
        g.fillRect(r);
        g.setColour(juce::Colour(0xff5a607a));
        g.drawHorizontalLine(r.getY(), (float)r.getX(), (float)r.getRight() - 1.0f);
        g.drawVerticalLine(r.getX(), (float)r.getY(), (float)r.getBottom() - 1.0f);
        g.setColour(juce::Colour(0xff101119));
        g.drawHorizontalLine(r.getBottom() - 1, (float)r.getX(), (float)r.getRight());
        g.drawVerticalLine(r.getRight() - 1, (float)r.getY(), (float)r.getBottom());

        if (title.isNotEmpty())
        {
            juce::ColourGradient winampHdr(juce::Colour(0xff4a4228), (float)r.getX() + 1.0f, (float)r.getY() + 1.0f,
                                           juce::Colour(0xff1e202d), (float)r.getX() + 1.0f, (float)r.getY() + 18.0f, false);
            g.setGradientFill(winampHdr);
            g.fillRect(r.getX() + 1, r.getY() + 1, r.getWidth() - 2, 17);
            g.setColour(juce::Colour(0xffb59b52));
            g.drawHorizontalLine(r.getY() + 18, (float)r.getX() + 1.0f, (float)r.getRight() - 1.0f);

            g.setColour(juce::Colour(0xffffd84d));
            g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
            g.drawText(title, r.getX() + titleIndent, r.getY() + 1, r.getWidth() - titleIndent - 4, 17,
                       juce::Justification::centredLeft, false);
        }
        return;
    }
    if (FLColors::isProToolsSkin || FLColors::isLogicSkin || FLColors::isSequoiaSkin || FLColors::isAuditionSkin || FLColors::isCubaseSkin || FLColors::isReaperSkin)
    {
        const float corner = FLColors::isLogicSkin ? 5.0f : (FLColors::isProToolsSkin ? 3.0f : 1.5f);
        g.setColour(FLColors::panelBg());
        g.fillRoundedRectangle(rf, corner);
        g.setColour(FLColors::panelBorder());
        g.drawRoundedRectangle(rf.reduced(0.5f), corner, 1.0f);

        if (title.isNotEmpty())
        {
            const juce::Colour hdrCol = FLColors::isProToolsSkin ? juce::Colour(0xff202328)
                                      : (FLColors::isLogicSkin ? juce::Colour(0xff1d2024)
                                      : (FLColors::isSequoiaSkin ? juce::Colour(0xff1a1d22)
                                      : (FLColors::isAuditionSkin ? juce::Colour(0xff181a1c)
                                      : (FLColors::isReaperSkin ? juce::Colour(0xff212627) : juce::Colour(0xff181b21)))));
            g.setColour(hdrCol);
            g.fillRoundedRectangle((float)r.getX() + 1.0f, (float)r.getY() + 1.0f, (float)r.getWidth() - 2.0f, 18.0f, corner);
            g.setColour(FLColors::panelBorder());
            g.drawHorizontalLine(r.getY() + 19, (float)r.getX() + 1.0f, (float)r.getRight() - 1.0f);

            // Subtle left accent bar in Cubase / Pro Tools / Sequoia / Audition / Reaper
            if (FLColors::isCubaseSkin || FLColors::isProToolsSkin || FLColors::isSequoiaSkin || FLColors::isAuditionSkin || FLColors::isReaperSkin)
            {
                g.setColour(FLColors::greenNeon());
                g.fillRect((float)r.getX() + 2.0f, (float)r.getY() + 4.0f, 2.5f, 12.0f);
            }

            g.setColour(FLColors::textBright());
            g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            g.drawText(title, r.getX() + titleIndent, r.getY() + 2, r.getWidth() - titleIndent - 4, 16,
                       juce::Justification::centredLeft, false);
        }
        return;
    }
    if (FLColors::isAbletonSkin)
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
    if (FLColors::isBlackSkin || FLColors::isWhiteSkin)
    {
        g.setColour(FLColors::panelBg());
        g.fillRoundedRectangle(rf, 4.0f);
        g.setColour(FLColors::panelBorder());
        g.drawRoundedRectangle(rf.reduced(0.5f), 4.0f, 1.0f);

        if (title.isNotEmpty())
        {
            g.setColour(FLColors::isWhiteSkin ? juce::Colour(0xfff1f5f9) : juce::Colour(0xff141418));
            g.fillRoundedRectangle((float)r.getX() + 1.0f, (float)r.getY() + 1.0f, (float)r.getWidth() - 2.0f, 18.0f, 3.0f);
            g.setColour(FLColors::isWhiteSkin ? juce::Colour(0xffe2e8f0) : juce::Colour(0xff27272a));
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
        g.setColour(juce::Colour(FLColors::isFLDarkSkin ? 0xff111317 : 0xff262b32));
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

// Draws a crisp, non-self-intersecting FL Studio open-end C-wrench (used in both Header Tab 3 & Precomputed Effects Sub-Tab 1)
static void drawFLOpenWrenchIcon(juce::Graphics& g, float cx, float cy, float scale,
                                 juce::Colour mainCol, juce::Colour cutoutCol)
{
    const float hx = cx + 2.8f * scale;
    const float hy = cy - 2.6f * scale;
    const float rOut = 5.4f * scale;
    const float rIn  = 2.6f * scale;

    // 1. Diagonal rounded handle with inner slot
    {
        juce::Path handleOuter;
        handleOuter.startNewSubPath(hx - 1.8f * scale, hy + 0.2f * scale);
        handleOuter.lineTo(cx - 7.8f * scale, cy + 6.2f * scale);
        handleOuter.lineTo(cx - 5.2f * scale, cy + 8.8f * scale);
        handleOuter.lineTo(hx + 0.8f * scale, hy + 2.8f * scale);
        handleOuter.closeSubPath();
        g.setColour(mainCol);
        g.fillPath(handleOuter.createPathWithRoundedCorners(1.4f * scale));
    }

    // 2. Outer C-head circle
    g.setColour(mainCol);
    g.fillEllipse(hx - rOut, hy - rOut, rOut * 2.0f, rOut * 2.0f);

    // 3. Carve inner jaw circle & right-facing jaw mouth opening cleanly (zero self-intersection!)
    g.setColour(cutoutCol);
    g.fillEllipse(hx - rIn, hy - rIn, rIn * 2.0f, rIn * 2.0f);

    juce::Path jawOpening;
    jawOpening.startNewSubPath(hx, hy - rIn * 0.92f);
    jawOpening.lineTo(hx + rOut + 1.5f * scale, hy - rIn * 1.05f);
    jawOpening.lineTo(hx + rOut + 1.5f * scale, hy + rIn * 1.05f);
    jawOpening.lineTo(hx, hy + rIn * 0.92f);
    jawOpening.closeSubPath();
    g.fillPath(jawOpening);

    // 4. Carve inner slot along the diagonal handle
    g.drawLine(cx - 5.6f * scale, cy + 6.6f * scale, hx - 2.4f * scale, hy + 3.4f * scale, 1.35f * scale);
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

    // "Fine tune" label (with generous spacing after "Add to key")
    g.setColour(FLColors::textBright());
    g.drawText("Fine tune", 272, 330, 56, 18, juce::Justification::centredLeft, false);

    // 1:1 FL Studio Multi-touch Pointing Hand Toggle Icon on far right (Image 1 when selected, Image 2 when unselected)
    {
        auto hr = keybHandBtnRect.toFloat();
        if (zoomedKeyboardMode)
        {
            // Angled trapezoid border matching FL Studio's orange-gold active frame in Image 1
            juce::Path frame;
            frame.startNewSubPath(hr.getX() + 7.0f, hr.getY() + 1.0f);
            frame.lineTo(hr.getRight() - 1.0f, hr.getY() + 1.0f);
            frame.lineTo(hr.getRight() - 1.0f, hr.getBottom() - 1.0f);
            frame.lineTo(hr.getX() + 1.0f, hr.getBottom() - 1.0f);
            frame.closeSubPath();

            g.setColour(juce::Colour(0xff1c1f24));
            g.fillPath(frame);
            g.setColour(juce::Colour(0xffffa63b));
            g.strokePath(frame, juce::PathStrokeType(1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Pointing hand silhouette tilted up-left with cuff (1:1 Image 1 & Image 2)
        const float hx = hr.getCentreX() + 2.0f;
        const float hy = hr.getCentreY() + 1.0f;
        const juce::Colour handCol = zoomedKeyboardMode ? juce::Colour(0xffffa63b) : FLColors::textBright();

        juce::Path hand;
        // Index finger pointing up-left
        hand.startNewSubPath(hx - 6.8f, hy - 5.8f);
        hand.lineTo(hx - 5.3f, hy - 7.1f);
        hand.lineTo(hx - 1.2f, hy - 3.4f);
        // Knuckles / folded fingers on top-right
        hand.quadraticTo(hx + 3.8f, hy - 3.8f, hx + 5.6f, hy - 0.5f);
        hand.lineTo(hx + 3.2f, hy + 3.2f);
        // Palm bottom & thumb pointing left
        hand.lineTo(hx - 2.0f, hy + 2.4f);
        hand.lineTo(hx - 6.2f, hy + 1.2f);
        hand.lineTo(hx - 5.8f, hy - 0.8f);
        hand.lineTo(hx - 2.8f, hy - 0.5f);
        hand.closeSubPath();

        g.setColour(handCol);
        g.fillPath(hand);

        // Wrist cuff line at bottom-right
        g.drawLine(hx + 1.5f, hy + 4.8f, hx + 5.6f, hy + 1.6f, 1.5f);
    }

    auto isBlackKey = [](int m) {
        int pc = m % 12;
        return (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
    };

    const float kbX = (float)bottomKeybRect.getX();
    const float kbY = (float)bottomKeybRect.getY();
    const float kbW = (float)bottomKeybRect.getWidth();

    // ==================== MODE A: ZOOMED TOUCH PIANO VIEW (B3..D6 = 17 White Keys, Full Height, Bottom Note Labels - Image 1) ====================
    if (zoomedKeyboardMode)
    {
        const float kbH = (float)bottomKeybRect.getHeight() + 12.0f; // Covers full height down to bottom of panel (no scrollbar!)
        juce::Graphics::ScopedSaveState clipKb(g);
        g.reduceClipRegion(bottomKeybRect.getX(), bottomKeybRect.getY(), bottomKeybRect.getWidth(), (int)kbH);

        const int startMidi = 47; // B3
        const int endMidi   = 74; // D6
        const int numWhiteKeys = 17; // B3, C4..B4 (7), C5..B5 (7), C6, D6 = 17 white keys
        const float whiteW = kbW / (float)numWhiteKeys;

        // 1. Draw wide white keys with note names at the bottom (B3, C4, D4, E4, F4, G4, A4, B4, C5, D5, E5, F5, G5, A5, B5, C6, D6)
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
                g.setColour(altOctave ? juce::Colour(0xffbcc3cc) : juce::Colour(0xffccd3dc));

            g.fillRect(wr);
            g.setColour(juce::Colour(0xff7c848e));
            g.drawRect(wr, 0.6f);

            const juce::String keyLabel = juce::String(noteNames[m % 12]) + juce::String(oct);
            g.setColour(m == rootNote ? juce::Colour(0xff0d2240) : juce::Colour(0xff525a64));
            g.setFont(juce::Font(juce::FontOptions(13.0f, m == rootNote ? juce::Font::bold : juce::Font::plain)));
            g.drawText(keyLabel, (int)wr.getX(), (int)(kbY + kbH - 22.0f), (int)whiteW, 18,
                       juce::Justification::centred, false);
            ++wIdx;
        }

        // 2. Draw wide 3D bevelled black keys Flush from Top of Keyboard (including left half-key A#3 and right half-key D#6 matching Image 1!)
        const float bw = whiteW * 0.68f;
        const float bh = kbH * 0.48f;
        auto drawZoomedBlackKey = [&](float bx, int midiNum)
        {
            juce::Rectangle<float> br(bx, kbY, bw, bh);
            g.setColour(midiNum == rootNote ? juce::Colour(0xff4c88d4)
                                            : (midiNum == previewHeldMidiNote ? FLColors::greenNeon() : juce::Colour(0xff1e2126)));
            g.fillRect(br);

            // Inner 3D matte cap with diagonal sheen matching Image 1
            auto inner = br.reduced(2.2f, 2.0f);
            juce::ColourGradient blkGrad(juce::Colour(0xff383d45), inner.getX(), inner.getY(),
                                         juce::Colour(0xff23272c), inner.getRight(), inner.getBottom(), false);
            g.setGradientFill(blkGrad);
            g.fillRect(inner);

            g.setColour(juce::Colour(0xff50565f));
            g.drawHorizontalLine((int)(br.getBottom() - 2.5f), inner.getX(), inner.getRight());
        };

        // Partial A#3 black key on far left edge (before B3)
        drawZoomedBlackKey(kbX - bw * 0.55f, 46);

        wIdx = 0;
        for (int m = startMidi; m <= endMidi; ++m)
        {
            if (!isBlackKey(m))
            {
                ++wIdx;
                continue;
            }
            drawZoomedBlackKey(kbX + (float)wIdx * whiteW - bw * 0.5f, m);
        }

        // Partial D#6 black key on far right edge (after D6)
        drawZoomedBlackKey(kbX + (float)numWhiteKeys * whiteW - bw * 0.45f, 75);
        return;
    }

    // ==================== MODE B: STANDARD OVERVIEW KEYBOARD (C2..C8 = 43 White Keys + Top Octave Header Strip + Bottom Scrollbar - Image 2) ====================
    g.setColour(juce::Colour(0xffd2d8e0));
    g.fillRect(bottomKeybRect);

    const int startMidi = 24; // C2
    const int endMidi   = 96; // C8
    const int numWhiteKeys = 43;
    const float whiteW = kbW / (float)numWhiteKeys;
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
            g.setColour(altOctave ? juce::Colour(0xffbcc3cc) : juce::Colour(0xffccd3dc));

        g.fillRect(wr);
        g.setColour(juce::Colour(0xff8e96a0));
        g.drawRect(wr, 0.5f);
        ++wIdx;
    }

    // Top header strip above black keys + Octave labels C2, C3, C4, C5, C6, C7, C8
    g.setColour(juce::Colour(0xff68707a).withAlpha(0.25f));
    g.fillRect(kbX, kbY, kbW, 14.0f);

    wIdx = 0;
    for (int m = startMidi; m <= endMidi; ++m)
    {
        if (isBlackKey(m)) continue;
        if (m % 12 == 0)
        {
            int oct = (m / 12);
            float wx = kbX + wIdx * whiteW;
            g.setColour(juce::Colour(0xff14171c));
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            g.drawText("C" + juce::String(oct), (int)wx + 1, (int)kbY + 1, 22, 13,
                       juce::Justification::centredLeft, false);
        }
        ++wIdx;
    }

    // Second pass: draw 3D black keys with glossy bottom-right bevel highlight (1:1 Image 2)
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
        float bh = kbH * 0.65f;
        juce::Rectangle<float> br(bx, kbY + 14.0f, bw, bh - 14.0f);

        g.setColour(m == rootNote ? (FLColors::isAbletonSkin ? juce::Colour(0xff009ec2) : juce::Colour(0xff4c88d4))
                                  : (m == previewHeldMidiNote ? FLColors::greenNeon() : juce::Colour(0xff1e2228)));
        g.fillRect(br);
        g.setColour(juce::Colour(0xff525a65));
        g.drawRect(br.reduced(1.2f, 1.2f), 0.7f);
    }

    // Bottom scrollbar bar under keyboard with '<' and '>' arrows + central pill handle (1:1 Image 2)
    juce::Rectangle<int> scrollBar(bottomKeybRect.getX(), bottomKeybRect.getBottom(), bottomKeybRect.getWidth(), 12);
    g.setColour(juce::Colour(0xff2b3038));
    g.fillRect(scrollBar);

    // Left '<' and Right '>' scrollbar arrows
    g.setColour(juce::Colour(0xff9aa3ad));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText("<", scrollBar.getX() + 2, scrollBar.getY(), 12, 12, juce::Justification::centred);
    g.drawText(">", scrollBar.getRight() - 14, scrollBar.getY(), 12, 12, juce::Justification::centred);

    const int regLo = juce::jlimit(24, 96, audioProcessor.keyRegionLow.load());
    const int regHi = juce::jlimit(regLo, 96, audioProcessor.keyRegionHigh.load());
    if (regLo > 24 || regHi < 96)
    {
        float rx0 = kbX + ((float)(regLo - 24) / 72.0f) * kbW;
        float rx1 = kbX + ((float)(regHi - 24 + 1) / 72.0f) * kbW;
        g.setColour(FLColors::orangeRing().withAlpha(0.85f));
        g.fillRoundedRectangle(rx0, (float)scrollBar.getY() + 3.0f, juce::jmax(6.0f, rx1 - rx0), 6.0f, 3.0f);
    }
    else
    {
        // Default central grey pill handle matching Image 2
        g.setColour(juce::Colour(0xffadb5bf));
        g.fillRoundedRectangle(kbX + kbW * 0.36f, (float)scrollBar.getY() + 4.0f, kbW * 0.14f, 4.0f, 2.0f);
    }
}

void FLSamplerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(FLColors::windowBg());

    // Scale all vector drawing uniformly when window is resized!
    const float scaleX = (float)getWidth()  / (float)kDesignW;
    const float scaleY = (float)getHeight() / (float)kDesignH;
    juce::Graphics::ScopedSaveState sss(g);
    g.addTransform(juce::AffineTransform::scale(scaleX, scaleY));

    // 1. Top Header Bar (y: 0..42)
    g.setColour(FLColors::headerBg());
    g.fillRect(0, 0, kDesignW, 42);
    g.setColour(FLColors::panelBorder());
    g.drawHorizontalLine(42, 0.0f, (float)kDesignW);

    // Highlight outer border in FL Neon Green when dragging an audio file anywhere over the plugin
    if (isWindowDragHover)
    {
        g.setColour(FLColors::greenNeon());
        g.drawRect(0, 0, kDesignW, kDesignH, 2);
    }

    // Skin Switcher Button ("SKIN: FL" -> "SKIN: FL DARK" -> "SKIN: LIVE" -> ...)
    {
        auto sr = skinToggleBtnRect.toFloat();
        g.setColour(FLColors::isWin98Skin ? juce::Colour(0xffc0c0c0) : FLColors::recessedBg());
        g.fillRoundedRectangle(sr, (FLColors::isAbletonSkin || FLColors::isWin98Skin) ? 0.0f : 3.0f);
        g.setColour(FLColors::skinMode > 0 ? (FLColors::isWin98Skin ? juce::Colours::white : FLColors::greenNeon()) : FLColors::panelBorder());
        g.drawRoundedRectangle(sr.reduced(0.5f), (FLColors::isAbletonSkin || FLColors::isWin98Skin) ? 0.0f : 3.0f, 1.0f);

        g.setColour(FLColors::isWin98Skin ? juce::Colours::black : (FLColors::skinMode > 0 ? FLColors::greenNeon() : FLColors::textBright()));
        g.setFont(juce::Font(juce::FontOptions(7.9f, juce::Font::bold)));
        const char* skinNames[FLColors::kNumSkins] = {
            "SKIN: FL", "SKIN: FL DARK", "SKIN: LIVE", "SKIN: LOGIC", "SKIN: PTOOLS", "SKIN: SEQUOIA", "SKIN: AUDITION",
            "SKIN: CUBASE", "SKIN: REAPER", "SKIN: WINAMP", "SKIN: WIN98", "SKIN: MACOSX", "SKIN: BLACK", "SKIN: WHITE"
        };
        g.drawText(skinNames[juce::jlimit(0, FLColors::kNumSkins - 1, FLColors::skinMode)], skinToggleBtnRect, juce::Justification::centred, false);
    }

    // FL Hint Bar readout in header bar when active
    if (currentHintText.isNotEmpty())
    {
        g.setColour(FLColors::recessedBg().withAlpha(0.95f));
        g.fillRoundedRectangle(198.0f, 9.0f, 118.0f, 24.0f, FLColors::isAbletonSkin ? 0.0f : 3.0f);
        g.setColour(FLColors::textDim());
        g.setFont(juce::Font(juce::FontOptions(9.5f)));
        g.drawText(currentHintText, 201, 9, 112, 24, juce::Justification::centredLeft, true);
    }

    const juce::Colour hdrIconInactiveCol = FLColors::isWin98Skin ? juce::Colours::white : FLColors::textBright();
    const juce::Colour hdrIconActiveCol   = FLColors::isWin98Skin ? juce::Colour(0xff00ffff) : FLColors::greenNeon();
    const juce::Colour hdrTabActiveBg     = FLColors::isWin98Skin ? juce::Colour(0xff000050) : FLColors::recessedBg();

    // 3 Left Sheet Tab Icons (1:1 Image 2: Filled Waveform, Curved AHDSR Nodes, Slanted Open-End Wrench)
    // Tab 0: Symmetric Filled Waveform icon
    {
        g.setColour(currentSheet == 0 ? hdrTabActiveBg : juce::Colours::transparentBlack);
        g.fillRect(tabRectWave);

        const float cx = (float)tabRectWave.getCentreX();
        const float cy = (float)tabRectWave.getCentreY();
        juce::Path waveFill;
        waveFill.startNewSubPath(cx - 11.5f, cy);
        waveFill.lineTo(cx - 8.2f,  cy - 0.6f);
        waveFill.lineTo(cx - 6.2f,  cy - 4.5f);
        waveFill.lineTo(cx - 4.2f,  cy - 1.2f);
        waveFill.lineTo(cx - 2.2f,  cy - 1.8f);
        waveFill.lineTo(cx,         cy - 9.2f);
        waveFill.lineTo(cx + 2.2f,  cy - 1.8f);
        waveFill.lineTo(cx + 4.2f,  cy - 1.2f);
        waveFill.lineTo(cx + 6.2f,  cy - 4.5f);
        waveFill.lineTo(cx + 8.2f,  cy - 0.6f);
        waveFill.lineTo(cx + 11.5f, cy);
        waveFill.lineTo(cx + 8.2f,  cy + 0.6f);
        waveFill.lineTo(cx + 6.2f,  cy + 4.5f);
        waveFill.lineTo(cx + 4.2f,  cy + 1.2f);
        waveFill.lineTo(cx + 2.2f,  cy + 1.8f);
        waveFill.lineTo(cx,         cy + 9.2f);
        waveFill.lineTo(cx - 2.2f,  cy + 1.8f);
        waveFill.lineTo(cx - 4.2f,  cy + 1.2f);
        waveFill.lineTo(cx - 6.2f,  cy + 4.5f);
        waveFill.lineTo(cx - 8.2f,  cy + 0.6f);
        waveFill.closeSubPath();

        g.setColour(currentSheet == 0 ? hdrIconActiveCol : hdrIconInactiveCol);
        g.fillPath(waveFill);
    }

    // Tab 1: Envelope / Instrument icon (3 hollow circle nodes + straight attack & curved exponential decay)
    {
        g.setColour(currentSheet == 1 ? hdrTabActiveBg : juce::Colours::transparentBlack);
        g.fillRect(tabRectEnv);

        const float cx = (float)tabRectEnv.getCentreX();
        const float cy = (float)tabRectEnv.getCentreY();
        const juce::Point<float> p1(cx - 6.5f, cy + 6.5f);
        const juce::Point<float> p2(cx - 3.5f, cy - 7.5f);
        const juce::Point<float> p3(cx + 6.5f, cy + 6.5f);

        juce::Path envCurve;
        envCurve.startNewSubPath(p1.x + 0.6f, p1.y - 2.5f);
        envCurve.lineTo(p2.x - 0.5f, p2.y + 2.5f);
        envCurve.startNewSubPath(p2.x + 0.8f, p2.y + 2.4f);
        envCurve.quadraticTo(p2.x + 2.2f, p3.y - 1.5f, p3.x - 2.4f, p3.y - 0.5f);

        g.setColour(currentSheet == 1 ? hdrIconActiveCol : hdrIconInactiveCol);
        g.strokePath(envCurve, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawEllipse(p1.x - 2.5f, p1.y - 2.5f, 5.0f, 5.0f, 1.7f);
        g.drawEllipse(p2.x - 2.5f, p2.y - 2.5f, 5.0f, 5.0f, 1.7f);
        g.drawEllipse(p3.x - 2.5f, p3.y - 2.5f, 5.0f, 5.0f, 1.7f);
    }

    // Tab 2: Open-End Wrench icon (Misc functions - 1:1 Image 3)
    {
        const juce::Colour bgCol = (currentSheet == 2) ? hdrTabActiveBg : FLColors::headerBg();
        g.setColour(currentSheet == 2 ? hdrTabActiveBg : juce::Colours::transparentBlack);
        g.fillRect(tabRectFunc);

        const float cx = (float)tabRectFunc.getCentreX();
        const float cy = (float)tabRectFunc.getCentreY();
        const juce::Colour wrenchCol = (currentSheet == 2) ? hdrIconActiveCol : hdrIconInactiveCol;
        drawFLOpenWrenchIcon(g, cx, cy, 1.0f, wrenchCol, bgCol);
    }

    // Master Header Labels (ON, PAN, VOL, PITCH, RANGE, TRACK)
    g.setColour(hdrIconInactiveCol);
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
        g.drawText(audioProcessor.getCachedFileName(), fileNameBoxRect, juce::Justification::centredLeft, true);

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
        g.setColour(FLColors::textBright());
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText("Mode", 384, 72, 38, 18, juce::Justification::centredLeft, false);
        if (formantShiftKnob.isVisible())
        {
            g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
            g.drawText("FORMANT", 380, 101, 46, 18, juce::Justification::centredLeft, false);
        }

        // Precomputed Effects Panel (x=264, y=138, w=256, h=152)
        drawFLPanel(g, { 264, 138, 256, 152 }, "Precomputed effects", 56);

        // Wrench & Vacuum Tube sub-tab icons on the left of "Precomputed effects" title (1:1 Image 1 & Image 2)
        {
            const juce::Colour subTabActBg = FLColors::isWin98Skin ? juce::Colour(0xff000050) : FLColors::recessedBg();
            const juce::Colour subTabInactBg = FLColors::isWin98Skin ? juce::Colour(0xff000080) : FLColors::panelBg();
            const juce::Colour wBgCol = !legacyPrecomputedTab ? subTabActBg : subTabInactBg;
            const juce::Colour bBgCol = legacyPrecomputedTab  ? subTabActBg : subTabInactBg;
            const juce::Colour inactIconCol = FLColors::isWin98Skin ? juce::Colours::white : FLColors::textBright();
            g.setColour(!legacyPrecomputedTab ? subTabActBg : juce::Colours::transparentBlack);
            g.fillRect(fxSubTabWrench);
            g.setColour(legacyPrecomputedTab ? subTabActBg : juce::Colours::transparentBlack);
            g.fillRect(fxSubTabBulb);

            // 1. Slanted Open-End Wrench icon (Gold-yellow when active, clean C-jaw without self-intersection)
            const float wcx = (float)fxSubTabWrench.getCentreX();
            const float wcy = (float)fxSubTabWrench.getCentreY();
            const juce::Colour wCol = !legacyPrecomputedTab ? juce::Colour(0xffffb82e) : inactIconCol;
            drawFLOpenWrenchIcon(g, wcx, wcy, 0.78f, wCol, wBgCol);

            // 2. Vintage Vacuum Tube / Valve icon (Solid white/bright when inactive, solid gold-yellow when selected — matching the Wrench icon!)
            const float bcx = (float)fxSubTabBulb.getCentreX();
            const float bcy = (float)fxSubTabBulb.getCentreY();
            const juce::Colour bCol = legacyPrecomputedTab ? juce::Colour(0xffffb82e) : inactIconCol;

            // Outer glass silhouette with top exhaust pip & rounded shoulders
            juce::Path tubeOuter;
            tubeOuter.startNewSubPath(bcx, bcy - 6.8f);
            tubeOuter.lineTo(bcx + 1.4f, bcy - 5.2f);
            tubeOuter.quadraticTo(bcx + 4.4f, bcy - 4.4f, bcx + 4.4f, bcy - 1.8f);
            tubeOuter.lineTo(bcx + 4.4f, bcy + 3.0f);
            tubeOuter.lineTo(bcx - 4.4f, bcy + 3.0f);
            tubeOuter.lineTo(bcx - 4.4f, bcy - 1.8f);
            tubeOuter.quadraticTo(bcx - 4.4f, bcy - 4.4f, bcx - 1.4f, bcy - 5.2f);
            tubeOuter.closeSubPath();

            g.setColour(bCol);
            g.fillPath(tubeOuter);

            // Inner cutout chamber below the dome
            juce::Rectangle<float> chamber(bcx - 3.0f, bcy - 2.4f, 6.0f, 5.2f);
            g.setColour(bBgCol);
            g.fillRoundedRectangle(chamber, 1.0f);

            // Vertical cathode/anode filaments inside the chamber in solid bCol
            g.setColour(bCol);
            g.fillRect(bcx - 1.7f, bcy - 1.5f, 1.0f, 3.8f);
            g.fillRect(bcx - 0.3f, bcy - 1.8f, 1.1f, 4.1f);
            g.fillRect(bcx + 1.1f, bcy - 1.5f, 0.9f, 3.8f);

            // Bottom pinch/stem + horizontal base ring & contact tip in solid bCol
            g.fillRect(bcx - 1.8f, bcy + 2.4f, 3.6f, 1.3f);
            g.fillRoundedRectangle(bcx - 3.0f, bcy + 4.2f, 6.0f, 1.5f, 0.7f);
            g.fillRect(bcx - 1.1f, bcy + 5.7f, 2.2f, 1.1f);
        }

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

            // Top-right preset dropdown triangle icon (matching Image 2 & Image 3)
            {
                juce::Path presetArrow;
                const float ax = (float)envPresetBtnRect.getCentreX();
                const float ay = (float)envPresetBtnRect.getCentreY();
                presetArrow.addTriangle(ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 2.8f);
                g.setColour(FLColors::textBright());
                g.fillPath(presetArrow);
            }

            // Dark recessed envelope graph box
            juce::Rectangle<int> envGraph(14, 106, 252, 86);
            g.setColour(FLColors::recessedBg());
            g.fillRect(envGraph);

            // Draw AHDSR curve with tension & circular nodes in target color (1:1 Image 1 for Volume vs Mod/Pitch)
            float d   = (float)envDelayKnob.getValue() * 52.0f;
            float a   = (float)envAttackKnob.getValue() * 88.0f;
            float h   = (float)envHoldKnob.getValue() * 80.0f;
            float dec = (float)envDecayKnob.getValue() * 68.0f;
            float sus = (float)envSustainKnob.getValue();
            float rel = (float)envReleaseKnob.getValue() * 72.0f;

            float x0   = (float)envGraph.getX() + 3.0f;
            float yBot = (float)envGraph.getBottom() - 2.0f;
            float yTop = (float)envGraph.getY() + 2.0f;

            // Volume envelope always renders its full green AHDSR shape (Image 1);
            // Mod X / Mod Y / Pitch render flat at bottom when disabled or AMT == 0 (Image 3).
            const bool isVolTarget = (currentInsTarget == 1);
            const float amtVal = isVolTarget ? 1.0f : (float)envAmountKnob.getValue();
            const bool isEnvActive = isVolTarget ? true : (volEnvOnBtn.getToggleState() && std::abs(amtVal) > 0.001f);

            // Draw grey reference frame box in background ONLY for Mod X / Mod Y / Pitch
            if (!isVolTarget)
            {
                g.setColour(FLColors::isWhiteSkin ? juce::Colour(0xffcbd5e1) : juce::Colour(0xff3e4652));
                const float refW = 84.0f;
                g.drawLine(x0, yBot, x0, yTop, 1.2f);
                g.drawLine(x0, yTop, x0 + refW, yTop, 1.2f);
                g.drawLine(x0 + refW, yTop, x0 + refW, yBot, 1.2f);
                g.drawLine(x0, yBot, x0 + refW + 26.0f, yBot, 1.2f);
            }

            float yPeak = isEnvActive ? (yBot - std::abs(amtVal) * (yBot - yTop)) : yBot;
            float ySus  = isEnvActive ? (yBot - (sus * std::abs(amtVal)) * (yBot - yTop)) : yBot;

            float x1 = x0 + d;
            float x2 = x1 + a;
            float x3 = x2 + h;
            float x4 = x3 + dec;
            float x5 = x4 + ((rel < 0.5f && dec < 0.5f && h > 2.0f) ? 24.0f : rel);

            const float attTens = isEnvActive ? (float)envAttTensionKnob.getValue() : 0.0f;
            const float decTens = isEnvActive ? (float)envDecTensionKnob.getValue() : 0.0f;
            const float relTens = isEnvActive ? (float)envRelTensionKnob.getValue() : 0.0f;

            juce::Path p;
            p.startNewSubPath(x0, yBot);
            p.lineTo(x1, yBot);
            if (a < 1.0f)
                p.lineTo(x2, yPeak);
            else
                p.quadraticTo((x1 + x2) * 0.5f, (yBot + yPeak) * 0.5f - attTens * 22.0f, x2, yPeak);

            p.lineTo(x3, yPeak);

            if (dec < 1.0f)
                p.lineTo(x4, ySus);
            else
                p.quadraticTo((x3 + x4) * 0.5f, (yPeak + ySus) * 0.5f - decTens * 22.0f, x4, ySus);

            if ((x5 - x4) < 1.0f || std::abs(ySus - yBot) < 0.5f)
                p.lineTo(x5, yBot);
            else
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

            {
                juce::Graphics::ScopedSaveState clipState(g);
                g.reduceClipRegion(envGraph);
                auto drawNode = [&](float nx, float ny)
                {
                    g.setColour(FLColors::recessedBg());
                    g.fillEllipse(nx - 3.5f, ny - 3.5f, 7.0f, 7.0f);
                    g.setColour(activeCol);
                    g.drawEllipse(nx - 3.5f, ny - 3.5f, 7.0f, 7.0f, 1.6f);
                };
                drawNode(x1, yBot);
                drawNode(x2, yPeak);
                drawNode(x3, yPeak);
                drawNode(x4, ySus);
                drawNode(x5, yBot);
            }

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
    skinToggleBtnRect = { 136, 9, 56, 24 }; // Placed immediately to the right of the Tab 2 Lupa icon (x=88..130)!

    masterOnBtn.setBounds(316, 10, 16, 16);
    masterPanKnob.setBounds(342, 2, 28, 28);
    masterVolKnob.setBounds(372, 2, 28, 28);
    masterPitchKnob.setBounds(406, 2, 28, 28);
    pitchRangeBox.setBounds(444, 5, 28, 22);
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
    useLoopPointsBtn.setBounds(158, 234, 98, 18);
    pingPongLoopBtn.setBounds(158, 258, 98, 18);

    // Time Stretching Panel (x=264, y=48, w=256, h=86)
    stretchPitchKnob.setBounds(272, 76, 34, 34);
    stretchMulKnob.setBounds(308, 76, 34, 34);
    stretchTimeKnob.setBounds(344, 76, 34, 34);
    stretchModeCombo.setBounds(424, 69, 90, 24);
    formantShiftKnob.setBounds(424, 98, 90, 24);

    // Precomputed Effects Panel (x=264, y=138, w=256, h=152)
    fxSubTabWrench = { 270, 141, 20, 18 };
    fxSubTabBulb   = { 292, 141, 20, 18 };

    removeDcBtn.setBounds(272, 166, 118, 18);
    reversePolBtn.setBounds(396, 166, 118, 18);
    normalizeBtn.setBounds(272, 188, 118, 18);
    fadeStereoBtn.setBounds(396, 188, 118, 18);
    reverseBtn.setBounds(272, 210, 118, 18);
    swapStereoBtn.setBounds(396, 210, 118, 18);

    smpStartKnob.setBounds(270,  236, 34, 34);
    lengthKnob.setBounds(313,    236, 34, 34);
    fadeInKnob.setBounds(354,    236, 34, 34);
    fadeOutKnob.setBounds(394,   236, 34, 34);
    crossfadeKnob.setBounds(434, 236, 34, 34);
    trimKnob.setBounds(478,      236, 34, 34);

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
    envPresetBtnRect = { 252, 84, 18, 18 };
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
    addToKeyBtn.setBounds(184, 330, 74, 18);
    fineTuneSlider.setBounds(328, 328, 96, 22);
    keybHandBtnRect    = { 482, 326, 36, 26 };
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
    if (tabRectFunc.contains(pt))
    {
        if (e.mods.isPopupMenu() || (currentSheet == 2 && !e.mods.isShiftDown()))
        {
            juce::PopupMenu m;
            m.addSectionHeader("Lupa / Quick Actions");
            m.addItem(1, "Sample settings", true, currentSheet == 0);
            m.addItem(2, "Envelope / instrument settings", true, currentSheet == 1);
            m.addItem(3, "Miscellaneous functions", true, currentSheet == 2);
            m.addSeparator();
            m.addItem(10, "Load sample file...");
            m.addItem(11, "Next sample in folder");
            m.addItem(12, "Previous sample in folder");
            m.addSeparator();

            const int curResamp = (int)std::round(audioProcessor.apvts.getRawParameterValue("resamplingQuality")->load());
            juce::PopupMenu resampMenu;
            resampMenu.addSectionHeader("Low quality / fastest");
            resampMenu.addItem(40, "2-point linear",  true, curResamp == 0);
            resampMenu.addItem(41, "6-point Hermite", true, curResamp == 1);
            resampMenu.addSeparator();
            resampMenu.addSectionHeader("Average quality");
            resampMenu.addItem(42, "16-point sinc",   true, curResamp == 2);
            resampMenu.addItem(43, "24-point sinc (Default)", true, curResamp == 3);
            resampMenu.addItem(44, "32-point sinc",   true, curResamp == 4);
            resampMenu.addSeparator();
            resampMenu.addSectionHeader("High quality / slowest");
            resampMenu.addItem(45, "64-point sinc",   true, curResamp == 5);
            resampMenu.addItem(46, "128-point sinc",  true, curResamp == 6);
            resampMenu.addItem(47, "256-point sinc",  true, curResamp == 7);
            resampMenu.addItem(48, "512-point sinc",  true, curResamp == 8);
            m.addSubMenu("FL Resampling Quality", resampMenu);

            juce::PopupMenu skinMenu;
            skinMenu.addItem(20, "SKIN: FL Studio 2026", true, FLColors::skinMode == 0);
            skinMenu.addItem(21, "SKIN: FL Studio Dark", true, FLColors::skinMode == 1);
            skinMenu.addItem(22, "SKIN: Ableton Live 12", true, FLColors::skinMode == 2);
            skinMenu.addItem(23, "SKIN: Apple Logic Pro X", true, FLColors::skinMode == 3);
            skinMenu.addItem(24, "SKIN: Avid Pro Tools", true, FLColors::skinMode == 4);
            skinMenu.addItem(25, "SKIN: MAGIX Sequoia", true, FLColors::skinMode == 5);
            skinMenu.addItem(26, "SKIN: Adobe Audition", true, FLColors::skinMode == 6);
            skinMenu.addItem(27, "SKIN: Steinberg Cubase 14", true, FLColors::skinMode == 7);
            skinMenu.addItem(28, "SKIN: Cockos REAPER 7", true, FLColors::skinMode == 8);
            skinMenu.addItem(29, "SKIN: Winamp Classic 2.x", true, FLColors::skinMode == 9);
            skinMenu.addItem(30, "SKIN: Windows 98 Classic", true, FLColors::skinMode == 10);
            skinMenu.addItem(31, "SKIN: Mac OS X Aqua", true, FLColors::skinMode == 11);
            skinMenu.addItem(32, "SKIN: BLACK Stealth", true, FLColors::skinMode == 12);
            skinMenu.addItem(33, "SKIN: WHITE Studio", true, FLColors::skinMode == 13);
            m.addSubMenu("Skin / Theme", skinMenu);

            juce::Component::SafePointer<FLSamplerAudioProcessorEditor> safeThis(this);
            m.showMenuAsync(juce::PopupMenu::Options(), [safeThis](int res)
            {
                if (safeThis == nullptr)
                    return;
                auto* self = safeThis.getComponent();
                if (res >= 1 && res <= 3)
                {
                    self->updateActiveSheet(res - 1);
                }
                else if (res == 10)
                {
                    self->openSampleFileChooser();
                }
                else if (res == 11 || res == 12)
                {
                    self->audioProcessor.stepFolderSample(res == 11 ? 1 : -1);
                    self->wavScope.refreshIfNeeded();
                    self->setHintText("Loaded: " + self->audioProcessor.getLoadedFileName());
                    self->repaint();
                }
                else if (res >= 20 && res <= 33)
                {
                    FLColors::setSkinMode(res - 20);
                    self->flLookAndFeel.refreshThemeColours();
                    self->contentCanvas.repaint();
                    self->repaint();
                }
                else if (res >= 40 && res <= 48)
                {
                    const int qIdx = res - 40;
                    if (auto* p = self->audioProcessor.apvts.getParameter("resamplingQuality"))
                        p->setValueNotifyingHost(p->convertTo0to1((float)qIdx));
                    static const char* kQualityNames[9] = {
                        "2-point linear", "6-point Hermite", "16-point sinc",
                        "24-point sinc", "32-point sinc", "64-point sinc",
                        "128-point sinc", "256-point sinc", "512-point sinc"
                    };
                    self->setHintText("FL Resampling Quality: " + juce::String(kQualityNames[qIdx]));
                    self->repaint();
                }
            });
            return;
        }
        updateActiveSheet(2);
        setHintText("Miscellaneous functions (click again or right-click for quick menu)");
        return;
    }
    if (skinToggleBtnRect.contains(pt))
    {
        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu skinMenu;
            skinMenu.addSectionHeader("Select Skin / Theme");
            skinMenu.addItem(20, "1. FL Studio 2026 (Default)", true, FLColors::skinMode == 0);
            skinMenu.addItem(21, "2. FL Studio Dark Theme", true, FLColors::skinMode == 1);
            skinMenu.addItem(22, "3. Ableton Live 12", true, FLColors::skinMode == 2);
            skinMenu.addItem(23, "4. Apple Logic Pro X", true, FLColors::skinMode == 3);
            skinMenu.addItem(24, "5. Avid Pro Tools", true, FLColors::skinMode == 4);
            skinMenu.addItem(25, "6. MAGIX Sequoia", true, FLColors::skinMode == 5);
            skinMenu.addItem(26, "7. Adobe Audition", true, FLColors::skinMode == 6);
            skinMenu.addItem(27, "8. Steinberg Cubase 14", true, FLColors::skinMode == 7);
            skinMenu.addItem(28, "9. Cockos REAPER 7", true, FLColors::skinMode == 8);
            skinMenu.addItem(29, "10. Winamp Classic 2.x", true, FLColors::skinMode == 9);
            skinMenu.addSeparator();
            skinMenu.addItem(30, "11. Microsoft Windows 98 Classic", true, FLColors::skinMode == 10);
            skinMenu.addItem(31, "12. Apple Mac OS X Aqua", true, FLColors::skinMode == 11);
            skinMenu.addSeparator();
            skinMenu.addItem(32, "13. BLACK Stealth", true, FLColors::skinMode == 12);
            skinMenu.addItem(33, "14. WHITE Studio", true, FLColors::skinMode == 13);
            juce::Component::SafePointer<FLSamplerAudioProcessorEditor> safeThis(this);
            skinMenu.showMenuAsync(juce::PopupMenu::Options(), [safeThis](int res)
            {
                if (safeThis == nullptr)
                    return;
                if (res >= 20 && res <= 33)
                {
                    FLColors::setSkinMode(res - 20);
                    safeThis->flLookAndFeel.refreshThemeColours();
                    safeThis->contentCanvas.repaint();
                    safeThis->repaint();
                }
            });
            return;
        }

        const int nextSkin = (FLColors::skinMode + 1) % FLColors::kNumSkins;
        FLColors::setSkinMode(nextSkin);
        flLookAndFeel.refreshThemeColours();
        static const char* kSkinHints[FLColors::kNumSkins] = {
            "Skin: FL Studio 2026 Look",
            "Skin: FL Studio Dark Theme",
            "Skin: Ableton Live 12 Look",
            "Skin: Apple Logic Pro X Quick Sampler",
            "Skin: Avid Pro Tools Insert Rack",
            "Skin: MAGIX Sequoia Mastering Suite",
            "Skin: Adobe Audition Spectral / Waveform Look",
            "Skin: Steinberg Cubase 14 Sampler Track",
            "Skin: Cockos REAPER 7 TCP Look",
            "Skin: Winamp Classic 2.x (Nullsoft)",
            "Skin: Microsoft Windows 98 Classic",
            "Skin: Apple Mac OS X Aqua",
            "Skin: BLACK (Pure Pitch-Black Stealth)",
            "Skin: WHITE (Pure White Studio)"
        };
        setHintText(kSkinHints[juce::jlimit(0, FLColors::kNumSkins - 1, nextSkin)]);
        contentCanvas.repaint();
        repaint();
        return;
    }

    if (currentSheet == 0)
    {
        if (openFileBtnRect.contains(pt) || fileNameBoxRect.contains(pt))
        {
            const auto recentFiles = audioProcessor.getRecentSamplePaths();
            if (e.mods.isPopupMenu() && !recentFiles.isEmpty())
            {
                juce::PopupMenu m;
                m.addSectionHeader("Recent Sample Files");
                for (int i = 0; i < recentFiles.size(); ++i)
                    m.addItem(100 + i, juce::File(recentFiles[i]).getFileName());
                juce::Component::SafePointer<FLSamplerAudioProcessorEditor> safeThis(this);
                m.showMenuAsync(juce::PopupMenu::Options(), [safeThis, recentFiles](int res)
                {
                    if (safeThis == nullptr)
                        return;
                    if (res >= 100)
                    {
                        int idx = res - 100;
                        if (idx >= 0 && idx < recentFiles.size())
                        {
                            juce::File f(recentFiles[idx]);
                            if (f.existsAsFile())
                            {
                                safeThis->audioProcessor.loadSampleFromFile(f);
                                safeThis->wavScope.refreshIfNeeded();
                                safeThis->repaint();
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
                if (currentInsTarget == 4)
                    audioProcessor.lfoShapeIdx.store(s, std::memory_order_relaxed);
                audioProcessor.targetLfos[(size_t)currentInsTarget].shape = s;
                setHintText(s == 0 ? "LFO shape: Sine" : (s == 1 ? "LFO shape: Triangle" : "LFO shape: Pulse"));
                repaint();
                return;
            }
        }

        if (currentInsTarget != 0 && envPresetBtnRect.contains(pt))
        {
            showEnvelopePresetMenu();
            return;
        }

        // Interactive AHDSR Graph Node Dragging
        juce::Rectangle<int> envGraph(14, 106, 252, 86);
        if (currentInsTarget != 0 && envGraph.contains(pt))
        {
            if (e.mods.isPopupMenu())
            {
                showEnvelopePresetMenu();
                return;
            }

            float d   = (float)envDelayKnob.getValue() * 52.0f;
            float a   = (float)envAttackKnob.getValue() * 88.0f;
            float h   = (float)envHoldKnob.getValue() * 80.0f;
            float dec = (float)envDecayKnob.getValue() * 68.0f;
            float rel = (float)envReleaseKnob.getValue() * 72.0f;

            float x0 = (float)envGraph.getX() + 3.0f;
            float x1 = x0 + d;
            float x2 = x1 + a;
            float x3 = x2 + h;
            float x4 = x3 + dec;
            float x5 = x4 + ((rel < 0.5f && dec < 0.5f && h > 2.0f) ? 24.0f : rel);

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
                if (currentInsTarget >= 2 && std::abs(envAmountKnob.getValue()) < 0.01)
                    envAmountKnob.setValue(1.0, juce::sendNotificationSync);
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
                    param->setValueNotifyingHost(param->convertTo0to1((float)i));
                repaint();
                return;
            }
        }
    }

    // Shared Root Note, Key Region Bar & Piano Keyboard (Sheets 1 & 2)
    if (currentSheet == 1 || currentSheet == 2)
    {
        if (keybHandBtnRect.contains(pt))
        {
            zoomedKeyboardMode = !zoomedKeyboardMode;
            setHintText(zoomedKeyboardMode ? "Touch / Large keyboard view (B3..D6)"
                                           : "Standard overview keyboard (C2..C8)");
            repaint();
            return;
        }

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
        if (!zoomedKeyboardMode && scrollBar.contains(pt))
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

        const juce::Rectangle<int> activeKeybBounds = zoomedKeyboardMode
            ? bottomKeybRect.withHeight(bottomKeybRect.getHeight() + 12)
            : bottomKeybRect;

        if (activeKeybBounds.contains(pt))
        {
            const int startMidi    = zoomedKeyboardMode ? 47 : 24; // B3..D6 (17 white keys) vs C2..C8 (43 white keys)
            const int endMidi      = zoomedKeyboardMode ? 74 : 96;
            const int numWhiteKeys = zoomedKeyboardMode ? 17 : 43;
            const float relX = (float)(pt.x - activeKeybBounds.getX()) / (float)activeKeybBounds.getWidth();
            const float relY = (float)(pt.y - activeKeybBounds.getY()) / (float)activeKeybBounds.getHeight();

            auto isBlackKey = [](int m) {
                int pc = m % 12;
                return (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
            };

            int midi = 60;
            bool hitBlack = false;
            const float blackKeyMaxRelY = zoomedKeyboardMode ? 0.48f : 0.65f;
            if (relY <= blackKeyMaxRelY)
            {
                const float whiteW = (float)activeKeybBounds.getWidth() / (float)numWhiteKeys;
                const float kbX = (float)activeKeybBounds.getX();
                const float bw = whiteW * (zoomedKeyboardMode ? 0.68f : 0.70f);

                if (zoomedKeyboardMode && pt.x <= (int)(kbX + bw * 0.45f))
                {
                    midi = 46; // Left partial A#3 black key
                    hitBlack = true;
                }
                else if (zoomedKeyboardMode && pt.x >= (int)(kbX + numWhiteKeys * whiteW - bw * 0.45f))
                {
                    midi = 75; // Right partial D#6 black key
                    hitBlack = true;
                }
                else
                {
                    int wIdx = 0;
                    for (int m = startMidi; m <= endMidi; ++m)
                    {
                        if (!isBlackKey(m))
                        {
                            ++wIdx;
                            continue;
                        }
                        float bx = kbX + ((float)wIdx - (zoomedKeyboardMode ? 0.34f : 0.35f)) * whiteW;
                        if (pt.x >= (int)bx && pt.x <= (int)(bx + bw))
                        {
                            midi = m;
                            hitBlack = true;
                            break;
                        }
                    }
                }
            }

            if (!hitBlack)
            {
                int wTarget = juce::jlimit(0, numWhiteKeys - 1, (int)(relX * numWhiteKeys));
                int wCount = 0;
                for (int m = startMidi; m <= endMidi; ++m)
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
