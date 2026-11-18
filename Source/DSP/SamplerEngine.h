#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>

namespace fls
{

// Lock-free inter-instance Choke Bus shared across all open FLSampler VST3 instances in the DAW!
// Allows linking multiple instances via the header TRACK box (1..125) or FuncSheet Group CUT / BY (1..99).
struct GlobalChokeBus
{
    static constexpr int kMaxGroups = 128;
    std::array<std::atomic<uint32_t>, kMaxGroups> groupEpoch {};
    std::array<std::atomic<uintptr_t>, kMaxGroups> lastTriggerOwner {};

    static GlobalChokeBus& instance() noexcept
    {
        static GlobalChokeBus bus;
        return bus;
    }

    void notifyTriggered(int group, uintptr_t ownerId) noexcept
    {
        if (group > 0 && group < kMaxGroups)
        {
            lastTriggerOwner[(size_t)group].store(ownerId, std::memory_order_relaxed);
            groupEpoch[(size_t)group].fetch_add(1, std::memory_order_release);
        }
    }
};

struct AHDSREnvelope
{
    bool  enabled = false;
    float delay   = 0.0f; // 0..1 (0..2s)
    float attack  = 0.0f; // 0..1 (0..4s)
    float hold    = 0.0f; // 0..1 (0..4s)
    float decay   = 0.5f; // 0..1 (0..4s)
    float sustain = 1.0f; // 0..1
    float release = 0.2f; // 0..1 (0..4s)
    float amount  = 0.0f; // -1..+1 (for Pitch/ModX/ModY/Pan)
    float attTension = 0.0f;  // -1..+1
    float decTension = -0.35f;// -1..+1
    float relTension = -0.45f;// -1..+1
    bool  tempoSync  = false;
};

struct LFOModulator
{
    float delay  = 0.0f; // 0..1
    float attack = 0.0f; // 0..1
    float amount = 0.0f; // -1..+1
    float speed  = 0.5f; // 0..1 (0.1 Hz .. 20 Hz)
    int   shape  = 0;    // 0 = Sine, 1 = Triangle, 2 = Pulse
    bool  tempoSync = true;
    bool  globalPhase = false;
};

// Tension curve shaper matching FL Studio's tension knob (-1..+1)
inline float applyFLTension(float t, float tension) noexcept
{
    t = juce::jlimit(0.0f, 1.0f, t);
    if (std::abs(tension) < 0.005f)
        return t;
    const float expFactor = std::pow(6.0f, -tension);
    return std::pow(t, expFactor);
}

struct SamplerRuntimeParams
{
    bool  masterOn       = true;
    float masterPan      = 0.0f; // -1..+1
    float masterVol      = 0.78f; // 0..1 (default 78% = 0dB in FL Channel Rack)
    float masterPitch    = 0.0f; // -1..+1 (scaled by pitchRange)
    int   pitchRange     = 2;    // 1..48 semitones
    int   chokeLinkTrack = 0;    // 0 = off ("---"), 1..125 = Cross-instance Choke Link Group
    double hostBpm       = 120.0;
    double hostPpqPos    = 0.0;
    double embeddedLoopBpm = 0.0;

    // MIDI Live Controllers
    float pitchWheelNorm = 0.0f; // -1..+1
    float modWheelNorm   = 0.0f; // 0..1
    float aftertouchNorm = 0.0f; // 0..1

    // Playback / Loop
    float startOffset    = 0.0f; // 0..1
    float loopStart      = 0.0f; // 0..1
    float loopEnd        = 1.0f; // 0..1
    bool  useLoopPoints  = false;
    bool  pingPongLoop   = false;
    int   declickMode    = 0;    // 0..5

    // Time Stretching
    bool  resampleOn     = true;
    float stretchPitch   = 0.0f; // -1200..+1200 cents
    float stretchMul     = 1.0f; // 0.25..4.0
    float stretchTime    = 0.0f; // 0 = off, >0 = target seconds
    int   stretchMode    = 0;    // 0 = Resample, 1..11 = Stretch modes
    float formantShift   = 0.0f; // -12..+12 semitones (Stretch Pro)

    // Instrument / Filter (InsSheet)
    int   rootNote       = 60;   // C5 in FL Studio (MIDI 60)
    int   keyRegionLow   = 0;    // Active MIDI key range low (0..127)
    int   keyRegionHigh  = 127;  // Active MIDI key range high (0..127)
    float fineTuneCents  = 0.0f; // -100..+100 cents
    bool  enableMainPitch = true;
    bool  addToKey       = false;

    float filterModX     = 1.0f; // 0..1 Cutoff
    float filterModY     = 0.0f; // 0..1 Resonance
    int   filterType     = 0;    // 0=Fast LP, 1=LP, 2=BP, 3=HP, 4=BS, 5=SVF LPx2

    // All 5 InsSheet Modulation Targets (0=Pan, 1=Vol, 2=ModX, 3=ModY, 4=Pitch)
    std::array<AHDSREnvelope, 5> targetEnvs {{
        { false, 0.0f, 0.05f, 0.0f, 0.30f, 0.0f, 0.20f, 0.0f, 0.0f, -0.35f, -0.45f }, // Pan
        { false, 0.0f, 0.02f, 0.0f, 0.35f, 0.8f, 0.20f, 1.0f, 0.0f, -0.35f, -0.45f }, // Vol
        { false, 0.0f, 0.05f, 0.0f, 0.30f, 0.0f, 0.20f, 0.0f, 0.0f, -0.35f, -0.45f }, // Mod X
        { false, 0.0f, 0.05f, 0.0f, 0.30f, 0.0f, 0.20f, 0.0f, 0.0f, -0.35f, -0.45f }, // Mod Y
        { false, 0.0f, 0.02f, 0.0f, 0.20f, 0.0f, 0.15f, 0.0f, 0.0f, -0.35f, -0.45f }  // Pitch
    }};
    std::array<LFOModulator, 5> targetLfos {};

    // Legacy aliases for Vol/Pitch
    AHDSREnvelope volEnv { false, 0.0f, 0.02f, 0.0f, 0.35f, 0.8f, 0.20f, 1.0f };
    LFOModulator  pitchLfo;

    // Misc Functions (FuncSheet)
    float volOffset      = 0.78f; // 0..1 (default 0.78 = unity)
    float panOffset      = 0.0f; // -1..+1
    float modXOffset     = 0.0f; // -1..+1
    float modYOffset     = 0.0f; // -1..+1
    int   maxPoly        = 16;   // 1..32
    bool  porta          = false;
    bool  mono           = false;
    bool  fullPorta      = false;
    float portaTime      = 0.08f; // 0..1
    bool  cutSelf        = false;
    int   cutGroup       = 0;    // 0..99 (Group Cut)
    int   cutByGroup     = 0;    // 0..99 (Group Cut By)
    float gateTime       = 1.0f; // 0..1 (1.0 = disabled)
    float shiftTime      = 0.0f; // 0..1 (0..200ms note delay)
    float swingMul       = 1.0f; // 0..1

