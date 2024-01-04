#pragma once
#include <juce_core/juce_core.h>

namespace math
{
	static constexpr double Pi = 3.141592653589793238462643383279502884197169399375105820974944592307816406286;
	static constexpr double Tau = 2. * Pi;

	template<typename T>
	inline T noteToFreqHz(T note, T refPitch = static_cast<T>(69), T xen = static_cast<T>(12), T masterTune = static_cast<T>(440)) noexcept
	{
		return masterTune * std::pow(static_cast<T>(2), (note - refPitch) / xen);
	}

	using String = juce::String;

	inline String pitchclassToString(int pitchclass) noexcept
	{
		switch (pitchclass)
		{
		case 0: return "C";
		case 1: return "C#";
		case 2: return "D";
		case 3: return "D#";
		case 4: return "E";
		case 5: return "F";
		case 6: return "F#";
		case 7: return "G";
		case 8: return "G#";
		case 9: return "A";
		case 10: return "A#";
		case 11: return "B";
		default: return "C";
		};
	}
}