#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#if defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
 #include <immintrin.h>
 #define FLS_HAS_SSE 1
#else
 #define FLS_HAS_SSE 0
#endif

namespace fls
{

// Precomputed 32-byte aligned Polyphase Windowed-Sinc Lookup Table with Dynamic Anti-Aliasing
// Supports all 7 FL Studio Sinc tap counts (16, 24, 32, 64, 128, 256, 512) and 16 octave band-limit levels,
// with linear sub-phase interpolation between adjacent polyphase entries so even slow pitch glides are 100% quantization-noise free.
struct PolyphaseSincLUT
{
    static constexpr int kNumPhases  = 512;
    static constexpr int kTablePhases = kNumPhases + 1; // 0..512 (phase 512 = frac 1.0 for branchless sub-phase lerp!)
    static constexpr int kNumCutoffs = 16;
    static constexpr int kMaxTaps    = 512;

    static constexpr std::array<int, 7> kTapSizes  {{ 16, 24, 32, 64, 128, 256, 512 }};
    // Cumulative tap offsets for each of the 7 tap sizes (total = 1032 floats per (cutoff, phase) slice)
    static constexpr std::array<int, 7> kTapOffsets {{ 0, 16, 40, 72, 136, 264, 520 }};
    static constexpr int kStridePerPhase = 1032; // Multiple of 8 (32-byte aligned for every table!)
    static constexpr int kTotalFloats    = kNumCutoffs * kTablePhases * kStridePerPhase;

    // Normalized cutoff ratios from 1.0 (no upward pitch shift) down to 0.125 (+3 octaves anti-aliased)
    static constexpr std::array<double, kNumCutoffs> kCutoffs {{
        1.000, 0.940, 0.880, 0.820, 0.760, 0.700, 0.640, 0.580,
        0.520, 0.460, 0.400, 0.340, 0.280, 0.220, 0.170, 0.125
    }};

    juce::HeapBlock<float> rawBlock;
    float* alignedData = nullptr;

    PolyphaseSincLUT() noexcept
    {
        // Allocate with extra 64 bytes padding and align base pointer to 32-byte boundary for SIMD
        rawBlock.allocate((size_t)kTotalFloats + 16, true);
        uintptr_t addr = reinterpret_cast<uintptr_t>(rawBlock.getData());
        uintptr_t alignedAddr = (addr + 31u) & ~uintptr_t(31u);
        alignedData = reinterpret_cast<float*>(alignedAddr);

        for (int c = 0; c < kNumCutoffs; ++c)
        {
            const double cutoff = kCutoffs[(size_t)c];
            for (int ph = 0; ph < kTablePhases; ++ph)
            {
                const double frac = (double)ph / (double)kNumPhases;
                float* sliceBase = alignedData + ((c * kTablePhases + ph) * kStridePerPhase);

                for (int s = 0; s < 7; ++s)
                {
                    const int N = kTapSizes[(size_t)s];
                    const int halfN = N / 2;
                    float* coeffs = sliceBase + kTapOffsets[(size_t)s];

                    double weightSum = 0.0;
                    for (int j = 0; j < N; ++j)
                    {
                        const int k = j - halfN + 1;
                        const double x = (double)k - frac;
                        const double sx = x * cutoff * juce::MathConstants<double>::pi;
                        const double sinc = (std::abs(sx) < 1e-8) ? 1.0 : (std::sin(sx) / sx);

                        // 4-term Blackman-Harris window (-92 dB sidelobe rejection, 1:1 FL Studio Sinc clarity)
                        const double wNorm = juce::jlimit(0.0, 1.0, 0.5 * (1.0 + x / (double)halfN));
                        const double wPhase = wNorm * juce::MathConstants<double>::twoPi;
                        const double win = 0.35875
                                         - 0.48829 * std::cos(wPhase)
                                         + 0.14128 * std::cos(2.0 * wPhase)
                                         - 0.01168 * std::cos(3.0 * wPhase);
                        const double w = sinc * win;
                        coeffs[j] = (float)w;
                        weightSum += w;
                    }

                    if (std::abs(weightSum) > 1e-9)
                    {
                        const float invSum = (float)(1.0 / weightSum);
                        for (int j = 0; j < N; ++j)
                            coeffs[j] *= invSum;
                    }
                }
            }
        }
    }

    static const PolyphaseSincLUT& instance() noexcept
    {
        static const PolyphaseSincLUT lut;
        return lut;
    }

    static inline int selectCutoffIndex(double pitchStep) noexcept
    {
        const double absStep = std::abs(pitchStep);
        if (absStep <= 1.01)
            return 0;
        const double targetCut = juce::jlimit(0.125, 1.0, 0.98 / absStep);
        // Map targetCut in [0.125..1.0] to index [0..15]
        const int idx = (int)std::round((1.0 - targetCut) / (1.0 - 0.125) * (double)(kNumCutoffs - 1));
        return juce::jlimit(0, kNumCutoffs - 1, idx);
    }

    inline const float* getCoeffs(int sincSizeIdx, int cutoffIdx, int phaseIdx) const noexcept
    {
        return alignedData
             + ((cutoffIdx * kTablePhases + phaseIdx) * kStridePerPhase)
             + kTapOffsets[(size_t)sincSizeIdx];
    }
};

// Lock-free inter-instance Choke Bus shared across all open FLSampler VST3 instances in the DAW!
// Allows linking multiple instances via the header TRACK box (1..125) or FuncSheet Group CUT / BY (1..99),
// and auto-assigning the first unused TRACK / CUT / BY slot across all open plugin instances when pressing "Cut self".
struct GlobalChokeBus
{
    static constexpr int kMaxGroups = 128;
    std::array<std::atomic<uint32_t>, kMaxGroups> groupEpoch {};
    std::array<std::atomic<uintptr_t>, kMaxGroups> lastTriggerOwner {};

    struct InstanceSlot
    {
        std::atomic<uintptr_t> ownerId { 0 };
        std::atomic<int>       track   { 0 };
        std::atomic<int>       cut     { 0 };
        std::atomic<int>       cutBy   { 0 };
    };
    static constexpr int kMaxInstances = 128;
    std::array<InstanceSlot, kMaxInstances> instances {};

    static GlobalChokeBus& instance() noexcept
    {
        static GlobalChokeBus bus;
        return bus;
    }

    void registerOrUpdateInstance(uintptr_t ownerId, int track, int cut, int cutBy) noexcept
    {
        if (ownerId == 0) return;
        for (auto& slot : instances)
        {
            if (slot.ownerId.load(std::memory_order_relaxed) == ownerId)
            {
                slot.track.store(track, std::memory_order_relaxed);
                slot.cut.store(cut, std::memory_order_relaxed);
                slot.cutBy.store(cutBy, std::memory_order_relaxed);
                return;
            }
        }
        for (auto& slot : instances)
        {
            uintptr_t expected = 0;
            if (slot.ownerId.compare_exchange_strong(expected, ownerId, std::memory_order_acq_rel))
            {
                slot.track.store(track, std::memory_order_relaxed);
                slot.cut.store(cut, std::memory_order_relaxed);
                slot.cutBy.store(cutBy, std::memory_order_relaxed);
                return;
            }
        }
    }

