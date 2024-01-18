#include "Allpass.h"
#include <cmath>
#include "Math.h"

namespace dsp
{
	///

	AllpassFirstOrder::AllpassFirstOrder() :
			y1(0.),
			g(.5)
		{}

	void AllpassFirstOrder::reset() noexcept
	{
		y1 = 0.;
	}

	void AllpassFirstOrder::copyFrom(const AllpassFirstOrder& other) noexcept
	{
		g = other.g;
	}

	void AllpassFirstOrder::updateParameters(double freqHz, double sampleRate) noexcept
	{
		const auto fc = std::tan(math::Pi * freqHz / sampleRate);
		g = (fc - 1.) / (fc + 1.);
	}

	double AllpassFirstOrder::operator()(double x) noexcept
	{
		auto y = y1 + g * x;
		y1 = x - g * y;
		return y;
	}

	///

	AllpassTransposedDirectFormII::AllpassTransposedDirectFormII() :
		a0(0.), a1(0.), a2(0.), b1(0.), b2(0.),
		z1(0.), z2(0.)
	{}

	void AllpassTransposedDirectFormII::reset() noexcept
	{
		z1 = 0.;
		z2 = 0.;
	}

	void AllpassTransposedDirectFormII::copyFrom(const AllpassTransposedDirectFormII& other) noexcept
	{
		a0 = other.a0;
		a1 = other.a1;
		a2 = other.a2;
		b1 = other.b1;
		b2 = other.b2;
	}

	void AllpassTransposedDirectFormII::updateParameters(double freq, double q, double fs) noexcept
	{
		const auto k = std::tan(math::Pi * freq / fs);
		const auto kk = k * k;
		const auto kq = k / q;
		const auto norm = 1. / (1. + kq + kk);
		a0 = (1. - kq + kk) * norm;
		a1 = 2. * (kk - 1.) * norm;
		a2 = 1.;
		b1 = a1;
		b2 = a0;
	}

	double AllpassTransposedDirectFormII::operator()(double x) noexcept
	{
		auto y = a0 * x + z1;
		z1 = a1 * x - b1 * y + z2;
		z2 = a2 * x - b2 * y;
		return y;
	}

	///

	Allpass2ndOrderDirectFormI::Allpass2ndOrderDirectFormI() :
		a0(0.), a1(0.), a2(0.), b1(0.), b2(0.),
		x1(0.), x2(0.), y1(0.), y2(0.)
	{
	}

	void Allpass2ndOrderDirectFormI::reset() noexcept
	{
		x1 = 0.;
		x2 = 0.;
		y1 = 0.;
		y2 = 0.;
	}

	void Allpass2ndOrderDirectFormI::copyFrom(const Allpass2ndOrderDirectFormI& other) noexcept
	{
		a0 = other.a0;
		a1 = other.a1;
		a2 = other.a2;
		b1 = other.b1;
		b2 = other.b2;
	}

	void Allpass2ndOrderDirectFormI::updateParameters(double freqHz, double fs) noexcept
	{
		const auto oc = math::Pi * freqHz / fs;
		const auto bw = std::tan(oc);
		const auto a = (bw - 1.) / (bw + 1.);
		const auto b = -std::cos(oc);

		a0 = -a;
		a1 = b * (1. - a);
		a2 = 1.;
		b1 = a1;
		b2 = a0;
	}

	double Allpass2ndOrderDirectFormI::operator()(double x0) noexcept
	{
		auto yn =
			a0 * x0
			+ a1 * x1
			+ a2 * x2
			- b1 * y1
			- b2 * y2;
		x2 = x1;
		x1 = x0;
		y2 = y1;
		y1 = yn;
		return yn;
	}

	///

	Allpass2ndOrderDirectFormIBW::Allpass2ndOrderDirectFormIBW() :
		a0(0.), a1(0.), a2(0.), b1(0.), b2(0.),
		x1(0.), x2(0.), y1(0.), y2(0.)
	{
	}

	void Allpass2ndOrderDirectFormIBW::reset() noexcept
	{
		x1 = 0.;
		x2 = 0.;
		y1 = 0.;
		y2 = 0.;
	}

	void Allpass2ndOrderDirectFormIBW::copyFrom(const Allpass2ndOrderDirectFormIBW& other) noexcept
	{
		a0 = other.a0;
		a1 = other.a1;
		a2 = other.a2;
		b1 = other.b1;
		b2 = other.b2;
	}

