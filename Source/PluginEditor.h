// PluginEditor.h - Boba UI, estilo Soothe3: fondo oscuro, espectro grande con la zona de reduccion
// resaltada, mandos finos con arco, y controles agrupados abajo. TODO en este unico archivo (header-only).
// PluginEditor.cpp solo necesita:  #include "PluginEditor.h"
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <BinaryData.h>
#include <array>
#include <complex>
#include <functional>
#include <initializer_list>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

namespace bobaui
{
namespace colours
{
    inline const juce::Colour bg1       { 0xff202226 };
    inline const juce::Colour bg2       { 0xff141518 };
    inline const juce::Colour panel     { 0xff1a1b1f };
    inline const juce::Colour panelEdge { 0xff2e3036 };
    inline const juce::Colour display   { 0xff0f1012 };
    inline const juce::Colour text      { 0xffe8e9ec };
    inline const juce::Colour dim       { 0xff80848c };
    inline const juce::Colour accent    { 0xffff7a50 };   // reduccion / valor de los mandos
    inline const juce::Colour accent2   { 0xffffb36b };   // hover
    inline const juce::Colour eq        { 0xff8ec9ff };   // curva del EQ de enfasis
    inline const juce::Colour seg       { 0xff3a3e46 };   // opcion activa en los selectores
}

constexpr float kWidth = 900.0f, kHeight = 540.0f;        // tamano logico; el editor lo escala

inline juce::Font font (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
class BobaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BobaLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, colours::display);
        setColour (juce::ComboBox::outlineColourId, colours::panelEdge);
        setColour (juce::ComboBox::textColourId, colours::text);
        setColour (juce::ComboBox::arrowColourId, colours::dim);
        setColour (juce::PopupMenu::backgroundColourId, colours::panel);
        setColour (juce::PopupMenu::textColourId, colours::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::seg);
        setColour (juce::PopupMenu::highlightedTextColourId, colours::text);
        setColour (juce::Label::textColourId, colours::dim);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (12.0f); }
    juce::Font getPopupMenuFont() override { return font (13.0f); }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        g.setColour (colours::display);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (colours::panelEdge);
        g.drawRoundedRectangle (r, 5.0f, 1.0f);

        const float cx = (float) width - 14.0f, cy = (float) height * 0.5f;
        juce::Path p;
        p.startNewSubPath (cx - 3.5f, cy - 1.5f);
        p.lineTo (cx, cy + 2.0f);
        p.lineTo (cx + 3.5f, cy - 1.5f);
        g.setColour (colours::dim);
        g.strokePath (p, juce::PathStrokeType (1.4f));
    }
};

//==============================================================================
// Mando rotativo fino: titulo arriba, arco de valor, valor abajo.
class Knob : public juce::Slider
{
public:
    using Formatter = std::function<juce::String (double)>;

    Knob (juce::String t, Formatter f) : title (std::move (t)), format (std::move (f))
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setMouseDragSensitivity (220);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        g.setColour (colours::dim);
        g.setFont (font (10.0f, true).withExtraKerningFactor (0.14f));
        g.drawText (title, b.removeFromTop (16.0f), juce::Justification::centred);

        auto valueArea = b.removeFromBottom (18.0f);
        const float dia = std::min (b.getWidth(), b.getHeight()) - 4.0f;
        const auto k = juce::Rectangle<float> (dia, dia).withCentre (b.getCentre());
        const float cx = k.getCentreX(), cy = k.getCentreY(), r = dia * 0.5f;

        const float a0 = -2.35f, a1 = 2.35f;
        const float prop = (float) valueToProportionOfLength (getValue());
        const float ang = a0 + prop * (a1 - a0);
        const bool hot = hover || isMouseButtonDown();

