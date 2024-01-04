#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{
	using BoundsF = juce::Rectangle<float>;
	using Bounds = juce::Rectangle<int>;
	using LineF = juce::Line<float>;
	using PointF = juce::Point<float>;
	using Colour = juce::Colour;
	using Graphics = juce::Graphics;
	using String = juce::String;
	using Font = juce::Font;

	BoundsF smallestBoundsIn(const LineF&) noexcept;

	BoundsF maxQuadIn(const BoundsF&) noexcept;

	BoundsF maxQuadIn(const Bounds&) noexcept;
}