    // Tracking (Vol vs Key)
    bool  trackingKeyMode = false; // false = Velocity (Vol) tracking, true = Key tracking
    float trackPan       = 0.0f; // -1..+1
    float trackModX      = 0.0f; // -1..+1
    float trackModY      = 0.0f; // -1..+1
    int   trackMidNote   = 60;   // 0..127

    // Arpeggiator
    int   arpDir         = 0;    // 0=Off, 1=Up, 2=Down, 3=Up-Down, 4=Sticky, 5=Random
    float arpTime        = 0.125f; // seconds per arp step
    float arpGate        = 0.80f;
    int   arpRange       = 1;    // 1..4 octaves
    int   arpRepeat      = 1;    // 1..8 repeats per note
    bool  arpSlide       = false;
    int   arpChord       = 0;    // 0=Auto, 1=Auto(sustain), 2=None, 3=Major, 4=Minor, 5=Sus2, 6=Sus4

    // Echo Delay / Fat mode
    int   echoCount      = 0;    // 0..10
    float echoFeed       = 0.5f; // 0..1
    float echoTime       = 0.18f;// seconds
    float echoPan        = 0.0f; // -1..+1
    float echoModX       = 0.5f; // 0..1
    float echoModY       = 0.5f; // 0..1
    float echoPitch      = 0.0f; // semitones per echo
    bool  echoPingPong   = false;
    bool  echoFatMode    = false;

    // Slice Markers (normalized 0..1)
    const std::vector<float>* slicePositions = nullptr;
};

class SamplerVoice
{
public:
    enum class EnvStage { Idle, Delay, Attack, Hold, Decay, Sustain, Release };

    struct EnvRuntimeState
    {
        EnvStage stage = EnvStage::Idle;
        double   stageTime = 0.0;
        float    level = 0.0f;
        float    releaseStartLevel = 0.0f;
    };

    bool  active       = false;
    int   midiNote     = 60;
    int   midiChannel  = 1;     // Supports MPE per-channel note expression & FL Slide Notes
    float mpePitchBend = 0.0f;  // Per-note MPE pitch bend in semitones
    float velocity     = 1.0f;
    double position    = 0.0;
    double sliceEndIdx = -1.0; // >0 when Slice Map plays a single slice
    double wsolaOffsetA = 0.0; // WSOLA correlation-aligned grain offset A
    double wsolaOffsetB = 0.0; // WSOLA correlation-aligned grain offset B
    double grainPosB   = 0.0;
    double grainPhase  = 0.0;
    int   loopDir      = 1; // +1 forward, -1 backward (ping-pong)
    double curNoteSemi = 60.0;
    double targetSemi  = 60.0;
    double noteAgeSec  = 0.0;
    int    shiftCountdownSamples = 0; // Time SHIFT sample-accurate delay before playback starts

    // 5 independent AHDSR states: 0=Pan, 1=Vol, 2=ModX, 3=ModY, 4=Pitch
    std::array<EnvRuntimeState, 5> envs {};

    // Attack & Release Declick ramps (for all 6 FL Declicking modes)
    float attDeclickLevel = 1.0f;
    float attDeclickStep  = 1.0f;
    bool  declicking   = false;
    float declickLevel = 1.0f;
    float declickStep  = 0.002f;

    // Biquad SVF state per channel (Stage 1 + Stage 2 for 24dB/oct SVF LPx2) + 2x Oversampling memory
    float z1L = 0.0f, z2L = 0.0f, z3L = 0.0f, z4L = 0.0f;
    float z1R = 0.0f, z2R = 0.0f, z3R = 0.0f, z4R = 0.0f;
    float prevInL = 0.0f, prevInR = 0.0f;
    float formLpL = 0.0f, formLpR = 0.0f;

    static double getDeclickDurationSec(int declickMode, bool isAttack) noexcept
    {
        switch (declickMode)
        {
            case 0: return isAttack ? 0.0 : 0.0006;    // Out only (no bleeding)
            case 1: return isAttack ? 0.0005 : 0.0025; // Transient (no bleeding)
            case 2: return isAttack ? 0.0015 : 0.010;  // Transient (bleeding)
            case 3: return isAttack ? 0.006  : 0.025;  // Generic (bleeding)
            case 4: return isAttack ? 0.025  : 0.100;  // Smooth (bleeding)
            case 5: return isAttack ? 0.060  : 0.250;  // Crossfade (bleeding)
            default: return 0.002;
        }
    }

    void start(int note, float vel, double startSampleIdx, double prevNoteSemi, bool usePorta,
               int declickMode, double sr, int shiftSamples = 0, double sliceEnd = -1.0, int channel = 1)
    {
        active       = true;
        midiNote     = note;
        midiChannel  = channel;
        mpePitchBend = 0.0f;
        velocity     = vel;
        position     = startSampleIdx;
        sliceEndIdx  = sliceEnd;
        wsolaOffsetA = 0.0;
        wsolaOffsetB = 0.0;
        grainPosB    = startSampleIdx;
        grainPhase   = 0.0;
        loopDir      = 1;
        targetSemi   = (double)note;
        curNoteSemi  = usePorta ? prevNoteSemi : targetSemi;
        noteAgeSec   = 0.0;
        shiftCountdownSamples = juce::jmax(0, shiftSamples);

        for (int idx = 0; idx < 5; ++idx)
        {
            envs[(size_t)idx].stage = EnvStage::Delay;
            envs[(size_t)idx].stageTime = 0.0;
            envs[(size_t)idx].level = (idx == 1) ? 1.0f : 0.0f;
            envs[(size_t)idx].releaseStartLevel = envs[(size_t)idx].level;
        }

        double attSec = getDeclickDurationSec(declickMode, true);
        if (attSec > 0.0001 && startSampleIdx > 4.0)
        {
            attDeclickLevel = 0.0f;
            attDeclickStep  = (float)(1.0 / juce::jmax(2.0, attSec * sr));
        }
        else
        {
            attDeclickLevel = 1.0f;
            attDeclickStep  = 1.0f;
        }

        declicking   = false;
        declickLevel = 1.0f;
        z1L = z2L = z3L = z4L = 0.0f;
        z1R = z2R = z3R = z4R = 0.0f;
        prevInL = prevInR = 0.0f;
        formLpL = formLpR = 0.0f;
    }