        juce::Path track;
        track.addCentredArc (cx, cy, r - 2.0f, r - 2.0f, 0.0f, a0, a1, true);
        g.setColour (juce::Colour (0xff34363c));
        g.strokePath (track, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        val.addCentredArc (cx, cy, r - 2.0f, r - 2.0f, 0.0f, a0, ang, true);
        g.setColour (hot ? colours::accent2 : colours::accent);
        g.strokePath (val, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const auto body = k.reduced (8.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34363c), body.getX(), body.getY(),
                                                 juce::Colour (0xff1c1d21), body.getX(), body.getBottom(), false));
        g.fillEllipse (body);
        g.setColour (juce::Colour (0xff4a4d55));
        g.drawEllipse (body, 1.0f);

        const float br = body.getWidth() * 0.5f, p0 = br * 0.35f, p1 = br * 0.85f;
        g.setColour (colours::text);
        g.drawLine (cx + p0 * std::sin (ang), cy - p0 * std::cos (ang),
                    cx + p1 * std::sin (ang), cy - p1 * std::cos (ang), 2.0f);

        g.setColour (hot ? colours::text : juce::Colour (0xffb4b7be));
        g.setFont (font (12.0f));
        g.drawText (format ? format (getValue()) : juce::String (getValue(), 1), valueArea, juce::Justification::centred);
    }

    void mouseEnter (const juce::MouseEvent& e) override { juce::Slider::mouseEnter (e); hover = true; repaint(); }
    void mouseExit  (const juce::MouseEvent& e) override { juce::Slider::mouseExit (e);  hover = false; repaint(); }

private:
    juce::String title;
    Formatter format;
    bool hover = false;
};

// Selector de dos opciones: L/R <-> M/S, INT <-> EXT.
class SegToggle : public juce::Button
{
public:
    SegToggle (juce::String off, juce::String on)
        : juce::Button (off + "/" + on), offText (std::move (off)), onText (std::move (on))
    {
        setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();
        const auto left = r.withWidth (r.getWidth() * 0.5f);
        const auto right = r.withLeft (r.getCentreX());

        g.setColour (colours::display);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (over ? colours::seg.brighter (0.15f) : colours::seg);
        g.fillRoundedRectangle ((on ? right : left).reduced (2.0f), 4.0f);
        g.setColour (colours::panelEdge);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);

        g.setFont (font (11.0f, true).withExtraKerningFactor (0.08f));
        g.setColour (on ? colours::dim : colours::text);
        g.drawText (offText, left, juce::Justification::centred);
        g.setColour (on ? colours::text : colours::dim);
        g.drawText (onText, right, juce::Justification::centred);
    }

private:
    juce::String offText, onText;
};

// Boton de texto encendido/apagado: SOLO, DELTA.
class TextToggle : public juce::Button
{
public:
    explicit TextToggle (juce::String t) : juce::Button (t) { setButtonText (t); setClickingTogglesState (true); }

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();
        g.setColour (on ? colours::accent : colours::display);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (on ? colours::accent : colours::panelEdge);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        g.setFont (font (11.0f, true).withExtraKerningFactor (0.1f));
        g.setColour (on ? juce::Colour (0xff1a1208) : (over ? colours::text : colours::dim));
        g.drawText (getButtonText(), r, juce::Justification::centred);
    }
};

// Engranaje de ajustes (FFT, overlap, modo de tiempo).
class GearButton : public juce::Button
{
public:
    GearButton() : juce::Button ("settings") { setClickingTogglesState (true); }

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        const auto r = getLocalBounds().toFloat().reduced (5.0f);
        const float rad = std::min (r.getWidth(), r.getHeight()) * 0.5f;
        const float cx = r.getCentreX(), cy = r.getCentreY();
        g.setColour (getToggleState() || over ? colours::text : colours::dim);
        g.drawEllipse (r.withSizeKeepingCentre (rad * 1.3f, rad * 1.3f), 1.8f);
        g.drawEllipse (r.withSizeKeepingCentre (rad * 0.5f, rad * 0.5f), 1.4f);
        for (int i = 0; i < 8; ++i)
        {
            const float a = (float) i * juce::MathConstants<float>::pi * 0.25f;
            g.drawLine (cx + std::sin (a) * rad * 0.8f, cy - std::cos (a) * rad * 0.8f,
                        cx + std::sin (a) * rad * 1.05f, cy - std::cos (a) * rad * 1.05f, 3.0f);
        }
    }
};

