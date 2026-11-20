#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"
#include "FLLookAndFeel.h"

namespace fls
{

class WAVScopeComponent : public juce::Component,
                          public juce::FileDragAndDropTarget
{
public:
    std::function<void()> onSampleDropped;
    std::function<void(const juce::String&)> onHintChanged;

    explicit WAVScopeComponent(FLSamplerAudioProcessor& proc) : processor(proc)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void refreshIfNeeded()
    {
        uint32_t rev = processor.getWaveformRevision();
        float ph = processor.getActivePlayheadNorm();
        if (rev != lastRevision)
        {
            lastRevision = rev;
            processor.getProcessedWaveformCopy(displayBuffer);
            repaint();
        }
        else if (std::abs(ph - lastPlayhead) > 0.002f)
        {
            lastPlayhead = ph;
            repaint();
        }
    }

    float normToPixelX(float normPos) const noexcept
    {
        const float w = (float)getWidth() - 4.0f;
        const float span = juce::jmax(0.001f, viewEndNorm - viewStartNorm);
        return 2.0f + ((normPos - viewStartNorm) / span) * w;
    }

    float pixelXToNorm(float px) const noexcept
    {
        const float w = juce::jmax(1.0f, (float)getWidth() - 4.0f);
        const float span = juce::jmax(0.001f, viewEndNorm - viewStartNorm);
        return juce::jlimit(0.0f, 1.0f, viewStartNorm + ((px - 2.0f) / w) * span);
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float cornerR = FLColors::isAbletonSkin ? 0.0f : 2.0f;

        // Dark charcoal / obsidian waveform background
        g.setColour(FLColors::waveBg());
        g.fillRoundedRectangle(b, cornerR);

        // Outer border (highlighted when dragging a sample file over the scope)
        g.setColour(isDragHover ? FLColors::greenNeon() : FLColors::panelBorder());
        g.drawRoundedRectangle(b.reduced(0.5f), cornerR, isDragHover ? 2.0f : 1.0f);

        const float w = b.getWidth();
        const float h = b.getHeight();
        const float midY = b.getCentreY();

        // Horizontal grid lines matching FL Studio 2026 screenshot (3 horizontal dark division lines)
        g.setColour(FLColors::waveGrid());
        g.drawHorizontalLine((int)std::round(h * 0.25f), 1.0f, w - 1.0f);
        g.drawHorizontalLine((int)std::round(midY),      1.0f, w - 1.0f);
        g.drawHorizontalLine((int)std::round(h * 0.75f), 1.0f, w - 1.0f);

        const int numSamples = displayBuffer.getNumSamples();
        if (numSamples <= 1)
        {
            g.setColour(isDragHover ? FLColors::greenNeon() : FLColors::textDim().withAlpha(0.65f));
            g.setFont(juce::Font(juce::FontOptions(12.5f)));
            g.drawText("Drop sample here (.wav / .mp3 / .flac / .ogg / .aiff) or click File folder icon",
                       getLocalBounds().reduced(12), juce::Justification::centred, false);
            return;
        }

        const float* chL = displayBuffer.getReadPointer(0);
        const float* chR = displayBuffer.getNumChannels() > 1 ? displayBuffer.getReadPointer(1) : chL;
        const int plotW = juce::jmax(1, getWidth() - 4);
        const int startSmp = juce::jlimit(0, numSamples - 1, (int)(viewStartNorm * (float)numSamples));
        const int endSmp   = juce::jlimit(startSmp + 1, numSamples, (int)(viewEndNorm * (float)numSamples));
        const int visSamples = juce::jmax(1, endSmp - startSmp);

        if (displayMode == 2) // Spectrum heat-bar view
        {
            for (int x = 0; x < plotW; x += 2)
            {
                int idx0 = startSmp + (int)((int64_t)x * visSamples / plotW);
                int idx1 = startSmp + (int)((int64_t)(x + 2) * visSamples / plotW);
                idx0 = juce::jlimit(0, numSamples - 1, idx0);
                idx1 = juce::jlimit(idx0 + 1, numSamples, idx1);

                float rms = 0.0f, zcr = 0.0f;
                for (int i = idx0; i < idx1; ++i)
                {
                    float v = 0.5f * (chL[i] + chR[i]);
                    rms += v * v;
                    if (i > idx0 && ((chL[i] >= 0.0f) != (chL[i - 1] >= 0.0f)))
                        zcr += 1.0f;
                }
                rms = std::sqrt(rms / (float)(idx1 - idx0));
                zcr = juce::jlimit(0.0f, 1.0f, zcr / (float)juce::jmax(1, (idx1 - idx0) / 3));
                float barH = juce::jlimit(2.0f, h - 6.0f, rms * h * 1.65f);
                juce::Colour col = juce::Colour::fromHSV(0.82f - zcr * 0.45f, 0.80f, juce::jlimit(0.25f, 1.0f, rms * 2.2f), 0.78f);
                g.setColour(col);
                g.fillRect(2.0f + (float)x, midY - barH * 0.5f, 1.8f, barH);
            }
        }
        else if (displayMode == 1 && displayBuffer.getNumChannels() > 1) // Dual L/R Stereo view
        {
            const float midL = h * 0.27f;
            const float midR = h * 0.73f;
            for (int ch = 0; ch < 2; ++ch)
            {
                const float* data = displayBuffer.getReadPointer(ch);
                const float cy = (ch == 0) ? midL : midR;
                juce::Path fillP;
                fillP.startNewSubPath(2.0f, cy);
                for (int x = 0; x < plotW; ++x)
                {
                    int idx0 = startSmp + (int)((int64_t)x * visSamples / plotW);
                    int idx1 = startSmp + (int)((int64_t)(x + 1) * visSamples / plotW);
                    idx0 = juce::jlimit(0, numSamples - 1, idx0);
                    idx1 = juce::jlimit(idx0 + 1, numSamples, idx1);
                    float maxV = -1.0f;
                    for (int i = idx0; i < idx1; ++i) maxV = juce::jmax(maxV, data[i]);
                    fillP.lineTo(2.0f + (float)x, cy - juce::jlimit(-1.0f, 1.0f, maxV) * (h * 0.21f));
                }
                for (int x = plotW - 1; x >= 0; --x)
                {
                    int idx0 = startSmp + (int)((int64_t)x * visSamples / plotW);
                    int idx1 = startSmp + (int)((int64_t)(x + 1) * visSamples / plotW);
                    idx0 = juce::jlimit(0, numSamples - 1, idx0);
                    idx1 = juce::jlimit(idx0 + 1, numSamples, idx1);
                    float minV = 1.0f;
                    for (int i = idx0; i < idx1; ++i) minV = juce::jmin(minV, data[i]);
                    fillP.lineTo(2.0f + (float)x, cy - juce::jlimit(-1.0f, 1.0f, minV) * (h * 0.21f));
                }
                fillP.closeSubPath();
                juce::Colour c = (ch == 0)
                    ? (FLColors::isAbletonSkin ? juce::Colour(0xff00d2ff) : juce::Colour(0xff9aa5b1))
                    : (FLColors::isAbletonSkin ? juce::Colour(0xffffb000) : juce::Colour(0xff7fb2cc));
                g.setColour(c.withAlpha(0.45f));
                g.fillPath(fillP);
            }
        }
        else // Default Mono / Merged Peak Envelope
        {
            juce::Path waveFillPath;
            juce::Path waveStrokePath;

            waveFillPath.startNewSubPath(2.0f, midY);
            for (int x = 0; x < plotW; ++x)
            {
                int idx0 = startSmp + (int)((int64_t)x * visSamples / plotW);
                int idx1 = startSmp + (int)((int64_t)(x + 1) * visSamples / plotW);
                idx0 = juce::jlimit(0, numSamples - 1, idx0);
                idx1 = juce::jlimit(idx0 + 1, numSamples, idx1);

                float maxV = -1.0f;
                for (int i = idx0; i < idx1; ++i)
                {
                    float v = 0.5f * (chL[i] + chR[i]);
                    if (v > maxV) maxV = v;
                }
                float y = midY - juce::jlimit(-1.0f, 1.0f, maxV) * (h * 0.44f);
                waveFillPath.lineTo(2.0f + (float)x, y);
                if (x == 0)
                    waveStrokePath.startNewSubPath(2.0f + (float)x, y);
                else
                    waveStrokePath.lineTo(2.0f + (float)x, y);
            }

            for (int x = plotW - 1; x >= 0; --x)
            {
                int idx0 = startSmp + (int)((int64_t)x * visSamples / plotW);
                int idx1 = startSmp + (int)((int64_t)(x + 1) * visSamples / plotW);
                idx0 = juce::jlimit(0, numSamples - 1, idx0);
                idx1 = juce::jlimit(idx0 + 1, numSamples, idx1);

                float minV = 1.0f;
                for (int i = idx0; i < idx1; ++i)
                {
                    float v = 0.5f * (chL[i] + chR[i]);
                    if (v < minV) minV = v;
                }
                float y = midY - juce::jlimit(-1.0f, 1.0f, minV) * (h * 0.44f);
                waveFillPath.lineTo(2.0f + (float)x, y);
            }
            waveFillPath.closeSubPath();

            const juce::Colour waveFillCol =
                FLColors::isWinampSkin   ? juce::Colour(0xff00ff00).withAlpha(0.35f)
              : FLColors::isWin98Skin    ? juce::Colour(0xff00ff40).withAlpha(0.32f)
              : FLColors::isMacOSXSkin   ? juce::Colour(0xff0066ee).withAlpha(0.30f)
              : FLColors::isProToolsSkin ? juce::Colour(0xff00d2ff).withAlpha(0.34f)
              : FLColors::isSequoiaSkin  ? juce::Colour(0xffffa019).withAlpha(0.33f)
              : FLColors::isAuditionSkin ? juce::Colour(0xff00f5b8).withAlpha(0.36f)
              : FLColors::isLogicSkin    ? juce::Colour(0xff26d6ff).withAlpha(0.34f)
              : FLColors::isCubaseSkin   ? juce::Colour(0xff00b7ff).withAlpha(0.34f)
              : FLColors::isReaperSkin   ? juce::Colour(0xff35d0a3).withAlpha(0.34f)
              : FLColors::isAbletonSkin  ? juce::Colour(0xff00d2ff).withAlpha(0.32f)
              : FLColors::isWhiteSkin    ? juce::Colour(0xff10b981).withAlpha(0.28f)
                                         : juce::Colour(0xff9aa5b1).withAlpha(0.42f);

            const juce::Colour waveStrokeCol =
                FLColors::isWinampSkin   ? juce::Colour(0xff00ff00).withAlpha(0.96f)
              : FLColors::isWin98Skin    ? juce::Colour(0xff00ff40).withAlpha(0.95f)
              : FLColors::isMacOSXSkin   ? juce::Colour(0xff0055d4).withAlpha(0.90f)
              : FLColors::isProToolsSkin ? juce::Colour(0xff52ff52).withAlpha(0.90f)
              : FLColors::isSequoiaSkin  ? juce::Colour(0xffffc14d).withAlpha(0.93f)
              : FLColors::isAuditionSkin ? juce::Colour(0xff2effc8).withAlpha(0.95f)
              : FLColors::isLogicSkin    ? juce::Colour(0xff5ee2ff).withAlpha(0.92f)
              : FLColors::isCubaseSkin   ? juce::Colour(0xff42ccff).withAlpha(0.92f)
              : FLColors::isReaperSkin   ? juce::Colour(0xff5ef2c8).withAlpha(0.92f)
              : FLColors::isAbletonSkin  ? juce::Colour(0xff42e3ff).withAlpha(0.92f)
              : FLColors::isWhiteSkin    ? juce::Colour(0xff059669).withAlpha(0.90f)
                                         : juce::Colour(0xffcfd7e0).withAlpha(0.85f);

            g.setColour(waveFillCol);
            g.fillPath(waveFillPath);

            g.setColour(waveStrokeCol);
            g.strokePath(waveStrokePath, juce::PathStrokeType(1.05f));
        }

        // Draw detected Slice Markers when "Load slice markers" is enabled or Stretch mode is Slice stretch / Slice map
        const int stretchMode = (int)processor.apvts.getRawParameterValue("stretchMode")->load();
        if (processor.loadSliceMarkers.load(std::memory_order_relaxed) || stretchMode == 5 || stretchMode == 6)
        {
            const auto& slices = processor.getSliceMarkers();
            g.setColour(juce::Colour(0xffffa033).withAlpha(0.68f));
            for (size_t i = 0; i < slices.size(); ++i)
            {
                float sx = normToPixelX(slices[i]);
                if (sx >= 2.0f && sx <= w - 2.0f)
                {
                    g.drawVerticalLine((int)sx, 2.0f, h - 2.0f);
                    juce::Path tri;
                    tri.addTriangle(sx, 2.0f, sx + 5.0f, 2.0f, sx, 7.0f);
                    g.fillPath(tri);
                }
            }
        }

        // Draw Fade IN / OUT envelope curves on waveform when active (just like FL Studio Sampler!)
        const float fadeInVal  = processor.apvts.getRawParameterValue("fadeIn")->load();
        const float fadeOutVal = processor.apvts.getRawParameterValue("fadeOut")->load();
        if (fadeInVal > 0.002f || fadeOutVal > 0.002f)
        {
            juce::Path envOverlay;
            float fiX = normToPixelX(std::pow(fadeInVal, 1.8f));
            float foX = normToPixelX(1.0f - std::pow(fadeOutVal, 1.8f));
            float x0  = normToPixelX(0.0f);
            float x1  = normToPixelX(1.0f);
            envOverlay.startNewSubPath(x0, h - 3.0f);
            envOverlay.quadraticTo(x0 + (fiX - x0) * 0.6f, h - 3.0f, fiX, 4.0f);
            envOverlay.lineTo(juce::jmax(fiX, foX), 4.0f);
            envOverlay.quadraticTo(foX + (x1 - foX) * 0.4f, h - 3.0f, x1, h - 3.0f);
            g.setColour(FLColors::orangeRing().withAlpha(0.65f));
            g.strokePath(envOverlay, juce::PathStrokeType(1.4f));
        }

        // Draw START OFFSET marker with top triangular orange flag if > 0
        const float startOfs = processor.apvts.getRawParameterValue("startOffset")->load();
        if (startOfs > 0.001f)
        {
            float sx = normToPixelX(startOfs);
            if (sx > 2.0f)
            {
                g.setColour(juce::Colours::black.withAlpha(0.32f));
                g.fillRect(2.0f, 2.0f, juce::jmin(w - 4.0f, sx - 2.0f), h - 4.0f);
            }
            if (sx >= 2.0f && sx <= w - 2.0f)
            {
                g.setColour(FLColors::orangeRing());
                g.drawVerticalLine((int)sx, 2.0f, h - 2.0f);
                juce::Path flag;
                flag.addTriangle(sx, 2.0f, sx + 8.0f, 2.0f, sx, 10.0f);
                g.fillPath(flag);
            }
        }

        // Draw FL Studio 1:1 Loop Start / Loop End crimson markers + shaded region + bottom "Loop" flag (Images 1 & 2)
        const float xfadeVal = processor.apvts.getRawParameterValue("crossfade")->load();
        const float xfLoopStart = processor.computedCrossfadeLoopStart.load(std::memory_order_relaxed);
        const bool hasXfadeLoop = (xfadeVal > 0.001f && xfLoopStart >= 0.0f);
        const bool useLoop = (processor.apvts.getRawParameterValue("useLoopPoints")->load() > 0.5f) || hasXfadeLoop;
        const bool pingPong = processor.apvts.getRawParameterValue("pingPongLoop")->load() > 0.5f;

        const float lStart = hasXfadeLoop ? xfLoopStart : processor.apvts.getRawParameterValue("loopStart")->load();
        const float lEnd   = hasXfadeLoop ? 1.0f        : processor.apvts.getRawParameterValue("loopEnd")->load();
        const bool showLoopRegion = useLoop && (hasXfadeLoop || lStart > 0.0005f || lEnd < 0.9995f);

        if (showLoopRegion)
        {
            float lx1 = juce::jlimit(2.0f, w - 2.0f, normToPixelX(lStart));
            float lx2 = juce::jlimit(2.0f, w - 2.0f, normToPixelX(lEnd));
            if (lx2 < lx1 + 2.0f) lx2 = lx1 + 2.0f;

            // Dark crimson/maroon tint over the active loop region (1:1 Images 1 & 2)
            g.setColour(juce::Colour(0xff7a1c24).withAlpha(0.24f));
            g.fillRect(lx1, 1.0f, lx2 - lx1, h - 2.0f);

            // Vertical crimson loop boundary lines at Loop Start and Loop End
            const juce::Colour loopCol(0xffc93b42);
            g.setColour(loopCol);
            g.drawLine(lx1, 1.0f, lx1, h - 1.0f, 1.3f);
            g.drawLine(lx2, 1.0f, lx2, h - 1.0f, 1.3f);

            // Bottom-left solid crimson marker tab + "Loop" text label (1:1 FL Studio Images 1 & 2)
            juce::Path botTab;
            botTab.startNewSubPath(lx1, h - 16.0f);
            botTab.lineTo(lx1 + 4.5f, h - 12.0f);
            botTab.lineTo(lx1 + 4.5f, h - 1.0f);
            botTab.lineTo(lx1, h - 1.0f);
            botTab.closeSubPath();
            g.fillPath(botTab);

            if (lx2 - lx1 > 34.0f)
            {
                g.setColour(juce::Colour(0xffc93b42));
                g.setFont(juce::Font(juce::FontOptions(12.0f)));
                g.drawText(pingPong ? "<-> Loop" : "Loop",
                           (int)(lx1 + 8.0f), (int)(h - 17.0f), 64, 15,
                           juce::Justification::centredLeft, false);
            }
        }

        // Zoom indicator bar if zoomed in
        if (viewStartNorm > 0.001f || viewEndNorm < 0.999f)
        {
            float barX = 4.0f + viewStartNorm * (w - 8.0f);
            float barW = juce::jmax(10.0f, (viewEndNorm - viewStartNorm) * (w - 8.0f));
            g.setColour(FLColors::greenNeon().withAlpha(0.55f));
            g.fillRoundedRectangle(barX, h - 4.5f, barW, 2.5f, 1.0f);
        }

        // Draw live playback cursor with glow
        const float ph = processor.getActivePlayheadNorm();
        if (ph >= viewStartNorm && ph <= viewEndNorm)
        {
            float px = normToPixelX(ph);
            g.setColour(FLColors::greenNeon().withAlpha(0.28f));
            g.drawLine(px, 2.0f, px, h - 2.0f, 4.5f);
            g.setColour(FLColors::greenNeon());
            g.drawLine(px, 2.0f, px, h - 2.0f, 1.8f);
        }
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (displayBuffer.getNumSamples() <= 1)
            return;

        const float focusNorm = pixelXToNorm((float)e.x);
        const float curSpan = juce::jmax(0.01f, viewEndNorm - viewStartNorm);
        const float zoomFactor = (wheel.deltaY > 0.0f) ? 0.80f : 1.25f;
        const float newSpan = juce::jlimit(0.02f, 1.0f, curSpan * zoomFactor);

        const float relRatio = (focusNorm - viewStartNorm) / curSpan;
        viewStartNorm = juce::jlimit(0.0f, 1.0f - newSpan, focusNorm - relRatio * newSpan);
        viewEndNorm   = juce::jlimit(viewStartNorm + 0.02f, 1.0f, viewStartNorm + newSpan);
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int hit = hitTestMarker(e.x);
        if (hit != 0)
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isMiddleButtonDown())
        {
            isPanningView = true;
            panStartMouseX = e.x;
            panOrigStart = viewStartNorm;
            panOrigEnd   = viewEndNorm;
            return;
        }

        // Right-click on waveform opens FL Studio Waveform Context Menu
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu m;
            const bool useLoop  = processor.apvts.getRawParameterValue("useLoopPoints")->load() > 0.5f;
            const bool pingPong = processor.apvts.getRawParameterValue("pingPongLoop")->load() > 0.5f;
            const bool rev      = processor.apvts.getRawParameterValue("reverse")->load() > 0.5f;
            const bool norm     = processor.apvts.getRawParameterValue("normalize")->load() > 0.5f;
            const bool zSnap    = processor.zeroCrossSnap.load();
            m.addItem(1, "Reload sample");
            m.addSeparator();
            m.addItem(2, "Use loop points", true, useLoop);
            m.addItem(3, "Ping pong loop", true, pingPong);
            m.addItem(4, "Set loop start here");
            m.addItem(5, "Set loop end here");
            m.addItem(8, "Snap loop/start to zero-crossing", true, zSnap);
            m.addSeparator();
            m.addItem(13, "Add slice marker here (Shift+Click)");
            m.addItem(14, "Remove nearest slice marker");
            m.addItem(15, "Clear custom slice markers");
            m.addSeparator();
            m.addItem(9, "Multichannel waveform view", true, displayMode == 1);
            m.addItem(10, "Spectrum view", true, displayMode == 2);
            m.addItem(11, "Reset zoom (100%)");
            m.addSeparator();
            m.addItem(6, "Normalize", true, norm);
            m.addItem(7, "Reverse", true, rev);

            const float clickNorm = processor.snapNormalizedToZeroCrossing(pixelXToNorm((float)e.x));
            const int nearSlice = hitTestSliceIndex(e.x, 12.0f);
            juce::Component::SafePointer<WAVScopeComponent> safeThis(this);
            m.showMenuAsync(juce::PopupMenu::Options(), [safeThis, clickNorm, nearSlice, useLoop, pingPong, rev, norm, zSnap](int res)
            {
                if (safeThis == nullptr)
                    return;
                auto* self = safeThis.getComponent();
                auto setParam = [self](const char* id, float v) {
                    if (auto* p = self->processor.apvts.getParameter(id))
                        p->setValueNotifyingHost(p->convertTo0to1(v));
                };
                if (res == 1 && self->processor.getLoadedFile().existsAsFile())
                    self->processor.loadSampleFromFile(self->processor.getLoadedFile());
                else if (res == 2) setParam("useLoopPoints", useLoop ? 0.0f : 1.0f);
                else if (res == 3) setParam("pingPongLoop", pingPong ? 0.0f : 1.0f);
                else if (res == 4) { setParam("useLoopPoints", 1.0f); setParam("loopStart", juce::jmin(0.98f, clickNorm)); }
                else if (res == 5) { setParam("useLoopPoints", 1.0f); setParam("loopEnd", juce::jmax(0.02f, clickNorm)); }
                else if (res == 8) self->processor.zeroCrossSnap.store(!zSnap);
                else if (res == 13) { float outP = 0.0f; self->processor.addOrMoveSliceMarker(-1, clickNorm, outP); }
                else if (res == 14 && nearSlice >= 0) self->processor.removeSliceMarker(nearSlice);
                else if (res == 15) self->processor.clearSliceMarkers();
                else if (res == 9) self->displayMode = (self->displayMode == 1) ? 0 : 1;
                else if (res == 10) self->displayMode = (self->displayMode == 2) ? 0 : 2;
                else if (res == 11) { self->viewStartNorm = 0.0f; self->viewEndNorm = 1.0f; }
                else if (res == 6) setParam("normalize", norm ? 0.0f : 1.0f);
                else if (res == 7) setParam("reverse", rev ? 0.0f : 1.0f);
                self->repaint();
            });
            return;
        }