    void unregisterInstance(uintptr_t ownerId) noexcept
    {
        if (ownerId == 0) return;
        for (auto& slot : instances)
        {
            if (slot.ownerId.load(std::memory_order_relaxed) == ownerId)
            {
                slot.track.store(0, std::memory_order_relaxed);
                slot.cut.store(0, std::memory_order_relaxed);
                slot.cutBy.store(0, std::memory_order_relaxed);
                slot.ownerId.store(0, std::memory_order_release);
            }
        }
    }

    int allocateNextFreeGroupForInstance(uintptr_t ownerId) noexcept
    {
        for (int candidate = 1; candidate <= 99; ++candidate)
        {
            bool usedByOther = false;
            for (const auto& slot : instances)
            {
                const uintptr_t other = slot.ownerId.load(std::memory_order_acquire);
                if (other != 0 && other != ownerId)
                {
                    if (slot.track.load(std::memory_order_relaxed) == candidate
                        || slot.cut.load(std::memory_order_relaxed) == candidate
                        || slot.cutBy.load(std::memory_order_relaxed) == candidate)
                    {
                        usedByOther = true;
                        break;
                    }
                }
            }
            if (!usedByOther)
            {
                registerOrUpdateInstance(ownerId, candidate, candidate, candidate);
                return candidate;
            }
        }
        registerOrUpdateInstance(ownerId, 1, 1, 1);
        return 1;
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
    std::atomic<bool>  enabled    { false };
    std::atomic<float> delay      { 0.0f };  // 0..1 (0..2s)
    std::atomic<float> attack     { 0.0f };  // 0..1 (0..4s)
    std::atomic<float> hold       { 0.0f };  // 0..1 (0..4s)
    std::atomic<float> decay      { 0.5f };  // 0..1 (0..4s)
    std::atomic<float> sustain    { 1.0f };  // 0..1
    std::atomic<float> release    { 0.2f };  // 0..1 (0..4s)
    std::atomic<float> amount     { 0.0f };  // -1..+1 (for Pitch/ModX/ModY/Pan)
    std::atomic<float> attTension { 0.0f };  // -1..+1
    std::atomic<float> decTension { -0.35f };// -1..+1
    std::atomic<float> relTension { -0.45f };// -1..+1
    std::atomic<bool>  tempoSync  { false };

    AHDSREnvelope() noexcept = default;
    AHDSREnvelope(bool en, float del, float att, float hld, float dec, float sus, float rel,
                  float amt = 0.0f, float aT = 0.0f, float dT = -0.35f, float rT = -0.45f, bool ts = false) noexcept
        : enabled(en), delay(del), attack(att), hold(hld), decay(dec), sustain(sus), release(rel),
          amount(amt), attTension(aT), decTension(dT), relTension(rT), tempoSync(ts) {}

    AHDSREnvelope(const AHDSREnvelope& o) noexcept
        : enabled(o.enabled.load(std::memory_order_relaxed)),
          delay(o.delay.load(std::memory_order_relaxed)),
          attack(o.attack.load(std::memory_order_relaxed)),
          hold(o.hold.load(std::memory_order_relaxed)),
          decay(o.decay.load(std::memory_order_relaxed)),
          sustain(o.sustain.load(std::memory_order_relaxed)),
          release(o.release.load(std::memory_order_relaxed)),
          amount(o.amount.load(std::memory_order_relaxed)),
          attTension(o.attTension.load(std::memory_order_relaxed)),
          decTension(o.decTension.load(std::memory_order_relaxed)),
          relTension(o.relTension.load(std::memory_order_relaxed)),
          tempoSync(o.tempoSync.load(std::memory_order_relaxed)) {}

    AHDSREnvelope& operator=(const AHDSREnvelope& o) noexcept
    {
        if (this != &o)
        {
            enabled.store(o.enabled.load(std::memory_order_relaxed), std::memory_order_relaxed);
            delay.store(o.delay.load(std::memory_order_relaxed), std::memory_order_relaxed);
            attack.store(o.attack.load(std::memory_order_relaxed), std::memory_order_relaxed);
            hold.store(o.hold.load(std::memory_order_relaxed), std::memory_order_relaxed);
            decay.store(o.decay.load(std::memory_order_relaxed), std::memory_order_relaxed);
            sustain.store(o.sustain.load(std::memory_order_relaxed), std::memory_order_relaxed);
            release.store(o.release.load(std::memory_order_relaxed), std::memory_order_relaxed);
            amount.store(o.amount.load(std::memory_order_relaxed), std::memory_order_relaxed);
            attTension.store(o.attTension.load(std::memory_order_relaxed), std::memory_order_relaxed);
            decTension.store(o.decTension.load(std::memory_order_relaxed), std::memory_order_relaxed);
            relTension.store(o.relTension.load(std::memory_order_relaxed), std::memory_order_relaxed);
            tempoSync.store(o.tempoSync.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }
};

struct LFOModulator
{
    std::atomic<float> delay       { 0.0f }; // 0..1
    std::atomic<float> attack      { 0.0f }; // 0..1
    std::atomic<float> amount      { 0.0f }; // -1..+1
    std::atomic<float> speed       { 0.5f }; // 0..1 (0.1 Hz .. 20 Hz)
    std::atomic<int>   shape       { 0 };    // 0 = Sine, 1 = Triangle, 2 = Pulse
    std::atomic<bool>  tempoSync   { true };
    std::atomic<bool>  globalPhase { false };

    LFOModulator() noexcept = default;
    LFOModulator(const LFOModulator& o) noexcept
        : delay(o.delay.load(std::memory_order_relaxed)),
          attack(o.attack.load(std::memory_order_relaxed)),
          amount(o.amount.load(std::memory_order_relaxed)),
          speed(o.speed.load(std::memory_order_relaxed)),
          shape(o.shape.load(std::memory_order_relaxed)),
          tempoSync(o.tempoSync.load(std::memory_order_relaxed)),
          globalPhase(o.globalPhase.load(std::memory_order_relaxed)) {}

    LFOModulator& operator=(const LFOModulator& o) noexcept
    {
        if (this != &o)
        {
            delay.store(o.delay.load(std::memory_order_relaxed), std::memory_order_relaxed);
            attack.store(o.attack.load(std::memory_order_relaxed), std::memory_order_relaxed);
            amount.store(o.amount.load(std::memory_order_relaxed), std::memory_order_relaxed);
            speed.store(o.speed.load(std::memory_order_relaxed), std::memory_order_relaxed);
            shape.store(o.shape.load(std::memory_order_relaxed), std::memory_order_relaxed);
            tempoSync.store(o.tempoSync.load(std::memory_order_relaxed), std::memory_order_relaxed);
            globalPhase.store(o.globalPhase.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }
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
    int   resamplingQuality = 3; // 0=2-pt linear, 1=6-pt Hermite, 2=16-pt sinc, 3=24-pt sinc (default), 4=32-pt, 5=64-pt, 6=128-pt, 7=256-pt, 8=512-pt sinc
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
            case 0: return isAttack ? 0.0    : 0.015;  // Out only (no bleeding) - 15ms smooth raised-cosine anti-click fade
            case 1: return isAttack ? 0.0010 : 0.018;  // Transient (no bleeding)
            case 2: return isAttack ? 0.0025 : 0.025;  // Transient (bleeding)
            case 3: return isAttack ? 0.006  : 0.035;  // Generic (bleeding)
            case 4: return isAttack ? 0.025  : 0.100;  // Smooth (bleeding)
            case 5: return isAttack ? 0.060  : 0.250;  // Crossfade (bleeding)
            default: return 0.015;
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
        if (attSec > 0.0001)
        {
            attDeclickLevel = 0.0f;
            attDeclickStep  = (float)(1.0 / juce::jmax(2.0, attSec * sr));
        }
        else if (startSampleIdx > 2.0)
        {
            // Even in "Out only", apply a micro 1.5ms fade-in when starting mid-sample (START OFFSET > 0) so starting off zero-crossing never clicks
            attDeclickLevel = 0.0f;
            attDeclickStep  = (float)(1.0 / juce::jmax(4.0, 0.0015 * sr));
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
            declickStep = (float)(1.0 / juce::jmax(16.0, declickSec * sr));
        }
        else
        {
            // Even when Volume Envelope is enabled, if REL knob is near 0, enforce at least getDeclickDurationSec
            // so releasing a sustained bass/808 note never clicks!
            double minDeclickSec = getDeclickDurationSec(declickMode, false);
            if (!declicking)
                declickStep = (float)(1.0 / juce::jmax(16.0, minDeclickSec * sr));
        }
    }

    static float advanceAHDSR(EnvRuntimeState& es, const AHDSREnvelope& cfg, double dt, bool isVolumeEnv, bool& voiceStillActive, double hostBpm = 120.0) noexcept
    {
        if (!cfg.enabled)
        {
            es.level = isVolumeEnv ? 1.0f : 0.0f;
            return es.level;
        }

        const double tempoScale = (cfg.tempoSync && hostBpm > 20.0) ? (120.0 / hostBpm) : 1.0;
        es.stageTime += dt;
        switch (es.stage)
        {
            case EnvStage::Delay:
            {
                const double dv = (double)cfg.delay.load(std::memory_order_relaxed);
                const double d = (dv * dv) * 2.0 * tempoScale;
                if (es.stageTime < d)
                {
                    es.level = 0.0f;
                    break;
                }
                es.stageTime -= d;
                es.stage = EnvStage::Attack;
                [[fallthrough]];
            }
            case EnvStage::Attack:
            {
                const double av = (double)cfg.attack.load(std::memory_order_relaxed);
                const double a = (av * av) * 3.0 * tempoScale;
                if (a > 0.0005 && es.stageTime < a)
                {
                    float t = (float)juce::jlimit(0.0, 1.0, es.stageTime / a);
                    es.level = applyFLTension(t, cfg.attTension.load(std::memory_order_relaxed));
                    break;
                }
                es.stageTime = juce::jmax(0.0, es.stageTime - a);
                es.stage = EnvStage::Hold;
                [[fallthrough]];
            }
            case EnvStage::Hold:
            {
                const double hv = (double)cfg.hold.load(std::memory_order_relaxed);
                const double h = (hv * hv) * 3.0 * tempoScale;
                if (es.stageTime < h)
                {
                    es.level = 1.0f;
                    break;
                }
                es.stageTime -= h;
                es.stage = EnvStage::Decay;
                [[fallthrough]];
            }
            case EnvStage::Decay:
            {
                const double decV = (double)cfg.decay.load(std::memory_order_relaxed);
                const double dec = (decV * decV) * 4.0 * tempoScale;
                const float susLvl = cfg.sustain.load(std::memory_order_relaxed);
                if (dec > 0.001 && es.stageTime < dec)
                {
                    float t = (float)juce::jlimit(0.0, 1.0, es.stageTime / dec);
                    float shaped = applyFLTension(t, cfg.decTension.load(std::memory_order_relaxed));
                    es.level = 1.0f + (susLvl - 1.0f) * shaped;
                    break;
                }
                es.stage = EnvStage::Sustain;
                [[fallthrough]];
            }
            case EnvStage::Sustain:
                es.level = cfg.sustain.load(std::memory_order_relaxed);
                break;
            case EnvStage::Release:
            {
                const double relV = (double)cfg.release.load(std::memory_order_relaxed);
                // Minimum 15ms smooth release floor so even with REL=0% releasing a sustained note never clicks!
                const double rel = juce::jmax(0.015, (relV * relV) * 4.0 * tempoScale);
                float t = (float)juce::jmin(1.0, es.stageTime / rel);
                float shaped = applyFLTension(t, cfg.relTension.load(std::memory_order_relaxed));
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
        if (std::abs(lfo.amount.load(std::memory_order_relaxed)) <= 0.001f)
            return 0.0;

        const double delV = (double)lfo.delay.load(std::memory_order_relaxed);
        const double lfoDelaySec = (delV * delV) * 1.5;
        if (noteAgeSec < lfoDelaySec)
            return 0.0;

        const double activeSec = noteAgeSec - lfoDelaySec;
        const double attV = (double)lfo.attack.load(std::memory_order_relaxed);
        const double lfoAttSec = (attV * attV) * 2.0;
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
        (void)PolyphaseSincLUT::instance(); // Pre-initialize 32-byte aligned polyphase Sinc LUT outside audio thread
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        echoBuffer.setSize(2, (int)(sr * 4.0) + juce::jmax(0, maxBlockSize));
        echoBuffer.clear();
        echoWritePos = 0;
        echoLpL = 0.0f;
        echoLpR = 0.0f;
        echoBpL = 0.0f;
        echoBpR = 0.0f;
        echoPitchModSamples = 0.0;
        arpTimer = 0.0;
        arpIndex = 0;
        arpStepCounter = 0;
        arpRepeatCounter = 0;
        arpPingDir = 1;
        resetAllVoices();
    }

    void resetAllVoices()
    {
        for (auto& v : voices)
            v.active = false;
        heldNotesCount = 0;
    }

    // Smoothly fade out all active voices on DAW Transport Stop / All Notes Off / All Sound Off (zero hard-cut click!)
    void softStopAllVoices(int declickMode) noexcept
    {
        heldNotesCount = 0;
        for (auto& v : voices)
        {
            if (v.active && !v.declicking)
                v.triggerRelease(declickMode, sr, false);
        }
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
                    addHeldNote(note);
                    return;
                }
            }
        }

        // Respect Key Region bounds (unless triggered as a waveform scrub preview)
        if (customStartNorm < 0.0f && (note < p.keyRegionLow || note > p.keyRegionHigh))
            return;

        if (!isArpGenerated && customStartNorm < 0.0f)
        {
            addHeldNote(note);
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
        // Also choke any existing active voice playing the EXACT SAME MIDI note so rapid machine-gun retriggering
        // of one key smoothly crossfades the previous voice instead of stacking 16 identical overlapping voices that clip the master bus!
        const bool selfGroupCut = (p.cutGroup > 0 && p.cutGroup == p.cutByGroup);
        for (auto& v : voices)
        {
            if (v.active && !v.declicking)
            {
                if (p.mono || p.cutSelf || selfGroupCut || p.maxPoly <= 1 || v.midiNote == note)
                    v.triggerRelease(p.declickMode, sr, false);
            }
        }

        // Count active non-declicking voices & steal oldest if exceeding maxPoly
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

        // Find free slot (NEVER overwrite a voice that is currently declicking unless all 32 voices are busy;
        // if all 32 are busy, pick the voice with the lowest current output amplitude so stealing never clicks!)
        int slot = -1;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].active)
            {
                slot = i;
                break;
            }
        }
        if (slot < 0)
        {
            float quietestLevel = 999.0f;
            int quietestIdx = 0;
            for (int i = 0; i < kMaxVoices; ++i)
            {
                const float curLvl = voices[i].declickLevel * voices[i].envs[1].level;
                if (voices[i].declicking && curLvl < quietestLevel)
                {
                    quietestLevel = curLvl;
                    quietestIdx = i;
                }
            }
            slot = quietestIdx;
        }

        const int len = sampleBuf.getNumSamples();
        const float startNorm = (customStartNorm >= 0.0f) ? customStartNorm : p.startOffset;
        double startIdx = juce::jlimit(0.0, (double)(len - 1), (double)startNorm * (double)(len - 1));
        double sliceEnd = -1.0;
        int effectiveNote = note;

        // Slice Map (mode 6) & Slice Stretch (mode 5): MIDI notes starting at C5 (60) trigger individual slices!
        if (customStartNorm < 0.0f && (p.stretchMode == 5 || p.stretchMode == 6) && p.slicePositions != nullptr && !p.slicePositions->empty())
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
        removeHeldNote(note);
        const bool volEnvOn = p.targetEnvs[1].enabled || p.volEnv.enabled;
        for (auto& v : voices)
        {
            if (v.active && !v.declicking && (v.midiNote == note || p.stretchMode == 5 || p.stretchMode == 6))
            {
                v.triggerRelease(p.declickMode, sr, volEnvOn);
            }
        }
    }

    // 1. 2-point Linear interpolation (FL Studio "2-point linear" — raw, bright, classic aliasing character)
    // Uses double precision internally and converts to float only at final output!
    static inline float readLinear2Point(const float* data, int len, double pos) noexcept
    {
        if (len <= 0) return 0.0f;
        const int idx = static_cast<int>(std::floor(pos));
        const double frac = pos - static_cast<double>(idx);
        const double y0 = (idx >= 0 && idx < len) ? static_cast<double>(data[idx]) : 0.0;
        const double y1 = (idx + 1 >= 0 && idx + 1 < len) ? static_cast<double>(data[idx + 1]) : y0;
        return static_cast<float>(y0 + frac * (y1 - y0));
    }

    // Read a single channel with 4-point Hermite cubic interpolation (internal helper)
    static inline float readCubic(const float* data, int len, double pos) noexcept
    {
        if (len <= 0) return 0.0f;
        const int i1 = static_cast<int>(std::floor(pos));
        const double frac = pos - static_cast<double>(i1);
        auto getSmp = [&](int i) noexcept -> double {
            return (i >= 0 && i < len) ? static_cast<double>(data[i]) : 0.0;
        };
        const double y0 = getSmp(i1 - 1);
        const double y1 = getSmp(i1);
        const double y2 = getSmp(i1 + 1);
        const double y3 = getSmp(i1 + 2);
        const double c0 = y1;
        const double c1 = 0.5 * (y2 - y0);
        const double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
        const double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
        return static_cast<float>(((c3 * frac + c2) * frac + c1) * frac + c0);
    }

    // 2. 6-point, 5th-order Hermite polynomial interpolation (FL Studio "6-point Hermite" using samples idx-2 .. idx+3)
    // Exact Laurent de Soras / Olli Niemitalo 6-point 5th-order Hermite curve in 64-bit double precision:
    //   At frac = 0: P(0) = y0, P'(0) = c1, P''(0) = 2*c2
    //   At frac = 1: P(1) = y1, P'(1) = (y2 - y0)/2 - (y3 - 2*y2 + 2*y0 - ym1)/12, P''(1) = y2 - 2*y1 + y0
    static inline float readHermite6Point(const float* data, int len, double pos) noexcept
    {
        if (len <= 0) return 0.0f;
        const int idx = static_cast<int>(std::floor(pos));
        const double frac = pos - static_cast<double>(idx);

        double ym2, ym1, y0, y1, y2, y3;
        if (idx >= 2 && idx + 3 < len)
        {
            // Fast path without branch checks
            ym2 = static_cast<double>(data[idx - 2]);
            ym1 = static_cast<double>(data[idx - 1]);
            y0  = static_cast<double>(data[idx]);
            y1  = static_cast<double>(data[idx + 1]);
            y2  = static_cast<double>(data[idx + 2]);
            y3  = static_cast<double>(data[idx + 3]);
        }
        else
        {
            auto getSmp = [&](int i) noexcept -> double {
                return (i >= 0 && i < len) ? static_cast<double>(data[i]) : 0.0;
            };
            ym2 = getSmp(idx - 2);
            ym1 = getSmp(idx - 1);
            y0  = getSmp(idx);
            y1  = getSmp(idx + 1);
            y2  = getSmp(idx + 2);
            y3  = getSmp(idx + 3);
        }

        const double d0 = (1.0 / 12.0) * (ym2 - y2) + (2.0 / 3.0) * (y1 - ym1);
        const double d1 = (1.0 / 12.0) * (ym1 - y3) + (2.0 / 3.0) * (y2 - y0);
        const double s0 = (-1.0 / 12.0) * (ym2 + y2) + (4.0 / 3.0) * (ym1 + y1) - 2.5 * y0;
        const double s1 = (-1.0 / 12.0) * (ym1 + y3) + (4.0 / 3.0) * (y0  + y2) - 2.5 * y1;
        const double dy = y1 - y0;

        const double c0 = y0;
        const double c1 = d0;
        const double c2 = 0.5 * s0;
        const double c3 = 10.0 * dy - 6.0 * d0 - 4.0 * d1 - 1.5 * s0 + 0.5 * s1;
        const double c4 = -15.0 * dy + 8.0 * d0 + 7.0 * d1 + 1.5 * s0 - s1;
        const double c5 = 6.0 * dy - 3.0 * d0 - 3.0 * d1 - 0.5 * s0 + 0.5 * s1;
        return static_cast<float>(((((c5 * frac + c4) * frac + c3) * frac + c2) * frac + c1) * frac + c0);
    }

    // 3. SIMD-Accelerated Windowed-Sinc Polyphase Interpolator (16, 24, 32, 64, 128, 256, 512 taps)
    // Uses 8-tap unrolled SIMD dot product on Fast Path with sub-phase linear interpolation between polyphase LUT rows,
    // and zero-padded Edge Path at sample boundaries.
    static inline float readPolyphaseSincSIMD(const float* data, int len, double pos, double pitchStep, int sincSizeIdx) noexcept
    {
        if (len <= 0) return 0.0f;
        const auto& lut = PolyphaseSincLUT::instance();
        const int clampedIdx = juce::jlimit(0, 6, sincSizeIdx);
        const int N = PolyphaseSincLUT::kTapSizes[(size_t)clampedIdx];
        const int halfN = N >> 1;

        const int center = (int)std::floor(pos);
        const double frac = juce::jlimit(0.0, 0.9999999, pos - (double)center);
        const double scaledPhase = frac * (double)PolyphaseSincLUT::kNumPhases;
        const int phaseIdx0 = juce::jlimit(0, PolyphaseSincLUT::kNumPhases - 1, (int)scaledPhase);
        const float phaseAlpha = (float)(scaledPhase - (double)phaseIdx0);
        const int cutoffIdx = PolyphaseSincLUT::selectCutoffIndex(pitchStep);
        const float* coeffs0 = lut.getCoeffs(clampedIdx, cutoffIdx, phaseIdx0);
        const float* coeffs1 = lut.getCoeffs(clampedIdx, cutoffIdx, phaseIdx0 + 1);

        const int startIdx = center - halfN + 1;

        // FAST PATH: entire N-tap window lies inside [0, len - 1] -> branchless SIMD dot product with sub-phase interpolation!
        if (startIdx >= 0 && (startIdx + N) <= len)
        {
            const float* src = data + startIdx;
#if FLS_HAS_SSE
            __m128 acc0_a = _mm_setzero_ps();
            __m128 acc0_b = _mm_setzero_ps();
            __m128 acc1_a = _mm_setzero_ps();
            __m128 acc1_b = _mm_setzero_ps();
            for (int j = 0; j < N; j += 8)
            {
                const __m128 s0 = _mm_loadu_ps(src + j);
                const __m128 s1 = _mm_loadu_ps(src + j + 4);

                acc0_a = _mm_add_ps(acc0_a, _mm_mul_ps(s0, _mm_load_ps(coeffs0 + j)));
                acc0_b = _mm_add_ps(acc0_b, _mm_mul_ps(s1, _mm_load_ps(coeffs0 + j + 4)));

                acc1_a = _mm_add_ps(acc1_a, _mm_mul_ps(s0, _mm_load_ps(coeffs1 + j)));
                acc1_b = _mm_add_ps(acc1_b, _mm_mul_ps(s1, _mm_load_ps(coeffs1 + j + 4)));
            }
            const __m128 sum0_4 = _mm_add_ps(acc0_a, acc0_b);
            const __m128 sum1_4 = _mm_add_ps(acc1_a, acc1_b);
            const __m128 alpha4 = _mm_set1_ps(phaseAlpha);
            const __m128 sum4   = _mm_add_ps(sum0_4, _mm_mul_ps(alpha4, _mm_sub_ps(sum1_4, sum0_4)));

            __m128 shuf = _mm_movehdup_ps(sum4);
            __m128 sums = _mm_add_ps(sum4, shuf);
            shuf = _mm_movehl_ps(shuf, sums);
            sums = _mm_add_ss(sums, shuf);
            return _mm_cvtss_f32(sums);
#else
            float sum0 = 0.0f, sum1 = 0.0f;
            for (int j = 0; j < N; j += 4)
            {
                sum0 += src[j] * coeffs0[j] + src[j + 1] * coeffs0[j + 1] + src[j + 2] * coeffs0[j + 2] + src[j + 3] * coeffs0[j + 3];
                sum1 += src[j] * coeffs1[j] + src[j + 1] * coeffs1[j + 1] + src[j + 2] * coeffs1[j + 2] + src[j + 3] * coeffs1[j + 3];
            }
            return sum0 + phaseAlpha * (sum1 - sum0);
#endif
        }

        // EDGE PATH: near start or end of sample buffer -> safe zero-padding out of bounds with exact window renormalization
        // so the N-tap Sinc kernel never drops in gain or alters transient punch at sample start/end!
        const int validBegin = juce::jmax(0, -startIdx);
        const int validEnd   = juce::jmin(N, len - startIdx);
        if (validBegin >= validEnd)
            return 0.0f;

        double sum = 0.0;
        double wSum = 0.0;
        const double alphaD = static_cast<double>(phaseAlpha);
        for (int j = validBegin; j < validEnd; ++j)
        {
            const double w = static_cast<double>(coeffs0[j]) + alphaD * static_cast<double>(coeffs1[j] - coeffs0[j]);
            sum  += static_cast<double>(data[startIdx + j]) * w;
            wSum += w;
        }
        return (std::abs(wSum) > 1e-7) ? static_cast<float>(sum / wSum) : static_cast<float>(sum);
    }

    // Unified 1:1 FL Studio Resampling dispatcher (called in both Resample mode and Stretch resampling stage)
    // qualityIndex:
    //   0 = 2-point linear
    //   1 = 6-point Hermite
    //   2 = 16-point sinc
    //   3 = 24-point sinc (Default)
    //   4 = 32-point sinc
    //   5 = 64-point sinc
    //   6 = 128-point sinc
    //   7 = 256-point sinc
    //   8 = 512-point sinc
    static inline float readResampledFL(const float* data, int len, double pos, double pitchStep, int qualityIndex) noexcept
    {
        switch (qualityIndex)
        {
            case 0:  return readLinear2Point(data, len, pos);
            case 1:  return readHermite6Point(data, len, pos);
            default: return readPolyphaseSincSIMD(data, len, pos, pitchStep, qualityIndex - 2);
        }
    }

    void renderBlock(juce::AudioBuffer<float>& output,
                     const juce::AudioBuffer<float>& sampleBuf,
                     double sampleFileRate,
                     const SamplerRuntimeParams& p,
                     int startSample = 0,
                     int numSamplesToRender = -1)
    {
        const int numSamples = (numSamplesToRender >= 0) ? numSamplesToRender : (output.getNumSamples() - startSample);
        const int smpLen = sampleBuf.getNumSamples();
        if (!p.masterOn || smpLen <= 1 || numSamples <= 0)
        {
            if (numSamplesToRender < 0)
            {
                activePlayheadNorm.store(-1.0f, std::memory_order_relaxed);
                activeVoiceAgeSec.store(-1.0f, std::memory_order_relaxed);
            }
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

        // Arpeggiator step advancing (with Chords, Repeat, Slide, and Swing) - zero heap allocations!
        if (p.arpDir > 0 && heldNotesCount > 0)
        {
            arpTimer += (double)numSamples / sr;
            double baseStepDur = juce::jmax(0.025, (double)p.arpTime * (120.0 / bpm));
            // Apply Swing multiplier to odd/even steps
            double swingOffset = ((arpStepCounter & 1) != 0) ? (1.0 + (double)p.swingMul * 0.35) : (1.0 - (double)p.swingMul * 0.25);
            const double stepDur = juce::jmax(0.02, baseStepDur * swingOffset);

            if (arpTimer >= stepDur)
            {
                arpTimer = std::fmod(arpTimer, stepDur);
                ++arpStepCounter;

                // Build chord interval table on stack when Chord mode (Major, Minor, Sus2, Sus4) is selected
                std::array<int, kMaxHeldNotes> arpNotes {};
                int numArpNotes = 0;
                if (p.arpChord >= 3 && heldNotesCount > 0)
                {
                    const int root = heldNotes[0];
                    arpNotes[0] = root;
                    if (p.arpChord == 3) { arpNotes[1] = root + 4; arpNotes[2] = root + 7; numArpNotes = 3; }      // Major
                    else if (p.arpChord == 4) { arpNotes[1] = root + 3; arpNotes[2] = root + 7; numArpNotes = 3; } // Minor
                    else if (p.arpChord == 5) { arpNotes[1] = root + 2; arpNotes[2] = root + 7; numArpNotes = 3; } // Sus2
                    else if (p.arpChord == 6) { arpNotes[1] = root + 5; arpNotes[2] = root + 7; numArpNotes = 3; } // Sus4
                    else { numArpNotes = 1; }
                }
                else
                {
                    numArpNotes = heldNotesCount;
                    for (int n = 0; n < numArpNotes; ++n)
                        arpNotes[(size_t)n] = heldNotes[(size_t)n];
                }

                const int numBase = juce::jmax(1, numArpNotes);
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

        float* outL = output.getWritePointer(0) + startSample;
        float* outR = (output.getNumChannels() > 1 ? output.getWritePointer(1) : output.getWritePointer(0)) + startSample;

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

                // Gate cutoff check (FuncSheet -> Time -> GATE and Arpeggiator GATE)
                const bool volEnvOn = p.targetEnvs[1].enabled || p.volEnv.enabled;
                if (!v.declicking)
                {
                    if (p.gateTime < 0.995f)
                    {
                        const double maxGateSec = 0.01 + std::pow((double)p.gateTime, 2.0) * 2.5;
                        if (v.noteAgeSec >= maxGateSec)
                            v.triggerRelease(p.declickMode, sr, volEnvOn);
                    }
                    if (p.arpDir > 0 && p.arpGate < 0.995f)
                    {
                        const double arpStepSec = juce::jmax(0.025, (double)p.arpTime * (120.0 / bpm));
                        const double maxArpGateSec = juce::jmax(0.008, arpStepSec * (double)p.arpGate);
                        if (v.noteAgeSec >= maxArpGateSec)
                            v.triggerRelease(p.declickMode, sr, volEnvOn);
                    }
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
                const float panEnvVal   = SamplerVoice::advanceAHDSR(v.envs[0], p.targetEnvs[0], dt, false, stillAlive, bpm);
                const float volEnvVal   = SamplerVoice::advanceAHDSR(v.envs[1], p.targetEnvs[1], dt, true,  stillAlive, bpm);
                const float modXEnvVal  = SamplerVoice::advanceAHDSR(v.envs[2], p.targetEnvs[2], dt, false, stillAlive, bpm);
                const float modYEnvVal  = SamplerVoice::advanceAHDSR(v.envs[3], p.targetEnvs[3], dt, false, stillAlive, bpm);
                const float pitchEnvVal = SamplerVoice::advanceAHDSR(v.envs[4], p.targetEnvs[4], dt, false, stillAlive, bpm);
                v.envs[1].level = volEnvVal;
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

                // Compute Pitch Shift & Playback Speed (including Pitch-Bend Wheel, MPE Bend, Add to key, Pitch Env & Pitch LFO)
                double noteDeltaSemi = p.enableMainPitch
                    ? (v.curNoteSemi - (p.addToKey ? (double)(120 - p.rootNote) : (double)p.rootNote))
                    : 0.0;
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
                else if ((p.stretchMode == 3 || p.stretchMode == 4 || p.stretchMode == 5 || p.stretchMode == 7) && p.embeddedLoopBpm > 20.0 && bpm > 20.0)
                {
                    // Auto BPM Loop sync when Stretch / Stretch pro / Slice stretch / Auto mode is selected
                    timeStretchFactor *= (p.embeddedLoopBpm / bpm);
                }

                float sL = 0.0f, sR = 0.0f;

                if (p.stretchMode == 0 || p.stretchMode == 6)
                {
                    // Classic FL Resample & Slice Map modes: uses selected resamplingQuality (2-pt linear, 6-pt Hermite, 16..512-pt SIMD Sinc) when Resample LED is on
                    const double step = rateRatio * pitchRatio / timeStretchFactor;
                    sL = p.resampleOn ? readResampledFL(smpL, smpLen, v.position, step, p.resamplingQuality)
                                      : smpL[juce::jlimit(0, smpLen - 1, (int)v.position)];
                    sR = p.resampleOn ? readResampledFL(smpR, smpLen, v.position, step, p.resamplingQuality)
                                      : smpR[juce::jlimit(0, smpLen - 1, (int)v.position)];

                    // Fat Mode unison stereo detune spread (also uses selected FL Resampling Quality!)
                    if (p.echoFatMode && p.echoCount > 0)
                    {
                        const double detuneOffset = std::sin(v.noteAgeSec * 2.4 * juce::MathConstants<double>::twoPi) * 18.0 * (double)juce::jmax(0.15f, std::abs(p.echoPitch) * 0.2f);
                        const double posFatL = juce::jlimit(0.0, (double)(smpLen - 1), v.position + detuneOffset);
                        const double posFatR = juce::jlimit(0.0, (double)(smpLen - 1), v.position - detuneOffset * 1.13);
                        sL = 0.65f * sL + 0.45f * (p.resampleOn ? readResampledFL(smpL, smpLen, posFatL, step, p.resamplingQuality) : smpL[(int)posFatL]);
                        sR = 0.65f * sR + 0.45f * (p.resampleOn ? readResampledFL(smpR, smpLen, posFatR, step, p.resamplingQuality) : smpR[(int)posFatR]);
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
                         // WSOLA cross-correlation alignment when grain A wraps around (guarded for short buffers)
                         if (smpLen > 160)
                         {
                             const int maxBase = smpLen - 96;
                             int baseIdx = juce::jlimit(32, maxBase, (int)v.position);
                             int refIdx  = juce::jlimit(32, maxBase, (int)(v.position + v.wsolaOffsetB + grainSize * 0.5));
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
                         else
                         {
                             v.wsolaOffsetA = 0.0;
                         }
                     }
                     else if (prevPhase < 0.5 && v.grainPhase >= 0.5)
                     {
                         // WSOLA cross-correlation alignment when grain B wraps around at 0.5 (guarded for short buffers)
                         if (smpLen > 160)
                         {
                             const int maxBase = smpLen - 96;
                             int baseIdx = juce::jlimit(32, maxBase, (int)v.position);
                             int refIdx  = juce::jlimit(32, maxBase, (int)(v.position + v.wsolaOffsetA + grainSize * 0.5));
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
                         else
                         {
                             v.wsolaOffsetB = 0.0;
                         }
                     }

                    double phA = v.grainPhase;
                    double phB = std::fmod(v.grainPhase + 0.5, 1.0);
                    float winA = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)phA));
                    float winB = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)phB));

                    double posA = juce::jlimit(0.0, (double)(smpLen - 1), v.position + v.wsolaOffsetA + (phA - 0.5) * grainSize * pitchRatio);
                    double posB = juce::jlimit(0.0, (double)(smpLen - 1), v.position + v.wsolaOffsetB + (phB - 0.5) * grainSize * pitchRatio);

                    if (p.resampleOn)
                    {
                        sL = readResampledFL(smpL, smpLen, posA, grainRate, p.resamplingQuality) * winA
                           + readResampledFL(smpL, smpLen, posB, grainRate, p.resamplingQuality) * winB;
                        sR = readResampledFL(smpR, smpLen, posA, grainRate, p.resamplingQuality) * winA
                           + readResampledFL(smpR, smpLen, posB, grainRate, p.resamplingQuality) * winB;
                    }
                    else
                    {
                        sL = smpL[(int)posA] * winA + smpL[(int)posB] * winB;
                        sR = smpR[(int)posA] * winA + smpR[(int)posB] * winB;
                    }

                    // Fat Mode unison stereo detune spread in Stretch modes too!
                    if (p.echoFatMode && p.echoCount > 0)
                    {
                        const double detuneOffset = std::sin(v.noteAgeSec * 2.4 * juce::MathConstants<double>::twoPi) * 14.0 * (double)juce::jmax(0.15f, std::abs(p.echoPitch) * 0.2f);
                        const double posFatL = juce::jlimit(0.0, (double)(smpLen - 1), posA + detuneOffset);
                        const double posFatR = juce::jlimit(0.0, (double)(smpLen - 1), posA - detuneOffset * 1.13);
                        sL = 0.72f * sL + 0.38f * (p.resampleOn ? readResampledFL(smpL, smpLen, posFatL, grainRate, p.resamplingQuality) : smpL[(int)posFatL]);
                        sR = 0.72f * sR + 0.38f * (p.resampleOn ? readResampledFL(smpR, smpLen, posFatR, grainRate, p.resamplingQuality) : smpR[(int)posFatR]);
                    }

                    // Spectral formant compensation filter for Stretch Pro
                    if (isStretchPro && (std::abs(p.formantShift) > 0.05f || std::abs(totalPitchSemi) > 0.25))
                    {
                        const float formTilt = juce::jlimit(-0.65f, 0.65f, (float)((p.formantShift - totalPitchSemi * 0.35) / 18.0));
                        v.formLpL += 0.14f * (sL - v.formLpL);
                        v.formLpR += 0.14f * (sR - v.formLpR);
                        if (!std::isfinite(v.formLpL)) v.formLpL = 0.0f;
                        if (!std::isfinite(v.formLpR)) v.formLpR = 0.0f;
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
                    const double loopSpan = juce::jmax(1.0, loopEndIdx - loopStartIdx);
                    if (p.pingPongLoop)
                    {
                        if (v.position >= loopEndIdx)
                        {
                            const double over = std::fmod(v.position - loopEndIdx, loopSpan);
                            v.position = juce::jlimit(loopStartIdx, loopEndIdx, loopEndIdx - over);
                            v.loopDir = -1;
                        }
                        else if (v.position <= loopStartIdx && v.loopDir < 0)
                        {
                            const double under = std::fmod(loopStartIdx - v.position, loopSpan);
                            v.position = juce::jlimit(loopStartIdx, loopEndIdx, loopStartIdx + under);
                            v.loopDir = 1;
                        }
                    }
                    else
                    {
                        if (v.position >= loopEndIdx)
                            v.position = loopStartIdx + std::fmod(v.position - loopStartIdx, loopSpan);
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
                // Only engage filter when Cutoff < 100%, Resonance > 0%, or a non-LP filter mode (BP/HP/BS/LPx2) is selected!
                if (effFilterModX < 0.995f || effFilterModY > 0.005f || p.filterType >= 2)
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
                    if (!std::isfinite(yCurL) || !std::isfinite(yCurR))
                    {
                        v.z1L = v.z2L = v.z3L = v.z4L = 0.0f;
                        v.z1R = v.z2R = v.z3R = v.z4R = 0.0f;
                        yMidL = yCurL = 0.0f;
                        yMidR = yCurR = 0.0f;
                    }
                    sL = 0.5f * (yMidL + yCurL);
                    sR = 0.5f * (yMidR + yCurR);
                }

                // Smooth S-curve (Hann raised-cosine) declick envelope + end-of-sample tail anti-click protection
                const float smoothDeclick = v.declicking
                    ? (0.5f * (1.0f - std::cos(juce::jlimit(0.0f, 1.0f, v.declickLevel) * juce::MathConstants<float>::pi)))
                    : 1.0f;
                const float smoothAttDeclick = (v.attDeclickLevel < 1.0f)
                    ? (0.5f * (1.0f - std::cos(juce::jlimit(0.0f, 1.0f, v.attDeclickLevel) * juce::MathConstants<float>::pi)))
                    : 1.0f;

                float endTailAntiClick = 1.0f;
                if (!p.useLoopPoints)
                {
                    const double endLimit = (v.sliceEndIdx > 0.0) ? v.sliceEndIdx : (double)(smpLen - 1);
                    const double remSamples = endLimit - v.position;
                    const double fadeTailSamples = juce::jmax(32.0, 0.008 * sr); // 8ms smooth fade at sample/slice end
                    if (remSamples >= 0.0 && remSamples < fadeTailSamples)
                    {
                        const float u = (float)(remSamples / fadeTailSamples);
                        endTailAntiClick = 0.5f * (1.0f - std::cos(u * juce::MathConstants<float>::pi));
                    }
                }

                // Voice gain + Pan (1:1 FL Studio Sampler unity reference: default Channel Vol 78% and Vol Offset 78% = 0 dB unity,
                // default MIDI velocity ~100/127 = 0.787 -> 1.0 unity gain, and center pan = 1.0 unity per channel!)
                const float lvlVolScale = (p.volOffset / 0.78f);
                const float masterVolScale = (p.masterVol / 0.78f);
                const float velScale = (v.velocity / 0.7874f);
                const float volLfoGain = juce::jlimit(0.0f, 2.0f, 1.0f + (float)volLfoVal * 0.5f);
                const float gain = velScale * volEnvVal * volLfoGain * smoothAttDeclick * smoothDeclick * endTailAntiClick * masterVolScale * lvlVolScale;
                const float pan = juce::jlimit(-1.0f, 1.0f,
                    p.masterPan + p.panOffset + trackDeltaNorm * p.trackPan
                    + panEnvVal * p.targetEnvs[0].amount + (float)panLfoVal);
                const float panAngle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
                const float gainL = gain * std::cos(panAngle) * 1.41421356f;
                const float gainR = gain * std::sin(panAngle) * 1.41421356f;

                if (outR == outL)
                {
                    outL[i] += 0.5f * (sL * gainL + sR * gainR);
                }
                else
                {
                    outL[i] += sL * gainL;
                    outR[i] += sR * gainR;
                }

                newestPlayhead = (float)(v.position / (double)juce::jmax(1, smpLen - 1));
                newestVoiceAge = (float)v.noteAgeSec;
            }
        }

        // Echo delay / Fat mode post-processing (with Tempo sync, Echo Pan, Echo Cutoff/Resonance SVF & Echo Pitch shift)
        if (p.echoCount > 0 && p.echoFeed > 0.005f && echoBuffer.getNumSamples() > 0)
        {
            const int delayBufLen = echoBuffer.getNumSamples();
            const double syncEchoSec = (double)p.echoTime * (120.0 / bpm);
            const double baseDelaySmp = juce::jlimit(4.0, (double)(delayBufLen - 8), syncEchoSec * sr);
            float* dL = echoBuffer.getWritePointer(0);
            float* dR = echoBuffer.getWritePointer(1);
            const float countScale = juce::jlimit(0.55f, 1.0f, (float)p.echoCount / 6.0f);
            const float fb = juce::jlimit(0.0f, 0.94f, p.echoFeed * countScale);
            const float damp = juce::jlimit(0.08f, 0.98f, p.echoModX);
            const float resFb = juce::jlimit(0.0f, 0.72f, p.echoModY * 0.72f);
            const float panBiasL = juce::jlimit(0.15f, 1.45f, 1.0f - p.echoPan * 0.55f);
            const float panBiasR = juce::jlimit(0.15f, 1.45f, 1.0f + p.echoPan * 0.55f);
            const double pitchRatio = (std::abs(p.echoPitch) > 0.05f) ? std::pow(2.0, (double)p.echoPitch / 1200.0) : 1.0;

            for (int i = 0; i < numSamples; ++i)
            {
                echoPitchModSamples += (1.0 - pitchRatio);
                if (echoPitchModSamples > baseDelaySmp * 0.45)  echoPitchModSamples -= baseDelaySmp * 0.45;
                if (echoPitchModSamples < -baseDelaySmp * 0.45) echoPitchModSamples += baseDelaySmp * 0.45;

                const double effDelay = juce::jlimit(2.0, (double)(delayBufLen - 4), baseDelaySmp + echoPitchModSamples);
                double readPos = (double)echoWritePos - effDelay;
                while (readPos < 0.0)
                    readPos += (double)delayBufLen;
                while (readPos >= (double)delayBufLen)
                    readPos -= (double)delayBufLen;

                // Circular buffer read helper with wrap-around for Echo Delay
                auto readCircularEcho = [&](const float* buf) noexcept -> float
                {
                    if (!p.resampleOn)
                        return buf[juce::jlimit(0, delayBufLen - 1, (int)readPos)];
                    const int i1 = (int)std::floor(readPos);
                    const float frac = (float)(readPos - (double)i1);
                    if (p.resamplingQuality == 0)
                    {
                        const float y0 = buf[(i1 + delayBufLen) % delayBufLen];
                        const float y1 = buf[(i1 + 1 + delayBufLen) % delayBufLen];
                        return y0 + frac * (y1 - y0);
                    }
                    // Fast path when well inside circular buffer bounds
                    if (i1 >= 256 && i1 + 256 < delayBufLen)
                        return readResampledFL(buf, delayBufLen, readPos, pitchRatio, p.resamplingQuality);
                    // Wrap-around 6-point, 5th-order C2 Hermite at circular buffer boundary
                    const double ym2 = static_cast<double>(buf[(i1 - 2 + delayBufLen) % delayBufLen]);
                    const double ym1 = static_cast<double>(buf[(i1 - 1 + delayBufLen) % delayBufLen]);
                    const double y0  = static_cast<double>(buf[(i1     + delayBufLen) % delayBufLen]);
                    const double y1  = static_cast<double>(buf[(i1 + 1 + delayBufLen) % delayBufLen]);
                    const double y2  = static_cast<double>(buf[(i1 + 2 + delayBufLen) % delayBufLen]);
                    const double y3  = static_cast<double>(buf[(i1 + 3 + delayBufLen) % delayBufLen]);
                    const double df  = static_cast<double>(frac);
                    const double d0  = (1.0 / 12.0) * (ym2 - y2) + (2.0 / 3.0) * (y1 - ym1);
                    const double d1  = (1.0 / 12.0) * (ym1 - y3) + (2.0 / 3.0) * (y2 - y0);
                    const double s0  = (-1.0 / 12.0) * (ym2 + y2) + (4.0 / 3.0) * (ym1 + y1) - 2.5 * y0;
                    const double s1  = (-1.0 / 12.0) * (ym1 + y3) + (4.0 / 3.0) * (y0  + y2) - 2.5 * y1;
                    const double dy  = y1 - y0;
                    const double c0  = y0;
                    const double c1  = d0;
                    const double c2  = 0.5 * s0;
                    const double c3  = 10.0 * dy - 6.0 * d0 - 4.0 * d1 - 1.5 * s0 + 0.5 * s1;
                    const double c4  = -15.0 * dy + 8.0 * d0 + 7.0 * d1 + 1.5 * s0 - s1;
                    const double c5  = 6.0 * dy - 3.0 * d0 - 3.0 * d1 - 0.5 * s0 + 0.5 * s1;
                    return static_cast<float>(((((c5 * df + c4) * df + c3) * df + c2) * df + c1) * df + c0);
                };

                const float tapL = readCircularEcho(dL);
                const float tapR = readCircularEcho(dR);

                // 2-pole resonant lowpass in feedback loop (MOD X = Cutoff, MOD Y = Resonance)
                const float inL = tapL - resFb * echoBpL;
                const float inR = tapR - resFb * echoBpR;
                echoLpL += damp * (inL - echoLpL);
                echoLpR += damp * (inR - echoLpR);
                echoBpL += damp * (echoLpL - echoBpL);
                echoBpR += damp * (echoLpR - echoBpR);

                if (!std::isfinite(echoLpL) || !std::isfinite(echoBpL)) { echoLpL = 0.0f; echoBpL = 0.0f; }
                if (!std::isfinite(echoLpR) || !std::isfinite(echoBpR)) { echoLpR = 0.0f; echoBpR = 0.0f; }

                if (outR == outL)
                {
                    const float wetMono = 0.5f * (echoBpL * fb * panBiasL + echoBpR * fb * panBiasR);
                    outL[i] += wetMono;
                    dL[echoWritePos] = outL[i];
                    dR[echoWritePos] = p.echoPingPong ? echoBpL * fb : outL[i];
                }
                else
                {
                    outL[i] += echoBpL * fb * panBiasL;
                    outR[i] += echoBpR * fb * panBiasR;
                    dL[echoWritePos] = outL[i];
                    dR[echoWritePos] = p.echoPingPong ? echoBpL * fb : outR[i];
                }
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
    static constexpr int kMaxHeldNotes = 32;

    void addHeldNote(int note) noexcept
    {
        for (int i = 0; i < heldNotesCount; ++i)
            if (heldNotes[(size_t)i] == note)
                return;
        if (heldNotesCount < kMaxHeldNotes)
            heldNotes[(size_t)heldNotesCount++] = note;
    }

    void removeHeldNote(int note) noexcept
    {
        for (int i = 0; i < heldNotesCount; ++i)
        {
            if (heldNotes[(size_t)i] == note)
            {
                for (int j = i; j < heldNotesCount - 1; ++j)
                    heldNotes[(size_t)j] = heldNotes[(size_t)(j + 1)];
                --heldNotesCount;
                --i;
            }
        }
    }

    double sr = 44100.0;
    std::array<SamplerVoice, kMaxVoices> voices;
    std::array<int, kMaxHeldNotes> heldNotes {};
    int    heldNotesCount = 0;
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
    float echoBpL = 0.0f, echoBpR = 0.0f;
    double echoPitchModSamples = 0.0;
    uint32_t lastSeenTrackEpoch = 0;
    uint32_t lastSeenCutByEpoch = 0;

    std::atomic<float> activePlayheadNorm { -1.0f };
    std::atomic<float> activeVoiceAgeSec  { -1.0f };
};

} // namespace fls