//==============================================================================
// Espectro (entrada rellena, salida en linea, reduccion resaltada) con el EQ de enfasis interactivo.
class EqDisplay : public juce::Component, private juce::Timer
{
public:
    explicit EqDisplay (BobaAudioProcessor& p) : proc (p)
    {
        fft.init (kFFT);
        for (int i = 0; i < kFFT; ++i)
            window[(size_t) i] = (float) (0.5 - 0.5 * std::cos (2.0 * boba::kPi * i / kFFT));
        specIn.fill (-140.0f);
        specOut.fill (-140.0f);

        auto P = [this] (const char* id) { return proc.apvts.getParameter (id); };
        nodes[0] = { "High-pass", highpass, P ("eqHp"),    nullptr,       nullptr };
        nodes[1] = { "Low",       peak,     P ("eq1Freq"), P ("eq1Gain"), P ("eq1Q") };
        nodes[2] = { "Mid",       peak,     P ("eq2Freq"), P ("eq2Gain"), P ("eq2Q") };
        nodes[3] = { "High",      peak,     P ("eq3Freq"), P ("eq3Gain"), P ("eq3Q") };
        nodes[4] = { "Low-pass",  lowpass,  P ("eqLp"),    nullptr,       nullptr };

        startTimerHz (30);
    }
    ~EqDisplay() override { stopTimer(); }

    void updateSpectrum()
    {
        proc.copyScope (false, scratch.data(), kFFT);
        analyse (scratch.data(), specIn);
        proc.copyScope (true, scratch.data(), kFFT);
        analyse (scratch.data(), specOut);
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float w = bounds.getWidth(), h = bounds.getHeight();

        g.setColour (colours::display);
        g.fillRoundedRectangle (bounds, 10.0f);
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip;
            clip.addRoundedRectangle (bounds, 10.0f);
            g.reduceClipRegion (clip, juce::AffineTransform());
            drawGrid (g, w, h);
            drawSpectra (g, w, h);
            drawEq (g, w, h);
            drawOverlay (g);
        }
        g.setColour (colours::panelEdge);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = nodeAt (e.position);
        if (h != hover) { hover = h; setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor); repaint(); }
    }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragging = nodeAt (e.position);
        if (dragging >= 0) beginGesture (dragging);
        repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging < 0) return;
        auto& n = nodes[(size_t) dragging];
        setReal (n.freq, xToFreq (e.position.x));
        if (n.gain != nullptr) setReal (n.gain, yToGain (e.position.y));
        repaint();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging >= 0) endGesture (dragging);
        dragging = -1;
        repaint();
    }
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const int i = nodeAt (e.position);
        if (i < 0) return;
        for (auto* p : { nodes[(size_t) i].freq, nodes[(size_t) i].gain, nodes[(size_t) i].q })
            if (p != nullptr)
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (p->getDefaultValue());
                p->endChangeGesture();
            }
        repaint();
    }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wd) override
    {
        const int i = nodeAt (e.position);
        if (i < 0 || nodes[(size_t) i].q == nullptr) return;
        auto* q = nodes[(size_t) i].q;
        q->beginChangeGesture();
        setReal (q, real (q) * std::exp (wd.deltaY * 2.0f));
        q->endChangeGesture();
        repaint();
    }

