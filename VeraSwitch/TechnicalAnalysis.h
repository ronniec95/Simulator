#pragma once
#include <vector>

namespace AARC {
    struct TSData;
    namespace TA {
        /* Takes any resolution input data and resamples it to find olhc bars for the super-sample period
         */
        auto resample(const TSData &in, const int mins = 5 /* Resample to this time unit, in minutes */)
            -> const TSData;

        /*
        Each TA has it's own set of parameters which we need to apply.
        Only apply for visible area to save on cpu cycles
        Note that TAs are variable independent so can be threaded
        */

        /* Calculates the 'period' returns as a probabilty distribution curve.
        The reason for 2 arrays as input is that if we are using an oscilator indicator then we want to execute on a
        close or low and sell on a high, or vice versa. It's slower though as 2 arrays have to be interleaved and that
        dirties the cache lines*/
        auto period_returns(const TSData &in, const size_t look_ahead_period,
                            const size_t start = std::numeric_limits<size_t>::min(),
                            const size_t fin   = std::numeric_limits<size_t>::max()) -> std::vector<float>;
        // auto rsi(const TSData &in, const size_t start, const size_t fin) -> vector<double>;
        // auto macd(const TSData &in, const size_t start, const size_t fin) -> vector<double>;
        // auto stoch(const TSData &in, const size_t start, const size_t fin) -> vector<double>;
        // auto sma(const TSData &in, const size_t start, const size_t fin) -> vector<double>;
    } // namespace TA
} // namespace AARC