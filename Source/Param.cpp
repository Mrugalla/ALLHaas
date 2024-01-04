#include "Param.h"
#include "Math.h"

namespace param
{
	using Range = juce::NormalisableRange<float>;

	namespace makeRange
	{
		Range biased(float start, float end, float bias) noexcept
		{
			// https://www.desmos.com/calculator/ps8q8gftcr
			const auto a = bias * .5f + .5f;
			const auto a2 = 2.f * a;
			const auto aM = 1.f - a;

			const auto r = end - start;
			const auto aR = r * a;
			if (bias != 0.f)
				return
			{
					start, end,
					[a2, aM, aR](float min, float, float x)
					{
						const auto denom = aM - x + a2 * x;
						if (denom == 0.f)
							return min;
						return min + aR * x / denom;
					},
					[a2, aM, aR](float min, float, float x)
					{
						const auto denom = a2 * min + aR - a2 * x - min + x;
						if (denom == 0.f)
							return 0.f;
						auto val = aM * (x - min) / denom;
						return val > 1.f ? 1.f : val;
					},
					[](float min, float max, float x)
					{
						return x < min ? min : x > max ? max : x;
					}
			};
			else return { start, end };
		}

		Range stepped(float start, float end, float steps = 1.f) noexcept
		{
			return { start, end, steps };
		}

		Range toggle() noexcept
		{
			return stepped(0.f, 1.f);
		}

		Range lin(float start, float end) noexcept
		{
			const auto range = end - start;

			return
			{
				start,
				end,
				[range](float min, float, float normalized)
				{
					return min + normalized * range;
				},
				[inv = 1.f / range](float min, float, float denormalized)
				{
					return (denormalized - min) * inv;
				},
				[](float min, float max, float x)
				{
					return juce::jlimit(min, max, x);
				}
			};
		}

		Range withCentre(float start, float end, float centre) noexcept
		{
			const auto r = end - start;
			const auto v = (centre - start) / r;

			return makeRange::biased(start, end, 2.f * v - 1.f);
		}

		Range foleysLogRange(float min, float max) noexcept
		{
			return
			{
				min, max,
				[](float start, float end, float normalised)
				{
					return start + (std::pow(2.f, normalised * 10.f) - 1.f) * (end - start) / 1023.f;
				},
				[](float start, float end, float value)
				{
					return (std::log(((value - start) * 1023.f / (end - start)) + 1.f) / std::log(2.f)) * .1f;
				},
				[](float start, float end, float value)
				{
					// optimised for frequencies: >3 kHz: 2 decimals
					if (value > 3000.f)
						return juce::jlimit(start, end, 100.f * juce::roundToInt(value / 100.f));

					// optimised for frequencies: 1-3 kHz: 1 decimal
					if (value > 1000.f)
						return juce::jlimit(start, end, 10.f * juce::roundToInt(value * .1f));

					return juce::jlimit(start, end, std::round(value));
				}
			};
		}

		Range quad(float min, float max, int numSteps) noexcept
		{
			return
			{
				min, max,
				[numSteps, range = max - min](float start, float, float x)
				{
					for (auto i = 0; i < numSteps; ++i)
						x *= x;
					return start + x * range;
				},
				[numSteps, rangeInv = 1.f / (max - min)](float start, float, float x)
				{
					x = (x - start) * rangeInv;
					for (auto i = 0; i < numSteps; ++i)
						x = std::sqrt(x);
					return x;
				},
				[](float start, float end, float x)
				{
					return juce::jlimit(start, end, x);
				}
			};
		}

		Range beats(float minDenominator, float maxDenominator, bool withZero)
		{
			std::vector<float> table;

			const auto minV = std::log2(minDenominator);
			const auto maxV = std::log2(maxDenominator);

			const auto numWholeBeatsF = static_cast<float>(minV - maxV);
			const auto numWholeBeatsInv = 1.f / numWholeBeatsF;

			const auto numWholeBeats = static_cast<int>(numWholeBeatsF);
			const auto numValues = numWholeBeats * 3 + 1 + (withZero ? 1 : 0);
			table.reserve(numValues);
			if (withZero)
				table.emplace_back(0.f);

			for (auto i = 0; i < numWholeBeats; ++i)
			{
				const auto iF = static_cast<float>(i);
				const auto x = iF * numWholeBeatsInv;

				const auto curV = minV - x * numWholeBeatsF;
				const auto baseVal = std::pow(2.f, curV);

				const auto valWhole = 1.f / baseVal;
				const auto valTriplet = valWhole * 1.666666666667f;
				const auto valDotted = valWhole * 1.75f;

				table.emplace_back(valWhole);
				table.emplace_back(valTriplet);
				table.emplace_back(valDotted);
			}
			table.emplace_back(1.f / maxDenominator);

			static constexpr float Eps = 1.f - std::numeric_limits<float>::epsilon();
			static constexpr float EpsInv = 1.f / Eps;

			const auto numValuesF = static_cast<float>(numValues);
			const auto numValuesInv = 1.f / numValuesF;
			const auto numValsX = numValuesInv * EpsInv;
			const auto normValsY = numValuesF * Eps;

			return
			{
				table.front(), table.back(),
				[table, normValsY](float, float, float normalized)
				{
					const auto valueIdx = normalized * normValsY;
					return table[static_cast<int>(valueIdx)];
				},
				[table, numValsX](float, float, float denormalized)
				{
					for (auto i = 0; i < table.size(); ++i)
						if (denormalized <= table[i])
							return static_cast<float>(i) * numValsX;
					return 0.f;
				},
				[](float start, float end, float denormalized)
				{
					return juce::jlimit(start, end, denormalized);
				}
			};
		}
	}