private:
    enum Kind { highpass, peak, lowpass };
    struct Node
    {
        const char* name = "";
        Kind kind = peak;
        juce::RangedAudioParameter* freq = nullptr;
        juce::RangedAudioParameter* gain = nullptr;
        juce::RangedAudioParameter* q    = nullptr;
    };

    static constexpr int kFFT = 2048;
    static constexpr int kBins = kFFT / 2 + 1;
    static constexpr float kDbTop = -15.0f, kDbBottom = -105.0f;   // rango visible del espectro

    void timerCallback() override
    {
        updateSpectrum();
        if (isShowing()) repaint();
    }

    double sr() const { const double s = proc.getSampleRate(); return s > 0.0 ? s : 48000.0; }

    // Magnitud en dB (con +4.5 dB/oct de inclinacion para que la musica se vea plana) suavizada en el tiempo.
    void analyse (const float* x, std::array<float, kBins>& smoothed)
    {
        for (int i = 0; i < kFFT; ++i) fbuf[(size_t) i] = { x[i] * window[(size_t) i], 0.0f };
        fft.forward (fbuf.data());
        const double binHz = sr() / kFFT;
        for (int k = 0; k < kBins; ++k)
        {
            const float mag = std::abs (fbuf[(size_t) k]) * (4.0f / (float) kFFT);
            float db = 20.0f * std::log10 (std::max (mag, 1.0e-7f));
            db += 4.5f * (float) std::log2 (std::max ((double) k, 0.5) * binHz / 1000.0);
            auto& s = smoothed[(size_t) k];
            s += (db - s) * (db > s ? 0.5f : 0.12f);
        }
    }

    float sampleSpectrum (const std::array<float, kBins>& s, float f0, float f1) const
    {
        const float binHz = (float) (sr() / kFFT);
        const float b0 = f0 / binHz, b1 = f1 / binHz;
        if (b1 - b0 < 1.0f)
        {
            const float b = juce::jlimit (0.0f, (float) (kBins - 1), 0.5f * (b0 + b1));
            const int i = (int) b, j = std::min (i + 1, kBins - 1);
            return s[(size_t) i] + (s[(size_t) j] - s[(size_t) i]) * (b - (float) i);
        }
        const int i0 = juce::jlimit (0, kBins - 1, (int) std::ceil (b0));
        const int i1 = std::max (i0, juce::jlimit (0, kBins - 1, (int) std::floor (b1)));
        float sum = 0.0f;
        for (int i = i0; i <= i1; ++i) sum += s[(size_t) i];
        return sum / (float) (i1 - i0 + 1);
    }

    void buildPoints (const std::array<float, kBins>& s, int n, float w, std::vector<float>& out)
    {
        out.assign ((size_t) n, 0.0f);
        for (int i = 0; i < n; ++i)
        {
            const float x = w * (float) i / (float) (n - 1);
            out[(size_t) i] = sampleSpectrum (s, xToFreq (x - 1.5f), xToFreq (x + 1.5f));
        }
        tmp = out;
        for (int i = 1; i < n - 1; ++i)
            out[(size_t) i] = 0.25f * tmp[(size_t) i - 1] + 0.5f * tmp[(size_t) i] + 0.25f * tmp[(size_t) i + 1];
    }

    void drawGrid (juce::Graphics& g, float w, float h)
    {
        const float lines[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
        for (float f : lines)
        {
            const float x = freqToX (f);
            const bool major = (f == 100.0f || f == 1000.0f || f == 10000.0f);
            g.setColour (juce::Colours::white.withAlpha (major ? 0.09f : 0.045f));
            g.drawVerticalLine ((int) x, 0.0f, h);
            g.setColour (colours::dim.withAlpha (0.8f));
            g.setFont (font (10.0f));
            const juce::String t = f >= 1000.0f ? juce::String ((int) (f / 1000.0f)) + "k" : juce::String ((int) f);
            g.drawText (t, juce::Rectangle<float> (x + 3.0f, h - 16.0f, 36.0f, 12.0f), juce::Justification::left);
        }
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        for (int i = 1; i < 6; ++i)
            g.drawHorizontalLine ((int) (h * (float) i / 6.0f), 0.0f, w);
    }

    void drawSpectra (juce::Graphics& g, float w, float h)
    {
        const int n = std::max (2, (int) (w / 2.0f) + 1);
        buildPoints (specIn, n, w, ptsIn);
        buildPoints (specOut, n, w, ptsOut);

        auto X = [w, n] (int i) { return w * (float) i / (float) (n - 1); };
        auto Y = [h] (float db) { return juce::jlimit (0.0f, h, h * (kDbTop - db) / (kDbTop - kDbBottom)); };

        juce::Path inLine, outLine, fill, red;
        for (int i = 0; i < n; ++i)
        {
            const float x = X (i), yi = Y (ptsIn[(size_t) i]), yo = Y (ptsOut[(size_t) i]);
            if (i == 0) { inLine.startNewSubPath (x, yi); outLine.startNewSubPath (x, yo); }
            else        { inLine.lineTo (x, yi);          outLine.lineTo (x, yo); }
        }
        fill = inLine;
        fill.lineTo (w, h);
        fill.lineTo (0.0f, h);
        fill.closeSubPath();

        red.startNewSubPath (0.0f, Y (ptsIn[0]));
        for (int i = 0; i < n; ++i) red.lineTo (X (i), Y (ptsIn[(size_t) i]));
        for (int i = n - 1; i >= 0; --i) red.lineTo (X (i), std::max (Y (ptsIn[(size_t) i]), Y (ptsOut[(size_t) i])));
        red.closeSubPath();

        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillPath (fill);

        g.setGradientFill (juce::ColourGradient (colours::accent.withAlpha (0.9f), 0.0f, 0.0f,
                                                 colours::accent.withAlpha (0.35f), 0.0f, h, false));
        g.fillPath (red);

        const juce::PathStrokeType thin (1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        const juce::PathStrokeType thick (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        g.setColour (juce::Colours::white.withAlpha (0.30f));
        g.strokePath (inLine, thin);
        g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.strokePath (outLine, thick);
    }

    void drawEq (juce::Graphics& g, float w, float)
    {
        const auto filters = buildFilters();
        const double fs = sr();
        const float zeroY = gainToY (0.0f);

        juce::Path curve, area;
        for (float x = 0.0f; x <= w; x += 2.0f)
        {
            const double db = juce::jlimit (-24.0, 24.0, responseDb (filters, (double) xToFreq (x), fs));
            const float y = gainToY ((float) db);
            if (x == 0.0f) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
        }
        area = curve;
        area.lineTo (w, zeroY);
        area.lineTo (0.0f, zeroY);
        area.closeSubPath();

        g.setColour (colours::eq.withAlpha (0.07f));
        g.fillPath (area);

        const float dashes[] = { 3.0f, 4.0f };
        g.setColour (colours::eq.withAlpha (0.25f));
        g.drawDashedLine (juce::Line<float> (0.0f, zeroY, w, zeroY), dashes, 2, 1.0f);

        g.setColour (colours::eq.withAlpha (0.9f));
        g.strokePath (curve, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        for (int i = 0; i < (int) nodes.size(); ++i)
        {
            const auto p = nodePos (i);
            const bool hot = (i == hover || i == dragging);
            const float r = hot ? 7.0f : 5.0f;
            g.setColour (i == dragging ? colours::eq : colours::display);
            g.fillEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f);
            g.setColour (colours::eq);
            g.drawEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f, 1.6f);
        }
    }

    void drawOverlay (juce::Graphics& g)
    {
        // leyenda
        struct Item { const char* label; juce::Colour c; float width; };
        const Item items[] = { { "INPUT", juce::Colours::white.withAlpha (0.35f), 66.0f },
                               { "OUTPUT", juce::Colours::white.withAlpha (0.92f), 76.0f },
                               { "REDUCTION", colours::accent, 96.0f } };
        float x = 14.0f;
        g.setFont (font (9.5f, true).withExtraKerningFactor (0.12f));
        for (const auto& it : items)
        {
            g.setColour (it.c);
            g.fillRoundedRectangle (x, 15.0f, 8.0f, 8.0f, 2.0f);
            g.setColour (colours::dim);
            g.drawText (it.label, juce::Rectangle<float> (x + 13.0f, 11.0f, it.width, 16.0f), juce::Justification::left);
            x += it.width + 6.0f;
        }

        // lectura del punto bajo el raton
        const int idx = dragging >= 0 ? dragging : hover;
        if (idx >= 0)
        {
            g.setColour (colours::eq);
            g.setFont (font (12.0f, true));
            g.drawText (describe (idx), getLocalBounds().reduced (14, 10).withHeight (16), juce::Justification::topRight);
        }
    }

    //----------------------------------------------------------------------- EQ
    std::array<boba::Biquad, 5> buildFilters() const
    {
        const double fs = sr();
        std::array<boba::Biquad, 5> f;
        f[0].setHighpass (fs, real (nodes[0].freq), 0.70710678);
        for (size_t i = 0; i < 3; ++i)
            f[1 + i].setPeak (fs, real (nodes[1 + i].freq), real (nodes[1 + i].q), real (nodes[1 + i].gain));
        f[4].setLowpass (fs, real (nodes[4].freq), 0.849070);
        return f;
    }

    static double responseDb (const std::array<boba::Biquad, 5>& f, double freq, double fs)
    {
        const double w = 2.0 * boba::kPi * freq / fs;
        const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
        double db = 0.0;
        for (const auto& b : f)
        {
            const std::complex<double> num = b.b0 + b.b1 * z1 + b.b2 * z2;
            const std::complex<double> den = 1.0 + b.a1 * z1 + b.a2 * z2;
            db += 20.0 * std::log10 (std::max (std::abs (num) / std::max (std::abs (den), 1.0e-12), 1.0e-6));
        }
        return db;
    }

    float freqToX (float f) const { return (float) (getWidth() * std::log ((double) f / 20.0) / std::log (1000.0)); }
    float xToFreq (float x) const { return (float) (20.0 * std::pow (1000.0, (double) x / (double) std::max (1, getWidth()))); }
    // escala del EQ: +-18 dB sobre la mitad superior de la pantalla
    float gainToY (float g) const { const float h = (float) getHeight(); return 0.26f * h - g / 18.0f * 0.20f * h; }
    float yToGain (float y) const { const float h = (float) getHeight(); return (0.26f * h - y) / (0.20f * h) * 18.0f; }

    juce::Point<float> nodePos (int i) const
    {
        const auto& n = nodes[(size_t) i];
        const float f = real (n.freq);
        if (n.gain != nullptr) return { freqToX (f), gainToY (real (n.gain)) };
        const double db = juce::jlimit (-24.0, 24.0, responseDb (buildFilters(), (double) f, sr()));
        return { freqToX (f), gainToY ((float) db) };
    }

    int nodeAt (juce::Point<float> p) const
    {
        int best = -1; float bestD = 12.0f;
        for (int i = 0; i < (int) nodes.size(); ++i)
        {
            const float d = nodePos (i).getDistanceFrom (p);
            if (d < bestD) { bestD = d; best = i; }
        }
        return best;
    }

    static float real (juce::RangedAudioParameter* p) { return p->convertFrom0to1 (p->getValue()); }
    static void setReal (juce::RangedAudioParameter* p, float v) { p->setValueNotifyingHost (p->convertTo0to1 (v)); }

    void beginGesture (int i)
    {
        auto& n = nodes[(size_t) i];
        n.freq->beginChangeGesture();
        if (n.gain != nullptr) n.gain->beginChangeGesture();
    }
    void endGesture (int i)
    {
        auto& n = nodes[(size_t) i];
        n.freq->endChangeGesture();
        if (n.gain != nullptr) n.gain->endChangeGesture();
    }

    juce::String describe (int i) const
    {
        const auto& n = nodes[(size_t) i];
        juce::String s (n.name);
        s << "   " << juce::String (real (n.freq), 0) << " Hz";
        if (n.gain != nullptr) s << "   " << juce::String (real (n.gain), 1) << " dB";
        if (n.q != nullptr)    s << "   Q " << juce::String (real (n.q), 2);
        return s;
    }

    BobaAudioProcessor& proc;
    std::array<Node, 5> nodes;
    int hover = -1, dragging = -1;

    boba::FFT fft;
    std::array<float, kFFT> window {}, scratch {};
    std::array<std::complex<float>, kFFT> fbuf {};
    std::array<float, kBins> specIn {}, specOut {};
    std::vector<float> ptsIn, ptsOut, tmp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqDisplay)
};

