#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace dsp
{
    using SIMD = juce::FloatVectorOperations;

    template<typename Float>
    inline Float msInSamples(Float ms, Float Fs) noexcept
    {
        return ms * Fs * static_cast<Float>(0.001);
    }

    template<typename Float>
    inline Float msInInc(Float ms, Float Fs) noexcept
    {
        return static_cast<Float>(1) / msInSamples(ms, Fs);
    }

    struct XFade
    {
        using AudioBuffer = juce::AudioBuffer<float>;
        static constexpr float Pi = 3.141592653589f;

        XFade() :
            buffer(),
            phase(0.),
            inc(0.),
            idx(0),
            fading(false)
        {}

        void prepare(double sampleRate, double lengthMs, int blockSize)
        {
            buffer.setSize(3, blockSize, false, true, false);
            inc = msInInc(lengthMs, sampleRate);
        }

        void init() noexcept
        {
            idx = 1 - idx;
            phase = 0.;
            fading = true;
        }

        float* const* getSamples() noexcept
        {
            return buffer.getArrayOfWritePointers();
        }

        void synthesizePhase(int numSamples) noexcept
        {
            auto xSamples = getSamples();
            auto xBuf = xSamples[2];

            for (auto s = 0; s < numSamples; ++s)
            {
                xBuf[s] = static_cast<float>(phase);
                phase += inc;
                if (phase > 1.)
                {
                    fading = false;
                    phase = 1.;
                    for (; s < numSamples; ++s)
                        xBuf[s] = 1.f;
                    return;
                }
            }
        }

        void operator()(float* const* samples, int numChannels, int numSamples) noexcept
        {
            synthesizePhase(numSamples);

            auto xSamples = getSamples();
            auto xBuf = xSamples[2];

            for (auto ch = 0; ch < numChannels; ++ch)
            {
                auto smpls = samples[ch];
                const auto xSmpls = xSamples[ch];

                for (auto s = 0; s < numSamples; ++s)
                {
                    const auto smpl = smpls[s];
                    const auto xSmpl = xSmpls[s];

                    const auto xFade = xBuf[s];
                    const auto xPi = xFade * Pi;
                    const auto frac = std::cos(xPi + Pi) + 1.f;
                    const auto xFrac = std::cos(xPi) + 1.f;

                    smpls[s] = (smpl * frac + xSmpl * xFrac) * .5f;
                }
            }
        }

        AudioBuffer buffer;
        double phase, inc;
        int idx;
        bool fading;
    };

    template<size_t NumTracks, bool Smooth>
    struct XFadeMixer
    {
        using AudioBuffer = juce::AudioBuffer<float>;
        static constexpr float Pi = 3.141592653589f;

        struct Track
        {
            Track() :
                gain(0.f),
                destGain(0.f),
                inc(0.f),
                fading(false)
            {}

            void disable() noexcept
            {
                destGain = 0.f;
            }

            void enable() noexcept
            {
                destGain = 1.f;
            }

            const bool isEnabled() const noexcept
            {
                return gain + destGain != 0.f;
            }

            const bool isFading() const noexcept
            {
                return destGain != gain;
            }

            void synthesizeGainValues(float* xBuf, int numSamples) noexcept
            {
                if (!isFading())
                {
                    SIMD::fill(xBuf, gain, numSamples);
                    fading = false;
                    return;
                }

                fading = true;
                synthesizeGainValuesInternal(xBuf, numSamples);

                if (Smooth)
                    makeSmooth(xBuf, numSamples);
            }

            void copy(float* const* dest, const float* const* src,
                int numChannels, int numSamples) const noexcept
            {
                if (fading)
                {
                    const auto xBuf = src[2];
                    for (auto ch = 0; ch < numChannels; ++ch)
                        SIMD::multiply(dest[ch], src[ch], xBuf, numSamples);
                }
                else if (gain == 1.)
                    for (auto ch = 0; ch < numChannels; ++ch)
                        SIMD::copy(dest[ch], src[ch], numSamples);
            }

            void add(float* const* dest, const float* const* src,
                int numChannels, int numSamples) const noexcept
            {
                if (fading)
                {
                    const auto xBuf = src[2];
                    for (auto ch = 0; ch < numChannels; ++ch)
                        SIMD::addWithMultiply(dest[ch], src[ch], xBuf, numSamples);
                }
                else if (gain == 1.)
                    for (auto ch = 0; ch < numChannels; ++ch)
                        SIMD::add(dest[ch], src[ch], numSamples);
            }

            float gain, destGain, inc;
        protected:
            bool fading;

            void synthesizeGainValuesInternal(float* xBuf, int numSamples) noexcept
            {
                if (destGain == 1.f)
                    for (auto s = 0; s < numSamples; ++s)
                    {
                        xBuf[s] = gain;
                        gain += inc;
                        if (gain >= 1.f)
                        {
                            gain = 1.f;
                            for (; s < numSamples; ++s)
                                xBuf[s] = gain;
                            return;
                        }
                    }
                else
                    for (auto s = 0; s < numSamples; ++s)
                    {
                        xBuf[s] = gain;
                        gain -= inc;
                        if (gain < 0.f)
                        {
                            gain = 0.f;
                            for (; s < numSamples; ++s)
                                xBuf[s] = gain;
                            return;
                        }
                    }
            }

            void makeSmooth(float* xBuf, int numSamples) const noexcept
            {
                for (auto s = 0; s < numSamples; ++s)
                    xBuf[s] = std::cos(xBuf[s] * Pi + Pi) * .5f + .5f;
            }
        };

        XFadeMixer() :
            buffer(),
            tracks(),
            idx(0)
        {
        }

        void prepare(float sampleRate, float lengthMs, int blockSize)
        {
            buffer.setSize(3 * NumTracks, blockSize, false, true, false);
            const auto inc = msInInc(lengthMs, sampleRate);
            for (auto& track : tracks)
            {
                track.gain = 0.f;
                track.inc = inc;
            }
            tracks[idx].gain = 1.f;
        }

        void init() noexcept
        {
            idx = (idx + 1) % NumTracks;
            for (auto& track : tracks)
                track.disable();
            tracks[idx].enable();
        }

        float* const* getSamples(int i) noexcept
        {
            return &buffer.getArrayOfWritePointers()[i * 3];
        }

        const float* const* getSamples(int i) const noexcept
        {
            return &buffer.getArrayOfReadPointers()[i * 3];
        }

        const int numTracksEnabled() const noexcept
        {
            auto sum = 0;
            for (auto i = 0; i < NumTracks; ++i)
                sum += (tracks[i].isEnabled() ? 1 : 0);
            return sum;
        }

        Track& operator[](int i) noexcept
        {
            return tracks[i];
        }

        bool stillFading() const noexcept
        {
            return tracks[idx].gain != 1.f;
        }

    protected:
        AudioBuffer buffer;
        std::array<Track, NumTracks> tracks;
    public:
        int idx;
    };

}