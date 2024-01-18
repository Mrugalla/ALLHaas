#pragma once
#include <array>
#include "Axioms.h"

namespace dsp
{
	/*
	Canonical Direct Form II (TDF-II)
	no bandwidth
	*/
	struct AllpassFirstOrder
	{
		AllpassFirstOrder();

		void reset() noexcept;

		void copyFrom(const AllpassFirstOrder&) noexcept;

		/* freqHz, sampleRate */
		void updateParameters(double, double) noexcept;

		double operator()(double) noexcept;
	private:
		double y1, g;
	};

	/*
	Canonical Form II 2nd Order (TDF-II)
	bad modulation
	*/
	struct AllpassTransposedDirectFormII
	{
		AllpassTransposedDirectFormII();

		void reset() noexcept;

		void copyFrom(const AllpassTransposedDirectFormII&) noexcept;

		/* freqHz, qHz, sampleRate */
		void updateParameters(double, double, double) noexcept;

		double operator()(double) noexcept;

	private:
		double a0, a1, a2, b1, b2;
		double z1, z2;
	};

	/*
	Biquad 2nd Order Direct Form I
	no bandwidth
	bad performance
	bad modulation
	numeric instability
	fixed-point friendly
	*/
	struct Allpass2ndOrderDirectFormI
	{
		Allpass2ndOrderDirectFormI();

		void reset() noexcept;

		void copyFrom(const Allpass2ndOrderDirectFormI&) noexcept;

		/* freqHz, sampleRate */
		void updateParameters(double, double) noexcept;

		double operator()(double) noexcept;

		double a0, a1, a2, b1, b2;
	private:
		double x1, x2, y1, y2;
	};

	/*
	Biquad 2nd Order Direct Form I
	bad performance
	bad modulation
	numeric instability
	fixed-point friendly
	*/
	struct Allpass2ndOrderDirectFormIBW
	{
		Allpass2ndOrderDirectFormIBW();

		void reset() noexcept;

		void copyFrom(const Allpass2ndOrderDirectFormIBW&) noexcept;

		/* freqHz, qHz, sampleRate */
		void updateParameters(double, double, double) noexcept;

		double operator()(double) noexcept;

		double a0, a1, a2, b1, b2;
	private:
		double x1, x2, y1, y2;
	};

	/*
	2 channels of AllpassTransposedDirectFormII filters
	*/
	struct AllpassStereo
	{
		AllpassStereo();

		void reset() noexcept;

		void copyFrom(const AllpassStereo&) noexcept;

		/* freqHz, qHz, sampleRate */
		void updateParameters(double, double, double) noexcept;

		/* smpl, ch */
		double operator()(double, int) noexcept;

	private:
		std::array<AllpassTransposedDirectFormII, 2> filters;
	};

	/*
	axiom::NumAllpassFilters channels of AllpassTransposedDirectFormII filters
	*/
	struct AllpassSlope
	{
		AllpassSlope();

		void reset() noexcept;

		/* freqHz, qHz, sampleRate, numFilters */
		void updateParameters(double, double, double, int) noexcept;

		/* other, numFilters */
		void copyFrom(const AllpassSlope&, int) noexcept;

		/* smpl */
		double operator()(double) noexcept;

	private:
		std::array<AllpassTransposedDirectFormII, axiom::NumAllpassFilters> allpasses;
		int numFilters;
	};

	/*
	axiom::NumAllpassFilters channels of AllpassStereo filters
	*/
	struct AllpassSlopeStereo
	{
		AllpassSlopeStereo();

		void reset() noexcept;

		/* freqHz, qHz, sampleRate, numFilters */
		void updateParameters(double, double, double, int) noexcept;

		/* smpl, ch */
		double operator()(double, int) noexcept;

	private:
		std::array<AllpassStereo, axiom::NumAllpassFilters> allpasses;
		int numFilters;
	};

	/*
	2 channels of AllpassSlope filters
	*/
	struct AllpassStereoSlope
	{
		AllpassStereoSlope();

		void reset() noexcept;

		/* freqHzL, freqHzR, qHzL, qHzR, sampleRate, numFiltersLeft, numFiltersRight */
		void updateParameters(double, double, double, double, double, int, int) noexcept;

		/* smpl, ch */
		double operator()(double, int) noexcept;

	private:
		std::array<AllpassSlope, 2> allpasses;
	};
}