    void triggerRelease(int declickMode, double sr, bool volEnvEnabled)
    {
        if (!active)
            return;

        for (int idx = 0; idx < 5; ++idx)
        {
            auto& es = envs[(size_t)idx];
            if (es.stage != EnvStage::Release)
            {
                es.releaseStartLevel = es.level;
                es.stage = EnvStage::Release;
                es.stageTime = 0.0;
            }
        }

        if (!volEnvEnabled)
        {
            double declickSec = getDeclickDurationSec(declickMode, false);
            declicking = true;
            declickStep = (float)(1.0 / juce::jmax(4.0, declickSec * sr));
        }
    }

    static float advanceAHDSR(EnvRuntimeState& es, const AHDSREnvelope& cfg, double dt, bool isVolumeEnv, bool& voiceStillActive) noexcept
    {
        if (!cfg.enabled)
        {
            es.level = isVolumeEnv ? 1.0f : 0.0f;
            return es.level;
        }

        es.stageTime += dt;
        switch (es.stage)
        {
            case EnvStage::Delay:
            {
                const double d = std::pow((double)cfg.delay, 2.0) * 2.0;
                es.level = 0.0f;
                if (es.stageTime >= d) { es.stage = EnvStage::Attack; es.stageTime = 0.0; }
                break;
            }
            case EnvStage::Attack:
            {
                const double a = std::pow((double)cfg.attack, 2.0) * 3.0;
                if (a <= 0.0005) { es.level = 1.0f; es.stage = EnvStage::Hold; es.stageTime = 0.0; }
                else
                {
                    float t = (float)juce::jmin(1.0, es.stageTime / a);
                    es.level = applyFLTension(t, cfg.attTension);
                    if (t >= 1.0f) { es.stage = EnvStage::Hold; es.stageTime = 0.0; }
                }
                break;
            }
            case EnvStage::Hold:
            {
                const double h = std::pow((double)cfg.hold, 2.0) * 3.0;
                es.level = 1.0f;
                if (es.stageTime >= h) { es.stage = EnvStage::Decay; es.stageTime = 0.0; }
                break;
            }
            case EnvStage::Decay:
            {
                const double dec = std::pow((double)cfg.decay, 2.0) * 4.0;
                if (dec <= 0.001) { es.level = cfg.sustain; es.stage = EnvStage::Sustain; }
                else
                {
                    float t = (float)juce::jmin(1.0, es.stageTime / dec);
                    float shaped = applyFLTension(t, cfg.decTension);
                    es.level = 1.0f + (cfg.sustain - 1.0f) * shaped;
                    if (t >= 1.0f) es.stage = EnvStage::Sustain;
                }
                break;
            }
            case EnvStage::Sustain:
                es.level = cfg.sustain;
                break;
            case EnvStage::Release:
            {
                const double rel = juce::jmax(0.002, std::pow((double)cfg.release, 2.0) * 4.0);
                float t = (float)juce::jmin(1.0, es.stageTime / rel);
                float shaped = applyFLTension(t, cfg.relTension);
                es.level = es.releaseStartLevel * (1.0f - shaped);
                if (t >= 1.0f && isVolumeEnv)
                    voiceStillActive = false;
                break;
            }
            default: break;
        }
        return es.level;
    }

    static double evalLFO(const LFOModulator& lfo, double noteAgeSec, double globalClockSec, double hostBpm) noexcept
    {
        if (std::abs(lfo.amount) <= 0.001f)
            return 0.0;

        const double lfoDelaySec = std::pow((double)lfo.delay, 2.0) * 1.5;
        if (noteAgeSec < lfoDelaySec)
            return 0.0;

        const double activeSec = noteAgeSec - lfoDelaySec;
        const double lfoAttSec = std::pow((double)lfo.attack, 2.0) * 2.0;
        const double lfoEnv = (lfoAttSec > 0.001) ? juce::jmin(1.0, activeSec / lfoAttSec) : 1.0;

        double lfoHz = 0.2 * std::pow(80.0, (double)lfo.speed);
        if (lfo.tempoSync && hostBpm > 20.0)
        {
            // Quantize LFO rate to 16 musical subdivisions of Host BPM (4 Bars .. 1/64, including Dotted & Triplets)
            static constexpr double kCyclesPerBeat[16] = {
                0.0625, 0.125, 0.25, 0.375, 0.5, 0.6667, 0.75, 1.0,
                1.3333, 1.5,   2.0,  2.6667, 3.0, 4.0,   6.0,  8.0
            };
            int idx = juce::jlimit(0, 15, (int)std::round(lfo.speed * 15.0f));
            lfoHz = (hostBpm / 60.0) * kCyclesPerBeat[idx];
        }

        const double timeRef = lfo.globalPhase ? globalClockSec : activeSec;
        const double ph = std::fmod(juce::jmax(0.0, timeRef * lfoHz), 1.0);
        double val = 0.0;
        if (lfo.shape == 1) // Triangle
            val = (ph < 0.5) ? (ph * 4.0 - 1.0) : (3.0 - ph * 4.0);
        else if (lfo.shape == 2) // Pulse
            val = (ph < 0.5) ? 1.0 : -1.0;
        else // Sine
            val = std::sin(juce::MathConstants<double>::twoPi * ph);

        return val * lfoEnv * (double)lfo.amount;
    }
};

class SamplerEngine
{
public:
    static constexpr int kMaxVoices = 32;

    void prepare(double sampleRate, int maxBlockSize)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        echoBuffer.setSize(2, (int)(sr * 4.0) + maxBlockSize);
        echoBuffer.clear();
        echoWritePos = 0;
        resetAllVoices();
    }

    void resetAllVoices()
    {
        for (auto& v : voices)
            v.active = false;
        heldNotes.clear();
    }

    static const char* getMusicalDivisionName(float norm0to1) noexcept
    {
        static constexpr const char* kNames[16] = {
            "4 Bars", "2 Bars", "1 Bar", "1/2D", "1/2", "1/2T", "1/4D", "1/4",
            "1/4T",   "1/8D",   "1/8",   "1/8T", "1/16D","1/16", "1/16T","1/32"
        };
        return kNames[juce::jlimit(0, 15, (int)std::round(norm0to1 * 15.0f))];
    }

    void setChannelPitchBend(int midiChannel, float normBend, int bendRangeSemitones) noexcept
    {
        if (midiChannel <= 1)
            return;
        for (auto& v : voices)
        {
            if (v.active && v.midiChannel == midiChannel)
                v.mpePitchBend = normBend * (float)bendRangeSemitones;
        }
    }