//==============================================================================
// FFT size / overlap / time mode (detras del engranaje).
class SettingsPanel : public juce::Component, private juce::Timer
{
public:
    explicit SettingsPanel (BobaAudioProcessor& p) : proc (p)
    {
        const struct { const char* id; const char* name; } rows[] = { { "fftSize", "FFT Size" }, { "overlap", "Overlap" }, { "timeMode", "Time Mode" } };
        for (const auto& r : rows)
        {
            auto it = std::make_unique<Item>();
            it->label.setText (r.name, juce::dontSendNotification);
            it->label.setFont (font (12.0f));
            if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (r.id)))
                it->box.addItemList (cp->choices, 1);
            it->att = std::make_unique<CA> (proc.apvts, r.id, it->box);
            addAndMakeVisible (it->label);
            addAndMakeVisible (it->box);
            items.push_back (std::move (it));
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colour (0xf2191a1d));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (colours::panelEdge);
        g.drawRoundedRectangle (r, 10.0f, 1.0f);

        g.setColour (colours::dim);
        g.setFont (font (10.0f, true).withExtraKerningFactor (0.14f));
        g.drawText ("SETTINGS", 14, 12, 150, 16, juce::Justification::left);

        const double fs = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
        const int lat = proc.getLatencySamples();
        g.setFont (font (11.0f));
        g.drawText ("Latency: " + juce::String (lat) + " samples (" + juce::String (1000.0 * lat / fs, 1) + " ms)",
                    14, 148, getWidth() - 28, 16, juce::Justification::left);
    }

    void resized() override
    {
        int y = 40;
        for (auto& it : items)
        {
            it->label.setBounds (14, y, 92, 26);
            it->box.setBounds (106, y, 112, 26);
            y += 36;
        }
    }

    void visibilityChanged() override
    {
        if (isVisible()) startTimerHz (4); else stopTimer();
    }

