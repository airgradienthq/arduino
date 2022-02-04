#pragma once

#include <Arduino.h>

namespace AirGradient {
    template<typename T, typename Total, size_t N>
    class MovingAverage {
    public:
        MovingAverage & addSample(T sample) {
            _total += sample;
            if (_numSamples < N)
                _samples[_numSamples++] = sample;
            else {
                auto index = ++_numSamples % N;
                _numSamples = N + index;
                T &oldest = _samples[index];
                _total -= oldest;
                oldest = sample;
            }
            return *this;
        }

        float getAverage() const {
            auto denominator = static_cast<float>(std::min(_numSamples, N));
            if (_total == 0 || denominator == 0) {
                return 0;
            }
            return _total / denominator;
        }

        bool hasReachCapacity() const { return _numSamples >= N; }

    private:
        T _samples[N];
        size_t _numSamples{0};
        Total _total{0};
    };
}



