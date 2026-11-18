#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/FLLookAndFeel.h"
#include "UI/WAVScopeComponent.h"

class FLSamplerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      public juce::FileDragAndDropTarget,
                                      private juce::Timer
{
public:
    explicit FLSamplerAudioProcessorEditor(FLSamplerAudioProcessor&);
    ~FLSamplerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    static constexpr int kDesignW = 526;
    static constexpr int kDesignH = 456;

    void timerCallback() override;
    void updateActiveSheet(int sheetIdx);
    void updatePrecomputedSubTab(bool showLegacy);
    void updateInsTarget(int targetIdx);
    void openSampleFileChooser();
    void setupKnob(fls::FLSlider& s, const juce::String& hintName, double defaultVal,
                   bool bipolar = false, bool greenRing = false, bool orangeRing = false);
    void drawRootNoteAndKeyboardPanel(juce::Graphics& g);
    void setHintText(const juce::String& text);

    FLSamplerAudioProcessor& audioProcessor;
    fls::FLLookAndFeel flLookAndFeel;

    // Resizable fixed-526x456 content canvas scaled proportionally on resize!
    juce::Component contentCanvas;
    juce::String currentHintText;
    bool isWindowDragHover = false;

    // Active Sheet (0 = Sample settings, 1 = Envelope / instrument, 2 = Misc functions)
    int currentSheet = 0;
    bool legacyPrecomputedTab = false;
    int currentInsTarget = 1; // 0=Panning, 1=Volume (default), 2=Mod X, 3=Mod Y, 4=Pitch
    int currentLfoShape  = 0; // 0=Sine, 1=Triangle, 2=Pulse
    bool trackingKeyMode = true; // false=Vol, true=Key

    // Top Header Bar Controls
    fls::FLLedCheckButton masterOnBtn { "", true };
    fls::FLSlider masterPanKnob, masterVolKnob, masterPitchKnob;
    fls::FLDigiWheel pitchRangeBox { 1, 48, 2, false, false };
    fls::FLDigiWheel trackBox      { 0, 125, 0, true, false };

    // Header Tab Areas (Waveform, Envelope, Wrench) & Precomputed Sub-Tab Icons (Wrench, Bulb)
    juce::Rectangle<int> tabRectWave, tabRectEnv, tabRectFunc, skinToggleBtnRect;
    juce::Rectangle<int> fxSubTabWrench, fxSubTabBulb;
    juce::Rectangle<int> openFileBtnRect, locateFileBtnRect, removeFileBtnRect, fileNameBoxRect;
    juce::Rectangle<int> bottomKeybRect, resetRootBtnRect, reverbSwitchRect;
    juce::Rectangle<int> insTargetRects[5];
    juce::Rectangle<int> lfoShapeRects[3];
    juce::Rectangle<int> arpDirIconRects[6];
    juce::Rectangle<int> trackVolTabRect, trackKeyTabRect;

    // Tab 1: Sample Settings Controls
    fls::FLLedCheckButton keepOnDiskBtn     { "Keep on disk" };
    fls::FLLedCheckButton resampleBtn       { "Resample", true };
    fls::FLLedCheckButton loadRegionsBtn    { "Load regions", true };
    fls::FLLedCheckButton loadSlicesBtn     { "Load slice markers" };
    fls::FLComboBox       declickCombo;

    // Playback Panel
    fls::FLSlider         startOffsetKnob, loopStartKnob, loopEndKnob;
    fls::FLLedCheckButton useLoopPointsBtn  { "Use loop points" };
    fls::FLLedCheckButton pingPongLoopBtn   { "Ping pong loop" };

    // Time Stretching Panel
    fls::FLSlider         stretchPitchKnob, stretchMulKnob, stretchTimeKnob, formantShiftKnob;
    fls::FLComboBox       stretchModeCombo;

    // Precomputed Effects Panel (Standard - Wrench sub-tab)
    fls::FLLedCheckButton removeDcBtn       { "Remove DC offset" };
    fls::FLLedCheckButton normalizeBtn      { "Normalize" };
    fls::FLLedCheckButton reverseBtn        { "Reverse" };
    fls::FLLedCheckButton reversePolBtn     { "Reverse polarity" };
    fls::FLLedCheckButton fadeStereoBtn     { "Fade stereo" };
    fls::FLLedCheckButton swapStereoBtn     { "Swap stereo" };
    fls::FLSlider         smpStartKnob, lengthKnob, fadeInKnob, fadeOutKnob, crossfadeKnob, trimKnob;

    // Precomputed Effects Panel (Legacy - Bulb sub-tab)
    fls::FLSlider         boostKnob, eqKnob, rmMixKnob, rmFreqKnob, cutKnob, resKnob, reverbKnob, stereoDelayKnob, pogoKnob;
    fls::FLLedCheckButton clipBtn           { "CLIP" };

    // Bottom WAVScope
    fls::WAVScopeComponent wavScope;

    // Tab 2: Envelope / Instrument Settings (InsSheet)
    fls::FLLedCheckButton volEnvOnBtn       { "Envelope" };
    fls::FLSlider         envDelayKnob, envAttackKnob, envHoldKnob, envDecayKnob, envSustainKnob, envReleaseKnob, envAmountKnob;
    fls::FLSlider         envAttTensionKnob, envDecTensionKnob, envRelTensionKnob;
    fls::FLLedCheckButton envTempoBtn       { "Tempo" };
    fls::FLSlider         lfoDelayKnob, lfoAttKnob, lfoAmtKnob, lfoSpeedKnob;
    fls::FLLedCheckButton lfoTempoBtn       { "Tempo" };
    fls::FLLedCheckButton lfoGlobalBtn      { "Global" };
    fls::FLSlider         filterModXKnob, filterModYKnob;
    fls::FLComboBox       filterTypeCombo;

    // Shared Bottom Panel for Tab 2 & Tab 3: Root note + Fine tune horizontal slider + C2..C8 keyboard
    fls::FLLedCheckButton enableMainPitchBtn{ "Enable main pitch", true };
    fls::FLLedCheckButton addToKeyBtn       { "Add to key" };
    fls::FLSlider         fineTuneSlider;

    // Tab 3: Miscellaneous Functions (FuncSheet)
    // Levels adjustment
    fls::FLSlider         lvlPanKnob, lvlVolKnob, lvlModXKnob, lvlModYKnob;
    // Polyphony
    fls::FLDigiWheel      maxPolyBox        { 0, 32, 0, true, true };
    fls::FLSlider         portaTimeKnob;
    fls::FLLedCheckButton portaBtn          { "Porta" };
    fls::FLLedCheckButton monoBtn           { "Mono" };
    // Time
    fls::FLSlider         gateTimeKnob, shiftKnob, swingKnob;
    fls::FLLedCheckButton fullPortaBtn      { "Full porta" };
    // Tracking
    fls::FLSlider         trackPanKnob, trackModXKnob, trackModYKnob, trackMidSlider;
    // Group
    fls::FLDigiWheel      cutGroupBox       { 0, 99, 0, true, true };
    fls::FLDigiWheel      cutByBox          { 0, 99, 0, true, true };
    juce::TextButton      cutSelfPushBtn    { "Cut self" };
    // Arpeggiator
    fls::FLSlider         arpTimeKnob, arpGateKnob;
    fls::FLDigiWheel      arpRangeBox       { 1, 8, 1, false, true };
    fls::FLDigiWheel      arpRepeatBox      { 0, 8, 0, true, true };
    fls::FLLedCheckButton arpSlideBtn       { "Slide" };
    fls::FLComboBox       arpChordCombo;
    // Echo delay / fat mode
    fls::FLSlider         echoFeedKnob, echoPanKnob, echoModXKnob, echoModYKnob, echoPitchKnob, echoTimeKnob;
    fls::FLDigiWheel      echoCountBox      { 0, 16, 4, false, true };
    fls::FLLedCheckButton echoPingPongBtn   { "Ping pong" };
    fls::FLLedCheckButton echoFatModeBtn    { "Fat mode" };

    // APVTS Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>>  comboAttachments;

    std::unique_ptr<juce::FileChooser> fileChooser;
    int previewHeldMidiNote = -1;
    int draggingEnvNodeIdx  = -1; // -1=none, 0=Delay, 1=Attack, 2=Hold, 3=Decay/Sustain, 4=Release
    int draggingKeyRegionEdge = 0; // 0=none, 1=low, 2=high

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLSamplerAudioProcessorEditor)
};
