#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
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
            && std::abs(pogo - o.pogo) < 1e-4f;
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

        // 2. Apply TRIM (trim trailing/leading silence below threshold)
        if (s.trim > 0.001f)
        {
            const float thresh = std::pow(s.trim, 2.5f) * 0.25f;
            int endIdx = startIdx + lenSamples - 1;
            while (endIdx > startIdx + 16)
            {
                float a0 = std::abs(source.getSample(0, endIdx));
                float a1 = srcChannels > 1 ? std::abs(source.getSample(1, endIdx)) : a0;
                if (juce::jmax(a0, a1) > thresh)
                    break;
                --endIdx;
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

        // 8. Legacy POGO (non-linear time-warp pitch bend envelope across sample)
        if (std::abs(s.pogo) > 0.005f && lenSamples > 32)
        {
            juce::AudioBuffer<float> temp(numCh, lenSamples);
            temp.makeCopyOf(dest);
            const double exponent = std::pow(4.0, (double)s.pogo);
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float* srcP = temp.getReadPointer(ch);
                float* dstP = dest.getWritePointer(ch);
                for (int i = 0; i < lenSamples; ++i)
                {
                    double norm = (double)i / (double)(lenSamples - 1);
                    double warped = std::pow(norm, exponent) * (double)(lenSamples - 1);
                    int idx0 = juce::jlimit(0, lenSamples - 1, (int)warped);
                    int idx1 = juce::jmin(lenSamples - 1, idx0 + 1);
                    float frac = (float)(warped - idx0);
                    dstP[i] = srcP[idx0] + frac * (srcP[idx1] - srcP[idx0]);
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

        // 10. Legacy Precomputed Filter (CUT / RES) & EQ
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
            const float cutoffHz = 40.0f * std::pow(450.0f, s.cut);
            const float q = 0.707f + s.res * 8.0f;
            const float w0 = juce::MathConstants<float>::twoPi * juce::jmin(cutoffHz, (float)sampleRate * 0.48f) / (float)sampleRate;
            const float alpha = std::sin(w0) / (2.0f * q);
            const float cosw0 = std::cos(w0);
            const float b0 = (1.0f - cosw0) * 0.5f;
            const float b1 = 1.0f - cosw0;
            const float b2 = (1.0f - cosw0) * 0.5f;
            const float a0 = 1.0f + alpha;
            const float a1 = -2.0f * cosw0;
            const float a2 = 1.0f - alpha;

            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
                for (int i = 0; i < lenSamples; ++i)
                {
                    const float x0 = d[i];
                    const float y0 = (b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2) / a0;
                    x2 = x1; x1 = x0;
                    y2 = y1; y1 = y0;
                    d[i] = y0;
                }
            }
        }

        // 11. Legacy BOOST + CLIP (with 2x Polyphase Half-Band Oversampling for zero-aliasing saturation)
        if (s.boost > 0.002f)
        {
            const float gain = 1.0f + s.boost * 15.0f;
            auto shapeSample = [clip = s.clip](float x) noexcept -> float
            {
                return clip ? juce::jlimit(-1.0f, 1.0f, x) : std::tanh(x);
            };
            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                float prevIn = 0.0f;
                float prevShapedMid = 0.0f;
                for (int i = 0; i < lenSamples; ++i)
                {
                    const float curIn = d[i] * gain;
                    const float midIn = 0.5f * (prevIn + curIn);
                    const float yMid  = shapeSample(midIn);
                    const float yCur  = shapeSample(curIn);
                    // Half-band 2x downsample filter [0.25, 0.5, 0.25]
                    d[i] = 0.25f * prevShapedMid + 0.5f * yCur + 0.25f * yMid;
                    prevIn = curIn;
                    prevShapedMid = yMid;
                }
            }
        }

        // 12. Legacy REVERB (A/B room/hall precomputed convolution-style Schroeder reverb)
        if (s.reverb > 0.004f && sampleRate > 0.0)
        {
            const float wet = s.reverb * 0.65f;
            const int d1 = (int)((s.reverbB ? 0.043 : 0.023) * sampleRate);
            const int d2 = (int)((s.reverbB ? 0.059 : 0.031) * sampleRate);
            const float fb = s.reverbB ? 0.72f : 0.55f;
            std::vector<float> comb1((size_t)juce::jmax(1, d1), 0.0f);
            std::vector<float> comb2((size_t)juce::jmax(1, d2), 0.0f);
            for (int ch = 0; ch < numCh; ++ch)
            {
                std::fill(comb1.begin(), comb1.end(), 0.0f);
                std::fill(comb2.begin(), comb2.end(), 0.0f);
                float* d = dest.getWritePointer(ch);
                int p1 = 0, p2 = 0;
                for (int i = 0; i < lenSamples; ++i)
                {
                    float x = d[i];
                    float y1 = comb1[(size_t)p1];
                    float y2 = comb2[(size_t)p2];
                    comb1[(size_t)p1] = x + y1 * fb;
                    comb2[(size_t)p2] = x + (y2 * fb * ((ch == 1) ? -1.0f : 1.0f));
                    p1 = (p1 + 1) % (int)comb1.size();
                    p2 = (p2 + 1) % (int)comb2.size();
                    d[i] = x + 0.5f * (y1 + y2) * wet;
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

        // 14. Loop Crossfade
        if (s.crossfade > 0.002f && lenSamples > 64)
        {
            const int xfadeLen = juce::jlimit(8, lenSamples / 2, (int)std::round(s.crossfade * 0.45f * lenSamples));
            const int loopEnd = juce::jlimit(xfadeLen, lenSamples - 1, (int)std::round(outLoopEndNorm * (lenSamples - 1)));
            const int loopStart = juce::jlimit(0, loopEnd - xfadeLen, (int)std::round(outLoopStartNorm * (lenSamples - 1)));
            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = dest.getWritePointer(ch);
                for (int i = 0; i < xfadeLen; ++i)
                {
                    const float t = (float)i / (float)xfadeLen;
                    const float a = std::sin(t * juce::MathConstants<float>::halfPi);
                    const float b = std::cos(t * juce::MathConstants<float>::halfPi);
                    int idxEnd = loopEnd - xfadeLen + i;
                    int idxStart = juce::jmin(lenSamples - 1, loopStart + i);
                    d[idxEnd] = d[idxEnd] * b + d[idxStart] * a;
                }
            }
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
