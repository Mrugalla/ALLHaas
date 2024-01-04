#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Axioms.h"

namespace param
{
	using Range = juce::NormalisableRange<float>;
	using String = juce::String;
	using ValToStr = std::function<String(float, int)>;
	using StrToVal = std::function<float(const String&)>;

	enum class PID
	{
		DistanceLM,
		CutoffLM,
		FeedbackLM,
		DistanceRS,
		CutoffRS,
		FeedbackRS,
		StereoConfig,
		Mono,
		NumParams
	};
	static constexpr int NumParams = static_cast<int>(PID::NumParams);

	String toString(PID);

	String toID(PID);

	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}