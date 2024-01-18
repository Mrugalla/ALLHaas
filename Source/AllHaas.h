#pragma once
#include "Allpass.h"
#include "XFade.h"

namespace dsp
{
	struct AllHaas
	{
		AllHaas();

		void prepare(double) noexcept;

		/* samples, cutoffLeft, cutoffRight, fbLeftHz,
		fbRightHz, numFiltersL, numFiltersR, numSamples */
		void operator()(float* const*,
			double, double,
			double, double,
			int, int, int) noexcept;

	protected:
		AllpassStereoSlope allpassFilters;
		double sampleRate;

		double cutoffLeft, cutoffRight, feedbackLeftHz, feedbackRightHz;
		int numFiltersL, numFiltersR;

		/* cutoffLeft, cutoffRight, fbLeftHz, fbRightHz, numFiltersL, numFiltersR */
		void updateParameters(double, double, double, double, int, int);

		/* samples, numSamples */
		void processFilters(float* const*, int) noexcept;
	};

	struct AllHaasXFade
	{
		static constexpr int NumTracks = 2;
		static constexpr float FadeLenMs = 40.f;

		AllHaasXFade();

		/* sampleRate, blockSize */
		void prepare(double, int);

		/* samples, cutoffLeft, cutoffRight, fbLeftHz,
		fbRightHz, numFiltersL, numFiltersR, numSamples */
		void operator()(float* const*,
			double, double,
			double, double,
			int, int, int) noexcept;

	protected:
		XFadeMixer<NumTracks, true> mixer;
		std::array<AllpassStereoSlope, NumTracks> filters;
		double sampleRate;

		double cutoffLeft, cutoffRight, feedbackLeftHz, feedbackRightHz;
		int numFiltersL, numFiltersR;

		void updateParameters(double, double,
			double, double,
			int, int) noexcept;

		void processFilters(float* const*, int) noexcept;
	};
}

/*

*/