        // Shift+Click adds or drags a manual Slice Marker directly on the waveform!
        if (e.mods.isShiftDown())
        {
            int sIdx = hitTestSliceIndex(e.x, 8.0f);
            float actualPos = 0.0f;
            processor.addOrMoveSliceMarker(sIdx, pixelXToNorm((float)e.x), actualPos);
            const auto& slices = processor.getSliceMarkers();
            for (int i = 0; i < (int)slices.size(); ++i)
            {
                if (std::abs(slices[(size_t)i] - actualPos) < 0.0005f)
                {
                    draggingMarker = 100 + i;
                    break;
                }
            }
            if (onHintChanged)
                onHintChanged("Slice marker: " + juce::String((int)std::round(actualPos * 100.0f)) + "%");
            repaint();
            return;
        }

        // Check if dragging Loop Start (1), Loop End (2), Start Offset (3), or Slice Marker (100+i) directly on the waveform!
        draggingMarker = hitTestMarker(e.x);
        if (draggingMarker != 0)
            return;

        // Alt+Drag or Ctrl+Drag exports the precomputed WAV directly to the DAW timeline!
        if (e.mods.isAltDown() || e.mods.isCtrlDown())
        {
            juce::File exported = processor.exportProcessedSampleToTempWav();
            if (exported.existsAsFile())
            {
                juce::DragAndDropContainer::performExternalDragDropOfFiles(
                    { exported.getFullPathName() }, false, this);
            }
            return;
        }