private:
    void timerCallback() override { repaint(); }

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    struct Item
    {
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<CA> att;
    };
    BobaAudioProcessor& proc;
    std::vector<std::unique_ptr<Item>> items;
};

//==============================================================================
// Todo en coordenadas logicas 900 x 540; el editor lo escala.
class Content : public juce::Component
{
public:
    explicit Content (BobaAudioProcessor& p)
        : depth ("Depth", [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; }),
          threshold ("Thresh", [] (double v) { return juce::String (v, 1) + " dB"; }),
          attack ("Attack", [] (double v) { return v < 100.0 ? juce::String (v, 1) + " ms" : juce::String (juce::roundToInt (v)) + " ms"; }),
          release ("Release", [] (double v) { return v < 100.0 ? juce::String (v, 1) + " ms" : juce::String (juce::roundToInt (v)) + " ms"; }),
          link ("Link", [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; }),
          stereo ("L/R", "M/S"), sidechain ("INT", "EXT"), solo ("SOLO"), delta ("DELTA"),
          eq (p), settings (p)
    {
        setLookAndFeel (&laf);
        art = juce::ImageCache::getFromMemory (BinaryData::boba_art_jpg, BinaryData::boba_art_jpgSize);

        for (auto* c : std::initializer_list<juce::Component*> { &depth, &threshold, &attack, &release, &link,
                                                                 &stereo, &sidechain, &solo, &delta, &gear, &eq })
            addAndMakeVisible (*c);
        addChildComponent (settings);

        depthAtt     = std::make_unique<SA> (p.apvts, "depth", depth);
        thresholdAtt = std::make_unique<SA> (p.apvts, "threshold", threshold);
        attackAtt    = std::make_unique<SA> (p.apvts, "attack", attack);
        releaseAtt   = std::make_unique<SA> (p.apvts, "release", release);
        linkAtt      = std::make_unique<SA> (p.apvts, "link", link);
        stereoAtt    = std::make_unique<BA> (p.apvts, "stereoMode", stereo);
        sidechainAtt = std::make_unique<BA> (p.apvts, "sidechain", sidechain);
        soloAtt      = std::make_unique<BA> (p.apvts, "solo", solo);
        deltaAtt     = std::make_unique<BA> (p.apvts, "delta", delta);

        gear.onClick = [this] { settings.setVisible (gear.getToggleState()); settings.toFront (false); };
    }

