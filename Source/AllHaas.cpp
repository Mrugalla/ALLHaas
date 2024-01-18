#include "AllHaas.h"
#include "Math.h"

namespace dsp
{
	AllHaas::AllHaas() :
		allpassFilters(),
		sampleRate(1.),
		cutoffLeft(-1.), cutoffRight(-1.),
		feedbackLeftHz(-1.), feedbackRightHz(-1.),
		numFiltersL(-1), numFiltersR(-1)
	{}

	void AllHaas::prepare(double _sampleRate) noexcept
	{
		sampleRate = _sampleRate;
		cutoffLeft = -1.;
	}

	void AllHaas::operator()(float* const* samples,
		double _cutoffLeft, double _cutoffRight,
		double _feedbackLeft, double _feedbackRight,
		int _numFiltersL, int _numFiltersR, int numSamples) noexcept
	{
		updateParameters(_cutoffLeft, _cutoffRight, _feedbackLeft, _feedbackRight, _numFiltersL, _numFiltersR);
		processFilters(samples, numSamples);
	}

	void AllHaas::updateParameters(double _cutoffLeft, double _cutoffRight,
		double _feedbackLeftHz, double _feedbackRightHz,
		int _numFiltersL, int _numFiltersR)
	{
		if (cutoffLeft == _cutoffLeft &&
			cutoffRight == _cutoffRight &&
			feedbackLeftHz == _feedbackLeftHz &&
			feedbackRightHz == _feedbackRightHz &&
			numFiltersL == _numFiltersL &&
			numFiltersR == _numFiltersR)
			return;

		cutoffLeft = _cutoffLeft;
		cutoffRight = _cutoffRight;
		feedbackLeftHz = _feedbackLeftHz;
		feedbackRightHz = _feedbackRightHz;
		numFiltersL = _numFiltersL;
		numFiltersR = _numFiltersR;

		const auto cutoffLeftHz = math::noteToFreqHz(cutoffLeft);
		const auto cutoffRightHz = math::noteToFreqHz(cutoffRight);

		allpassFilters.updateParameters(cutoffLeftHz, cutoffRightHz, feedbackLeftHz, feedbackRightHz, sampleRate, numFiltersL, numFiltersR);
	}

	void AllHaas::processFilters(float* const* samples, int numSamples) noexcept
	{
		for (auto ch = 0; ch < 2; ++ch)
		{
			auto smpls = samples[ch];

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto dry = smpls[s];
				const auto wet = allpassFilters(static_cast<double>(dry), ch);
				smpls[s] = static_cast<float>(wet);
			}
		}
	}

	/// ///////////////////////////////////

	AllHaasXFade::AllHaasXFade() :
		mixer(),
		filters(),
		sampleRate(1.),
		cutoffLeft(-1.), cutoffRight(-1.),
		feedbackLeftHz(-1.), feedbackRightHz(-1.),
		numFiltersL(-1), numFiltersR(-1)
	{}

	void AllHaasXFade::prepare(double _sampleRate, int blockSize)
	{
		sampleRate = _sampleRate;
		mixer.prepare(static_cast<float>(sampleRate), FadeLenMs, blockSize);
		cutoffLeft = -1.;
	}

	/* samples, cutoffLeft, cutoffRight, fbLeftHz,
	fbRightHz, numFiltersL, numFiltersR, numSamples */
	void AllHaasXFade::operator()(float* const* samples,
		double _cutoffLeft, double _cutoffRight,
		double _fbLeftHz, double _fbRightHz,
		int _numFiltersL, int _numFiltersR, int numSamples) noexcept
	{
		updateParameters(_cutoffLeft, _cutoffRight, _fbLeftHz, _fbRightHz, _numFiltersL, _numFiltersR);
		processFilters(samples, numSamples);
	}

	void AllHaasXFade::updateParameters(double _cutoffLeft, double _cutoffRight,
		double _feedbackLeftHz, double _feedbackRightHz,
		int _numFiltersL, int _numFiltersR) noexcept
	{
		if (mixer.stillFading())
			return;

		if (cutoffLeft == _cutoffLeft &&
			cutoffRight == _cutoffRight &&
			feedbackLeftHz == _feedbackLeftHz &&
			feedbackRightHz == _feedbackRightHz &&
			numFiltersL == _numFiltersL &&
			numFiltersR == _numFiltersR)
			return;

		cutoffLeft = _cutoffLeft;
		cutoffRight = _cutoffRight;
		feedbackLeftHz = _feedbackLeftHz;
		feedbackRightHz = _feedbackRightHz;
		numFiltersL = _numFiltersL;
		numFiltersR = _numFiltersR;

		const auto cutoffLeftHz = math::noteToFreqHz(cutoffLeft);
		const auto cutoffRightHz = math::noteToFreqHz(cutoffRight);

		mixer.init();
		filters[mixer.idx].updateParameters(cutoffLeftHz, cutoffRightHz, feedbackLeftHz, feedbackRightHz, sampleRate, numFiltersL, numFiltersR);
		filters[mixer.idx].reset();
	}

	void AllHaasXFade::processFilters(float* const* samples, int numSamples) noexcept
	{
		auto sumSamples = mixer.getSamples(0);
		{
			auto& track = mixer[0];
			if (track.isEnabled())
			{
				track.synthesizeGainValues(sumSamples[2], numSamples);
				auto& filter = filters[0];

				for (auto ch = 0; ch < 2; ++ch)
				{
					const auto smpls = samples[ch];
					auto xSmpls = sumSamples[ch];

					for (auto s = 0; s < numSamples; ++s)
					{
						const auto dry = smpls[s];
						const auto wet = filter(static_cast<double>(dry), ch);
						xSmpls[s] = static_cast<float>(wet);
					}
				}

				track.copy(sumSamples, sumSamples, 2, numSamples);
			}
			else
				for (auto ch = 0; ch < 2; ++ch)
					SIMD::clear(sumSamples[ch], numSamples);
		}

		for (auto i = 1; i < NumTracks; ++i)
		{
			auto& track = mixer[i];
			if (track.isEnabled())
			{
				auto xSamples = mixer.getSamples(i);
				track.synthesizeGainValues(xSamples[2], numSamples);
				auto& filter = filters[i];

				for (auto ch = 0; ch < 2; ++ch)
				{
					const auto smpls = samples[ch];
					auto xSmpls = xSamples[ch];

					for (auto s = 0; s < numSamples; ++s)
					{
						const auto dry = smpls[s];
						const auto wet = filter(static_cast<double>(dry), ch);
						xSmpls[s] = static_cast<float>(wet);
					}
				}

				track.add(sumSamples, xSamples, 2, numSamples);
			}
		}

		for (auto ch = 0; ch < 2; ++ch)
			SIMD::copy(samples[ch], sumSamples[ch], numSamples);
	}
}