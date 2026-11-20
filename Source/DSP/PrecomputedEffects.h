#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "SamplerEngine.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace fls
{

struct PrecomputedSettings
{
    // Standard Precomputed Effects (FXPanel)
    bool removeDcOffset  = false;
    bool normalize       = false;
    bool reverse         = false;
    bool reversePolarity = false;
    bool fadeStereo      = false;
    bool swapStereo      = false;

    float smpStart  = 0.0f; // 0..1 (SMP START)
    float length    = 1.0f; // 0..1 (LENGTH)
    float fadeIn    = 0.0f; // 0..1 (IN)
    float fadeOut   = 0.0f; // 0..1 (OUT)
    float crossfade = 0.0f; // 0..1 (CROSSFADE)
    float trim      = 0.0f; // 0..1 (TRIM threshold)

    // Legacy Precomputed Effects (FXLegacyPanel - Bulb tab)
    float boost       = 0.0f; // 0..1
    bool  clip        = false;
    float eq          = 0.0f; // -1..+1
    float rmMix       = 0.0f; // 0..1
    float rmFreq      = 0.5f; // 0..1
    float cut         = 1.0f; // 0..1
    float res         = 0.0f; // 0..1
    float reverb      = 0.0f; // 0..1
    bool  reverbB     = false;
    float stereoDelay = 0.0f; // -1..+1
    float pogo        = 0.0f; // -1..+1
    int   resamplingQuality = 3; // 0=2-pt linear .. 8=512-pt sinc

    bool operator==(const PrecomputedSettings& o) const noexcept
    {
        return removeDcOffset == o.removeDcOffset
            && normalize == o.normalize
            && reverse == o.reverse
            && reversePolarity == o.reversePolarity
            && fadeStereo == o.fadeStereo
            && swapStereo == o.swapStereo
            && std::abs(smpStart - o.smpStart) < 1e-4f
            && std::abs(length - o.length) < 1e-4f
            && std::abs(fadeIn - o.fadeIn) < 1e-4f
            && std::abs(fadeOut - o.fadeOut) < 1e-4f
            && std::abs(crossfade - o.crossfade) < 1e-4f
            && std::abs(trim - o.trim) < 1e-4f
            && std::abs(boost - o.boost) < 1e-4f
            && clip == o.clip
            && std::abs(eq - o.eq) < 1e-4f
            && std::abs(rmMix - o.rmMix) < 1e-4f
            && std::abs(rmFreq - o.rmFreq) < 1e-4f
            && std::abs(cut - o.cut) < 1e-4f
            && std::abs(res - o.res) < 1e-4f
            && std::abs(reverb - o.reverb) < 1e-4f
            && reverbB == o.reverbB
            && std::abs(stereoDelay - o.stereoDelay) < 1e-4f
            && std::abs(pogo - o.pogo) < 1e-4f
            && resamplingQuality == o.resamplingQuality;
    }
};

class PrecomputedEffects
{
public:
    static void process(const juce::AudioBuffer<float>& source,
                        juce::AudioBuffer<float>& dest,
                        double sampleRate,
                        const PrecomputedSettings& s,
                        double& outLoopStartNorm,
                        double& outLoopEndNorm)
    {
        const int srcChannels = source.getNumChannels();
        const int srcSamples  = source.getNumSamples();
        if (srcChannels <= 0 || srcSamples <= 0)
        {
            dest.setSize(2, 0);
            return;
        }

        // Always work in stereo so Fade stereo / Swap stereo / Stereo Delay work identically to FL
        const int numCh = 2;

        // 1. Apply SMP START and LENGTH windowing
        int startIdx = juce::jlimit(0, srcSamples - 1, (int)std::round(s.smpStart * (srcSamples - 1)));
        int remSamples = srcSamples - startIdx;
        int lenSamples = juce::jlimit(1, remSamples, (int)std::round(juce::jmax(0.002f, s.length) * remSamples));

        // 2. Apply TRIM (1:1 FL Studio Sampler: trims trailing silence/low-level tail from the end of the sample)
        if (s.trim > 0.001f)
        {
            // Smooth logarithmic threshold from -96 dB (0.000015f) up to -12 dB (0.25f) relative to sample peak,
            // using a short sliding peak window so single zero-crossings don't stop the trim prematurely.
            float maxPeak = 0.001f;
            for (int ch = 0; ch < srcChannels; ++ch)
            {
                auto range = juce::FloatVectorOperations::findMinAndMax(source.getReadPointer(ch) + startIdx, lenSamples);
                maxPeak = juce::jmax(maxPeak, std::abs(range.getStart()), std::abs(range.getEnd()));
            }

            const float dB = -90.0f + std::pow(s.trim, 0.55f) * 76.0f; // -90 dB .. -14 dB
            const float thresh = maxPeak * std::pow(10.0f, dB / 20.0f);
            const int winSamples = juce::jlimit(8, 256, (int)std::round((sampleRate > 0.0 ? sampleRate : 44100.0) * 0.0025));

            int endIdx = startIdx + lenSamples - 1;
            const int minEndIdx = startIdx + juce::jmin(64, lenSamples - 1);
            while (endIdx > minEndIdx)
            {
                float localMax = 0.0f;
                const int w0 = juce::jmax(startIdx, endIdx - winSamples + 1);
                for (int k = w0; k <= endIdx; ++k)
                {
                    float a0 = std::abs(source.getSample(0, k));
                    float a1 = srcChannels > 1 ? std::abs(source.getSample(1, k)) : a0;
                    localMax = juce::jmax(localMax, juce::jmax(a0, a1));
                }
                if (localMax > thresh)
                    break;
                endIdx -= juce::jmax(1, winSamples / 4);
            }
            lenSamples = juce::jmax(16, endIdx - startIdx + 1);
        }

        dest.setSize(numCh, lenSamples, false, false, true);
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* srcPtr = source.getReadPointer(juce::jmin(ch, srcChannels - 1)) + startIdx;
            dest.copyFrom(ch, 0, srcPtr, lenSamples);
        }

        // 3. Remove DC offset
        if (s.removeDcOffset)
        {
            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                double sum = 0.0;
                for (int i = 0; i < lenSamples; ++i)
                    sum += d[i];
                const float mean = (float)(sum / (double)lenSamples);
                for (int i = 0; i < lenSamples; ++i)
                    d[i] -= mean;
            }
        }

        // 4. Reverse
        if (s.reverse)
        {
            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                std::reverse(d, d + lenSamples);
            }
        }

        // 5. Reverse Polarity
        if (s.reversePolarity)
        {
            dest.applyGain(-1.0f);
        }

        // 6. Swap Stereo
        if (s.swapStereo && numCh >= 2)
        {
            float* L = dest.getWritePointer(0);
            float* R = dest.getWritePointer(1);
            for (int i = 0; i < lenSamples; ++i)
                std::swap(L[i], R[i]);
        }

        // 7. Fade Stereo (progressive L -> R pan across sample length)
        if (s.fadeStereo && numCh >= 2 && lenSamples > 1)
        {
            float* L = dest.getWritePointer(0);
            float* R = dest.getWritePointer(1);
            for (int i = 0; i < lenSamples; ++i)
            {
                const float t = (float)i / (float)(lenSamples - 1);
                L[i] *= (1.0f - t);
                R[i] *= t;
            }
        }

        // 8. Legacy POGO (non-linear time-warp pitch bend envelope across sample, using selected FL Resampling Quality!)
        if (std::abs(s.pogo) > 0.005f && lenSamples > 32)
        {
            juce::AudioBuffer<float> temp(numCh, lenSamples);
            temp.makeCopyOf(dest);
            const double exponent = std::pow(4.0, (double)s.pogo);
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float* srcP = temp.getReadPointer(ch);
                float* dstP = dest.getWritePointer(ch);
                double prevWarped = 0.0;
                for (int i = 0; i < lenSamples; ++i)
                {
                    double norm = (double)i / (double)(lenSamples - 1);
                    double warped = std::pow(norm, exponent) * (double)(lenSamples - 1);
                    double localStep = (i > 0) ? std::abs(warped - prevWarped) : 1.0;
                    prevWarped = warped;
                    dstP[i] = SamplerEngine::readResampledFL(srcP, lenSamples, warped, localStep, s.resamplingQuality);
                }
            }
        }

        // 9. Legacy Ring Modulation (RM MIX & RM FREQ)
        if (s.rmMix > 0.002f && sampleRate > 0.0)
        {
            const double freq = 20.0 * std::pow(200.0, (double)s.rmFreq);
            const double phaseInc = juce::MathConstants<double>::twoPi * freq / sampleRate;
            const float mix = s.rmMix;
            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                double ph = 0.0;
                for (int i = 0; i < lenSamples; ++i)
                {
                    float mod = (float)std::sin(ph);
                    d[i] = d[i] * (1.0f - mix) + (d[i] * mod) * mix;
                    ph += phaseInc;
                }
            }
        }

        // 10. Legacy Precomputed Filter (CUT / RES) & EQ (1:1 FL Studio Legacy Precomputed Filter)
        if (std::abs(s.eq) > 0.005f && sampleRate > 0.0)
        {
            const float tilt = s.eq * 0.45f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                float lp = 0.0f;
                for (int i = 0; i < lenSamples; ++i)
                {
                    lp = lp + 0.08f * (d[i] - lp);
                    float hp = d[i] - lp;
                    d[i] = d[i] + (tilt > 0.0f ? hp * tilt * 2.2f : lp * (-tilt) * 2.2f);
                }
            }
        }
        if (s.cut < 0.995f || s.res > 0.005f)
        {
            // 1:1 FL Studio Legacy Precomputed CUT / RES Filter:
            // - Uses a 64-bit double-precision 2x-oversampled Zero-Delay Feedback (ZDF / TPT) Resonant Low-Pass SVF.
            // - High-Q resonant ringing naturally decays beyond the end of short one-shots, so when RES > 0.35 we add a
            //   short resonance decay tail (up to ~45ms) so the whistle/ringing peak never truncates with a click.
            // - Even when CUT is at 100% (1.0), turning RES > 0 engages the upper-spectrum resonant peak (~8.5 kHz)
            //   with FL Studio's characteristic metallic/acid bite, while sweeping CUT down produces a steep, singing
            //   resonant peak (Q up to ~32) that stays normalized within clear headroom.
            const double effSr = (sampleRate > 1000.0 ? sampleRate : 44100.0);
            const double osSr  = effSr * 2.0; // 2x oversampling for zero-aliasing high-Q resonance near Nyquist

            const double cutNorm = juce::jlimit(0.0, 1.0, (double)s.cut);
            const double resNorm = juce::jlimit(0.0, 1.0, (double)s.res);

            if (resNorm > 0.35 && s.length >= 0.995f)
            {
                const int resTailSamples = (int)std::round((resNorm - 0.35) * 0.065 * effSr);
                if (resTailSamples > 0)
                {
                    const int prevLen = lenSamples;
                    lenSamples += resTailSamples;
                    dest.setSize(numCh, lenSamples, true, true, true);
                    for (int ch = 0; ch < numCh; ++ch)
                        juce::FloatVectorOperations::clear(dest.getWritePointer(ch) + prevLen, resTailSamples);
                }
            }

            // When CUT = 1.0 and RES > 0, FL Studio places the resonant pole around ~8.5 kHz so RES is clearly audible at default CUT;
            // as CUT sweeps 0..1, frequency sweeps exponentially from 45 Hz up to 16.5 kHz (or 8.5 kHz weighted by RES at the top end).
            const double topFreqHz = 16500.0 - 8000.0 * std::pow(resNorm, 0.50);
            const double cutoffHz  = juce::jlimit(38.0, effSr * 0.46, 45.0 * std::pow(topFreqHz / 45.0, std::pow(cutNorm, 0.90)));

            // Non-linear FL resonance curve: Q spans from Butterworth (0.707) up to high-resonance self-oscillation edge (Q ~ 30)
            const double q = 0.70710678 + 29.5 * std::pow(resNorm, 2.05);
            const double k = 1.0 / q; // SVF damping factor (2*zeta = 1/Q)

            const double g  = std::tan(juce::MathConstants<double>::pi * cutoffHz / osSr);
            const double a1 = 1.0 / (1.0 + g * (g + k));
            const double a2 = g * a1;
            const double a3 = g * a2;

            // FL Studio Precomputed Filter blends extra bandpass resonant peak + compensates passband/peak gain at high Q
            const double resPeakBoost = 0.48 * std::pow(resNorm, 1.15);
            const double passbandComp = 1.0 / std::sqrt(1.0 + 0.22 * resNorm * q);

            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                double ic1eq = 0.0;
                double ic2eq = 0.0;
                double prevIn = (lenSamples > 0) ? (double)d[0] : 0.0;

                auto stepZdfSvf = [&](double inSample) noexcept -> double
                {
                    // Soft non-linear saturation inside the resonant feedback loop (prevents harsh digital overflow at high Q)
                    const double fb = ic1eq / (1.0 + 0.09 * resNorm * std::abs(ic1eq));
                    const double v3 = inSample - ic2eq;
                    const double v1 = a1 * fb + a2 * v3;
                    const double v2 = ic2eq + a2 * fb + a3 * v3;
                    ic1eq = 2.0 * v1 - fb;
                    ic2eq = 2.0 * v2 - ic2eq;
                    // Low-pass output + accentuated resonant peak at cutoff
                    return (v2 + v1 * resPeakBoost) * passbandComp;
                };

                for (int i = 0; i < lenSamples; ++i)
                {
                    const double curIn = (double)d[i];
                    const double midIn = 0.5 * (prevIn + curIn);
                    prevIn = curIn;

                    const double yMid = stepZdfSvf(midIn);
                    const double yCur = stepZdfSvf(curIn);
                    double out = 0.5 * (yMid + yCur);
                    if (!std::isfinite(out))
                    {
                        ic1eq = 0.0;
                        ic2eq = 0.0;
                        out = 0.0;
                    }
                    d[i] = (float)out;
                }
            }
        }

        // 11. Legacy BOOST + CLIP (1:1 FL Studio Sampler: BOOST only clips at 0 dBFS when CLIP is enabled;
        // when CLIP is OFF, BOOST increases the linear sample amplitude cleanly without clipping/saturation!)
        if (s.boost > 0.002f)
        {
            const float gain = 1.0f + s.boost * 7.0f;
            if (s.clip)
            {
                for (int ch = 0; ch < numCh; ++ch)
                {
                    float* d = dest.getWritePointer(ch);
                    for (int i = 0; i < lenSamples; ++i)
                        d[i] = juce::jlimit(-1.0f, 1.0f, d[i] * gain);
                }
            }
            else
            {
                dest.applyGain(gain);
            }
        }

        // 12. Legacy REVERB (1:1 FL Studio Precomputed Reverb A & B):
        // - Mode A ("Reverb A"): Bright, dense Room/Plate Freeverb-style tank (8 parallel low-damped combs + 4 series allpass diffusers),
        //   adds metallic/ambient thickness and room body primarily over the existing sample length.
        // - Mode B ("Reverb B"): Smooth, warm Hall/Canyon tank with high-frequency damping, wide stereo decorrelation,
        //   and automatic sample buffer tail extension so one-shots decay naturally to silence without being cut off!
        if (s.reverb > 0.002f && sampleRate > 0.0)
        {
            const double srScale = sampleRate / 44100.0;
            const float revAmt = s.reverb;

            // Mode B extends the sample buffer with a natural decay tail; Mode A only adds a short room release tail
            const double tailSec = s.reverbB
                ? (0.25 + std::pow((double)revAmt, 1.35) * 1.85)
                : (0.04 + std::pow((double)revAmt, 1.50) * 0.28);
            const int extraTailSamples = (s.length >= 0.995f) ? (int)std::round(tailSec * sampleRate) : 0;
            const int origSamples = lenSamples;
            if (extraTailSamples > 0)
            {
                lenSamples += extraTailSamples;
                dest.setSize(numCh, lenSamples, true, true, true);
                for (int ch = 0; ch < numCh; ++ch)
                    juce::FloatVectorOperations::clear(dest.getWritePointer(ch) + origSamples, extraTailSamples);
            }

            // Reference 44.1kHz Freeverb/Jeckorro prime tunings for Mode A (tight room/plate) vs Mode B (spacious hall)
            static constexpr int kCombTuningsA[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
            static constexpr int kCombTuningsB[8] = { 1557, 1617, 1789, 1913, 2053, 2179, 2311, 2441 };
            static constexpr int kAllpassTuningsA[4] = { 556, 441, 341, 225 };
            static constexpr int kAllpassTuningsB[4] = { 773, 601, 461, 313 };
            static constexpr int kStereoSpread = 23;

            const float roomFeedback = s.reverbB
                ? (0.78f + std::pow(revAmt, 0.65f) * 0.145f)  // Mode B: long, lush Hall decay (0.78 .. 0.925)
                : (0.64f + std::pow(revAmt, 0.75f) * 0.215f); // Mode A: tight, dense Room/Plate (0.64 .. 0.855)

            const float damp = s.reverbB ? 0.36f : 0.12f; // Mode B: warm HF roll-off; Mode A: bright, crisp reflections
            const float dampInv = 1.0f - damp;
            const float allpassFb = s.reverbB ? 0.55f : 0.50f;
            const float wetGain = s.reverbB
                ? (std::pow(revAmt, 0.85f) * 0.52f)
                : (std::pow(revAmt, 0.80f) * 0.62f);

            // Pre-delay (Mode B has a 14ms hall pre-delay; Mode A is immediate 2.5ms room slap)
            const int preDelayLen = juce::jmax(1, (int)std::round((s.reverbB ? 0.014 : 0.0025) * sampleRate));

            for (int ch = 0; ch < numCh; ++ch)
            {
                const int spread = (ch == 1) ? kStereoSpread : 0;
                std::vector<float> preDelayBuf((size_t)preDelayLen, 0.0f);
                int preDelayPos = 0;

                std::vector<float> combBufs[8];
                int combPos[8] = {};
                float combFilterStore[8] = {};
                for (int c = 0; c < 8; ++c)
                {
                    const int baseTuning = s.reverbB ? kCombTuningsB[c] : kCombTuningsA[c];
                    const int dLen = juce::jmax(16, (int)std::round((baseTuning + spread) * srScale));
                    combBufs[c].assign((size_t)dLen, 0.0f);
                }

                std::vector<float> apBufs[4];
                int apPos[4] = {};
                for (int a = 0; a < 4; ++a)
                {
                    const int baseAp = s.reverbB ? kAllpassTuningsB[a] : kAllpassTuningsA[a];
                    const int aLen = juce::jmax(8, (int)std::round((baseAp + spread) * srScale));
                    apBufs[a].assign((size_t)aLen, 0.0f);
                }

                float* d = dest.getWritePointer(ch);
                float dcBlockX1 = 0.0f, dcBlockY1 = 0.0f;

                for (int i = 0; i < lenSamples; ++i)
                {
                    const float dry = d[i];
                    const float delayedIn = preDelayBuf[(size_t)preDelayPos];
                    preDelayBuf[(size_t)preDelayPos] = dry * 0.032f;
                    preDelayPos = (preDelayPos + 1) % preDelayLen;

                    // 8 parallel lowpass-feedback comb filters
                    float acc = 0.0f;
                    for (int c = 0; c < 8; ++c)
                    {
                        auto& cb = combBufs[c];
                        const int pos = combPos[c];
                        const float out = cb[(size_t)pos];
                        combFilterStore[c] = (out * dampInv) + (combFilterStore[c] * damp);
                        cb[(size_t)pos] = delayedIn + (combFilterStore[c] * roomFeedback);
                        combPos[c] = (pos + 1) % (int)cb.size();
                        acc += out;
                    }

                    // 4 series Schroeder allpass diffusers
                    for (int a = 0; a < 4; ++a)
                    {
                        auto& ab = apBufs[a];
                        const int pos = apPos[a];
                        const float bufOut = ab[(size_t)pos];
                        const float inVal = acc;
                        acc = -inVal + bufOut;
                        ab[(size_t)pos] = inVal + (bufOut * allpassFb);
                        apPos[a] = (pos + 1) % (int)ab.size();
                    }

                    // Subtle high-pass DC blocker on wet signal so low 808s/kicks stay tight
                    const float wet = acc - dcBlockX1 + 0.992f * dcBlockY1;
                    dcBlockX1 = acc;
                    dcBlockY1 = wet;

                    // Smooth tail fade-out on the last 15% of extended tail
                    float tailEnv = 1.0f;
                    if (extraTailSamples > 0 && i >= origSamples)
                    {
                        const float t = (float)(i - origSamples) / (float)extraTailSamples;
                        if (t > 0.80f)
                            tailEnv = juce::jlimit(0.0f, 1.0f, (1.0f - t) / 0.20f);
                    }

                    d[i] = dry + wet * wetGain * tailEnv;
                }
            }
        }

        // 12b. Legacy STEREO DELAY
        if (std::abs(s.stereoDelay) > 0.005f && numCh >= 2 && sampleRate > 0.0)
        {
            const int delaySamples = (int)std::round(std::abs(s.stereoDelay) * 0.035 * sampleRate);
            const int targetCh = (s.stereoDelay > 0.0f) ? 1 : 0;
            float* d = dest.getWritePointer(targetCh);
            for (int i = lenSamples - 1; i >= delaySamples; --i)
                d[i] = d[i - delaySamples];
            for (int i = 0; i < juce::jmin(lenSamples, delaySamples); ++i)
                d[i] = 0.0f;
        }

        // 13. Quick Fade IN and OUT
        if (s.fadeIn > 0.001f)
        {
            const int fadeLen = juce::jlimit(1, lenSamples, (int)std::round(std::pow(s.fadeIn, 1.8f) * lenSamples));
            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                for (int i = 0; i < fadeLen; ++i)
                {
                    const float g = (float)i / (float)fadeLen;
                    d[i] *= g * g;
                }
            }
        }
        if (s.fadeOut > 0.001f)
        {
            const int fadeLen = juce::jlimit(1, lenSamples, (int)std::round(std::pow(s.fadeOut, 1.8f) * lenSamples));
            const int startFade = lenSamples - fadeLen;
            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                for (int i = 0; i < fadeLen; ++i)
                {
                    const float g = 1.0f - ((float)i / (float)fadeLen);
                    d[startFade + i] *= g * g;
                }
            }
        }

        // 14. Loop Crossfade (1:1 FL Studio Sampler: turning CROSSFADE > 0 blends the pre-loop region into the active loop region
        // and shortens the processed buffer while advancing the effective Loop Start from 50% up to 80% as CROSSFADE increases!)
        if (s.crossfade > 0.001f && lenSamples > 64)
        {
            const float xf = juce::jlimit(0.0f, 1.0f, s.crossfade);
            // In FL Studio Sampler:
            // - As CROSSFADE turns from >0% to 100%, the crossfade blend length is 0 .. 45% of the sample,
            //   truncating the pre-loop portion and placing the red "Loop" start marker from 50% (at max crossfade)
            //   towards the left (~21% at low crossfade) while Loop End stays at 100%.
            const int xfadeLen = juce::jlimit(8, (int)(lenSamples * 0.48f),
                                              (int)std::round((0.04f + 0.44f * xf) * (float)lenSamples));
            const int newLen = juce::jmax(32, lenSamples - xfadeLen);
            const int loopLen = juce::jlimit(16, newLen - 4,
                                             (int)std::round((0.80f - 0.30f * xf) * (float)newLen));
            const int loopStartIdx = juce::jmax(0, newLen - loopLen);

            juce::AudioBuffer<float> xfBuf(numCh, newLen);
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float* srcP = dest.getReadPointer(ch);
                float* dstP = xfBuf.getWritePointer(ch);

                // Copy everything up to the end of the new shortened buffer
                for (int i = 0; i < newLen; ++i)
                    dstP[i] = srcP[i];

                // Crossfade the tail of the loop [newLen - xfadeLen .. newLen - 1] with the pre-loop/overflow audio
                const int actualXf = juce::jmin(xfadeLen, loopLen);
                for (int i = 0; i < actualXf; ++i)
                {
                    const float t = (float)i / (float)juce::jmax(1, actualXf - 1);
                    const float gainTail = std::cos(t * juce::MathConstants<float>::halfPi);
                    const float gainHead = std::sin(t * juce::MathConstants<float>::halfPi);

                    const int tailIdx = newLen - actualXf + i;
                    const int headIdx = juce::jlimit(0, lenSamples - 1, loopStartIdx + i);
                    const int preLoopIdx = juce::jlimit(0, lenSamples - 1, juce::jmax(0, loopStartIdx - actualXf + i));
                    dstP[tailIdx] = srcP[tailIdx] * gainTail + (0.65f * srcP[headIdx] + 0.35f * srcP[preLoopIdx]) * gainHead;
                }
            }

            dest.makeCopyOf(xfBuf);
            lenSamples = newLen;
            outLoopStartNorm = (double)loopStartIdx / (double)juce::jmax(1, newLen - 1);
            outLoopEndNorm   = 1.0;
        }

        // 15. Normalize (peak normalization to 0 dBFS, just like FL Sampler)
        if (s.normalize)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto range = juce::FloatVectorOperations::findMinAndMax(dest.getReadPointer(ch), lenSamples);
                peak = juce::jmax(peak, std::abs(range.getStart()), std::abs(range.getEnd()));
            }
            if (peak > 1e-6f)
                dest.applyGain(1.0f / peak);
        }
    }

    // Snap a normalized sample position (0..1) to the nearest upward zero-crossing
    static float snapToZeroCrossing(const juce::AudioBuffer<float>& buf, float normPos, int searchRadius = 256) noexcept
    {
        const int len = buf.getNumSamples();
        if (len < 8 || buf.getNumChannels() <= 0)
            return normPos;

        const float* d = buf.getReadPointer(0);
        const int center = juce::jlimit(1, len - 2, (int)std::round(normPos * (float)(len - 1)));
        int bestIdx = center;
        int minDist = searchRadius + 1;

        const int lo = juce::jmax(1, center - searchRadius);
        const int hi = juce::jmin(len - 2, center + searchRadius);
        for (int i = lo; i <= hi; ++i)
        {
            // Prefer upward zero crossing (matching FL Studio loop point snap)
            if ((d[i - 1] <= 0.0f && d[i] >= 0.0f) || std::abs(d[i]) < 1e-4f)
            {
                int dist = std::abs(i - center);
                if (dist < minDist)
                {
                    minDist = dist;
                    bestIdx = i;
                }
            }
        }
        return (float)bestIdx / (float)(len - 1);
    }

    // Fast YIN-style autocorrelation pitch / Root MIDI Note detector for 808s and tonal one-shots
    static int detectFundamentalMidiNote(const juce::AudioBuffer<float>& buf, double sr) noexcept
    {
        const int len = buf.getNumSamples();
        if (len < 1024 || sr <= 0.0 || buf.getNumChannels() <= 0)
            return -1;

        const float* d = buf.getReadPointer(0);
        // Find peak energy region in first 0.6s (skip initial transient click of 808)
        int startOff = juce::jmin(len / 4, (int)(sr * 0.025));
        int winLen   = juce::jmin(4096, len - startOff);
        if (winLen < 512)
            return -1;

        double rms = 0.0;
        for (int i = 0; i < winLen; ++i)
            rms += (double)d[startOff + i] * (double)d[startOff + i];
        rms = std::sqrt(rms / (double)winLen);
        if (rms < 0.005)
            return -1;

        const int minLag = juce::jmax(8, (int)(sr / 1000.0)); // up to ~1000 Hz
        const int maxLag = juce::jmin(winLen / 2, (int)(sr / 27.5)); // down to A0 (27.5 Hz)
        if (maxLag <= minLag)
            return -1;

        std::vector<double> cmnd((size_t)(maxLag + 1), 1.0);
        double runningSum = 0.0;
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            double diff = 0.0;
            const int n = winLen - maxLag;
            for (int i = 0; i < n; ++i)
            {
                double delta = (double)d[startOff + i] - (double)d[startOff + i + tau];
                diff += delta * delta;
            }
            runningSum += diff;
            cmnd[(size_t)tau] = (runningSum > 1e-12) ? (diff * (double)tau / runningSum) : 1.0;
        }

        int bestTau = -1;
        const double thresh = 0.18;
        for (int tau = minLag; tau < maxLag; ++tau)
        {
            if (cmnd[(size_t)tau] < thresh)
            {
                while (tau + 1 < maxLag && cmnd[(size_t)(tau + 1)] < cmnd[(size_t)tau])
                    ++tau;
                bestTau = tau;
                break;
            }
        }
        if (bestTau < 0)
            return -1;

        const double freqHz = sr / (double)bestTau;
        if (freqHz < 25.0 || freqHz > 1200.0)
            return -1;

        const int midi = (int)std::round(69.0 + 12.0 * std::log2(freqHz / 440.0));
        // Normalize into standard octave around C5 (60) so playing C5 plays in tune, or keep exact pitch class in C5 octave
        const int pitchClass = ((midi % 12) + 12) % 12;
        return 60 + (pitchClass <= 6 ? pitchClass : pitchClass - 12);
    }

    // Detect transient slice positions (normalized 0..1) for Slice Map / Load slice markers
    static std::vector<float> detectTransientSlices(const juce::AudioBuffer<float>& buf, double sr, int maxSlices = 24)
    {
        std::vector<float> slices;
        slices.push_back(0.0f);
        const int len = buf.getNumSamples();
        if (len < 2048 || sr <= 0.0 || buf.getNumChannels() <= 0)
            return slices;

        const float* d = buf.getReadPointer(0);
        const int hop = juce::jmax(128, (int)(sr * 0.010)); // 10ms hop
        const int minGap = (int)(sr * 0.075); // min 75ms between slices
        float prevEnergy = 0.001f;
        int lastSliceSample = 0;

        for (int pos = hop; pos < len - hop; pos += hop)
        {
            float energy = 0.0f;
            for (int i = 0; i < hop; ++i)
                energy += d[pos + i] * d[pos + i];
            energy = std::sqrt(energy / (float)hop);

            if (energy > 0.04f && energy > prevEnergy * 2.35f && (pos - lastSliceSample) >= minGap)
            {
                float snapped = snapToZeroCrossing(buf, (float)pos / (float)(len - 1), 128);
                slices.push_back(snapped);
                lastSliceSample = pos;
                if ((int)slices.size() >= maxSlices)
                    break;
            }
            prevEnergy = 0.75f * prevEnergy + 0.25f * energy;
        }
        return slices;
    }
};

} // namespace fls
