#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace fls
{

namespace FLColors
{
    // 0 = FL Studio 2026 ("SKIN: FL"), 1 = Ableton Live 12 ("SKIN: LIVE"), 2 = GrowthTune Private ("PRIVATE"), 3 = Pure Black ("SKIN: BLACK")
    inline int  skinMode = 0;
    inline bool isAbletonSkin = false;
    inline bool isPrivateSkin = false;
    inline bool isBlackSkin   = false;

    inline void setSkinMode(int mode) noexcept
    {
        skinMode = juce::jlimit(0, 3, mode);
        isAbletonSkin = (skinMode == 1);
        isPrivateSkin = (skinMode == 2);
        isBlackSkin   = (skinMode == 3);
    }

    // Exact GrowthTune vector signature ("papielguapo") for bottom-right overlay
    inline void drawPapielguapoSignature(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour)
    {
        juce::Path signature;
        signature.startNewSubPath(0,22);signature.lineTo(12,2);signature.cubicTo(25,-3,25,7,13,12);signature.lineTo(7,12);
        signature.startNewSubPath(14,18);signature.cubicTo(21,6,29,9,22,18);signature.cubicTo(18,24,13,22,18,16);signature.lineTo(28,12);signature.lineTo(24,20);signature.lineTo(34,14);
        signature.startNewSubPath(29,29);signature.lineTo(39,10);signature.cubicTo(49,5,48,15,40,19);signature.lineTo(36,18);signature.lineTo(49,13);
        signature.startNewSubPath(51,10);signature.lineTo(46,20);signature.quadraticTo(49,23,57,14);
        signature.startNewSubPath(54,6);signature.lineTo(55,4);
        signature.startNewSubPath(54,17);signature.cubicTo(66,16,67,9,61,11);signature.cubicTo(50,17,56,26,68,17);
        signature.cubicTo(82,1,80,-3,73,8);signature.lineTo(68,19);signature.quadraticTo(68,23,78,16);
        signature.cubicTo(86,6,93,10,86,18);signature.cubicTo(79,26,77,16,84,13);signature.lineTo(91,11);signature.lineTo(81,29);signature.cubicTo(75,37,70,29,88,22);
        signature.lineTo(95,13);signature.lineTo(91,20);signature.quadraticTo(92,24,101,14);signature.lineTo(98,20);signature.quadraticTo(101,22,108,14);
        signature.cubicTo(116,7,122,10,115,18);signature.cubicTo(108,26,106,17,112,13);signature.lineTo(120,11);signature.lineTo(116,20);signature.lineTo(127,13);
        signature.startNewSubPath(121,29);signature.lineTo(132,10);signature.cubicTo(42+100,5,142,15,134,19);signature.lineTo(129,18);signature.lineTo(143,13);
        signature.cubicTo(153,6,156,12,148,19);signature.cubicTo(140,25,137,17,146,12);signature.quadraticTo(149,17,155,13);

        auto transform = signature.getTransformToScaleToFit(area, true);
        g.setColour(juce::Colours::black.withAlpha(0.75f));
        g.strokePath(signature, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), transform.translated(0.0f, 1.0f));
        g.setColour(colour);
        g.strokePath(signature, juce::PathStrokeType(1.75f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), transform);
    }

    inline void drawPapielguapoSignature(juce::Graphics& g, float x, float y, float height, juce::Colour colour)
    {
        const float width = height * 4.2f;
        drawPapielguapoSignature(g, juce::Rectangle<float>(x, y, width, height), colour);
    }

    inline juce::Colour windowBg()    { return isBlackSkin ? juce::Colour(0xff050506) : (isPrivateSkin ? juce::Colour(0xff121316) : (isAbletonSkin ? juce::Colour(0xff2b2b2b) : juce::Colour(0xff2f353d))); }
    inline juce::Colour headerBg()    { return isBlackSkin ? juce::Colour(0xff0a0a0c) : (isPrivateSkin ? juce::Colour(0xff17181c) : (isAbletonSkin ? juce::Colour(0xff1e1e1e) : juce::Colour(0xff2b3038))); }
    inline juce::Colour panelBg()     { return isBlackSkin ? juce::Colour(0xff0b0b0e) : (isPrivateSkin ? juce::Colour(0xff17181c) : (isAbletonSkin ? juce::Colour(0xff383838) : juce::Colour(0xff343a42))); }
    inline juce::Colour panelBorder() { return isBlackSkin ? juce::Colour(0xff222226) : (isPrivateSkin ? juce::Colour(0xff2e3038) : (isAbletonSkin ? juce::Colour(0xff141414) : juce::Colour(0xff20242a))); }
    inline juce::Colour recessedBg()  { return isBlackSkin ? juce::Colour(0xff030304) : (isPrivateSkin ? juce::Colour(0xff0d0e10) : (isAbletonSkin ? juce::Colour(0xff1f1f1f) : juce::Colour(0xff24282f))); }
    inline juce::Colour waveBg()      { return isBlackSkin ? juce::Colour(0xff020203) : (isPrivateSkin ? juce::Colour(0xff0d0d10) : (isAbletonSkin ? juce::Colour(0xff181818) : juce::Colour(0xff262a31))); }
    inline juce::Colour waveGrid()    { return isBlackSkin ? juce::Colour(0xff151518) : (isPrivateSkin ? juce::Colour(0xff24222c) : (isAbletonSkin ? juce::Colour(0xff262626) : juce::Colour(0xff1e2228))); }
    inline juce::Colour textBright()  { return isBlackSkin ? juce::Colour(0xfffafafa) : (isPrivateSkin ? juce::Colour(0xfff2f1f7) : (isAbletonSkin ? juce::Colour(0xffe6e6e6) : juce::Colour(0xffd5dbe2))); }
    inline juce::Colour textDim()     { return isBlackSkin ? juce::Colour(0xff8e8e96) : (isPrivateSkin ? juce::Colour(0xffa09eac) : (isAbletonSkin ? juce::Colour(0xff9e9e9e) : juce::Colour(0xff9aa3ad))); }
    inline juce::Colour greenNeon()   { return isBlackSkin ? juce::Colour(0xffffffff) : (isPrivateSkin ? juce::Colour(0xffec4899) : (isAbletonSkin ? juce::Colour(0xffffb000) : juce::Colour(0xff67ff86))); } // Pure White / GrowthTune Hot Pink / Ableton Gold / FL Neon Green
    inline juce::Colour greenLed()    { return isBlackSkin ? juce::Colour(0xffdcdce6) : (isPrivateSkin ? juce::Colour(0xffa855f7) : (isAbletonSkin ? juce::Colour(0xffffb900) : juce::Colour(0xff8be836))); } // Silver-White / GrowthTune Neon Violet / Ableton Yellow / FL LED Green
    inline juce::Colour orangeRing()  { return isBlackSkin ? juce::Colour(0xffa1a1aa) : (isPrivateSkin ? juce::Colour(0xff06b6d4) : (isAbletonSkin ? juce::Colour(0xff06b6d4) : juce::Colour(0xffff9e2c))); } // Platinum / GrowthTune Cyan / Ableton Cyan / FL Orange
    inline juce::Colour maroonBox()   { return isBlackSkin ? juce::Colour(0xff121215) : (isPrivateSkin ? juce::Colour(0xff1e1628) : (isAbletonSkin ? juce::Colour(0xff222222) : juce::Colour(0xff70353d))); }
    inline juce::Colour maroonText()  { return isBlackSkin ? juce::Colour(0xffffffff) : (isPrivateSkin ? juce::Colour(0xffeccdf1) : (isAbletonSkin ? juce::Colour(0xffffb000) : juce::Colour(0xffffb3ba))); }
}

class FLLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FLLookAndFeel()
    {
        refreshThemeColours();
    }

    void refreshThemeColours()
    {
        setColour(juce::ComboBox::backgroundColourId, FLColors::recessedBg());
        setColour(juce::ComboBox::textColourId, FLColors::textBright());
        setColour(juce::ComboBox::outlineColourId, FLColors::panelBorder());
        setColour(juce::ComboBox::arrowColourId, FLColors::textDim());
        setColour(juce::PopupMenu::backgroundColourId, FLColors::isPrivateSkin ? juce::Colour(0xff141219) : (FLColors::isAbletonSkin ? juce::Colour(0xff222222) : juce::Colour(0xff262b33)));
        setColour(juce::PopupMenu::textColourId, FLColors::textBright());
        setColour(juce::PopupMenu::highlightedBackgroundColourId, FLColors::isPrivateSkin ? juce::Colour(0xff7c3aed) : (FLColors::isAbletonSkin ? juce::Colour(0xffffb000) : juce::Colour(0xff3e4754)));
        setColour(juce::PopupMenu::highlightedTextColourId, FLColors::isAbletonSkin ? juce::Colours::black : juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        const float cx = (float)x + (float)width * 0.5f;
        const float cy = (float)y + (float)height * 0.5f;
        const float radius = juce::jmin((float)width, (float)height) * 0.5f - 3.0f;
        if (radius <= 4.0f)
            return;

        const bool isGreenKnob  = slider.getProperties().getWithDefault("flGreenRing", false);
        const bool isOrangeKnob = slider.getProperties().getWithDefault("flOrangeRing", false);
        const bool isBipolar    = slider.getProperties().getWithDefault("flBipolar", false);
        const int  customHex    = slider.getProperties().getWithDefault("flRingColour", 0);

        const float angle  = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const float startA = isBipolar ? (rotaryStartAngle + rotaryEndAngle) * 0.5f : rotaryStartAngle;

        // ==================== SKIN 4: PURE BLACK STEALTH ("SKIN: BLACK") ====================
        if (FLColors::isBlackSkin)
        {
            g.setColour(juce::Colour(0xff040405));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
            g.setColour(juce::Colour(0xff27272a));
            g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.1f);

            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            if (std::abs(a1 - a0) > 0.015f || isOrangeKnob)
            {
                juce::Path arcPath;
                arcPath.addCentredArc(cx, cy, radius - 1.4f, radius - 1.4f, 0.0f, a0, a1, true);
                g.setColour(juce::Colours::white.withAlpha(0.92f));
                g.strokePath(arcPath, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            const float innerR = radius - 3.8f;
            g.setColour(juce::Colour(0xff0e0e11));
            g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

            const float px = cx + std::sin(angle) * (innerR * 0.85f);
            const float py = cy - std::cos(angle) * (innerR * 0.85f);
            g.setColour(juce::Colours::white);
            g.drawLine(cx, cy, px, py, 1.8f);
            return;
        }

        // ==================== SKIN 3: GROWTHTUNE "PRIVATE" NEON OBSIDIAN DIAL STYLE ====================
        if (FLColors::isPrivateSkin)
        {
            // Obsidian dark housing with purple/magenta neon glow & radial tick ring (matching GrowthTune Knob.cpp)
            g.setColour(juce::Colour(0xff0d0e10));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            // Subtle outer bevel
            g.setColour(juce::Colour(0xff2e3038));
            g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.1f);

            // Soft neon halo under lit arc
            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            if (std::abs(a1 - a0) > 0.015f)
            {
                juce::Path glowArc;
                glowArc.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, a0, a1, true);
                juce::ColourGradient halo(juce::Colour(0xffec4899).withAlpha(0.28f), cx - radius, cy + radius,
                                          juce::Colour(0xff06b6d4).withAlpha(0.32f), cx + radius, cy - radius, false);
                g.setGradientFill(halo);
                g.strokePath(glowArc, juce::PathStrokeType(4.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // Radial tick ring (GrowthTune signature rose -> violet -> cyan ticks)
            const int numTicks = 21;
            for (int i = 0; i < numTicks; ++i)
            {
                const float u = (float)i / (float)(numTicks - 1);
                const float ta = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * u;
                const bool isLit = (ta >= a0 - 0.02f && ta <= a1 + 0.02f) && (std::abs(angle - startA) > 0.015f || !isBipolar);
                juce::Colour tickCol = (u < 0.5f)
                    ? juce::Colour(0xffec4899).interpolatedWith(juce::Colour(0xffa855f7), u * 2.0f)
                    : juce::Colour(0xffa855f7).interpolatedWith(juce::Colour(0xff06b6d4), (u - 0.5f) * 2.0f);
                g.setColour(isLit ? tickCol.brighter(0.2f) : tickCol.withAlpha(0.22f));
                const float rIn = radius - 3.2f;
                const float rOut = radius - 0.4f;
                g.drawLine(cx + std::sin(ta) * rIn, cy - std::cos(ta) * rIn,
                           cx + std::sin(ta) * rOut, cy - std::cos(ta) * rOut, 1.35f);
            }

            // Inner obsidian cap
            const float innerR = radius - 4.2f;
            juce::ColourGradient capG(juce::Colour(0xff23212b), cx, cy - innerR,
                                      juce::Colour(0xff131218), cx, cy + innerR, false);
            g.setGradientFill(capG);
            g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

            // Lavender/White pointer line + glowing tip dot
            const float px = cx + std::sin(angle) * (innerR * 0.82f);
            const float py = cy - std::cos(angle) * (innerR * 0.82f);
            g.setColour(juce::Colour(0xffeccdf1));
            g.drawLine(cx, cy, px, py, 1.7f);
            g.setColour(juce::Colour(0xffec4899));
            g.fillEllipse(px - 1.6f, py - 1.6f, 3.2f, 3.2f);
            return;
        }

        // ==================== ABLETON LIVE 12 FLAT MACRO DIAL STYLE ====================
        if (FLColors::isAbletonSkin)
        {
            const juce::Colour abletonArcCol = (customHex != 0)
                ? juce::Colour((juce::uint32)customHex)
                : (isOrangeKnob ? juce::Colour(0xff00d2ff) : juce::Colour(0xffffb000)); // Ableton Cyan or Gold-Orange

            // Flat dark circular recessed well
            g.setColour(juce::Colour(0xff1c1c1c));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            // Unfilled arc track (Ableton dark slate track)
            juce::Path bgTrack;
            bgTrack.addCentredArc(cx, cy, radius - 1.5f, radius - 1.5f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff2c2c2c));
            g.strokePath(bgTrack, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

            // Active Ableton flat arc
            if (std::abs(angle - startA) > 0.015f || isOrangeKnob)
            {
                juce::Path valArc;
                float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
                float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
                valArc.addCentredArc(cx, cy, radius - 1.5f, radius - 1.5f, 0.0f, a0, a1, true);
                g.setColour(abletonArcCol);
                g.strokePath(valArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
            }

            // Flat matte inner circle + crisp white radial pointer line from center to edge (100% Ableton Live dial!)
            const float innerR = radius - 3.6f;
            g.setColour(juce::Colour(0xff2b2b2b));
            g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

            const float px = cx + std::sin(angle) * (radius - 0.5f);
            const float py = cy - std::cos(angle) * (radius - 0.5f);
            g.setColour(juce::Colour(0xfff0f0f0));
            g.drawLine(cx, cy, px, py, 1.8f);
            return;
        }

        // ==================== FL STUDIO 2026 3D KNOB STYLE ====================
        const juce::Colour customRingCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex)
                                                            : (isGreenKnob ? FLColors::greenNeon()
                                                                           : (isOrangeKnob ? FLColors::orangeRing()
                                                                                           : juce::Colour(0xff99a4b0)));

        // Outer dark track ring
        g.setColour(juce::Colour(0xff1d2127));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.5f);

        // Subtle tick marks around outer circumference
        g.setColour(juce::Colour(0xff4a525d));
        for (int t = 0; t <= 6; ++t)
        {
            float a = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * ((float)t / 6.0f);
            float sx = cx + std::sin(a) * (radius + 1.5f);
            float sy = cy - std::cos(a) * (radius + 1.5f);
            float ex = cx + std::sin(a) * (radius + 2.8f);
            float ey = cy - std::cos(a) * (radius + 2.8f);
            g.drawLine(sx, sy, ex, ey, 1.0f);
        }

        if (isOrangeKnob && customHex == 0)
        {
            juce::Path fullRing;
            fullRing.addCentredArc(cx, cy, radius, radius, 0.0f, rotaryStartAngle, angle, true);
            g.setColour(FLColors::orangeRing());
            g.strokePath(fullRing, juce::PathStrokeType(2.3f));
        }
        else if (std::abs(angle - startA) > 0.02f || customHex != 0)
        {
            juce::Path valArc;
            valArc.addCentredArc(cx, cy, radius, radius, 0.0f,
                                 juce::jmin(startA, angle), juce::jmax(startA, angle), true);
            g.setColour(customRingCol);
            g.strokePath(valArc, juce::PathStrokeType(2.2f));
        }

        // Inner 3D cap with metallic bevel
        const float capR = radius - 2.6f;
        juce::ColourGradient capGrad(juce::Colour(0xff434b56), cx, cy - capR,
                                     juce::Colour(0xff272c33), cx, cy + capR, false);
        g.setGradientFill(capGrad);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        g.setColour(juce::Colour(0xff191c21));
        g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

        // Pointer dot/line on cap
        const float pLen = capR * 0.78f;
        const float px = cx + std::sin(angle) * pLen;
        const float py = cy - std::cos(angle) * pLen;
        g.setColour(customRingCol);
        g.fillEllipse(px - 1.5f, py - 1.5f, 3.0f, 3.0f);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
        g.setColour(FLColors::recessedBg());
        g.fillRoundedRectangle(bounds, FLColors::isAbletonSkin ? 0.0f : 2.5f);

        g.setColour(FLColors::panelBorder());
        g.drawRoundedRectangle(bounds, FLColors::isAbletonSkin ? 0.0f : 2.5f, 1.0f);

        // Small downward triangle on right
        juce::Path arrow;
        float ax = (float)width - 12.0f;
        float ay = (float)height * 0.5f - 2.0f;
        arrow.addTriangle(ax, ay, ax + 6.0f, ay, ax + 3.0f, ay + 4.5f);
        g.setColour(box.isEnabled() ? (FLColors::isAbletonSkin ? juce::Colour(0xffffb000) : FLColors::textBright()) : FLColors::textDim());
        g.fillPath(arrow);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::Font(juce::FontOptions(12.0f));
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float, float,
                          const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        const float cy = (float)y + (float)height * 0.5f;
        const float startX = (float)x + 6.0f;
        const float endX   = (float)(x + width) - 6.0f;
        const float midX   = (startX + endX) * 0.5f;

        if (FLColors::isAbletonSkin)
        {
            // Ableton flat rectangular slider bar
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(startX, cy - 4.0f, endX - startX, 8.0f);
            g.setColour(juce::Colour(0xff101010));
            g.drawRect(startX, cy - 4.0f, endX - startX, 8.0f, 1.0f);

            g.setColour(juce::Colour(0xffffb000));
            g.fillRect(juce::jmin(midX, sliderPos), cy - 3.0f, juce::jmax(2.0f, std::abs(sliderPos - midX)), 6.0f);
            g.setColour(juce::Colours::white);
            g.drawLine(sliderPos, cy - 5.0f, sliderPos, cy + 5.0f, 2.0f);
            return;
        }

        // Horizontal recessed track slot
        g.setColour(juce::Colour(0xff191c20));
        g.fillRoundedRectangle(startX, cy - 2.0f, endX - startX, 4.0f, 2.0f);

        // Tick marks under track
        g.setColour(juce::Colour(0xff23272e));
        for (int i = 0; i <= 8; ++i)
        {
            float tx = startX + (endX - startX) * (i / 8.0f);
            g.drawVerticalLine((int)tx, cy + 4.0f, cy + 10.0f);
        }

        const bool isOrange = slider.getProperties().getWithDefault("flOrangeRing", false);
        if (isOrange && std::abs(sliderPos - midX) > 1.0f)
        {
            g.setColour(FLColors::orangeRing());
            g.fillRect(juce::jmin(midX, sliderPos), cy - 1.2f, std::abs(sliderPos - midX), 2.4f);
        }

        // Pill-shaped thumb handle with vertical indicator line
        const float thumbW = 10.0f;
        const float thumbH = 16.0f;
        juce::Rectangle<float> thumb(sliderPos - thumbW * 0.5f, cy - thumbH * 0.5f, thumbW, thumbH);
        g.setColour(juce::Colour(0xff383f48));
        g.fillRoundedRectangle(thumb, 4.0f);
        g.setColour(juce::Colour(0xff14171b));
        g.drawRoundedRectangle(thumb, 4.0f, 1.0f);

        g.setColour(isOrange ? FLColors::orangeRing() : juce::Colour(0xffff5252));
        g.drawLine(sliderPos, thumb.getY() + 3.0f, sliderPos, thumb.getBottom() - 3.0f, 1.5f);
    }
};

// FL Studio Rotary/Linear Slider with Authentic FL Studio Mouse Actions:
// - Middle-Click (Wheel Button), Alt+Left-Click, or Double-Click: Instant Reset to Default Value!
// - Right-Click: FL Studio Context Menu ("Reset", "Set to 0% / 25% / 50% / 75% / 100%", "Copy value", "Paste value")
// - Mouse Wheel: Smoothly increments/decrements knob/slider value when hovering
// - Ctrl + Drag: Fine-grained precision adjustment
class FLSlider : public juce::Slider
{
public:
    std::function<void(const juce::String&)> onHintChanged;

    FLSlider()
    {
        setVelocityModeParameters(1.0, 1, 0.08, true, juce::ModifierKeys::ctrlModifier);
        setDoubleClickReturnValue(true, 0.0);
    }

    void setDefaultResetValue(double defVal)
    {
        defaultResetVal = defVal;
        hasCustomDefault = true;
        setDoubleClickReturnValue(true, defVal);
    }

    double getResetValue() const
    {
        if (hasCustomDefault)
            return defaultResetVal;
        return getDoubleClickReturnValue();
    }

    void resetToDefault()
    {
        setValue(getResetValue(), juce::sendNotificationSync);
        emitHint();
    }

    void mouseEnter(const juce::MouseEvent& e) override
    {
        juce::Slider::mouseEnter(e);
        emitHint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // FL Studio signature behavior: Middle-Click or Alt+Click resets immediately!
        if (e.mods.isMiddleButtonDown() || e.mods.isAltDown())
        {
            resetToDefault();
            return;
        }

        // Right-Click opens FL Studio Context Menu ("Reset", "Copy value", "Paste value", presets)
        if (e.mods.isRightButtonDown())
        {
            showFLContextMenu();
            return;
        }

        juce::Slider::mouseDown(e);
        emitHint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.mods.isMiddleButtonDown() || e.mods.isRightButtonDown())
            return;
        juce::Slider::mouseDrag(e);
        emitHint();
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (!isEnabled())
            return;
        const double range = getMaximum() - getMinimum();
        if (range <= 0.0)
            return;
        const double step = range * 0.025 * (wheel.deltaY > 0.0f ? 1.0 : -1.0);
        setValue(juce::jlimit(getMinimum(), getMaximum(), getValue() + step), juce::sendNotificationSync);
        emitHint();
    }

private:
    double defaultResetVal = 0.0;
    bool hasCustomDefault = false;
    static inline double sClipboardNorm = 0.5;

    void emitHint()
    {
        if (!onHintChanged)
            return;
        const double norm = valueToProportionOfLength(getValue());
        const bool bipolar = getProperties().getWithDefault("flBipolar", false);
        juce::String pctStr;
        if (bipolar)
        {
            int pct = (int)std::round((norm * 2.0 - 1.0) * 100.0);
            pctStr = (pct > 0 ? "+" : "") + juce::String(pct) + "%";
        }
        else
        {
            int pct = (int)std::round(norm * 100.0);
            pctStr = juce::String(pct) + "%";
        }
        juce::String title = getName().isNotEmpty() ? getName() : getTooltip();
        if (title.isNotEmpty())
            onHintChanged(title + ": " + pctStr + " (" + getTextFromValue(getValue()) + ")");
        else
            onHintChanged(pctStr);
    }

    void showFLContextMenu()
    {
        juce::PopupMenu m;
        m.addItem(1, "Reset");
        m.addSeparator();
        m.addItem(2, "Copy value");
        m.addItem(3, "Paste value (" + juce::String((int)std::round(sClipboardNorm * 100.0)) + "%)");
        m.addSeparator();
        m.addItem(10, "Set to 0%");
        m.addItem(11, "Set to 25%");
        m.addItem(12, "Set to 50%");
        m.addItem(13, "Set to 75%");
        m.addItem(14, "Set to 100%");

        m.showMenuAsync(juce::PopupMenu::Options(), [this](int res)
        {
            if (res == 1)
                resetToDefault();
            else if (res == 2)
                sClipboardNorm = valueToProportionOfLength(getValue());
            else if (res == 3)
                setValue(proportionOfLengthToValue(juce::jlimit(0.0, 1.0, sClipboardNorm)), juce::sendNotificationSync);
            else if (res >= 10 && res <= 14)
            {
                double p = (res - 10) * 0.25;
                setValue(proportionOfLengthToValue(p), juce::sendNotificationSync);
            }
            emitHint();
        });
    }
};