    ~Content() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        g.setGradientFill (juce::ColourGradient (colours::bg1, 0.0f, 0.0f, colours::bg2, 0.0f, kHeight, false));
        g.fillAll();

        // logo + nombre
        if (art.isValid())
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip;
            clip.addRoundedRectangle (20.0f, 10.0f, 32.0f, 32.0f, 8.0f);
            g.reduceClipRegion (clip, juce::AffineTransform());
            const int side = std::min (art.getWidth(), art.getHeight());
            g.drawImage (art, 20, 10, 32, 32, (art.getWidth() - side) / 2, (art.getHeight() - side) / 2, side, side);
        }
        g.setColour (colours::text);
        g.setFont (font (20.0f, true).withExtraKerningFactor (0.3f));
        g.drawText ("BOBA", 62, 7, 160, 24, juce::Justification::left);
        g.setColour (colours::dim);
        g.setFont (font (9.5f, true).withExtraKerningFactor (0.18f));
        g.drawText ("SPECTRAL SIDECHAIN COMPRESSOR", 62, 30, 320, 14, juce::Justification::left);

        // panel inferior
        const juce::Rectangle<float> panel (20.0f, 400.0f, 860.0f, 124.0f);
        g.setColour (colours::panel);
        g.fillRoundedRectangle (panel, 12.0f);
        g.setColour (colours::panelEdge);
        g.drawRoundedRectangle (panel.reduced (0.5f), 12.0f, 1.0f);
        g.drawLine (165.0f, 418.0f, 165.0f, 506.0f, 1.0f);
        g.drawLine (600.0f, 418.0f, 600.0f, 506.0f, 1.0f);

        g.setColour (colours::dim);
        g.setFont (font (9.5f, true).withExtraKerningFactor (0.14f));
        g.drawText ("STEREO MODE", 620, 411, 118, 13, juce::Justification::left);
        g.drawText ("SIDE CHAIN",  746, 411, 118, 13, juce::Justification::left);
        g.drawText ("MONITOR",     620, 469, 118, 13, juce::Justification::left);
    }

    void resized() override
    {
        depth.setBounds (40, 404, 110, 116);
        threshold.setBounds (194, 408, 92, 108);
        attack.setBounds (294, 408, 92, 108);
        release.setBounds (394, 408, 92, 108);
        link.setBounds (494, 408, 92, 108);

        stereo.setBounds (620, 426, 118, 28);
        sidechain.setBounds (746, 426, 118, 28);
        solo.setBounds (620, 484, 118, 28);
        delta.setBounds (746, 484, 118, 28);

        gear.setBounds (848, 12, 28, 28);
        eq.setBounds (20, 56, 860, 332);
        settings.setBounds (640, 64, 232, 176);
    }

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    BobaLookAndFeel laf;
    juce::Image art;

    Knob depth, threshold, attack, release, link;
    SegToggle stereo, sidechain;
    TextToggle solo, delta;
    GearButton gear;
    EqDisplay eq;
    SettingsPanel settings;

    std::unique_ptr<SA> depthAtt, thresholdAtt, attackAtt, releaseAtt, linkAtt;
    std::unique_ptr<BA> stereoAtt, sidechainAtt, soloAtt, deltaAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Content)
};

} // namespace bobaui

//==============================================================================
class BobaEditor : public juce::AudioProcessorEditor
{
public:
    explicit BobaEditor (BobaAudioProcessor& p) : juce::AudioProcessorEditor (p), content (p)
    {
        addAndMakeVisible (content);
        setResizeLimits (600, 360, 1800, 1080);
        getConstrainer()->setFixedAspectRatio ((double) bobaui::kWidth / (double) bobaui::kHeight);
        setResizable (true, true);
        setSize ((int) bobaui::kWidth, (int) bobaui::kHeight);
    }

    void paint (juce::Graphics& g) override { g.fillAll (bobaui::colours::bg2); }

    void resized() override
    {
        const float s = (float) getWidth() / bobaui::kWidth;
        content.setTransform (juce::AffineTransform::scale (s));
        content.setBounds (0, 0, (int) bobaui::kWidth, (int) bobaui::kHeight);
    }

private:
    bobaui::Content content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BobaEditor)
};
