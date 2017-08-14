#pragma once
#include <utility>
#include <vector>

namespace AARC {
    struct TSData;
    namespace TA {
        /* Takes any resolution input data and resamples it to find olhc bars for the super-sample period
         */
        auto resample(const TSData &in, const int mins = 5 /* Resample to this time unit, in minutes */)
            -> AARC::TSData;

        /* This takes the input data and averages out two consecutive data points that are >tolerance away from each
         * other */
        auto smooth_outliers(const TSData &in, const float tolerance) -> const TSData;

        /*
        Each TA has it's own set of parameters which we need to apply.
        Only apply for visible area to save on cpu cycles
        Note that TAs are variable independent so can be threaded
        */

        /* Calculates the 'period' returns as a probabilty distribution curve.
        The reason for 2 arrays as input is that if we are using an oscilator indicator then we want to execute on a
        close or low and sell on a high, or vice versa. It's slower though as 2 arrays have to be interleaved and that
        dirties the cache lines*/
        enum class PeriodReturnType { CLOSELOW, CLOSEHIGH, CLOSECLOSE };
        auto period_returns(const TSData &in, const size_t look_ahead_period, const PeriodReturnType pr_type,
                            const size_t start = std::numeric_limits<size_t>::min(),
                            const size_t fin   = std::numeric_limits<size_t>::max()) -> std::vector<float>;

        /* Talking a float array and calculates the histogram buckets , and returns as a tuple of bucket,count */
        auto histogram(const std::vector<float> &in, const float bucket_size) -> std::vector<size_t>;

        /* RSI with parameters */
        auto rsi(const std::vector<float> &in, const size_t period) -> std::vector<float>;

        // auto stoch(const TSData &in, const size_t start, const size_t fin) -> vector<double>;
        auto ema(const std::vector<float> &in, const size_t period) -> std::vector<float>;

        auto sma(const std::vector<float> &in, const size_t period) -> std::vector<float>;

        auto wma(const std::vector<float> &in, const size_t period) -> std::vector<float>;

        auto macd(const std::vector<float> &in, const size_t upper_period, const size_t lower_period,
                  const size_t crossover_period) -> std::vector<float>;

        /* Scale between -1 and 1 */
        auto scale(const std::vector<float> &in, const float a = -1.0f, const float b = 1.0f) noexcept
            -> std::vector<float>;

    } // namespace TA
} // namespace AARC