// FL Studio ComboBox with Mouse Wheel Scrolling (scroll up/down to cycle items without opening popup!)
// and Middle-Click / Alt+Click reset to first item
class FLComboBox : public juce::ComboBox
{
public:
    std::function<void(const juce::String&)> onHintChanged;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isMiddleButtonDown() || e.mods.isAltDown())
        {
            if (getNumItems() > 0)
                setSelectedItemIndex(0, juce::sendNotificationSync);
            return;
        }
        juce::ComboBox::mouseDown(e);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        const int count = getNumItems();
        if (count <= 1)
            return;
        const int cur = getSelectedItemIndex();
        const int delta = (wheel.deltaY > 0.0f) ? -1 : 1; // Scroll up = previous item, scroll down = next item
        const int next = juce::jlimit(0, count - 1, (cur < 0 ? 0 : cur) + delta);
        if (next != cur)
        {
            setSelectedItemIndex(next, juce::sendNotificationSync);
            if (onHintChanged)
                onHintChanged((getName().isNotEmpty() ? getName() + ": " : "") + getItemText(next));
        }
    }
};

// FL Studio circular LED check button (used in Content, Playback, Precomputed effects, Header ON)
class FLLedCheckButton : public juce::Button
{
public:
    explicit FLLedCheckButton(const juce::String& labelText, bool brightGreen = false)
        : juce::Button(labelText), useBrightGreen(brightGreen)
    {
        setClickingTogglesState(true);
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void paintButton(juce::Graphics& g, bool isOver, bool) override
    {
        const float h = (float)getHeight();
        const float ledSize = 9.0f;
        const float ledX = 2.0f;
        const float ledY = (h - ledSize) * 0.5f;

        if (FLColors::isAbletonSkin)
        {
            // Ableton Live flat square yellow-gold activator box
            g.setColour(juce::Colour(0xff141414));
            g.fillRect(ledX - 1.0f, ledY - 1.0f, ledSize + 2.0f, ledSize + 2.0f);
            g.setColour(getToggleState() ? juce::Colour(0xffffb900)
                                         : (isOver ? juce::Colour(0xff4a4a4a) : juce::Colour(0xff2c2c2c)));
            g.fillRect(ledX, ledY, ledSize, ledSize);

            g.setColour(isEnabled() ? (getToggleState() ? juce::Colours::white : FLColors::textBright()) : FLColors::textDim());
            g.setFont(juce::Font(juce::FontOptions(12.0f)));
            g.drawText(getButtonText(), (int)(ledX + ledSize + 6.0f), 0,
                       getWidth() - (int)(ledX + ledSize + 6.0f), getHeight(),
                       juce::Justification::centredLeft, false);
            return;
        }

        // Outer dark ring
        g.setColour(juce::Colour(0xff181b20));
        g.fillEllipse(ledX - 1.0f, ledY - 1.0f, ledSize + 2.0f, ledSize + 2.0f);

        if (getToggleState())
        {
            juce::Colour c = useBrightGreen ? FLColors::greenLed() : juce::Colour(0xff9ab0c7);
            g.setColour(c);
            g.fillEllipse(ledX, ledY, ledSize, ledSize);

            g.setColour(juce::Colours::white.withAlpha(0.55f));
            g.fillEllipse(ledX + 2.0f, ledY + 1.8f, 3.0f, 3.0f);
        }
        else
        {
            g.setColour(isOver ? juce::Colour(0xff454d58) : juce::Colour(0xff323841));
            g.fillEllipse(ledX, ledY, ledSize, ledSize);
        }

        g.setColour(isEnabled() ? (isOver ? juce::Colours::white : FLColors::textBright()) : FLColors::textDim());
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText(getButtonText(), (int)(ledX + ledSize + 6.0f), 0,
                   getWidth() - (int)(ledX + ledSize + 6.0f), getHeight(),
                   juce::Justification::centredLeft, false);
    }

private:
    bool useBrightGreen = false;
};

// Maroon numeric drag box (used for RANGE "2" and TRACK "---" and Max Poly / Arp / Echo digits)
class FLDigiWheel : public juce::Component
{
public:
    std::function<void(int)> onValueChanged;