        // Clicking anywhere on the waveform previews the entire sample from Start Offset / Loop Start (1:1 FL Studio Sampler)!
        int root = (int)processor.apvts.getRawParameterValue("rootNote")->load();
        const bool useLoop = processor.apvts.getRawParameterValue("useLoopPoints")->load() > 0.5f;
        const float startOfs = processor.apvts.getRawParameterValue("startOffset")->load();
        const float loopStart = processor.apvts.getRawParameterValue("loopStart")->load();
        const float playStart = (startOfs > 0.0001f) ? startOfs : (useLoop ? loopStart : 0.0f);
        processor.triggerPreviewNote(root, 0.9f, playStart);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (isPanningView)
        {
            const float span = panOrigEnd - panOrigStart;
            const float deltaNorm = -((float)(e.x - panStartMouseX) / (float)juce::jmax(1, getWidth() - 4)) * span;
            viewStartNorm = juce::jlimit(0.0f, 1.0f - span, panOrigStart + deltaNorm);
            viewEndNorm   = viewStartNorm + span;
            repaint();
            return;
        }

        if (draggingMarker == 0)
            return;

        const float rawNorm = pixelXToNorm((float)e.x);
        const float norm = processor.snapNormalizedToZeroCrossing(rawNorm);
        if (draggingMarker == 1) // Loop Start
        {
            const float curEnd = processor.apvts.getRawParameterValue("loopEnd")->load();
            const float clamped = juce::jlimit(0.0f, juce::jmax(0.0f, curEnd - 0.01f), norm);
            if (auto* p = processor.apvts.getParameter("loopStart"))
                p->setValueNotifyingHost(p->convertTo0to1(clamped));
            if (onHintChanged)
                onHintChanged("Loop start: " + juce::String((int)std::round(clamped * 100.0f)) + "%");
        }
        else if (draggingMarker == 2) // Loop End
        {
            const float curStart = processor.apvts.getRawParameterValue("loopStart")->load();
            const float clamped = juce::jlimit(juce::jmin(0.99f, curStart + 0.01f), 1.0f, norm);
            if (auto* p = processor.apvts.getParameter("loopEnd"))
                p->setValueNotifyingHost(p->convertTo0to1(clamped));
            if (onHintChanged)
                onHintChanged("Loop end: " + juce::String((int)std::round(clamped * 100.0f)) + "%");
        }
        else if (draggingMarker == 3) // Start Offset
        {
            if (auto* p = processor.apvts.getParameter("startOffset"))
                p->setValueNotifyingHost(p->convertTo0to1(norm));
            if (onHintChanged)
                onHintChanged("Sample start offset: " + juce::String((int)std::round(norm * 100.0f)) + "%");
        }
        else if (draggingMarker >= 100) // Manual Slice Marker Drag
        {
            int sliceIdx = draggingMarker - 100;
            float actualPos = 0.0f;
            processor.addOrMoveSliceMarker(sliceIdx, norm, actualPos);
            const auto& slices = processor.getSliceMarkers();
            for (int i = 0; i < (int)slices.size(); ++i)
            {
                if (std::abs(slices[(size_t)i] - actualPos) < 0.0005f)
                {
                    draggingMarker = 100 + i;
                    break;
                }
            }
            if (onHintChanged)
                onHintChanged("Slice #" + juce::String(sliceIdx + 1) + ": " + juce::String((int)std::round(actualPos * 100.0f)) + "%");
        }
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (isPanningView)
        {
            isPanningView = false;
            return;
        }
        if (draggingMarker != 0)
        {
            draggingMarker = 0;
            return;
        }
        int root = (int)processor.apvts.getRawParameterValue("rootNote")->load();
        processor.stopPreviewNote(root);
    }

    bool isInterestedInFileDrag(const juce::StringArray& files) override
    {
        for (const auto& f : files)
            if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".mp3")
                || f.endsWithIgnoreCase(".flac") || f.endsWithIgnoreCase(".ogg")
                || f.endsWithIgnoreCase(".aiff") || f.endsWithIgnoreCase(".aif"))
                return true;
        return false;
    }

    void fileDragEnter(const juce::StringArray&, int, int) override
    {
        isDragHover = true;
        repaint();
    }

    void fileDragExit(const juce::StringArray&) override
    {
        isDragHover = false;
        repaint();
    }

    void filesDropped(const juce::StringArray& files, int, int) override
    {
        isDragHover = false;
        for (const auto& path : files)
        {
            juce::File file(path);
            if (file.existsAsFile() && processor.loadSampleFromFile(file))
            {
                refreshIfNeeded();
                if (onSampleDropped)
                    onSampleDropped();
                break;
            }
        }
        repaint();
    }

