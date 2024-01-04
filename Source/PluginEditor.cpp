#include "PluginProcessor.h"
#include "PluginEditor.h"

ALLHaasAudioProcessorEditor::ALLHaasAudioProcessorEditor(ALLHaasAudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    oscilloscope(p.oscilloscope),
    slidersLM
    {
		Slider(p, param::PID::DistanceLM),
		Slider(p, param::PID::CutoffLM),
		Slider(p, param::PID::FeedbackLM)
	},
    slidersRS
    {
        Slider(p, param::PID::DistanceRS),
        Slider(p, param::PID::CutoffRS),
        Slider(p, param::PID::FeedbackRS)
	},
    stereoConfigButton(p, param::PID::StereoConfig),
    monoButton(p, param::PID::Mono),
    isMidSide(p.params[static_cast<int>(param::PID::StereoConfig)]->getValue() > .5f),
    laf()
{
    addAndMakeVisible(oscilloscope);
    addAndMakeVisible(stereoConfigButton);
    addAndMakeVisible(monoButton);
    for(auto& slider: slidersLM)
        addAndMakeVisible(slider);
    for(auto& slider: slidersRS)
        addAndMakeVisible(slider);

    if (isMidSide)
    {
        slidersLM[kDistance].setName("Distance M");
        slidersLM[kCutoff].setName("Cutoff M");
        slidersLM[kFeedback].setName("Feedback M");
        slidersRS[kDistance].setName("Distance S");
        slidersRS[kCutoff].setName("Cutoff S");
        slidersRS[kFeedback].setName("Feedback S");
    }
    else
    {
        slidersLM[kDistance].setName("Distance L");
        slidersLM[kCutoff].setName("Cutoff L");
        slidersLM[kFeedback].setName("Feedback L");
        slidersRS[kDistance].setName("Distance R");
        slidersRS[kCutoff].setName("Cutoff R");
        slidersRS[kFeedback].setName("Feedback R");
    }

    stereoConfigButton.setName("sc");
    monoButton.setName("mn");

    setLookAndFeel(&laf);

    const auto& user = *audioProcessor.props.getUserSettings();
    const auto width = user.getIntValue("editorWidth", 422);
    const auto height = user.getIntValue("editorHeight", 344);
    setOpaque(true);
    setResizable(true, true);
    setWantsKeyboardFocus(false);
    setSize(width, height);
    startTimerHz(8);
}

ALLHaasAudioProcessorEditor::~ALLHaasAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void ALLHaasAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void ALLHaasAudioProcessorEditor::timerCallback()
{
    const auto stereoConfig = audioProcessor.params[static_cast<int>(param::PID::StereoConfig)]->getValue() > .5f;
    if (isMidSide != stereoConfig)
    {
        isMidSide = stereoConfig;
        if (isMidSide)
        {
            slidersLM[kDistance].setName("Distance M");
            slidersLM[kCutoff].setName("Cutoff M");
            slidersLM[kFeedback].setName("Feedback M");
            slidersRS[kDistance].setName("Distance S");
            slidersRS[kCutoff].setName("Cutoff S");
            slidersRS[kFeedback].setName("Feedback S");
        }
        else
        {
            slidersLM[kDistance].setName("Distance L");
            slidersLM[kCutoff].setName("Cutoff L");
            slidersLM[kFeedback].setName("Feedback L");
            slidersRS[kDistance].setName("Distance R");
            slidersRS[kCutoff].setName("Cutoff R");
            slidersRS[kFeedback].setName("Feedback R");
        }
        repaint();
    }
}

void ALLHaasAudioProcessorEditor::resized()
{
    if (getWidth() < 100)
        setSize(100, getHeight());
    else if(getHeight() < 100)
		setSize(getWidth(), 100);

    const auto bounds = getLocalBounds().toFloat();
    auto thicc = std::min(bounds.getWidth(), bounds.getHeight()) * .1f;
    thicc = thicc < 1.f ? 1.f : thicc;

    oscilloscope.setBounds(gui::maxQuadIn(bounds).reduced(thicc).toNearestInt());
    auto rad = oscilloscope.getWidth() * .5f;
    const juce::Point<float> centre
    {
        static_cast<float>(oscilloscope.getX()) + rad,
        static_cast<float>(oscilloscope.getY()) + rad
    };

    auto buttonArea = bounds.withY(bounds.getHeight() * .8f).withWidth(bounds.getWidth() * .5f);
    buttonArea.setHeight(bounds.getHeight() - buttonArea.getY());

    stereoConfigButton.setBounds(gui::maxQuadIn(buttonArea).toNearestIntEdges());
    monoButton.setBounds(gui::maxQuadIn(buttonArea.withX(buttonArea.getRight())).toNearestIntEdges());

    const auto sliderWidth = thicc * 2.f;
    const auto sliderWHalf = sliderWidth * .5f;

    rad *= 1.2f;
    const auto angleInc = 3.14f / static_cast<float>(kNumFilterParametersPerChannel + 2);
    auto angle = angleInc;
    for(auto i = 0; i < kNumFilterParametersPerChannel; ++i)
    {
        auto pt = juce::Line<float>::fromStartAndAngle(centre, rad, -angle).getEnd();
        juce::Rectangle<float> bnds(pt.x - sliderWHalf, pt.y - sliderWHalf, sliderWidth, sliderWidth);
        slidersLM[i].setBounds(bnds.toNearestIntEdges());
        pt = juce::Line<float>::fromStartAndAngle(centre, rad, angle).getEnd();
        bnds.setX(pt.x - sliderWHalf);
        bnds.setY(pt.y - sliderWHalf);
        slidersRS[i].setBounds(bnds.toNearestIntEdges());
        angle += angleInc;
    }

    auto& user = *audioProcessor.props.getUserSettings();
    user.setValue("editorWidth", getWidth());
    user.setValue("editorHeight", getHeight());
}

/*

todo:

*/