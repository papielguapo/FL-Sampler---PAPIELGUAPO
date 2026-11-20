#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace fls
{

namespace FLColors
{
    // 14 Skins in exact user-requested cycling order:
    // 0  = FL Studio 2026 ("SKIN: FL")
    // 1  = FL Studio 2026 Dark Theme ("SKIN: FL DARK")
    // 2  = Ableton Live 12 ("SKIN: LIVE")
    // 3  = Apple Logic Pro X ("SKIN: LOGIC")
    // 4  = Avid Pro Tools ("SKIN: PTOOLS")
    // 5  = MAGIX Sequoia ("SKIN: SEQUOIA")
    // 6  = Adobe Audition ("SKIN: AUDITION")
    // 7  = Steinberg Cubase 14 ("SKIN: CUBASE")
    // 8  = Cockos REAPER 7 ("SKIN: REAPER")
    // 9  = Nullsoft Winamp 2.x Classic ("SKIN: WINAMP")
    // 10 = Microsoft Windows 98 Classic ("SKIN: WIN98")
    // 11 = Apple Mac OS X Aqua ("SKIN: MACOSX")
    // 12 = Pure Black Stealth ("SKIN: BLACK")
    // 13 = Pure White Studio ("SKIN: WHITE")
    static constexpr int kNumSkins = 14;
    inline int  skinMode       = 0;
    inline bool isFLDarkSkin   = false;
    inline bool isAbletonSkin  = false;
    inline bool isLogicSkin    = false;
    inline bool isProToolsSkin = false;
    inline bool isSequoiaSkin  = false;
    inline bool isAuditionSkin = false;
    inline bool isCubaseSkin   = false;
    inline bool isReaperSkin   = false;
    inline bool isWinampSkin   = false;
    inline bool isWin98Skin    = false;
    inline bool isMacOSXSkin   = false;
    inline bool isBlackSkin    = false;
    inline bool isWhiteSkin    = false;

    inline void setSkinMode(int mode) noexcept
    {
        skinMode       = juce::jlimit(0, kNumSkins - 1, mode);
        isFLDarkSkin   = (skinMode == 1);
        isAbletonSkin  = (skinMode == 2);
        isLogicSkin    = (skinMode == 3);
        isProToolsSkin = (skinMode == 4);
        isSequoiaSkin  = (skinMode == 5);
        isAuditionSkin = (skinMode == 6);
        isCubaseSkin   = (skinMode == 7);
        isReaperSkin   = (skinMode == 8);
        isWinampSkin   = (skinMode == 9);
        isWin98Skin    = (skinMode == 10);
        isMacOSXSkin   = (skinMode == 11);
        isBlackSkin    = (skinMode == 12);
        isWhiteSkin    = (skinMode == 13);
    }

    inline bool isLightTheme() noexcept
    {
        return isWhiteSkin || isWin98Skin || isMacOSXSkin;
    }

    inline juce::Colour windowBg()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff14161a); // FL Studio 2026 Dark Theme
            case 2:  return juce::Colour(0xff2b2b2b); // Ableton Live 12
            case 3:  return juce::Colour(0xff1b1d20); // Logic Pro X Space Gray
            case 4:  return juce::Colour(0xff25272b); // Pro Tools Dark Slate
            case 5:  return juce::Colour(0xff1c1f24); // MAGIX Sequoia Broadcast Charcoal-Steel
            case 6:  return juce::Colour(0xff191a1c); // Adobe Audition Carbon Dark
            case 7:  return juce::Colour(0xff171a1f); // Cubase 14 Deep Charcoal
            case 8:  return juce::Colour(0xff242829); // REAPER 7 Default TCP Charcoal-Olive
            case 9:  return juce::Colour(0xff21212e); // Winamp Classic Dark Navy-Charcoal Chassis
            case 10: return juce::Colour(0xffc0c0c0); // Windows 98 Classic Grey
            case 11: return juce::Colour(0xffdcdde0); // Mac OS X Brushed Metal
            case 12: return juce::Colour(0xff050506); // Black Stealth
            case 13: return juce::Colour(0xfff4f5f7); // White Studio
            default: return juce::Colour(0xff2f353d); // FL Studio 2026
        }
    }

    inline juce::Colour headerBg()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff101216); // FL Dark Top Header
            case 2:  return juce::Colour(0xff1e1e1e);
            case 3:  return juce::Colour(0xff141518); // Logic Pro X LCD Bar
            case 4:  return juce::Colour(0xff1a1c20); // Pro Tools Top Transport Bar
            case 5:  return juce::Colour(0xff131519); // MAGIX Sequoia Top Mastering Header
            case 6:  return juce::Colour(0xff121315); // Adobe Audition Top Workspace Bar
            case 7:  return juce::Colour(0xff111317); // Cubase Top Toolbar
            case 8:  return juce::Colour(0xff1b1e1f); // REAPER Top Transport Bar
            case 9:  return juce::Colour(0xff181924); // Winamp Classic Gold-Bronzed Title Bar Base
            case 10: return juce::Colour(0xff000080); // Win98 Navy Title Bar (#000080)
            case 11: return juce::Colour(0xffc8cacf); // Mac OS X Aqua Top Metal Bar
            case 12: return juce::Colour(0xff0a0a0c);
            case 13: return juce::Colour(0xffe9ecf0);
            default: return juce::Colour(0xff2b3038);
        }
    }

    inline juce::Colour panelBg()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff1b1e23); // FL Dark Panel Surface
            case 2:  return juce::Colour(0xff383838);
            case 3:  return juce::Colour(0xff26292e); // Logic Pro X Channel Strip Module
            case 4:  return juce::Colour(0xff31343a); // Pro Tools Insert Module Rack
            case 5:  return juce::Colour(0xff262a31); // MAGIX Sequoia Brushed Mastering Strip
            case 6:  return juce::Colour(0xff232528); // Adobe Audition Docked Panel Dark Grey
            case 7:  return juce::Colour(0xff21252d); // Cubase Inspector Zone
            case 8:  return juce::Colour(0xff2e3436); // REAPER 7 Track Control Panel (TCP) Slate
            case 9:  return juce::Colour(0xff2a2b3b); // Winamp Classic Main EQ / Playlist Panel
            case 10: return juce::Colour(0xffc0c0c0); // Win98 3D Face
            case 11: return juce::Colour(0xffeceef2); // Mac OS X Aqua Pinstripe Card
            case 12: return juce::Colour(0xff0b0b0e);
            case 13: return juce::Colour(0xffffffff);
            default: return juce::Colour(0xff343a42);
        }
    }

    inline juce::Colour panelBorder()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff0c0e11); // FL Dark Border
            case 2:  return juce::Colour(0xff141414);
            case 3:  return juce::Colour(0xff121316);
            case 4:  return juce::Colour(0xff141619);
            case 5:  return juce::Colour(0xff3a404a); // MAGIX Sequoia Metallic Steel Frame
            case 6:  return juce::Colour(0xff101113); // Adobe Audition Dark Panel Divider
            case 7:  return juce::Colour(0xff0f1115);
            case 8:  return juce::Colour(0xff16191a); // REAPER TCP Divider
            case 9:  return juce::Colour(0xff525573); // Winamp Classic Metallic Bevel Rim
            case 10: return juce::Colour(0xff808080); // Win98 Shadow Grey
            case 11: return juce::Colour(0xffa6acb5); // Mac OS X Aqua Frame Border
            case 12: return juce::Colour(0xff222226);
            case 13: return juce::Colour(0xffcfd6e0);
            default: return juce::Colour(0xff20242a);
        }
    }

    inline juce::Colour recessedBg()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff111317); // FL Dark Recessed Well
            case 2:  return juce::Colour(0xff1f1f1f);
            case 3:  return juce::Colour(0xff14161a); // Logic Pro X Dark Well
            case 4:  return juce::Colour(0xff14171a); // Pro Tools LCD Recessed Display
            case 5:  return juce::Colour(0xff13161a); // MAGIX Sequoia Deep Console Well
            case 6:  return juce::Colour(0xff141517); // Adobe Audition Recessed Control Slot
            case 7:  return juce::Colour(0xff13161b); // Cubase Dark Slot
            case 8:  return juce::Colour(0xff181b1c); // REAPER Recessed Meter/Readout Well
            case 9:  return juce::Colour(0xff080a08); // Winamp Classic Black LCD Readout Well
            case 10: return juce::Colour(0xffffffff); // Win98 Sunken White Edit Box
            case 11: return juce::Colour(0xffffffff); // Mac OS X Aqua White Recessed Well
            case 12: return juce::Colour(0xff030304);
            case 13: return juce::Colour(0xffe2e6ec);
            default: return juce::Colour(0xff24282f);
        }
    }

    inline juce::Colour waveBg()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff0d0f13); // FL Dark Deep Waveform Scope
            case 2:  return juce::Colour(0xff181818);
            case 3:  return juce::Colour(0xff11161c); // Logic Pro X Quick Sampler Display
            case 4:  return juce::Colour(0xff0d131a); // Pro Tools Edit Track Waveform Well
            case 5:  return juce::Colour(0xff0f1318); // MAGIX Sequoia Mastering Object Wave Scope
            case 6:  return juce::Colour(0xff0b0f0e); // Adobe Audition Classic Black-Teal Waveform Editor
            case 7:  return juce::Colour(0xff101318); // Cubase Sampler Track Display
            case 8:  return juce::Colour(0xff131919); // REAPER ReaSamplOmatic5000 Dark Teal-Slate Scope
            case 9:  return juce::Colour(0xff000000); // Winamp Classic Pure Black Spectrum Analyzer Screen
            case 10: return juce::Colour(0xff001208); // Win98 Sound Recorder Classic CRT Scope
            case 11: return juce::Colour(0xfff4f8fc); // Mac OS X Aqua Glossy Scope
            case 12: return juce::Colour(0xff020203);
            case 13: return juce::Colour(0xfff8fafc);
            default: return juce::Colour(0xff262a31);
        }
    }

    inline juce::Colour waveGrid()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff171a21); // FL Dark Subtle Grid
            case 2:  return juce::Colour(0xff262626);
            case 3:  return juce::Colour(0xff1c242e);
            case 4:  return juce::Colour(0xff1a2634);
            case 5:  return juce::Colour(0xff1d242e); // MAGIX Sequoia dB Scale Grid
            case 6:  return juce::Colour(0xff142622); // Adobe Audition Dark Emerald Grid Lines
            case 7:  return juce::Colour(0xff1c222b);
            case 8:  return juce::Colour(0xff212b2b); // REAPER Arrange Grid Line
            case 9:  return juce::Colour(0xff102810); // Winamp Classic Dotted Green Analyzer Grid
            case 10: return juce::Colour(0xff003818); // Win98 Dark Green Oscilloscope Grid
            case 11: return juce::Colour(0xffd6e2f0);
            case 12: return juce::Colour(0xff151518);
            case 13: return juce::Colour(0xffe1e6ee);
            default: return juce::Colour(0xff1e2228);
        }
    }

    inline juce::Colour textBright()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xffdce3ec); // FL Dark Crisp Text
            case 2:  return juce::Colour(0xffe6e6e6);
            case 3:  return juce::Colour(0xffe5e9f0);
            case 4:  return juce::Colour(0xffe4ecf5);
            case 5:  return juce::Colour(0xffe6ecf2); // MAGIX Sequoia Silver-White Mastering Readout
            case 6:  return juce::Colour(0xffdfe6e5); // Adobe Audition Crisp UI Label
            case 7:  return juce::Colour(0xffdce3ec);
            case 8:  return juce::Colour(0xffd9e2e2); // REAPER Crisp TCP Label
            case 9:  return juce::Colour(0xff00ff00); // Winamp Classic Iconic Matrix Green Text
            case 10: return juce::Colour(0xff000000); // Win98 Pure Black Text
            case 11: return juce::Colour(0xff14181f); // Mac OS X Charcoal Text
            case 12: return juce::Colour(0xfffafafa);
            case 13: return juce::Colour(0xff1e242b);
            default: return juce::Colour(0xffd5dbe2);
        }
    }

    inline juce::Colour textDim()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff848e9c); // FL Dark Dim Text
            case 2:  return juce::Colour(0xff9e9e9e);
            case 3:  return juce::Colour(0xff8e96a3);
            case 4:  return juce::Colour(0xff98a4b3);
            case 5:  return juce::Colour(0xff95a0ad);
            case 6:  return juce::Colour(0xff8a9492);
            case 7:  return juce::Colour(0xff848e9c);
            case 8:  return juce::Colour(0xff8d9999);
            case 9:  return juce::Colour(0xffb5a67c); // Winamp Classic Champagne-Bronze Labels
            case 10: return juce::Colour(0xff424242);
            case 11: return juce::Colour(0xff525a66);
            case 12: return juce::Colour(0xff8e8e96);
            case 13: return juce::Colour(0xff647080);
            default: return juce::Colour(0xff9aa3ad);
        }
    }

    inline juce::Colour greenNeon()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff52ff7a); // FL Dark Neon Green
            case 2:  return juce::Colour(0xffffb000); // Ableton Gold
            case 3:  return juce::Colour(0xff26d6ff); // Logic Pro X Quick Sampler Cyan-Blue
            case 4:  return juce::Colour(0xff4eff4e); // Pro Tools Phosphor Meter Green
            case 5:  return juce::Colour(0xffffa019); // MAGIX Sequoia Broadcast Mastering Amber-Gold
            case 6:  return juce::Colour(0xff00f5b8); // Adobe Audition Iconic Spectral Mint-Spring Green
            case 7:  return juce::Colour(0xff00b7ff); // Cubase Steinberg Electric Blue
            case 8:  return juce::Colour(0xff35d0a3); // REAPER 7 Seafoam Teal-Mint Accent
            case 9:  return juce::Colour(0xff00ff00); // Winamp Classic Neon Lime Green (#00FF00)
            case 10: return juce::Colour(0xff008080); // Win98 Classic Teal / Sound Recorder Green
            case 11: return juce::Colour(0xff0066ee); // Mac OS X Aqua Blue Gel
            case 12: return juce::Colour(0xffffffff); // Black Stealth White
            case 13: return juce::Colour(0xff10b981); // White Studio Emerald
            default: return juce::Colour(0xff67ff86); // FL Neon Green
        }
    }

    inline juce::Colour greenLed()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff7ce62e); // FL Dark LED Green
            case 2:  return juce::Colour(0xffffb900);
            case 3:  return juce::Colour(0xff17c2ff); // Logic Pro X Blue Button LED
            case 4:  return juce::Colour(0xff38ff38); // Pro Tools Bright Green LED
            case 5:  return juce::Colour(0xffffb32c); // MAGIX Sequoia Gold Console Switch LED
            case 6:  return juce::Colour(0xff00e5a3); // Adobe Audition Mint-Cyan Toggle LED
            case 7:  return juce::Colour(0xffff7b1c); // Cubase Active Orange Button LED
            case 8:  return juce::Colour(0xff42e695); // REAPER FX Enabled Mint-Green LED
            case 9:  return juce::Colour(0xff00ff00); // Winamp Classic Bright Green Indicator
            case 10: return juce::Colour(0xff00a800); // Win98 Green Check Indicator
            case 11: return juce::Colour(0xff007aff); // Mac OS X Aqua Glowing Blue
            case 12: return juce::Colour(0xffdcdce6);
            case 13: return juce::Colour(0xff16a34a);
            default: return juce::Colour(0xff8be836);
        }
    }

    inline juce::Colour orangeRing()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xffff9419); // FL Dark Vibrant Orange Ring
            case 2:  return juce::Colour(0xff06b6d4); // Ableton Cyan
            case 3:  return juce::Colour(0xffff9f1c); // Logic Pro X Warm Amber
            case 4:  return juce::Colour(0xff00bfff); // Pro Tools Automation Blue
            case 5:  return juce::Colour(0xff38b6ff); // MAGIX Sequoia Spectral Cyan-Blue Ring
            case 6:  return juce::Colour(0xff29b6f6); // Adobe Audition Multitrack Azure-Cyan Ring
            case 7:  return juce::Colour(0xffff7b1c); // Cubase Amber-Orange
            case 8:  return juce::Colour(0xffe6a13c); // REAPER Warm Gold-Amber Send Ring
            case 9:  return juce::Colour(0xffff9900); // Winamp Classic EQ Amber-Orange Peak
            case 10: return juce::Colour(0xff000080); // Win98 Navy Blue
            case 11: return juce::Colour(0xff0055d4); // Mac OS X Deep Aqua
            case 12: return juce::Colour(0xffa1a1aa); // Black Platinum
            case 13: return juce::Colour(0xffea580c); // White Studio Orange
            default: return juce::Colour(0xffff9e2c); // FL Orange
        }
    }

    inline juce::Colour maroonBox()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xff4a1f26); // FL Dark Deep Crimson Readout Box
            case 2:  return juce::Colour(0xff222222);
            case 3:  return juce::Colour(0xff121922); // Logic Pro X Blue-Black LCD
            case 4:  return juce::Colour(0xff101812); // Pro Tools Dark Green Counter LCD
            case 5:  return juce::Colour(0xff11151c); // MAGIX Sequoia Timecode Readout Box
            case 6:  return juce::Colour(0xff0d1413); // Adobe Audition Time Display LCD
            case 7:  return juce::Colour(0xff12161d); // Cubase Digit Readout
            case 8:  return juce::Colour(0xff171c1c); // REAPER Dark Teal Readout Box
            case 9:  return juce::Colour(0xff000000); // Winamp Classic Pure Black kbps/kHz Digital Box
            case 10: return juce::Colour(0xffffffff); // Win98 White Sunken SpinBox
            case 11: return juce::Colour(0xffffffff); // Mac OS X Aqua White Box
            case 12: return juce::Colour(0xff121215);
            case 13: return juce::Colour(0xffe2e7ef);
            default: return juce::Colour(0xff70353d);
        }
    }

    inline juce::Colour maroonText()
    {
        switch (skinMode)
        {
            case 1:  return juce::Colour(0xffff9ca6); // FL Dark Crimson Readout Text
            case 2:  return juce::Colour(0xffffb000);
            case 3:  return juce::Colour(0xff42dcff); // Logic Pro X Cyan LCD Digits
            case 4:  return juce::Colour(0xff52ff52); // Pro Tools Green LCD Digits
            case 5:  return juce::Colour(0xffffb32c); // MAGIX Sequoia Amber-Gold Timecode Digits
            case 6:  return juce::Colour(0xff00f5b8); // Adobe Audition Mint-Emerald Timecode Digits
            case 7:  return juce::Colour(0xff38c8ff); // Cubase Cyan-Blue Readout
            case 8:  return juce::Colour(0xff47e6b8); // REAPER Seafoam Mint Digits
            case 9:  return juce::Colour(0xff00ff00); // Winamp Classic Green 7-Segment Digits
            case 10: return juce::Colour(0xff000080); // Win98 Navy Digits
            case 11: return juce::Colour(0xff0044aa); // Mac OS X Aqua Blue Digits
            case 12: return juce::Colour(0xffffffff);
            case 13: return juce::Colour(0xff0f172a);
            default: return juce::Colour(0xffffb3ba);
        }
    }
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
        setColour(juce::PopupMenu::backgroundColourId,
                  FLColors::isWin98Skin ? juce::Colour(0xffc0c0c0)
                                        : (FLColors::isLightTheme() ? juce::Colour(0xffffffff)
                                                                    : (FLColors::isBlackSkin ? juce::Colour(0xff0b0b0e)
                                                                                             : (FLColors::isAbletonSkin ? juce::Colour(0xff222222) : juce::Colour(0xff262b33)))));
        setColour(juce::PopupMenu::textColourId, FLColors::textBright());
        setColour(juce::PopupMenu::highlightedBackgroundColourId,
                  FLColors::isWin98Skin ? juce::Colour(0xff000080)
                                        : (FLColors::isMacOSXSkin ? juce::Colour(0xff0066ee)
                                                                  : (FLColors::isWhiteSkin ? juce::Colour(0xff10b981)
                                                                                           : (FLColors::isBlackSkin ? juce::Colour(0xff27272a)
                                                                                                                    : (FLColors::isAbletonSkin ? juce::Colour(0xffffb000) : juce::Colour(0xff3e4754))))));
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

        // ==================== SKIN 8: APPLE MAC OS X AQUA ("SKIN: MACOSX") ====================
        if (FLColors::isMacOSXSkin)
        {
            const juce::Colour aquaCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex).darker(0.1f)
                                                          : (isOrangeKnob ? juce::Colour(0xff0055d4) : juce::Colour(0xff0066ee));
            g.setColour(juce::Colour(0xffb4bac2));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            if (std::abs(a1 - a0) > 0.015f || isOrangeKnob)
            {
                juce::Path arcPath;
                arcPath.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, a0, a1, true);
                g.setColour(aquaCol);
                g.strokePath(arcPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // Brushed aluminum & Aqua glass cap with top white specular highlight
            const float capR = radius - 3.0f;
            juce::ColourGradient capGrad(juce::Colour(0xfffdfeff), cx, cy - capR,
                                         juce::Colour(0xffc9d0da), cx, cy + capR, false);
            g.setGradientFill(capGrad);
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xff7c8591));
            g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

            // Top glossy Aqua reflection pill
            g.setColour(juce::Colours::white.withAlpha(0.75f));
            g.fillEllipse(cx - capR * 0.55f, cy - capR * 0.82f, capR * 1.1f, capR * 0.55f);

            const float px = cx + std::sin(angle) * (capR * 0.78f);
            const float py = cy - std::cos(angle) * (capR * 0.78f);
            g.setColour(aquaCol);
            g.drawLine(cx, cy, px, py, 1.8f);
            return;
        }

        // ==================== SKIN 7: WINDOWS 98 CLASSIC ("SKIN: WIN98") ====================
        if (FLColors::isWin98Skin)
        {
            // Classic Win98 3D bevelled circular dial + radial pixel ticks
            g.setColour(juce::Colour(0xff404040));
            for (int t = 0; t <= 8; ++t)
            {
                float a = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * ((float)t / 8.0f);
                g.drawLine(cx + std::sin(a) * (radius + 0.5f), cy - std::cos(a) * (radius + 0.5f),
                           cx + std::sin(a) * (radius + 2.4f), cy - std::cos(a) * (radius + 2.4f), 1.0f);
            }

            const float capR = radius - 1.5f;
            // Win98 3D highlight (top-left white) and shadow (bottom-right dark grey/black)
            g.setColour(juce::Colour(0xff404040));
            g.fillEllipse(cx - capR + 1.0f, cy - capR + 1.0f, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colours::white);
            g.fillEllipse(cx - capR - 0.8f, cy - capR - 0.8f, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xffc0c0c0));
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xff808080));
            g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

            const float px = cx + std::sin(angle) * (capR * 0.78f);
            const float py = cy - std::cos(angle) * (capR * 0.78f);
            g.setColour(juce::Colour(0xff000080));
            g.drawLine(cx, cy, px, py, 2.0f);
            return;
        }

        // ==================== SKIN 5: COCKOS REAPER 7 ("SKIN: REAPER") ====================
        if (FLColors::isReaperSkin)
        {
            const juce::Colour reaperCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex)
                                                            : (isOrangeKnob ? juce::Colour(0xffe6a13c) : juce::Colour(0xff35d0a3));
            g.setColour(juce::Colour(0xff141819));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            juce::Path bgTrack;
            bgTrack.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff232a2c));
            g.strokePath(bgTrack, juce::PathStrokeType(2.2f));

            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            if (std::abs(a1 - a0) > 0.015f || isOrangeKnob)
            {
                juce::Path valArc;
                valArc.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, a0, a1, true);
                g.setColour(reaperCol);
                g.strokePath(valArc, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            const float capR = radius - 3.3f;
            juce::ColourGradient capGrad(juce::Colour(0xff3b4346), cx, cy - capR,
                                         juce::Colour(0xff222729), cx, cy + capR, false);
            g.setGradientFill(capGrad);
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xff111415));
            g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

            const float px = cx + std::sin(angle) * (capR * 0.85f);
            const float py = cy - std::cos(angle) * (capR * 0.85f);
            g.setColour(reaperCol.brighter(0.35f));
            g.drawLine(cx, cy, px, py, 1.8f);
            return;
        }

        // ==================== SKIN 6: WINAMP CLASSIC 2.x ("SKIN: WINAMP") ====================
        if (FLColors::isWinampSkin)
        {
            const juce::Colour winampCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex)
                                                            : (isOrangeKnob ? juce::Colour(0xffffa800) : juce::Colour(0xff00ff00));
            // Classic Winamp 2.x recessed dark LCD well with green-to-orange-to-red EQ LED dots around knob
            g.setColour(juce::Colour(0xff07080d));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
            g.setColour(juce::Colour(0xff4c506b));
            g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);

            const int numLeds = 15;
            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            for (int i = 0; i < numLeds; ++i)
            {
                const float u = (float)i / (float)(numLeds - 1);
                const float la = rotaryStartAngle + u * (rotaryEndAngle - rotaryStartAngle);
                const bool lit = (la >= a0 - 0.03f && la <= a1 + 0.03f) && (std::abs(angle - startA) > 0.015f || isOrangeKnob);
                const float lx = cx + std::sin(la) * (radius - 1.2f);
                const float ly = cy - std::cos(la) * (radius - 1.2f);
                juce::Colour ledCol = (customHex != 0)
                    ? winampCol
                    : (u < 0.55f ? juce::Colour(0xff00ff00) : (u < 0.82f ? juce::Colour(0xffffb800) : juce::Colour(0xffff2a00)));
                g.setColour(lit ? ledCol : juce::Colour(0xff1b1d29));
                g.fillRect(lx - 1.1f, ly - 1.1f, 2.2f, 2.2f);
            }

            // Winamp metallic bronze/silver beveled rotary cap
            const float capR = radius - 3.3f;
            juce::ColourGradient capGrad(juce::Colour(0xff6b708b), cx - capR * 0.4f, cy - capR,
                                         juce::Colour(0xff232533), cx + capR * 0.4f, cy + capR, false);
            g.setGradientFill(capGrad);
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xffb59b52));
            g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

            const float px = cx + std::sin(angle) * (capR * 0.82f);
            const float py = cy - std::cos(angle) * (capR * 0.82f);
            g.setColour(winampCol);
            g.drawLine(cx, cy, px, py, 1.8f);
            return;
        }

        // ==================== SKIN 4: STEINBERG CUBASE 14 ("SKIN: CUBASE") ====================
        if (FLColors::isCubaseSkin)
        {
            const juce::Colour cubaseCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex)
                                                            : (isOrangeKnob ? juce::Colour(0xffff7b1c) : juce::Colour(0xff00b7ff));
            g.setColour(juce::Colour(0xff101318));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            juce::Path bgTrack;
            bgTrack.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff2b313b));
            g.strokePath(bgTrack, juce::PathStrokeType(2.4f));

            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            if (std::abs(a1 - a0) > 0.015f || isOrangeKnob)
            {
                juce::Path valArc;
                valArc.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, a0, a1, true);
                g.setColour(cubaseCol);
                g.strokePath(valArc, juce::PathStrokeType(2.5f));
            }

            const float capR = radius - 3.4f;
            juce::ColourGradient capGrad(juce::Colour(0xff343a45), cx, cy - capR,
                                         juce::Colour(0xff1d2128), cx, cy + capR, false);
            g.setGradientFill(capGrad);
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xff0d0f13));
            g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

            const float px0 = cx + std::sin(angle) * (capR * 0.35f);
            const float py0 = cy - std::cos(angle) * (capR * 0.35f);
            const float px1 = cx + std::sin(angle) * (capR * 0.90f);
            const float py1 = cy - std::cos(angle) * (capR * 0.90f);
            g.setColour(cubaseCol.brighter(0.25f));
            g.drawLine(px0, py0, px1, py1, 1.8f);
            return;
        }

        // ==================== SKIN 5: APPLE LOGIC PRO X ("SKIN: LOGIC") ====================
        if (FLColors::isLogicSkin)
        {
            const juce::Colour logicCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex)
                                                           : (isOrangeKnob ? juce::Colour(0xffff9f1c) : juce::Colour(0xff26d6ff));
            g.setColour(juce::Colour(0xff121417));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            juce::Path bgTrack;
            bgTrack.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff2d3138));
            g.strokePath(bgTrack, juce::PathStrokeType(2.2f));

            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            if (std::abs(a1 - a0) > 0.015f || isOrangeKnob)
            {
                juce::Path valArc;
                valArc.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, a0, a1, true);
                g.setColour(logicCol);
                g.strokePath(valArc, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // Brushed dark space-gray aluminum cap
            const float capR = radius - 3.2f;
            juce::ColourGradient capGrad(juce::Colour(0xff4a4f57), cx - capR * 0.5f, cy - capR,
                                         juce::Colour(0xff23262b), cx + capR * 0.5f, cy + capR, false);
            g.setGradientFill(capGrad);
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xff5e646e));
            g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 0.8f);

            const float px = cx + std::sin(angle) * (capR * 0.82f);
            const float py = cy - std::cos(angle) * (capR * 0.82f);
            g.setColour(juce::Colours::white);
            g.drawLine(cx, cy, px, py, 1.6f);
            return;
        }

        // ==================== SKIN 4B: AVID PRO TOOLS ("SKIN: PTOOLS") ====================
        if (FLColors::isProToolsSkin)
        {
            const juce::Colour ptCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex)
                                                        : (isOrangeKnob ? juce::Colour(0xff00bfff) : juce::Colour(0xff4eff4e));
            // Pro Tools LED Dot Ring around metallic insert rotary encoder
            const int numLeds = 15;
            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            for (int i = 0; i < numLeds; ++i)
            {
                const float u = (float)i / (float)(numLeds - 1);
                const float la = rotaryStartAngle + u * (rotaryEndAngle - rotaryStartAngle);
                const bool lit = (la >= a0 - 0.03f && la <= a1 + 0.03f) && (std::abs(angle - startA) > 0.015f || isOrangeKnob);
                const float lx = cx + std::sin(la) * (radius - 0.5f);
                const float ly = cy - std::cos(la) * (radius - 0.5f);
                g.setColour(lit ? ptCol : juce::Colour(0xff171a1e));
                g.fillEllipse(lx - 1.2f, ly - 1.2f, 2.4f, 2.4f);
            }

            const float capR = radius - 3.2f;
            juce::ColourGradient capGrad(juce::Colour(0xff59606b), cx, cy - capR,
                                         juce::Colour(0xff292d33), cx, cy + capR, false);
            g.setGradientFill(capGrad);
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xff15171a));
            g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.1f);

            const float px = cx + std::sin(angle) * (capR * 0.85f);
            const float py = cy - std::cos(angle) * (capR * 0.85f);
            g.setColour(juce::Colours::white);
            g.drawLine(cx, cy, px, py, 1.7f);
            return;
        }

        // ==================== SKIN 4C: MAGIX SEQUOIA ("SKIN: SEQUOIA") ====================
        if (FLColors::isSequoiaSkin)
        {
            const juce::Colour seqCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex)
                                                         : (isOrangeKnob ? juce::Colour(0xff38b6ff) : juce::Colour(0xffffa019));
            // Precision German Mastering Console encoder: dark recessed collar + 11 ticks + brushed steel cap
            g.setColour(juce::Colour(0xff111419));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            juce::Path bgTrack;
            bgTrack.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff2c323d));
            g.strokePath(bgTrack, juce::PathStrokeType(2.3f));

            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            if (std::abs(a1 - a0) > 0.015f || isOrangeKnob)
            {
                juce::Path valArc;
                valArc.addCentredArc(cx, cy, radius - 1.2f, radius - 1.2f, 0.0f, a0, a1, true);
                g.setColour(seqCol);
                g.strokePath(valArc, juce::PathStrokeType(2.5f));
            }

            const float capR = radius - 3.2f;
            juce::ColourGradient capGrad(juce::Colour(0xff525a68), cx, cy - capR,
                                         juce::Colour(0xff262b33), cx, cy + capR, false);
            g.setGradientFill(capGrad);
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xff737d8c));
            g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 0.9f);

            const float px = cx + std::sin(angle) * (capR * 0.84f);
            const float py = cy - std::cos(angle) * (capR * 0.84f);
            g.setColour(seqCol.brighter(0.25f));
            g.drawLine(cx, cy, px, py, 1.8f);
            return;
        }

        // ==================== SKIN 4D: ADOBE AUDITION ("SKIN: AUDITION") ====================
        if (FLColors::isAuditionSkin)
        {
            const juce::Colour auCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex)
                                                        : (isOrangeKnob ? juce::Colour(0xff29b6f6) : juce::Colour(0xff00f5b8));
            // Adobe Spectrum dark flat dial with glowing mint/teal arc
            g.setColour(juce::Colour(0xff121315));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            juce::Path bgTrack;
            bgTrack.addCentredArc(cx, cy, radius - 1.3f, radius - 1.3f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff2c3034));
            g.strokePath(bgTrack, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            if (std::abs(a1 - a0) > 0.015f || isOrangeKnob)
            {
                juce::Path valArc;
                valArc.addCentredArc(cx, cy, radius - 1.3f, radius - 1.3f, 0.0f, a0, a1, true);
                g.setColour(auCol);
                g.strokePath(valArc, juce::PathStrokeType(2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            const float capR = radius - 3.5f;
            g.setColour(juce::Colour(0xff25272b));
            g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
            g.setColour(juce::Colour(0xff3a3e44));
            g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

            const float px = cx + std::sin(angle) * (capR * 0.85f);
            const float py = cy - std::cos(angle) * (capR * 0.85f);
            g.setColour(auCol);
            g.drawLine(cx, cy, px, py, 1.8f);
            return;
        }

        // ==================== SKIN 4: PURE WHITE STUDIO ("SKIN: WHITE") ====================
        if (FLColors::isWhiteSkin)
        {
            g.setColour(juce::Colour(0xffe2e8f0));
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
            g.setColour(juce::Colour(0xffcbd5e1));
            g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.1f);

            const juce::Colour ringCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex).darker(0.15f)
                                                          : (isGreenKnob ? FLColors::greenNeon()
                                                                         : (isOrangeKnob ? FLColors::orangeRing()
                                                                                         : juce::Colour(0xff334155)));
            const float a0 = isBipolar ? juce::jmin(startA, angle) : rotaryStartAngle;
            const float a1 = isBipolar ? juce::jmax(startA, angle) : angle;
            if (std::abs(a1 - a0) > 0.015f || isOrangeKnob)
            {
                juce::Path arcPath;
                arcPath.addCentredArc(cx, cy, radius - 1.3f, radius - 1.3f, 0.0f, a0, a1, true);
                g.setColour(ringCol);
                g.strokePath(arcPath, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            const float innerR = radius - 3.6f;
            juce::ColourGradient capG(juce::Colour(0xffffffff), cx, cy - innerR,
                                      juce::Colour(0xfff1f5f9), cx, cy + innerR, false);
            g.setGradientFill(capG);
            g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
            g.setColour(juce::Colour(0xff94a3b8));
            g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);

            const float px = cx + std::sin(angle) * (innerR * 0.78f);
            const float py = cy - std::cos(angle) * (innerR * 0.78f);
            g.setColour(ringCol);
            g.fillEllipse(px - 1.6f, py - 1.6f, 3.2f, 3.2f);
            return;
        }

        // ==================== SKIN 3: PURE BLACK STEALTH ("SKIN: BLACK") ====================
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

        // ==================== FL STUDIO 2026 & FL DARK 3D KNOB STYLE ====================
        const juce::Colour customRingCol = (customHex != 0) ? juce::Colour((juce::uint32)customHex)
                                                            : (isGreenKnob ? FLColors::greenNeon()
                                                                           : (isOrangeKnob ? FLColors::orangeRing()
                                                                                           : juce::Colour(FLColors::isFLDarkSkin ? 0xff7f8a98 : 0xff99a4b0)));

        // Outer dark track ring
        g.setColour(juce::Colour(FLColors::isFLDarkSkin ? 0xff0c0e12 : 0xff1d2127));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.5f);

        // Subtle tick marks around outer circumference
        g.setColour(juce::Colour(FLColors::isFLDarkSkin ? 0xff363d47 : 0xff4a525d));
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
            if (angle - rotaryStartAngle > 0.015f)
            {
                juce::Path fullRing;
                fullRing.addCentredArc(cx, cy, radius, radius, 0.0f, rotaryStartAngle, angle, true);
                g.setColour(FLColors::orangeRing());
                g.strokePath(fullRing, juce::PathStrokeType(2.3f));
            }
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
        juce::ColourGradient capGrad(juce::Colour(FLColors::isFLDarkSkin ? 0xff2a2f37 : 0xff434b56), cx, cy - capR,
                                     juce::Colour(FLColors::isFLDarkSkin ? 0xff16191e : 0xff272c33), cx, cy + capR, false);
        g.setGradientFill(capGrad);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        g.setColour(juce::Colour(FLColors::isFLDarkSkin ? 0xff0b0d10 : 0xff191c21));
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

        if (FLColors::isWin98Skin)
        {
            // Classic Win98 sunken white box with raised 3D grey drop button on right
            g.setColour(juce::Colours::white);
            g.fillRect(0, 0, width, height);
            g.setColour(juce::Colour(0xff808080));
            g.drawHorizontalLine(0, 0.0f, (float)width - 1.0f);
            g.drawVerticalLine(0, 0.0f, (float)height - 1.0f);
            g.setColour(juce::Colour(0xff404040));
            g.drawHorizontalLine(1, 1.0f, (float)width - 2.0f);
            g.drawVerticalLine(1, 1.0f, (float)height - 2.0f);

            const int btnW = 16;
            const int bx = width - btnW - 2;
            g.setColour(juce::Colour(0xffc0c0c0));
            g.fillRect(bx, 2, btnW, height - 4);
            g.setColour(juce::Colours::white);
            g.drawHorizontalLine(2, (float)bx, (float)(bx + btnW - 1));
            g.drawVerticalLine(bx, 2.0f, (float)(height - 3));
            g.setColour(juce::Colour(0xff404040));
            g.drawHorizontalLine(height - 3, (float)bx, (float)(bx + btnW));
            g.drawVerticalLine(bx + btnW - 1, 2.0f, (float)(height - 2));

            juce::Path arrow;
            float ax = (float)bx + 4.5f;
            float ay = (float)height * 0.5f - 2.0f;
            arrow.addTriangle(ax, ay, ax + 6.0f, ay, ax + 3.0f, ay + 4.0f);
            g.setColour(juce::Colours::black);
            g.fillPath(arrow);
            return;
        }

        if (FLColors::isMacOSXSkin)
        {
            // Mac OS X Aqua glossy pill combo with blue Aqua arrow button on right
            juce::ColourGradient pillG(juce::Colours::white, 0.0f, 0.0f,
                                       juce::Colour(0xffe6ebf2), 0.0f, (float)height, false);
            g.setGradientFill(pillG);
            g.fillRoundedRectangle(bounds, 5.0f);

            auto capRect = bounds.withLeft((float)width - 18.0f);
            juce::ColourGradient aquaCap(juce::Colour(0xff66b2ff), capRect.getX(), capRect.getY(),
                                         juce::Colour(0xff005ce6), capRect.getX(), capRect.getBottom(), false);
            g.setGradientFill(aquaCap);
            g.fillRoundedRectangle(capRect, 5.0f);

            g.setColour(juce::Colour(0xff8e99a8));
            g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

            juce::Path arrow;
            float ax = (float)width - 12.0f;
            float ay = (float)height * 0.5f - 2.0f;
            arrow.addTriangle(ax, ay, ax + 6.0f, ay, ax + 3.0f, ay + 4.5f);
            g.setColour(juce::Colours::white);
            g.fillPath(arrow);
            return;
        }

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
        const float cy = (float)y + (float)height * 0.5f - 2.0f;
        const float startX = (float)x + 8.0f;
        const float endX   = (float)(x + width) - 8.0f;
        const float midX   = (startX + endX) * 0.5f;
        const float clampedPos = juce::jlimit(startX, endX, sliderPos);

        if (FLColors::isAbletonSkin)
        {
            // Ableton flat rectangular slider bar
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(startX, cy - 4.0f, endX - startX, 8.0f);
            g.setColour(juce::Colour(0xff101010));
            g.drawRect(startX, cy - 4.0f, endX - startX, 8.0f, 1.0f);

            g.setColour(juce::Colour(0xffffb000));
            g.fillRect(juce::jmin(midX, clampedPos), cy - 3.0f, juce::jmax(2.0f, std::abs(clampedPos - midX)), 6.0f);
            g.setColour(juce::Colours::white);
            g.drawLine(clampedPos, cy - 5.0f, clampedPos, cy + 5.0f, 2.0f);
            return;
        }

        if (FLColors::isWinampSkin)
        {
            // Classic Winamp 2.x sunken black EQ/Volume groove with green-to-orange fill & bronze-metallic rectangular thumb
            g.setColour(juce::Colour(0xff07080d));
            g.fillRect(startX, cy - 3.0f, endX - startX, 6.0f);
            g.setColour(juce::Colour(0xff4c506b));
            g.drawRect(startX, cy - 3.0f, endX - startX, 6.0f, 1.0f);

            if (std::abs(clampedPos - midX) > 0.8f)
            {
                g.setColour(juce::Colour(0xff00ff00));
                g.fillRect(juce::jmin(midX, clampedPos), cy - 2.0f, std::abs(clampedPos - midX), 4.0f);
            }

            juce::Rectangle<int> th((int)clampedPos - 6, (int)cy - 6, 12, 12);
            juce::ColourGradient thG(juce::Colour(0xffd2b86a), (float)th.getX(), (float)th.getY(),
                                     juce::Colour(0xff6b5a2b), (float)th.getX(), (float)th.getBottom(), false);
            g.setGradientFill(thG);
            g.fillRect(th);
            g.setColour(juce::Colour(0xffffe599));
            g.drawHorizontalLine(th.getY(), (float)th.getX(), (float)th.getRight() - 1.0f);
            g.drawVerticalLine(th.getX(), (float)th.getY(), (float)th.getBottom() - 1.0f);
            g.setColour(juce::Colour(0xff14151f));
            g.drawRect(th, 1);
            g.setColour(juce::Colour(0xff00ff00));
            g.drawVerticalLine((int)clampedPos, (float)th.getY() + 2.0f, (float)th.getBottom() - 2.0f);
            return;
        }

        if (FLColors::isWin98Skin)
        {
            // Classic Win98 sunken track channel + 3D raised rectangular thumb + ruler ticks
            g.setColour(juce::Colour(0xff808080));
            g.fillRect(startX, cy - 2.0f, endX - startX, 4.0f);
            g.setColour(juce::Colours::white);
            g.drawHorizontalLine((int)(cy + 2.0f), startX, endX);

            g.setColour(juce::Colour(0xff404040));
            for (int i = 0; i <= 16; ++i)
            {
                float tx = startX + (endX - startX) * ((float)i / 16.0f);
                g.drawVerticalLine((int)tx, cy + 4.0f, cy + (i % 4 == 0 ? 10.0f : 7.5f));
            }

            juce::Rectangle<int> th((int)clampedPos - 5, (int)cy - 7, 10, 14);
            g.setColour(juce::Colour(0xffc0c0c0));
            g.fillRect(th);
            g.setColour(juce::Colours::white);
            g.drawHorizontalLine(th.getY(), (float)th.getX(), (float)th.getRight() - 1.0f);
            g.drawVerticalLine(th.getX(), (float)th.getY(), (float)th.getBottom() - 1.0f);
            g.setColour(juce::Colour(0xff404040));
            g.drawHorizontalLine(th.getBottom() - 1, (float)th.getX(), (float)th.getRight());
            g.drawVerticalLine(th.getRight() - 1, (float)th.getY(), (float)th.getBottom());
            return;
        }

        // Horizontal recessed track slot
        const bool isLight = FLColors::isLightTheme();
        g.setColour(isLight ? juce::Colour(0xffcbd5e1) : juce::Colour(0xff23272e));
        g.fillRoundedRectangle(startX, cy - 1.5f, endX - startX, 3.0f, 1.5f);
        g.setColour(isLight ? juce::Colour(0xff94a3b8) : juce::Colour(0xff181b20));
        g.drawRoundedRectangle(startX, cy - 1.5f, endX - startX, 3.0f, 1.5f, 0.8f);

        // 1:1 FL Studio ruler tick marks under track (24 subdivisions: major at 0, 12, 24, medium at 6, 18)
        g.setColour(isLight ? juce::Colour(0xff94a3b8) : juce::Colour(0xff23272e));
        const int numTicks = 24;
        for (int i = 0; i <= numTicks; ++i)
        {
            const float tx = startX + (endX - startX) * ((float)i / (float)numTicks);
            const bool isMajor = (i == 0 || i == 12 || i == 24);
            const bool isMed   = (i == 6 || i == 18);
            const float tickBottom = cy + (isMajor ? 11.5f : (isMed ? 9.5f : 8.0f));
            g.drawLine(tx, cy + 3.0f, tx, tickBottom, 1.0f);
        }

        const bool isOrange = slider.getProperties().getWithDefault("flOrangeRing", false);
        const bool isRedBipolar = slider.getProperties().getWithDefault("flRedSlider", false)
                               || slider.getProperties().getWithDefault("flBipolar", false);
        const juce::Colour accentCol = FLColors::isBlackSkin ? juce::Colours::white
                                     : (FLColors::isMacOSXSkin ? juce::Colour(0xff0066ee)
                                     : (isOrange && !slider.getProperties().getWithDefault("flRedSlider", false)
                                         ? FLColors::orangeRing()
                                         : juce::Colour(0xffff5c6c)));

        if ((isOrange || isRedBipolar) && std::abs(clampedPos - midX) > 0.8f)
        {
            g.setColour(accentCol);
            g.fillRect(juce::jmin(midX, clampedPos), cy - 1.1f, std::abs(clampedPos - midX), 2.2f);
        }

        // 1:1 FL Studio circular 3D thumb handle with vertical indicator line
        const float thumbR = 7.2f;
        juce::Rectangle<float> thumb(clampedPos - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f);

        // Drop shadow
        g.setColour(juce::Colours::black.withAlpha(isLight ? 0.18f : 0.45f));
        g.fillEllipse(thumb.translated(0.0f, 1.2f));

        // Metallic circular gradient body
        juce::ColourGradient thumbGrad(isLight ? juce::Colour(0xffffffff) : juce::Colour(0xff525a65), clampedPos, thumb.getY(),
                                       isLight ? juce::Colour(0xffe2e8f0) : juce::Colour(0xff2b3038), clampedPos, thumb.getBottom(), false);
        g.setGradientFill(thumbGrad);
        g.fillEllipse(thumb);

        // Outer dark border & inner top specular rim
        g.setColour(isLight ? juce::Colour(0xff64748b) : juce::Colour(0xff16191e));
        g.drawEllipse(thumb, 1.1f);

        g.setColour(accentCol);
        g.drawLine(clampedPos, thumb.getY() + 2.5f, clampedPos, thumb.getBottom() - 2.5f, 1.6f);
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

        juce::Component::SafePointer<FLSlider> safeThis(this);
        m.showMenuAsync(juce::PopupMenu::Options(), [safeThis](int res)
        {
            if (safeThis == nullptr)
                return;
            if (res == 1)
                safeThis->resetToDefault();
            else if (res == 2)
                sClipboardNorm = safeThis->valueToProportionOfLength(safeThis->getValue());
            else if (res == 3)
                safeThis->setValue(safeThis->proportionOfLengthToValue(juce::jlimit(0.0, 1.0, sClipboardNorm)), juce::sendNotificationSync);
            else if (res >= 10 && res <= 14)
            {
                double p = (res - 10) * 0.25;
                safeThis->setValue(safeThis->proportionOfLengthToValue(p), juce::sendNotificationSync);
            }
            safeThis->emitHint();
        });
    }
};

