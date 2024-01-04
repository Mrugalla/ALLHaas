#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct Slider :
    juce::Slider
{
    static constexpr float ScrollSpeed = .8f;
    static constexpr float SensitiveScrollSpeed = .2f;

    Slider(ALLHaasAudioProcessor& p, param::PID pID) :
        juce::Slider(),
        attach(*p.apvts.getParameter(param::toID(pID)), *this, nullptr)
    {
        setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setWantsKeyboardFocus(false);
    }

    void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override
    {
        auto nWheel = wheel;
        const auto speed = evt.mods.isCtrlDown() ? SensitiveScrollSpeed : ScrollSpeed;
        nWheel.deltaY *= speed;
        juce::Slider::mouseWheelMove(evt, nWheel);
    }

protected:
    juce::SliderParameterAttachment attach;
};

struct Button :
    juce::ToggleButton
{
	Button(ALLHaasAudioProcessor& p, param::PID pID) :
		juce::ToggleButton(param::toString(pID)),
        attach(*p.apvts.getParameter(param::toID(pID)), *this, nullptr)
	{
        setWantsKeyboardFocus(false);
    }

    protected:
        juce::ButtonParameterAttachment attach;
};

struct Label :
    public juce::Label
{
    Label(juce::String&& _name) :
        juce::Label(_name)
    {
		setJustificationType(juce::Justification::centred);
		setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
		setColour(juce::Label::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
        setFont(24);
        setWantsKeyboardFocus(false);
	}
};

struct LAF :
    juce::LookAndFeel_V4
{
    static constexpr float Pi = 3.1415926535897932384626433832795f;
    static constexpr float Tau = 2.f * Pi;
    static constexpr float HalfPi = .5f * Pi;
    static constexpr float QuartPi = .25f * Pi;
    static constexpr float Thicc = 2.f;

    LAF()
    {
    }

    void drawRotarySlider(juce::Graphics& g,
        int x, int y, int w, int h, float val,
        float, float, juce::Slider& slider) override
    {
        g.setColour(juce::Colours::limegreen);
        const auto bounds = juce::Rectangle<int>(x, y, w, h).toFloat();
        const auto nameArea = bounds.withHeight(bounds.getHeight() * .2f);
        g.drawFittedText(slider.getName(), nameArea.toNearestInt(), juce::Justification::centred, 1);
        const auto knobArea = gui::maxQuadIn(bounds.withY(nameArea.getBottom()).withHeight(bounds.getHeight() - nameArea.getHeight()));

        const auto angleStart = -Pi + QuartPi;
        const auto angleRange = Tau - HalfPi;
        const auto angleEnd = angleStart + angleRange * val;
        const auto innerCircleProportion = .2f;

        const auto rad = knobArea.getWidth() * .5f;
        const juce::Point<float> centre
        (
            knobArea.getX() + rad,
            knobArea.getY() + rad
        );

        const auto innerArea = knobArea.reduced(Thicc * 2.f);

        path.clear();
        path.addCentredArc(centre.x, centre.y, rad, rad, angleStart, 0.f, angleRange, true);
        path.addPieSegment(innerArea.getX(), innerArea.getY(), innerArea.getWidth(), innerArea.getHeight(), angleStart, angleEnd, innerCircleProportion);
        juce::PathStrokeType stroke(Thicc, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::butt);
        g.strokePath(path, stroke);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn,
        bool highlighted, bool down)
    {
        g.setColour(juce::Colours::limegreen);
        const auto bounds = btn.getLocalBounds().toFloat().reduced(Thicc);
        const auto name = btn.getName();
        const auto val = btn.getToggleState();
        if (name == "sc")
        {
            const auto txt = val ? "M/S" : "L/R";
            g.drawFittedText(txt, bounds.toNearestInt(), juce::Justification::centred, 1);
        }
        else if (name == "mn")
        {
            g.drawFittedText("mono", bounds.toNearestInt(), juce::Justification::centred, 1);
            if (val)
            {
                g.setColour(juce::Colours::limegreen.withAlpha(.5f));
                g.fillRoundedRectangle(bounds, Thicc);
            }
        }

        if (down)
        {
            g.setColour(juce::Colours::limegreen.withAlpha(.5f));
            g.fillRoundedRectangle(bounds, Thicc);
        } 
        if (highlighted)
        {
            g.setColour(juce::Colours::limegreen);
            g.drawRoundedRectangle(bounds, Thicc, Thicc);
        }
	}

    juce::Path path;
};

struct ALLHaasAudioProcessorEditor :
    public juce::AudioProcessorEditor,
    public juce::Timer
{
    enum { kDistance, kCutoff, kFeedback, kNumFilterParametersPerChannel };

    ALLHaasAudioProcessorEditor(ALLHaasAudioProcessor&);
    ~ALLHaasAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    ALLHaasAudioProcessor& audioProcessor;

    gui::XYOscilloscope oscilloscope;
    std::array<Slider, kNumFilterParametersPerChannel> slidersLM;
    std::array<Slider, kNumFilterParametersPerChannel> slidersRS;
    Button stereoConfigButton, monoButton;
    bool isMidSide;

    LAF laf;
};