private:
    int hitTestSliceIndex(int mouseX, float tolerancePx = 5.0f) const
    {
        const int stretchMode = (int)processor.apvts.getRawParameterValue("stretchMode")->load();
        if (!processor.loadSliceMarkers.load(std::memory_order_relaxed) && stretchMode != 5 && stretchMode != 6)
            return -1;
        const auto& slices = processor.getSliceMarkers();
        for (int i = 0; i < (int)slices.size(); ++i)
        {
            float sx = normToPixelX(slices[(size_t)i]);
            if (std::abs((float)mouseX - sx) <= tolerancePx)
                return i;
        }
        return -1;
    }

    int hitTestMarker(int mouseX) const
    {
        const float xfadeVal = processor.apvts.getRawParameterValue("crossfade")->load();
        const float xfLoopStart = processor.computedCrossfadeLoopStart.load(std::memory_order_relaxed);
        const bool hasXfadeLoop = (xfadeVal > 0.001f && xfLoopStart >= 0.0f);
        const bool useLoop = (processor.apvts.getRawParameterValue("useLoopPoints")->load() > 0.5f) || hasXfadeLoop;
        const float lStart = hasXfadeLoop ? xfLoopStart : processor.apvts.getRawParameterValue("loopStart")->load();
        const float lEnd   = hasXfadeLoop ? 1.0f        : processor.apvts.getRawParameterValue("loopEnd")->load();
        const bool showLoopRegion = useLoop && (hasXfadeLoop || lStart > 0.0005f || lEnd < 0.9995f);
        if (showLoopRegion)
        {
            const float lx1 = normToPixelX(lStart);
            const float lx2 = normToPixelX(lEnd);
            if (std::abs((float)mouseX - lx1) <= 6.0f) return 1;
            if (std::abs((float)mouseX - lx2) <= 6.0f) return 2;
        }
        const float startOfs = processor.apvts.getRawParameterValue("startOffset")->load();
        if (startOfs > 0.001f)
        {
            const float sx = normToPixelX(startOfs);
            if (std::abs((float)mouseX - sx) <= 6.0f) return 3;
        }
        int sIdx = hitTestSliceIndex(mouseX, 5.0f);
        if (sIdx >= 0)
            return 100 + sIdx;
        return 0;
    }

    FLSamplerAudioProcessor& processor;
    juce::AudioBuffer<float> displayBuffer;
    uint32_t lastRevision = 0;
    float lastPlayhead = -1.0f;
    float viewStartNorm = 0.0f;
    float viewEndNorm   = 1.0f;
    bool  isPanningView = false;
    int   panStartMouseX = 0;
    float panOrigStart  = 0.0f;
    float panOrigEnd    = 1.0f;
    int   displayMode   = 0; // 0=Mono Peak, 1=Stereo L/R, 2=Spectrum
    int draggingMarker = 0; // 0=none, 1=loopStart, 2=loopEnd, 3=startOffset
    bool isDragHover = false;
};

} // namespace fls
