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
        const float cornerR = FLColors::isAbletonSkin ? 0.0f : (FLColors::isPrivateSkin ? 4.0f : 2.0f);

        // Dark charcoal / obsidian waveform background
        g.setColour(FLColors::waveBg());
        g.fillRoundedRectangle(b, cornerR);

        if (FLColors::isPrivateSkin)
        {
            juce::ColourGradient glow(juce::Colour(0xffa855f7).withAlpha(0.11f), b.getCentreX(), b.getY(),
                                      juce::Colours::transparentBlack, b.getCentreX(), b.getBottom(), false);
            g.setGradientFill(glow);
            g.fillRoundedRectangle(b, cornerR);
        }

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
                    ? (FLColors::isPrivateSkin ? juce::Colour(0xffec4899) : (FLColors::isAbletonSkin ? juce::Colour(0xff00d2ff) : juce::Colour(0xff9aa5b1)))
                    : (FLColors::isPrivateSkin ? juce::Colour(0xff06b6d4) : (FLColors::isAbletonSkin ? juce::Colour(0xffffb000) : juce::Colour(0xff7fb2cc)));
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

            g.setColour(FLColors::isPrivateSkin ? juce::Colour(0xffa855f7).withAlpha(0.34f)
                        : (FLColors::isAbletonSkin ? juce::Colour(0xff00d2ff).withAlpha(0.32f)
                                                   : juce::Colour(0xff9aa5b1).withAlpha(0.42f)));
            g.fillPath(waveFillPath);

            g.setColour(FLColors::isPrivateSkin ? juce::Colour(0xffec4899).withAlpha(0.92f)
                        : (FLColors::isAbletonSkin ? juce::Colour(0xff42e3ff).withAlpha(0.92f)
                                                   : juce::Colour(0xffcfd7e0).withAlpha(0.85f)));
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

        // Draw FL Studio 1:1 Loop Start / Loop End crimson markers + shaded region + top inward flags + Crossfade cone
        const bool useLoop  = processor.apvts.getRawParameterValue("useLoopPoints")->load() > 0.5f;
        const bool pingPong = processor.apvts.getRawParameterValue("pingPongLoop")->load() > 0.5f;
        if (useLoop)
        {
            const float lStart = processor.apvts.getRawParameterValue("loopStart")->load();
            const float lEnd   = processor.apvts.getRawParameterValue("loopEnd")->load();
            const float xfade  = processor.apvts.getRawParameterValue("crossfade")->load();

            float lx1 = juce::jlimit(2.0f, w - 2.0f, normToPixelX(lStart));
            float lx2 = juce::jlimit(2.0f, w - 2.0f, normToPixelX(lEnd));
            if (lx2 < lx1 + 2.0f) lx2 = lx1 + 2.0f;

            g.setColour(juce::Colour(0xffff3b3b).withAlpha(0.16f));
            g.fillRect(lx1, 2.0f, lx2 - lx1, h - 4.0f);

            if (xfade > 0.002f)
            {
                float xfW = xfade * 0.45f * (lx2 - lx1);
                juce::Path xfPath;
                xfPath.startNewSubPath(lx2 - xfW, h - 2.0f);
                xfPath.lineTo(lx2, 2.0f);
                xfPath.lineTo(lx2, h - 2.0f);
                xfPath.closeSubPath();
                g.setColour(juce::Colour(0xffff9e2c).withAlpha(0.22f));
                g.fillPath(xfPath);
                g.setColour(juce::Colour(0xffff9e2c).withAlpha(0.75f));
                g.drawLine(lx2 - xfW, h - 2.0f, lx2, 2.0f, 1.3f);
            }

            const juce::Colour loopCol(0xffff4a4a);
            g.setColour(loopCol);
            g.drawLine(lx1, 2.0f, lx1, h - 2.0f, 1.5f);
            g.drawLine(lx2, 2.0f, lx2, h - 2.0f, 1.5f);

            juce::Path startFlag, endFlag;
            startFlag.addTriangle(lx1, 2.0f, lx1 + 9.0f, 2.0f, lx1, 11.0f);
            endFlag.addTriangle(lx2, 2.0f, lx2 - 9.0f, 2.0f, lx2, 11.0f);
            g.fillPath(startFlag);
            g.fillPath(endFlag);

            g.drawLine(lx1, h - 2.0f, lx1 + 6.0f, h - 2.0f, 2.0f);
            g.drawLine(lx2 - 6.0f, h - 2.0f, lx2, h - 2.0f, 2.0f);

            if (lx2 - lx1 > 44.0f)
            {
                g.setColour(juce::Colour(0xff1e2228).withAlpha(0.75f));
                g.fillRoundedRectangle(lx1 + 10.0f, 4.0f, pingPong ? 56.0f : 34.0f, 13.0f, 2.5f);
                g.setColour(juce::Colour(0xffff8a8a));
                g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
                g.drawText(pingPong ? "<-> LOOP" : "LOOP",
                           (int)(lx1 + 10.0f), 4, pingPong ? 56 : 34, 13,
                           juce::Justification::centred, false);
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
            m.addItem(12, "Detect pitch & set root note (" + juce::MidiMessage::getMidiNoteName(processor.getDetectedSampleRootNote(), true, true, 5) + ")");
            m.addSeparator();
            m.addItem(2, "Use loop points", true, useLoop);
            m.addItem(3, "Ping pong loop", true, pingPong);
            m.addItem(4, "Set loop start here");
            m.addItem(5, "Set loop end here");
            m.addItem(8, "Snap loop/start to zero-crossing", true, zSnap);
            m.addSeparator();
            m.addItem(9, "Multichannel waveform view", true, displayMode == 1);
            m.addItem(10, "Spectrum view", true, displayMode == 2);
            m.addItem(11, "Reset zoom (100%)");
            m.addSeparator();
            m.addItem(6, "Normalize", true, norm);
            m.addItem(7, "Reverse", true, rev);

            const float clickNorm = processor.snapNormalizedToZeroCrossing(pixelXToNorm((float)e.x));
            m.showMenuAsync(juce::PopupMenu::Options(), [this, clickNorm, useLoop, pingPong, rev, norm, zSnap](int res)
            {
                auto setParam = [this](const char* id, float v) {
                    if (auto* p = processor.apvts.getParameter(id))
                        p->setValueNotifyingHost(p->convertTo0to1(v));
                };
                if (res == 1 && processor.getLoadedFile().existsAsFile())
                    processor.loadSampleFromFile(processor.getLoadedFile());
                else if (res == 12) setParam("rootNote", (float)processor.getDetectedSampleRootNote());
                else if (res == 2) setParam("useLoopPoints", useLoop ? 0.0f : 1.0f);
                else if (res == 3) setParam("pingPongLoop", pingPong ? 0.0f : 1.0f);
                else if (res == 4) { setParam("useLoopPoints", 1.0f); setParam("loopStart", juce::jmin(0.98f, clickNorm)); }
                else if (res == 5) { setParam("useLoopPoints", 1.0f); setParam("loopEnd", juce::jmax(0.02f, clickNorm)); }
                else if (res == 8) processor.zeroCrossSnap.store(!zSnap);
                else if (res == 9) displayMode = (displayMode == 1) ? 0 : 1;
                else if (res == 10) displayMode = (displayMode == 2) ? 0 : 2;
                else if (res == 11) { viewStartNorm = 0.0f; viewEndNorm = 1.0f; }
                else if (res == 6) setParam("normalize", norm ? 0.0f : 1.0f);
                else if (res == 7) setParam("reverse", rev ? 0.0f : 1.0f);
                repaint();
            });
            return;
        }

        // Check if dragging Loop Start (1), Loop End (2), or Start Offset (3) marker directly on the waveform!
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

        // Click-position scrub preview: clicking anywhere on the waveform plays from that position!
        const float clickNorm = pixelXToNorm((float)e.x);
        int root = (int)processor.apvts.getRawParameterValue("rootNote")->load();
        processor.triggerPreviewNote(root, 0.9f, clickNorm);
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
    int hitTestMarker(int mouseX) const
    {
        const bool useLoop = processor.apvts.getRawParameterValue("useLoopPoints")->load() > 0.5f;
        if (useLoop)
        {
            const float lStart = processor.apvts.getRawParameterValue("loopStart")->load();
            const float lEnd   = processor.apvts.getRawParameterValue("loopEnd")->load();
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
