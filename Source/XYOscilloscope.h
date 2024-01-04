#pragma once
#include <atomic>

namespace dsp
{
	struct XYOscilloscope
	{
		XYOscilloscope() :
			x(0), y(0)
		{}

		void operator()(float* const* samples, int numSamples) noexcept
		{
			const auto maxIdx = findMax(samples, numSamples);
			x.store(samples[0][maxIdx]);
			y.store(samples[1][maxIdx]);
		}

		std::atomic<float> x, y;
	private:
		int findMax(float* const* samples, int numSamples) const noexcept
		{
			const auto i0 = findMax(samples[0], numSamples);
			const auto i1 = findMax(samples[1], numSamples);
			const auto smpl0 = samples[0][i0] * samples[0][i0];
			const auto smpl1 = samples[1][i1] * samples[1][i1];
			return smpl0 > smpl1 ? i0 : i1;
		}

		int findMax(float* smpls, int numSamples) const noexcept
		{
			auto max = smpls[0] * smpls[0];
			auto idx = 0;
			for (int i = 1; i < numSamples; ++i)
			{
				const auto smpl = smpls[i] * smpls[i];
				if (max < smpl)
				{
					max = smpl;
					idx = i;
				}
			}
				
			return idx;
		}
	};
}

#include "Layout.h"

namespace gui
{
	struct XYOscilloscope :
		public juce::Component,
		public juce::Timer
	{
		static constexpr int LineOrder = 5;
		static constexpr int NumLines = 1 << LineOrder;
		static constexpr int MaxLine = NumLines - 1;
		static constexpr float Pi = 3.1415926535897932384626433832795f;
		static constexpr float PiOver4 = Pi / 4.f;
		static constexpr unsigned int ColourBG = 0xff051000;

		enum class RemapMode
		{
			Classic,
			LensX,
			LensY,
			LensXY,
			NuModes
		};

		XYOscilloscope(const dsp::XYOscilloscope& o) :
			oscilloscope(o),
			centreX(0.f), centreY(0.f), posX(.5f), posY(.5f),
			lines(),
			lineColours(),
			lineIdx(0),
			remapMode(RemapMode::LensY),
			silence(true)
		{
			Colour bgCol(ColourBG);
			for (auto i = 0; i < NumLines; ++i)
			{
				auto a = static_cast<float>(i) / static_cast<float>(NumLines);
				lineColours[i] = juce::Colours::limegreen.interpolatedWith(bgCol, a * a);
			}
			
			setOpaque(true);
			startTimerHz(60);
		}

		void timerCallback() override
		{
			const auto x = juce::jlimit(-1.f, 1.f, oscilloscope.x.load());
			const auto y = juce::jlimit(-1.f, 1.f, -oscilloscope.y.load());

			if (posX != x || posY != y)
				updateLines(x, y);
			else if (x + y == 0.f)
				updateSilence();
			else
				return;

			repaint();
		}

		// https://www.desmos.com/calculator/j1riykitwk
		// https://www.desmos.com/calculator/nn6kspoc3r
		void updateLines(float x, float y)
		{
			silence = false;
			posX = x;
			posY = y;

			const auto lastLineIdx = lineIdx;
			lineIdx = (lineIdx + 1) & MaxLine;

			PointF nPos(x, y);
			switch (remapMode)
			{
			case RemapMode::LensX:
				nPos.setX(x * std::sqrt(1.f - y * y));
				break;
			case RemapMode::LensY:
				nPos.setY(y * std::sqrt(1.f - x * x));
				break;
			case RemapMode::LensXY:
				nPos = PointF
				(
					x * std::sqrt(1.f - y * y),
					y * std::sqrt(1.f - x * x)
				);
				break;
			}

			lines[lineIdx] = LineF
			(
				static_cast<float>(lines[lastLineIdx].getEndX()),
				static_cast<float>(lines[lastLineIdx].getEndY()),
				nPos.x, nPos.y
			);
		}

		void updateSilence()
		{
			silence = true;
		}

		void paint(Graphics& g) override
		{
			g.fillAll(juce::Colours::black);
			g.setColour(Colour(ColourBG));
			g.fillEllipse(getLocalBounds().toFloat());
			if (silence)
			{
				g.setColour(lineColours[0]);
				g.fillEllipse(centreX, centreY, 1.f, 1.f);
			}
			else
				for (auto i = 0; i < NumLines; ++i)
				{
					auto idx = lineIdx - i;
					if (idx < 0)
						idx += NumLines;
					g.setColour(lineColours[i]);
					g.drawLine
					(
						lines[idx].getStartX() * centreX + centreX,
						lines[idx].getStartY() * centreY + centreY,
						lines[idx].getEndX() * centreX + centreX,
						lines[idx].getEndY() * centreY + centreY,
						1.f
					);
				}
		}

		void resized() override
		{
			const auto w = static_cast<float>(getWidth());
			const auto h = static_cast<float>(getHeight());
			centreX = w * .5f;
			centreY = h * .5f;
			for (auto& line : lines)
				line = juce::Line<float>(0.f, 0.f, 0.f, 0.f);
		}

		const dsp::XYOscilloscope& oscilloscope;
		float centreX, centreY;
		float posX, posY;
		std::array<LineF, NumLines> lines;
		std::array<Colour, NumLines> lineColours;
		int lineIdx;
		RemapMode remapMode;
		bool silence;
	};
}

/*

todo:

*/