    FLDigiWheel(int minV, int maxV, int defaultV, bool showDashesWhenZero = false, bool greyTheme = false)
        : minVal(minV), maxVal(maxV), val(defaultV), defVal(defaultV), dashZero(showDashesWhenZero), isGrey(greyTheme)
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }

    void setValue(int v, bool notify = false)
    {
        val = juce::jlimit(minVal, maxVal, v);
        repaint();
        if (notify && onValueChanged)
            onValueChanged(val);
    }

    int getValue() const noexcept { return val; }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(FLColors::isAbletonSkin ? juce::Colour(0xff1e1e1e) : (isGrey ? juce::Colour(0xff525862) : FLColors::maroonBox()));
        g.fillRoundedRectangle(b, FLColors::isAbletonSkin ? 0.0f : 3.0f);

        g.setColour(FLColors::panelBorder());
        g.drawRoundedRectangle(b, FLColors::isAbletonSkin ? 0.0f : 3.0f, 1.0f);

        g.setColour(FLColors::isAbletonSkin ? juce::Colour(0xffffb000) : (isGrey ? juce::Colour(0xffdce2e8) : FLColors::maroonText()));
        g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        juce::String txt = (dashZero && val == 0) ? "---" : juce::String(val);
        g.drawText(txt, getLocalBounds().withTrimmedRight(isGrey ? 8 : 0), juce::Justification::centred, false);