	void Allpass2ndOrderDirectFormIBW::updateParameters(double freq, double q, double fs) noexcept
	{
		const auto K = std::tan(math::Pi * freq / fs);
		const auto kk = K * K;
		const auto norm = 1. / (1. + K / q + kk);
		a0 = (1. - K / q + kk) * norm;
		a1 = 2. * (kk - 1.) * norm;
		a2 = 1.;
		b1 = a1;
		b2 = a0;
	}

	double Allpass2ndOrderDirectFormIBW::operator()(double x0) noexcept
	{
		const auto yn =
			a0 * x0
			+ a1 * x1
			+ a2 * x2
			- b1 * y1
			- b2 * y2;
		x2 = x1;
		x1 = x0;
		y2 = y1;
		y1 = yn;
		return yn;
	}

	///

	AllpassStereo::AllpassStereo() :
		filters()
	{}

	void AllpassStereo::reset() noexcept
	{
		filters[0].reset();
		filters[1].reset();
	}

	void AllpassStereo::copyFrom(const AllpassStereo& other) noexcept
	{
		filters[0].copyFrom(other.filters[0]);
		filters[1].copyFrom(other.filters[1]);
	}

	void AllpassStereo::updateParameters(double freq, double q, double fs) noexcept
	{
		filters[0].updateParameters(freq, q, fs);
		filters[1].copyFrom(filters[0]);
	}

	double AllpassStereo::operator()(double x, int ch) noexcept
	{
		return filters[ch](x);
	}

	///

	AllpassSlope::AllpassSlope() :
		allpasses(),
		numFilters(axiom::NumAllpassFilters)
	{}

	void AllpassSlope::reset() noexcept
	{
		for (auto i = 0; i < numFilters; ++i)
			allpasses[i].reset();
	}

	void AllpassSlope::updateParameters(double freq, double q, double fs, int _numFilters) noexcept
	{
		numFilters = _numFilters;
		allpasses[0].updateParameters(freq, q, fs);
		for (auto i = 1; i < numFilters; ++i)
			allpasses[i].copyFrom(allpasses[0]);
	}

	void AllpassSlope::copyFrom(const AllpassSlope& other, int _numFilters) noexcept
	{
		numFilters = _numFilters;
		for (auto i = 0; i < numFilters; ++i)
			allpasses[i].copyFrom(other.allpasses[0]);
	}

	double AllpassSlope::operator()(double x) noexcept
	{
		auto y = x;
		for (auto i = 0; i < numFilters; ++i)
		{
			auto& allpass = allpasses[i];
			y = allpass(y);
		}
		return y;
	}

	///

	AllpassSlopeStereo::AllpassSlopeStereo() :
		allpasses(),
		numFilters(axiom::NumAllpassFilters)
	{}

	void AllpassSlopeStereo::reset() noexcept
	{
		for (auto i = 0; i < numFilters; ++i)
			allpasses[i].reset();
	}

	void AllpassSlopeStereo::updateParameters(double freq, double q, double fs, int _numFilters) noexcept
	{
		numFilters = _numFilters;
		allpasses[0].updateParameters(freq, q, fs);
		for (auto i = 1; i < numFilters; ++i)
			allpasses[i].copyFrom(allpasses[0]);
	}

	double AllpassSlopeStereo::operator()(double x, int ch) noexcept
	{
		auto y = x;
		for (auto i = 0; i < numFilters; ++i)
		{
			auto& allpass = allpasses[i];
			y = allpass(y, ch);
		}
		return y;
	}

	///

	AllpassStereoSlope::AllpassStereoSlope() :
		allpasses()
	{
	}

	void AllpassStereoSlope::reset() noexcept
	{
		allpasses[0].reset();
		allpasses[1].reset();
	}

	/* freqHzL freqHzR, qHzL, qHzR, sampleRate, numFiltersLeft, numFiltersRight */
	void AllpassStereoSlope::updateParameters(double freqHzL, double freqHzR,
		double qHzL, double qHzR, double sampleRate,
		int numFiltersL, int numFiltersR) noexcept
	{
		allpasses[0].updateParameters(freqHzL, qHzL, sampleRate, numFiltersL);
		allpasses[1].updateParameters(freqHzR, qHzR, sampleRate, numFiltersR);
	}

	/* smpl, ch */
	double AllpassStereoSlope::operator()(double smpl, int ch) noexcept
	{
		auto& allpass = allpasses[ch];
		return allpass(smpl);
	}
}