// FL Studio ComboBox with Mouse Wheel Scrolling (scroll up/down to cycle items without opening popup!)
// and Middle-Click / Alt+Click reset to first item
class FLComboBox : public juce::ComboBox
{
public:
    std::function<void(const juce::String&)> onHintChanged;
    int defaultResetIndex = 0;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isMiddleButtonDown() || e.mods.isAltDown())
        {
            if (getNumItems() > 0)
                setSelectedItemIndex(juce::jlimit(0, getNumItems() - 1, defaultResetIndex), juce::sendNotificationSync);
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

        if (FLColors::isWin98Skin)
        {
            // Classic Win98 sunken white 3D checkbox with crisp black checkmark
            const int boxS = 11;
            const int bx = 2;
            const int by = ((int)h - boxS) / 2;
            g.setColour(juce::Colours::white);
            g.fillRect(bx, by, boxS, boxS);
            g.setColour(juce::Colour(0xff808080));
            g.drawHorizontalLine(by, (float)bx, (float)(bx + boxS - 1));
            g.drawVerticalLine(bx, (float)by, (float)(by + boxS - 1));
            g.setColour(juce::Colour(0xff404040));
            g.drawHorizontalLine(by + 1, (float)(bx + 1), (float)(bx + boxS - 2));
            g.drawVerticalLine(bx + 1, (float)(by + 1), (float)(by + boxS - 2));
            g.setColour(juce::Colour(0xffdfdfdf));
            g.drawHorizontalLine(by + boxS - 1, (float)bx, (float)(bx + boxS));
            g.drawVerticalLine(bx + boxS - 1, (float)by, (float)(by + boxS));

            if (getToggleState())
            {
                juce::Path chk;
                chk.startNewSubPath((float)bx + 2.5f, (float)by + 5.5f);
                chk.lineTo((float)bx + 4.8f, (float)by + 8.0f);
                chk.lineTo((float)bx + 8.8f, (float)by + 2.8f);
                g.setColour(juce::Colours::black);
                g.strokePath(chk, juce::PathStrokeType(1.6f));
            }

            g.setColour(getParentComponent() && getBounds().getY() < 40 ? juce::Colours::white : juce::Colours::black);
            g.setFont(juce::Font(juce::FontOptions(12.0f)));
            g.drawText(getButtonText(), bx + boxS + 5, 0, getWidth() - (bx + boxS + 5), getHeight(),
                       juce::Justification::centredLeft, false);
            return;
        }

        // Outer dark ring
        const bool isLight = FLColors::isLightTheme();
        g.setColour(isLight ? juce::Colour(0xff94a3b8) : juce::Colour(0xff181b20));
        g.fillEllipse(ledX - 1.0f, ledY - 1.0f, ledSize + 2.0f, ledSize + 2.0f);

        if (getToggleState())
        {
            juce::Colour c = useBrightGreen ? FLColors::greenLed() : (isLight ? juce::Colour(0xff0284c7) : juce::Colour(0xff9ab0c7));
            g.setColour(c);
            g.fillEllipse(ledX, ledY, ledSize, ledSize);

            g.setColour(juce::Colours::white.withAlpha(0.55f));
            g.fillEllipse(ledX + 2.0f, ledY + 1.8f, 3.0f, 3.0f);
        }
        else
        {
            g.setColour(isLight ? (isOver ? juce::Colour(0xffcbd5e1) : juce::Colour(0xffe2e8f0))
                                : (isOver ? juce::Colour(0xff454d58) : juce::Colour(0xff323841)));
            g.fillEllipse(ledX, ledY, ledSize, ledSize);
        }

        g.setColour(isEnabled() ? (isOver ? (isLight ? juce::Colours::black : juce::Colours::white) : FLColors::textBright()) : FLColors::textDim());
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
        if (FLColors::isWin98Skin)
        {
            g.setColour(juce::Colours::white);
            g.fillRect(getLocalBounds());
            g.setColour(juce::Colour(0xff808080));
            g.drawHorizontalLine(0, 0.0f, (float)getWidth() - 1.0f);
            g.drawVerticalLine(0, 0.0f, (float)getHeight() - 1.0f);
            g.setColour(juce::Colour(0xff404040));
            g.drawHorizontalLine(1, 1.0f, (float)getWidth() - 2.0f);
            g.drawVerticalLine(1, 1.0f, (float)getHeight() - 2.0f);
            g.setColour(juce::Colour(0xff000080));
            g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
            juce::String txt = (dashZero && val == 0) ? "---" : juce::String(val);
            g.drawText(txt, getLocalBounds().withTrimmedRight(isGrey ? 8 : 0), juce::Justification::centred, false);
            return;
        }

        g.setColour(FLColors::isLightTheme() ? juce::Colour(0xffe2e8f0)
                    : (FLColors::isAbletonSkin ? juce::Colour(0xff1e1e1e)
                                               : (isGrey && (FLColors::skinMode == 0 || FLColors::isFLDarkSkin)
                                                   ? juce::Colour(FLColors::isFLDarkSkin ? 0xff2b3038 : 0xff525862)
                                                   : FLColors::maroonBox())));
        g.fillRoundedRectangle(b, FLColors::isAbletonSkin ? 0.0f : 3.0f);

        g.setColour(FLColors::panelBorder());
        g.drawRoundedRectangle(b, FLColors::isAbletonSkin ? 0.0f : 3.0f, 1.0f);

        g.setColour(FLColors::isLightTheme() ? FLColors::maroonText()
                    : (FLColors::isAbletonSkin ? juce::Colour(0xffffb000)
                                               : (isGrey && (FLColors::skinMode == 0 || FLColors::isFLDarkSkin)
                                                   ? juce::Colour(0xffdce2e8)
                                                   : FLColors::maroonText())));
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
            juce::Component::SafePointer<FLDigiWheel> safeThis(this);
            m.showMenuAsync(juce::PopupMenu::Options(), [safeThis](int res)
            {
                if (safeThis != nullptr && res == 1)
                    safeThis->setValue(safeThis->defVal, true);
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