    void noteOn(int note, float velocity, const juce::AudioBuffer<float>& sampleBuf, const SamplerRuntimeParams& p,
                float customStartNorm = -1.0f, bool isArpGenerated = false, int midiChannel = 1)
    {
        if (sampleBuf.getNumSamples() <= 1)
            return;

        // FL Studio Slide Note / Legato MPE: if a note arrives on MIDI Channel >= 2 while another note is held,
        // or when Porta + Mono are active and a note is already playing, glide the active voice in-place!
        if (!isArpGenerated && customStartNorm < 0.0f && (midiChannel >= 2 || (p.porta && p.mono)))
        {
            for (auto& v : voices)
            {
                if (v.active && !v.declicking)
                {
                    v.targetSemi = (double)note;
                    if (midiChannel == 1)
                        v.midiNote = note;
                    lastTriggeredNote = (double)note;
                    if (std::find(heldNotes.begin(), heldNotes.end(), note) == heldNotes.end())
                        heldNotes.push_back(note);
                    return;
                }
            }
        }

        // Respect Key Region bounds (unless triggered as a waveform scrub preview)
        if (customStartNorm < 0.0f && (note < p.keyRegionLow || note > p.keyRegionHigh))
            return;

        if (!isArpGenerated && customStartNorm < 0.0f)
        {
            if (std::find(heldNotes.begin(), heldNotes.end(), note) == heldNotes.end())
                heldNotes.push_back(note);
        }

        // 1. Broadcast cross-instance choke on Header TRACK (1..125) and FuncSheet Group CUT (1..99)
        auto& bus = GlobalChokeBus::instance();
        const uintptr_t myId = reinterpret_cast<uintptr_t>(this);
        if (p.chokeLinkTrack > 0 && p.chokeLinkTrack < GlobalChokeBus::kMaxGroups)
        {
            bus.notifyTriggered(p.chokeLinkTrack, myId);
            lastSeenTrackEpoch = bus.groupEpoch[(size_t)p.chokeLinkTrack].load(std::memory_order_acquire);
        }
        if (p.cutGroup > 0 && p.cutGroup < GlobalChokeBus::kMaxGroups)
        {
            bus.notifyTriggered(p.cutGroup, myId);
            if (p.cutByGroup == p.cutGroup)
                lastSeenCutByEpoch = bus.groupEpoch[(size_t)p.cutGroup].load(std::memory_order_acquire);
        }

        // 2. Self-choke if Mono, Cut Self, MaxPoly == 1, or Cut == CutBy (>0)
        const bool selfGroupCut = (p.cutGroup > 0 && p.cutGroup == p.cutByGroup);
        if (p.mono || p.cutSelf || selfGroupCut || p.maxPoly <= 1)
        {
            for (auto& v : voices)
                if (v.active)
                    v.triggerRelease(p.declickMode, sr, false);
        }

        // Count active voices & steal oldest if exceeding maxPoly
        int activeCount = 0;
        int oldestIdx = -1;
        double maxAge = -1.0;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices[i].active && !voices[i].declicking)
            {
                ++activeCount;
                if (voices[i].noteAgeSec > maxAge)
                {
                    maxAge = voices[i].noteAgeSec;
                    oldestIdx = i;
                }
            }
        }
        if (activeCount >= juce::jmax(1, p.maxPoly) && oldestIdx >= 0)
            voices[oldestIdx].triggerRelease(p.declickMode, sr, false);

        // Find free slot
        int slot = 0;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].active)
            {
                slot = i;
                break;
            }
        }

        const int len = sampleBuf.getNumSamples();
        const float startNorm = (customStartNorm >= 0.0f) ? customStartNorm : p.startOffset;
        double startIdx = juce::jlimit(0.0, (double)(len - 1), (double)startNorm * (double)(len - 1));
        double sliceEnd = -1.0;
        int effectiveNote = note;

        // Slice Map (mode 8) & Slice Stretch (mode 9): MIDI notes starting at C5 (60) trigger individual slices!
        if (customStartNorm < 0.0f && (p.stretchMode == 8 || p.stretchMode == 9) && p.slicePositions != nullptr && !p.slicePositions->empty())
        {
            const auto& slices = *p.slicePositions;
            const int sliceIdx = juce::jlimit(0, (int)slices.size() - 1, note - 60);
            startIdx = juce::jlimit(0.0, (double)(len - 1), (double)slices[(size_t)sliceIdx] * (double)(len - 1));
            if (sliceIdx + 1 < (int)slices.size())
                sliceEnd = juce::jlimit(startIdx + 1.0, (double)(len - 1), (double)slices[(size_t)(sliceIdx + 1)] * (double)(len - 1));
            effectiveNote = p.rootNote; // Slice Map plays slices at root pitch unless global pitch is shifted
        }

        const int shiftSamples = (p.shiftTime > 0.001f && customStartNorm < 0.0f)
            ? (int)std::round(std::pow((double)p.shiftTime, 1.6) * 0.250 * sr)
            : 0;

        const bool useGlide = p.porta || (isArpGenerated && p.arpSlide);
        voices[slot].start(effectiveNote, velocity, startIdx, lastTriggeredNote, useGlide, p.declickMode, sr, shiftSamples, sliceEnd, midiChannel);
        lastTriggeredNote = (double)effectiveNote;
    }

    void noteOff(int note, const SamplerRuntimeParams& p)
    {
        heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        const bool volEnvOn = p.targetEnvs[1].enabled || p.volEnv.enabled;
        for (auto& v : voices)
        {
            if (v.active && (v.midiNote == note || p.stretchMode == 8 || p.stretchMode == 9))
            {
                if (volEnvOn || p.useLoopPoints)
                    v.triggerRelease(p.declickMode, sr, volEnvOn);
            }
        }
    }

    // Read a single channel with 4-point Hermite cubic interpolation
    static inline float readCubic(const float* data, int len, double pos) noexcept
    {
        if (len <= 0) return 0.0f;
        int i1 = (int)std::floor(pos);
        float frac = (float)(pos - (double)i1);
        int i0 = juce::jlimit(0, len - 1, i1 - 1);
        int i2 = juce::jlimit(0, len - 1, i1 + 1);
        int i3 = juce::jlimit(0, len - 1, i1 + 2);
        i1 = juce::jlimit(0, len - 1, i1);

        const float y0 = data[i0], y1 = data[i1], y2 = data[i2], y3 = data[i3];
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    // 16-tap Blackman-Harris Windowed-Sinc anti-aliased interpolation (1:1 FL Studio high-quality Resample)
    static inline float readWindowedSinc(const float* data, int len, double pos, double pitchStep) noexcept
    {
        if (len <= 0) return 0.0f;
        if (pitchStep <= 1.02)
            return readCubic(data, len, pos);

        const int center = (int)std::floor(pos);
        const double frac = pos - (double)center;
        const double cutoff = juce::jlimit(0.15, 0.98, 1.0 / pitchStep);
        double sum = 0.0;
        double weightSum = 0.0;

        constexpr int kHalfTaps = 8;
        for (int k = -kHalfTaps + 1; k <= kHalfTaps; ++k)
        {
            const int idx = juce::jlimit(0, len - 1, center + k);
            const double x = (double)k - frac;
            const double sx = x * cutoff * juce::MathConstants<double>::pi;
            const double sinc = (std::abs(sx) < 1e-7) ? 1.0 : (std::sin(sx) / sx);
            const double wPhase = 0.5 * (1.0 + x / (double)kHalfTaps) * juce::MathConstants<double>::twoPi;
            const double win = 0.42 - 0.5 * std::cos(wPhase) + 0.08 * std::cos(2.0 * wPhase);
            const double w = sinc * win;
            sum += (double)data[idx] * w;
            weightSum += w;
        }
        return (weightSum > 1e-8) ? (float)(sum / weightSum) : data[juce::jlimit(0, len - 1, center)];
    }

    void renderBlock(juce::AudioBuffer<float>& output,
                     const juce::AudioBuffer<float>& sampleBuf,
                     double sampleFileRate,
                     const SamplerRuntimeParams& p)
    {
        const int numSamples = output.getNumSamples();
        const int smpLen = sampleBuf.getNumSamples();
        if (!p.masterOn || smpLen <= 1 || numSamples <= 0)
        {
            activePlayheadNorm.store(-1.0f, std::memory_order_relaxed);
            activeVoiceAgeSec.store(-1.0f, std::memory_order_relaxed);
            return;
        }

        // Check Cross-Instance Choke Link Bus (Header TRACK 1..125 & FuncSheet CUT BY 1..99)
        {
            auto& bus = GlobalChokeBus::instance();
            const uintptr_t myId = reinterpret_cast<uintptr_t>(this);

            if (p.chokeLinkTrack > 0 && p.chokeLinkTrack < GlobalChokeBus::kMaxGroups)
            {
                const uint32_t curEpoch = bus.groupEpoch[(size_t)p.chokeLinkTrack].load(std::memory_order_acquire);
                if (curEpoch != lastSeenTrackEpoch)
                {
                    lastSeenTrackEpoch = curEpoch;
                    if (bus.lastTriggerOwner[(size_t)p.chokeLinkTrack].load(std::memory_order_relaxed) != myId)
                    {
                        for (auto& v : voices)
                            if (v.active && !v.declicking)
                                v.triggerRelease(p.declickMode, sr, false);
                    }
                }
            }

            if (p.cutByGroup > 0 && p.cutByGroup < GlobalChokeBus::kMaxGroups)
            {
                const uint32_t curCutEpoch = bus.groupEpoch[(size_t)p.cutByGroup].load(std::memory_order_acquire);
                if (curCutEpoch != lastSeenCutByEpoch)
                {
                    lastSeenCutByEpoch = curCutEpoch;
                    if (bus.lastTriggerOwner[(size_t)p.cutByGroup].load(std::memory_order_relaxed) != myId)
                    {
                        for (auto& v : voices)
                            if (v.active && !v.declicking)
                                v.triggerRelease(p.declickMode, sr, false);
                    }
                }
            }
        }

        const double bpm = (p.hostBpm > 20.0) ? p.hostBpm : 120.0;

        // Arpeggiator step advancing (with Chords, Repeat, Slide, and Swing)
        if (p.arpDir > 0 && !heldNotes.empty())
        {
            arpTimer += (double)numSamples / sr;
            double baseStepDur = juce::jmax(0.025, (double)p.arpTime * (120.0 / bpm));
            // Apply Swing multiplier to odd/even steps
            double swingOffset = ((arpStepCounter & 1) != 0) ? (1.0 + (double)p.swingMul * 0.35) : (1.0 - (double)p.swingMul * 0.25);
            const double stepDur = juce::jmax(0.02, baseStepDur * swingOffset);

            if (arpTimer >= stepDur)
            {
                arpTimer -= stepDur;
                ++arpStepCounter;

                // Build chord interval table when Chord mode (Major, Minor, Sus2, Sus4) is selected
                std::vector<int> arpNotes;
                if (p.arpChord >= 3 && !heldNotes.empty())
                {
                    const int root = heldNotes[0];
                    arpNotes.push_back(root);
                    if (p.arpChord == 3) { arpNotes.push_back(root + 4); arpNotes.push_back(root + 7); }      // Major
                    else if (p.arpChord == 4) { arpNotes.push_back(root + 3); arpNotes.push_back(root + 7); } // Minor
                    else if (p.arpChord == 5) { arpNotes.push_back(root + 2); arpNotes.push_back(root + 7); } // Sus2
                    else if (p.arpChord == 6) { arpNotes.push_back(root + 5); arpNotes.push_back(root + 7); } // Sus4
                }
                else
                {
                    arpNotes = heldNotes;
                }

                const int numBase = juce::jmax(1, (int)arpNotes.size());
                const int totalSteps = numBase * juce::jmax(1, p.arpRange);

                if (++arpRepeatCounter >= juce::jmax(1, p.arpRepeat))
                {
                    arpRepeatCounter = 0;
                    if (p.arpDir == 5) // Random
                        arpIndex = juce::Random::getSystemRandom().nextInt(totalSteps);
                    else if (p.arpDir == 3 && totalSteps > 1) // Up-Down
                    {
                        arpIndex += arpPingDir;
                        if (arpIndex >= totalSteps - 1) { arpIndex = totalSteps - 1; arpPingDir = -1; }
                        else if (arpIndex <= 0) { arpIndex = 0; arpPingDir = 1; }
                    }
                    else
                        arpIndex = (arpIndex + 1) % totalSteps;
                }

                int stepInCycle = juce::jlimit(0, totalSteps - 1, arpIndex);
                if (p.arpDir == 2) // Down
                    stepInCycle = totalSteps - 1 - stepInCycle;

                int baseIdx = stepInCycle % numBase;
                int oct = stepInCycle / numBase;
                int trigNote = juce::jlimit(0, 127, arpNotes[(size_t)baseIdx] + oct * 12);
                noteOn(trigNote, 0.85f, sampleBuf, p, -1.0f, true);
            }
        }

        const float* smpL = sampleBuf.getReadPointer(0);
        const float* smpR = sampleBuf.getNumChannels() > 1 ? sampleBuf.getReadPointer(1) : smpL;

        float* outL = output.getWritePointer(0);
        float* outR = output.getNumChannels() > 1 ? output.getWritePointer(1) : outL;

        const double rateRatio = (sampleFileRate > 0.0 ? sampleFileRate : sr) / sr;
        const double loopStartIdx = juce::jlimit(0.0, (double)(smpLen - 2), (double)p.loopStart * (double)(smpLen - 1));
        const double loopEndIdx   = juce::jlimit(loopStartIdx + 1.0, (double)(smpLen - 1), (double)p.loopEnd * (double)(smpLen - 1));

        float newestPlayhead = -1.0f;
        float newestVoiceAge = -1.0f;
        const double dt = 1.0 / sr;

        for (auto& v : voices)
        {
            if (!v.active)
                continue;

            for (int i = 0; i < numSamples; ++i)
            {
                // Sample-accurate Time SHIFT delay before voice starts
                if (v.shiftCountdownSamples > 0)
                {
                    --v.shiftCountdownSamples;
                    continue;
                }

                v.noteAgeSec += dt;
                const double globalClockSec = globalSampleCounter * dt;
                ++globalSampleCounter;

                // Gate cutoff check (FuncSheet -> Time -> GATE or Arp Gate)
                const bool volEnvOn = p.targetEnvs[1].enabled || p.volEnv.enabled;
                if (p.gateTime < 0.995f && !v.declicking)
                {
                    const double maxGateSec = 0.01 + std::pow((double)p.gateTime, 2.0) * 2.5;
                    if (v.noteAgeSec >= maxGateSec)
                        v.triggerRelease(p.declickMode, sr, volEnvOn);
                }

                // Portamento pitch glide (Full porta scales glide time by semitone distance)
                if ((p.porta || p.arpSlide) && std::abs(v.curNoteSemi - v.targetSemi) > 1e-4)
                {
                    double distScale = p.fullPorta ? juce::jlimit(0.25, 3.0, std::abs(v.targetSemi - v.curNoteSemi) / 12.0) : 1.0;
                    const double glideTau = juce::jmax(0.002, (double)p.portaTime * 0.45 * distScale);
                    const double alpha = 1.0 - std::exp(-dt / glideTau);
                    v.curNoteSemi += (v.targetSemi - v.curNoteSemi) * alpha;
                }
                else
                {
                    v.curNoteSemi = v.targetSemi;
                }

                // Advance all 5 AHDSR Envelopes (0=Pan, 1=Vol, 2=ModX, 3=ModY, 4=Pitch)
                bool stillAlive = true;
                const float panEnvVal   = SamplerVoice::advanceAHDSR(v.envs[0], p.targetEnvs[0], dt, false, stillAlive);
                const float volEnvVal   = SamplerVoice::advanceAHDSR(v.envs[1], p.targetEnvs[1], dt, true,  stillAlive);
                const float modXEnvVal  = SamplerVoice::advanceAHDSR(v.envs[2], p.targetEnvs[2], dt, false, stillAlive);
                const float modYEnvVal  = SamplerVoice::advanceAHDSR(v.envs[3], p.targetEnvs[3], dt, false, stillAlive);
                const float pitchEnvVal = SamplerVoice::advanceAHDSR(v.envs[4], p.targetEnvs[4], dt, false, stillAlive);
                v. envs[1].level = volEnvVal;
                if (!stillAlive)
                {
                    v.active = false;
                    break;
                }

                // Evaluate all 5 LFOs (with Tempo Sync, Global vs Voice phase, Delay, Attack, and Shape)
                const double panLfoVal   = SamplerVoice::evalLFO(p.targetLfos[0], v.noteAgeSec, globalClockSec, bpm);
                const double volLfoVal   = SamplerVoice::evalLFO(p.targetLfos[1], v.noteAgeSec, globalClockSec, bpm);
                const double modXLfoVal  = SamplerVoice::evalLFO(p.targetLfos[2], v.noteAgeSec, globalClockSec, bpm);
                const double modYLfoVal  = SamplerVoice::evalLFO(p.targetLfos[3], v.noteAgeSec, globalClockSec, bpm);
                const double pitchLfoVal = SamplerVoice::evalLFO(p.targetLfos[4], v.noteAgeSec, globalClockSec, bpm)
                                         + ((p.modWheelNorm > 0.005f) ? std::sin(v.noteAgeSec * 6.0 * juce::MathConstants<double>::twoPi) * (double)p.modWheelNorm * 0.5 : 0.0);

                // Attack & Release Declicking ramps
                if (v.attDeclickLevel < 1.0f)
                    v.attDeclickLevel = juce::jmin(1.0f, v.attDeclickLevel + v.attDeclickStep);

                if (v.declicking)
                {
                    v.declickLevel -= v.declickStep;
                    if (v.declickLevel <= 0.0f)
                    {
                        v.active = false;
                        break;
                    }
                }

                // Compute Pitch Shift & Playback Speed (including Pitch-Bend Wheel, MPE Bend, Pitch Env & Pitch LFO)
                double noteDeltaSemi = p.enableMainPitch ? (v.curNoteSemi - (double)p.rootNote) : 0.0;
                double totalPitchSemi = noteDeltaSemi
                                      + (double)(p.masterPitch + p.pitchWheelNorm) * (double)p.pitchRange
                                      + (double)v.mpePitchBend
                                      + (double)p.fineTuneCents / 100.0
                                      + (double)p.stretchPitch / 100.0
                                      + (double)(pitchEnvVal * p.targetEnvs[4].amount) * 24.0
                                      + pitchLfoVal * 12.0;

                const double pitchRatio = std::pow(2.0, totalPitchSemi / 12.0);

                // Time stretch multiplier (MUL, TIME, and Auto BPM Loop sync)
                double timeStretchFactor = (double)juce::jlimit(0.25f, 4.0f, p.stretchMul);
                if (p.stretchTime > 0.001f)
                {
                    double origDur = (double)smpLen / (sampleFileRate > 0.0 ? sampleFileRate : sr);
                    double targetDur = (double)p.stretchTime * 4.0 * (120.0 / bpm);
                    if (targetDur > 0.02)
                        timeStretchFactor *= (targetDur / juce::jmax(0.01, origDur));
                }
                else if ((p.stretchMode == 2 || p.stretchMode == 7 || p.stretchMode == 9) && p.embeddedLoopBpm > 20.0 && bpm > 20.0)
                {
                    // Auto BPM Loop sync when Stretch / Auto mode is selected
                    timeStretchFactor *= (p.embeddedLoopBpm / bpm);
                }

                float sL = 0.0f, sR = 0.0f;

                if (p.stretchMode == 0 || p.stretchMode == 8)
                {
                    // Classic FL Resample mode: Windowed-Sinc anti-aliased when Resample LED is on, raw vintage step when off
                    const double step = rateRatio * pitchRatio / timeStretchFactor;
                    sL = p.resampleOn ? readWindowedSinc(smpL, smpLen, v.position, step) : smpL[juce::jlimit(0, smpLen - 1, (int)v.position)];
                    sR = p.resampleOn ? readWindowedSinc(smpR, smpLen, v.position, step) : smpR[juce::jlimit(0, smpLen - 1, (int)v.position)];

                    // Fat Mode unison stereo detune spread
                    if (p.echoFatMode && p.echoCount > 0)
                    {
                        const double detuneOffset = std::sin(v.noteAgeSec * 2.4 * juce::MathConstants<double>::twoPi) * 18.0 * (double)juce::jmax(0.15f, std::abs(p.echoPitch) * 0.2f);
                        const double posFatL = juce::jlimit(0.0, (double)(smpLen - 1), v.position + detuneOffset);
                        const double posFatR = juce::jlimit(0.0, (double)(smpLen - 1), v.position - detuneOffset * 1.13);
                        sL = 0.65f * sL + 0.45f * readCubic(smpL, smpLen, posFatL);
                        sR = 0.65f * sR + 0.45f * readCubic(smpR, smpLen, posFatR);
                    }

                    v.position += step * (double)v.loopDir;
                }
                else
                {
                    // True WSOLA (Waveform Similarity Overlap-Add) + Formant Spectral Tilt Preserver ("Stretch" & "Stretch pro")
                    const bool isStretchPro = (p.stretchMode == 4);
                    const double formantRatio = isStretchPro ? std::pow(2.0, (double)p.formantShift / 12.0) : 1.0;
                    const double grainSize = juce::jlimit(384.0, 2048.0, 1280.0 / formantRatio);
                    const double advancePos = rateRatio / timeStretchFactor;
                    const double grainRate  = pitchRatio * rateRatio;
                    const double prevPhase  = v.grainPhase;

                    v.grainPhase += std::abs(grainRate - advancePos) / grainSize + (1.0 / grainSize);
                    if (v.grainPhase >= 1.0)
                    {
                        v.grainPhase -= 1.0;
                        // WSOLA cross-correlation alignment when grain A wraps around
                        int baseIdx = juce::jlimit(32, smpLen - 96, (int)v.position);
                        int refIdx  = juce::jlimit(32, smpLen - 96, (int)(v.position + v.wsolaOffsetB + grainSize * 0.5));
                        float bestCorr = -1e9f;
                        int bestDelta = 0;
                        for (int d = -24; d <= 24; d += 3)
                        {
                            float c = 0.0f;
                            for (int k = 0; k < 24; k += 4)
                                c += smpL[refIdx + k] * smpL[baseIdx + d + k];
                            if (c > bestCorr) { bestCorr = c; bestDelta = d; }
                        }
                        v.wsolaOffsetA = (double)bestDelta;
                    }
                    else if (prevPhase < 0.5 && v.grainPhase >= 0.5)
                    {
                        // WSOLA cross-correlation alignment when grain B wraps around at 0.5
                        int baseIdx = juce::jlimit(32, smpLen - 96, (int)v.position);
                        int refIdx  = juce::jlimit(32, smpLen - 96, (int)(v.position + v.wsolaOffsetA + grainSize * 0.5));
                        float bestCorr = -1e9f;
                        int bestDelta = 0;
                        for (int d = -24; d <= 24; d += 3)
                        {
                            float c = 0.0f;
                            for (int k = 0; k < 24; k += 4)
                                c += smpL[refIdx + k] * smpL[baseIdx + d + k];
                            if (c > bestCorr) { bestCorr = c; bestDelta = d; }
                        }
                        v.wsolaOffsetB = (double)bestDelta;
                    }

                    double phA = v.grainPhase;
                    double phB = std::fmod(v.grainPhase + 0.5, 1.0);
                    float winA = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)phA));
                    float winB = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)phB));

                    double posA = juce::jlimit(0.0, (double)(smpLen - 1), v.position + v.wsolaOffsetA + (phA - 0.5) * grainSize * pitchRatio);
                    double posB = juce::jlimit(0.0, (double)(smpLen - 1), v.position + v.wsolaOffsetB + (phB - 0.5) * grainSize * pitchRatio);

                    sL = readCubic(smpL, smpLen, posA) * winA + readCubic(smpL, smpLen, posB) * winB;
                    sR = readCubic(smpR, smpLen, posA) * winA + readCubic(smpR, smpLen, posB) * winB;

                    // Spectral formant compensation filter for Stretch Pro
                    if (isStretchPro && (std::abs(p.formantShift) > 0.05f || std::abs(totalPitchSemi) > 0.25))
                    {
                        const float formTilt = juce::jlimit(-0.65f, 0.65f, (float)((p.formantShift - totalPitchSemi * 0.35) / 18.0));
                        v.formLpL += 0.14f * (sL - v.formLpL);
                        v.formLpR += 0.14f * (sR - v.formLpR);
                        const float hpL = sL - v.formLpL;
                        const float hpR = sR - v.formLpR;
                        sL += (formTilt >= 0.0f) ? hpL * formTilt * 1.4f : v.formLpL * (-formTilt) * 1.4f;
                        sR += (formTilt >= 0.0f) ? hpR * formTilt * 1.4f : v.formLpR * (-formTilt) * 1.4f;
                    }

                    v.position += advancePos * (double)v.loopDir;
                }

                // Slice boundary stop (for Slice Map / Slice Stretch)
                if (v.sliceEndIdx > 0.0 && v.position >= v.sliceEndIdx)
                {
                    v.active = false;
                    break;
                }

                // Loop points handling (Forward & Ping-Pong)
                if (p.useLoopPoints && v.sliceEndIdx < 0.0)
                {
                    if (p.pingPongLoop)
                    {
                        if (v.position >= loopEndIdx)
                        {
                            v.position = loopEndIdx - (v.position - loopEndIdx);
                            v.loopDir = -1;
                        }
                        else if (v.position <= loopStartIdx && v.loopDir < 0)
                        {
                            v.position = loopStartIdx + (loopStartIdx - v.position);
                            v.loopDir = 1;
                        }
                    }
                    else
                    {
                        if (v.position >= loopEndIdx)
                            v.position = loopStartIdx + std::fmod(v.position - loopStartIdx, juce::jmax(1.0, loopEndIdx - loopStartIdx));
                    }
                }
                else if (v.sliceEndIdx < 0.0)
                {
                    if (v.position >= (double)(smpLen - 1) || v.position < 0.0)
                    {
                        v.active = false;
                        break;
                    }
                }

                // Tracking source: Key tracking (MIDI note vs Middle note) OR Velocity tracking (Vol mode)
                const float trackDeltaNorm = p.trackingKeyMode
                    ? juce::jlimit(-1.0f, 1.0f, (float)(v.midiNote - p.trackMidNote) / 36.0f)
                    : juce::jlimit(-1.0f, 1.0f, (v.velocity - 0.78f) * 2.0f);

                const float effFilterModX = juce::jlimit(0.0f, 1.0f,
                    p.filterModX + p.modXOffset + trackDeltaNorm * p.trackModX * 0.5f
                    + modXEnvVal * p.targetEnvs[2].amount + (float)modXLfoVal * 0.5f + p.aftertouchNorm * 0.35f);
                const float effFilterModY = juce::jlimit(0.0f, 1.0f,
                    p.filterModY + p.modYOffset + trackDeltaNorm * p.trackModY * 0.5f
                    + modYEnvVal * p.targetEnvs[3].amount + (float)modYLfoVal * 0.5f);

                // 6-Mode SVF Filter with 2x Oversampling (0=Fast LP, 1=LP, 2=BP, 3=HP, 4=BS, 5=SVF LPx2 dual 24dB/oct with saturation)
                if (effFilterModX < 0.995f || effFilterModY > 0.005f || p.filterType != 0)
                {
                    float cutNorm = juce::jlimit(0.02f, 0.995f, effFilterModX);
                    float cutoffHz = 35.0f * std::pow(500.0f, cutNorm);
                    const float osSr = (float)sr * 2.0f;
                    float g = std::tan(juce::MathConstants<float>::pi * juce::jmin(cutoffHz, osSr * 0.45f) / osSr);
                    float k = 2.0f - 1.92f * juce::jlimit(0.0f, 0.98f, effFilterModY);
                    float a1 = 1.0f / (1.0f + g * (g + k));
                    float a2 = g * a1;
                    float a3 = g * a2;

                    auto processSVFStage = [&](float in, float& ic1eq, float& ic2eq) -> float
                    {
                        float v3 = in - ic2eq;
                        float v1 = a1 * ic1eq + a2 * v3;
                        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
                        ic1eq = 2.0f * v1 - ic1eq;
                        ic2eq = 2.0f * v2 - ic2eq;
                        switch (p.filterType)
                        {
                            case 2: return v1;                  // BP
                            case 3: return in - k * v1 - v2;    // HP
                            case 4: return in - k * v1;         // BS (Notch)
                            default: return v2;                 // Fast LP / LP / SVF LPx2
                        }
                    };

                    // 2x Zero-Delay Feedback Oversampled Pass (midpoint + current sample)
                    const float midL = 0.5f * (v.prevInL + sL);
                    const float midR = 0.5f * (v.prevInR + sR);
                    v.prevInL = sL;
                    v.prevInR = sR;

                    float yMidL = processSVFStage(midL, v.z1L, v.z2L);
                    float yMidR = processSVFStage(midR, v.z1R, v.z2R);
                    float yCurL = processSVFStage(sL,   v.z1L, v.z2L);
                    float yCurR = processSVFStage(sR,   v.z1R, v.z2R);

                    if (p.filterType == 5) // SVF LPx2: second cascaded 12dB stage + resonance soft-clip
                    {
                        yMidL = std::tanh(processSVFStage(yMidL, v.z3L, v.z4L) * 1.15f);
                        yMidR = std::tanh(processSVFStage(yMidR, v.z3R, v.z4R) * 1.15f);
                        yCurL = std::tanh(processSVFStage(yCurL, v.z3L, v.z4L) * 1.15f);
                        yCurR = std::tanh(processSVFStage(yCurR, v.z3R, v.z4R) * 1.15f);
                    }
                    sL = 0.5f * (yMidL + yCurL);
                    sR = 0.5f * (yMidR + yCurR);
                }

                // Voice gain + Pan (with Vol LFO, Pan Env, Pan LFO, Levels adjustment & Tracking)
                const float lvlVolScale = (p.volOffset / 0.78f);
                const float volLfoGain  = juce::jlimit(0.0f, 2.0f, 1.0f + (float)volLfoVal * 0.5f);
                const float gain = v.velocity * volEnvVal * volLfoGain * v.attDeclickLevel * v.declickLevel * p.masterVol * lvlVolScale;
                const float pan = juce::jlimit(-1.0f, 1.0f,
                    p.masterPan + p.panOffset + trackDeltaNorm * p.trackPan
                    + panEnvVal * p.targetEnvs[0].amount + (float)panLfoVal);
                const float panAngle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
                const float gainL = gain * std::cos(panAngle) * 1.4142f;
                const float gainR = gain * std::sin(panAngle) * 1.4142f;

                outL[i] += sL * gainL;
                outR[i] += sR * gainR;

                newestPlayhead = (float)(v.position / (double)juce::jmax(1, smpLen - 1));
                newestVoiceAge = (float)v.noteAgeSec;
            }
        }

        // Echo delay / Fat mode post-processing (with Tempo sync, Echo Pan, Echo Cutoff damping)
        if (p.echoCount > 0 && p.echoFeed > 0.005f && echoBuffer.getNumSamples() > 0)
        {
            const int delayBufLen = echoBuffer.getNumSamples();
            const double syncEchoSec = (double)p.echoTime * (120.0 / bpm);
            const int delaySmp = juce::jlimit(1, delayBufLen - 1, (int)std::round(syncEchoSec * sr));
            float* dL = echoBuffer.getWritePointer(0);
            float* dR = echoBuffer.getWritePointer(1);
            const float fb = juce::jlimit(0.0f, 0.93f, p.echoFeed);
            const float damp = juce::jlimit(0.15f, 0.98f, p.echoModX);
            const float panBiasL = juce::jlimit(0.2f, 1.4f, 1.0f - p.echoPan * 0.5f);
            const float panBiasR = juce::jlimit(0.2f, 1.4f, 1.0f + p.echoPan * 0.5f);

            for (int i = 0; i < numSamples; ++i)
            {
                int readPos = (echoWritePos - delaySmp + delayBufLen) % delayBufLen;
                echoLpL = echoLpL + damp * (dL[readPos] - echoLpL);
                echoLpR = echoLpR + damp * (dR[readPos] - echoLpR);
                outL[i] += echoLpL * fb * panBiasL;
                outR[i] += echoLpR * fb * panBiasR;
                dL[echoWritePos] = outL[i];
                dR[echoWritePos] = p.echoPingPong ? echoLpL * fb : outR[i];
                echoWritePos = (echoWritePos + 1) % delayBufLen;
            }
        }

        activePlayheadNorm.store(newestPlayhead, std::memory_order_relaxed);
        activeVoiceAgeSec.store(newestVoiceAge, std::memory_order_relaxed);
    }

    float getActivePlayheadNorm() const noexcept
    {
        return activePlayheadNorm.load(std::memory_order_relaxed);
    }

    float getActiveVoiceAgeSec() const noexcept
    {
        return activeVoiceAgeSec.load(std::memory_order_relaxed);
    }

private:
    double sr = 44100.0;
    std::array<SamplerVoice, kMaxVoices> voices;
    std::vector<int> heldNotes;
    double lastTriggeredNote = 60.0;
    double arpTimer = 0.0;
    int    arpIndex = 0;
    int    arpStepCounter = 0;
    int    arpRepeatCounter = 0;
    int    arpPingDir = 1;
    uint64_t globalSampleCounter = 0;

    juce::AudioBuffer<float> echoBuffer;
    int echoWritePos = 0;
    float echoLpL = 0.0f, echoLpR = 0.0f;
    uint32_t lastSeenTrackEpoch = 0;
    uint32_t lastSeenCutByEpoch = 0;

    std::atomic<float> activePlayheadNorm { -1.0f };
    std::atomic<float> activeVoiceAgeSec  { -1.0f };
};

} // namespace fls