        if (isGrey)
        {
            // Small up/down step arrows on the right side
            float ax = b.getRight() - 7.0f;
            float cy = b.getCentreY();
            juce::Path upA, dnA;
            upA.addTriangle(ax, cy - 2.0f, ax + 5.0f, cy - 2.0f, ax + 2.5f, cy - 5.5f);
            dnA.addTriangle(ax, cy + 2.0f, ax + 5.0f, cy + 2.0f, ax + 2.5f, cy + 5.5f);
            g.setColour(juce::Colour(0xff9aa3ad));
            g.fillPath(upA);
            g.fillPath(dnA);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isMiddleButtonDown() || e.mods.isAltDown())
        {
            setValue(defVal, true);
            return;
        }
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu m;
            m.addItem(1, "Reset");
            m.showMenuAsync(juce::PopupMenu::Options(), [this](int res)
            {
                if (res == 1)
                    setValue(defVal, true);
            });
            return;
        }
        if (isGrey && e.x >= getWidth() - 10)
        {
            setValue(val + (e.y < getHeight() / 2 ? 1 : -1), true);
            return;
        }
        dragStartVal = val;
        dragStartY = e.y;
    }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        setValue(defVal, true);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.mods.isMiddleButtonDown() || e.mods.isRightButtonDown())
            return;
        int delta = (dragStartY - e.y) / 5;
        setValue(dragStartVal + delta, true);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        setValue(val + (wheel.deltaY > 0 ? 1 : -1), true);
    }

private:
    int minVal = 0, maxVal = 48, val = 2, defVal = 2;
    int dragStartVal = 2, dragStartY = 0;
    bool dashZero = false;
    bool isGrey = false;
};

} // namespace fls
