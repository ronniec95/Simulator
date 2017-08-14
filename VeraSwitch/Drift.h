#pragma once
#include "TimeSeries.h"
#include <vector>

namespace AARC {
    namespace Drift {
        auto pr_rsi_short(const TSData &in, const std::vector<float> &rsi, const std::vector<float> &period_returns,
                          const float upper_threshold) -> std::vector<size_t>;
        auto pr_rsi_long(const TSData &in, const std::vector<float> &rsi, const std::vector<float> &period_returns,
                         const float upper_threshold) -> std::vector<size_t>;
    } // namespace Drift
} // namespace AARC