	using String = juce::String;
	using ValToStr = std::function<String(float, int)>;
	using StrToVal = std::function<float(const String&)>;

	namespace valToStr
	{
		ValToStr hz()
		{
			return [](float v, int)
				{
					if (v >= 10000.f)
						return String(v * .001).substring(0, 4) + " khz";
					else if (v >= 1000.f)
						return String(v * .001).substring(0, 3) + " khz";
					else
						return String(v).substring(0, 5) + " hz";
				};
		}

		ValToStr note()
		{
			return [](float v, int)
				{
					if (v >= 0.f)
					{
						//enum pitchclass { C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B, Num };

						const auto note = static_cast<int>(std::round(v));
						const auto octave = note / 12 - 1;
						const auto noteName = note % 12;
						return math::pitchclassToString(noteName) + String(octave);
					}
					return String("?");
				};
		}

		ValToStr pitch()
		{
			return [noteFunc = note(), hzFunc = hz()](float v, int)
				{
					return noteFunc(v, 0) + "; " + hzFunc(math::noteToFreqHz(v), 0);
				};
		}
	}

	String toString(PID pID)
	{
		switch (pID)
		{
		case PID::DistanceLM: return "Distance L/M";
		case PID::CutoffLM: return "Cutoff L/M";
		case PID::FeedbackLM: return "Feedback L/M";
		case PID::DistanceRS: return "Distance R/S";
		case PID::CutoffRS: return "Cutoff R/S";
		case PID::FeedbackRS: return "Feedback R/S";
		case PID::StereoConfig: return "Stereo Config";
		case PID::Mono: return "Mono";
		default: jassertfalse; return {};
		}
	}

	String toID(PID pID)
	{
		return toString(pID).removeCharacters(" ").toLowerCase().removeCharacters("/");
	}

	std::unique_ptr<juce::AudioParameterFloat> makeParam(PID pID,
		const Range& range = Range(0.f, 1.f), float defaultVal = .5f,
		ValToStr valToStrFunc = [](float val, int) { return String(val, 2); },
		StrToVal strToValFunc = [](const String& str) { return str.getFloatValue(); })
	{
		const auto genericP = juce::AudioProcessorParameter::genericParameter;
		return std::make_unique<juce::AudioParameterFloat>
			(
				toID(pID),
				toString(pID),
				range,
				defaultVal,
				"",
				genericP,
				valToStrFunc,
				strToValFunc
			);
	}

	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
	{
		juce::AudioProcessorValueTreeState::ParameterLayout layout;

		const auto rangeDistance = makeRange::stepped(1.f, static_cast<float>(axiom::NumAllpassFilters));
		const auto defaultDistance = 1.f;

		const auto rangePitch = makeRange::lin(12.f, 127.f);
		const auto valToStrPitch = valToStr::pitch();

		const auto rangeFB = makeRange::withCentre(.1f, 20.f, 3.f);
		const auto defaultFB = .1f;
		const auto valToStrHz = valToStr::hz();

		const auto rangeStereoConfig = makeRange::toggle();
		const auto valToStrStereoConfig = [](float val, int)
		{
				return val < .5f ? "L/R" : "M/S";
		};
		const auto strToValStereoConfig = [](const String& str)
		{
			return str.getFloatValue() < .5f ? 0.f : 1.f;
		};

		layout.add(makeParam(PID::DistanceLM, rangeDistance, defaultDistance));
		layout.add(makeParam(PID::CutoffLM, rangePitch, 48.f, valToStrPitch));
		layout.add(makeParam(PID::FeedbackLM, rangeFB, defaultFB, valToStrHz));
		layout.add(makeParam(PID::DistanceRS, rangeDistance, defaultDistance));
		layout.add(makeParam(PID::CutoffRS, rangePitch, 62.f, valToStrPitch));
		layout.add(makeParam(PID::FeedbackRS, rangeFB, defaultFB, valToStrHz));
		layout.add(makeParam(PID::StereoConfig, rangeStereoConfig, 0.f, valToStrStereoConfig, strToValStereoConfig));
		layout.add(makeParam(PID::Mono, makeRange::toggle(), 0.f));
		return layout;
	}
}