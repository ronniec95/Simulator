#include "Drift.h"
#include "TechnicalAnalysis.h"
#include "TimeSeriesCSVFactory.h"
#include "Utilities.h"
#include <doctest\doctest.h>
#include <spdlog\spdlog.h>

/* When RSI > upper_threshold, find the return after N periods, bucket into 20 buckets */
auto AARC::Drift::pr_rsi_short(const TSData &in, const std::vector<float> &rsi,
                               const std::vector<float> &period_returns, const float upper_threshold)
    -> std::vector<size_t> {
    MethodLogger mlog("Drift::pr_rsi_short");

    using namespace std;
    const auto &max_idx       = rsi.size() - (in.ts_.size() - period_returns.size());
    const auto &rsi_pr_offset = period_returns.size() - rsi.size();

    // Find positions of RSI where they go above upper_threshold
    const auto &rsi_upper_positions = [&rsi, &max_idx, &rsi_pr_offset, &upper_threshold, &in]() {
        vector<size_t> res;
        for (auto &&i = size_t(0); i < max_idx;) {
            if (rsi[i] > upper_threshold) {
                res.emplace_back(i + rsi_pr_offset);
                while (rsi[i] > upper_threshold) ++i;
            }
            ++i;
        }
        return res;
    }();

    const auto &signal_returns = [&rsi_upper_positions, &period_returns, &mlog]() {
        vector<float> signal_returns;
        for (auto &&pos : rsi_upper_positions) { signal_returns.emplace_back(period_returns[pos]); }
        return signal_returns;
    }();

    // Minmax
    const auto &mm_it = minmax_element(begin(signal_returns), end(signal_returns));
    return AARC::TA::histogram(signal_returns, (*mm_it.second - *mm_it.first) / 15.0f);
}

void find_optimal(const AARC::TSData &in) {
    using namespace std;
    for (auto &&sample : {60, 120, 240}) {
        const auto &resample = AARC::TA::resample(in, sample);
        CHECK(!resample.ts_.empty());
        const auto &smooth = AARC::TA::smooth_outliers(resample, 0.03f);
        CHECK(!smooth.ts_.empty());
        for (auto &&rsi_lookback : {3, 4, 5, 10, 15}) {
            const auto &rsi            = AARC::TA::rsi(smooth.close_, rsi_lookback);
            const auto &period_returns = AARC::TA::period_returns(smooth, 3, AARC::TA::PeriodReturnType::CLOSELOW);
            const auto &probability    = AARC::Drift::pr_rsi_short(smooth, rsi, period_returns, 80.0);

            for_each(begin(probability), end(probability), [](auto &&val) { printf("%zd,", val); });
            printf("\n");
            if (!probability.empty()) {

                // split and sum cdf
                const auto &prob = [&probability]() -> std::pair<size_t, size_t> {
                    const auto &mid_point = probability.size() / 2;
                    auto &&     sum_up    = size_t(0);
                    auto &&     sum_down  = size_t(0);
                    for (auto i = size_t(0); i < mid_point - 1; i++) { sum_down += probability[i]; }
                    for (auto i = mid_point; i < probability.size(); i++) { sum_up += probability[i]; }
                    return std::make_pair(sum_up, sum_down);
                }();
                printf("Resample %d Period Returns %d RSI %d = [success:%zd ] [fail:%zd] [indeterminate:%zd]\n", sample,
                       3, rsi_lookback, prob.second, prob.first, probability[probability.size() / 2]);
            }
        }
    }
}

TEST_CASE("MLRsi") {

    static auto const filename =
        "H:\\Users\\Mushfaque.Cradle\\Downloads\\HISTDATA_COM_ASCII_EURUSD_M1201703\\data2.csv";
    const auto &input = AARC::TimeSeries_CSV::read_csv_file(filename);
    CHECK(!input.ts_.empty());
    const auto &resample = AARC::TA::resample(input, 60);
    CHECK(!resample.ts_.empty());
    const auto &smooth = AARC::TA::smooth_outliers(resample, 0.03f);
    CHECK(!smooth.ts_.empty());
    const auto &rsi            = AARC::TA::rsi(smooth.close_, 5);
    const auto &period_returns = AARC::TA::period_returns(smooth, 5, AARC::TA::PeriodReturnType::CLOSECLOSE);
    const auto &probability    = AARC::Drift::pr_rsi_short(smooth, rsi, period_returns, 90.0);
    find_optimal(input);
}