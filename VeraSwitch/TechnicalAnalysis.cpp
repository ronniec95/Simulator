#include "TechnicalAnalysis.h"
#include "Split.h"
#include "TimeSeries.h"
#include <deque>
#include <doctest\doctest.h>
#include <future>
#include <numeric>
#include <ppl.h>
#include <spdlog\spdlog.h>
#include <vector>

auto AARC::TA::resample(const AARC::TSData &in, const int mins) -> AARC::TSData {
    using namespace std;
    if (in.ts_.empty() || mins > in.ts_.size()) return TSData();
    const auto start = in.ts_.front(), fin = in.ts_.back();
    if ((fin - start) < mins) { return TSData(); }
    const auto sz        = (fin - start) / mins - 1;
    auto       positions = vector<pair<size_t, size_t>>();
    positions.reserve(static_cast<size_t>(sz));
    auto current = start;

    generate_n(back_inserter(positions), sz, [&in, &current, &mins]() -> decltype(positions)::value_type {
        const auto lb = lower_bound(begin(in.ts_), end(in.ts_), current);
        const auto ub = upper_bound(begin(in.ts_), end(in.ts_), current + mins);
        current += mins;
        if (lb == ub) return {0, 0};
        return {distance(begin(in.ts_), lb), distance(begin(in.ts_), ub)};
    });
    // Sync point here

    // Runs in parallel
    auto &ts = async(launch::async,
                     [&positions, &in /* won't go out of scope before end of function so can take by reference */]() {
                         return accumulate(begin(positions), end(positions), vector<decltype(in.ts_)::value_type>(),
                                           [&in](auto &&acc, auto &&val) {
                                               acc.emplace_back(in.ts_[get<0>(val)]);
                                               return acc;
                                           });
                     });

    auto &open = async(launch::async, [ pos = positions, open = in.open_ ]() {
        return accumulate(begin(pos), end(pos), vector<decltype(open)::value_type>(), [&open](auto &&acc, auto &&val) {
            acc.emplace_back(open[get<0>(val)]);
            return acc;
        });
    });
    auto &close = async(launch::async, [&in, &positions]() {
        return accumulate(begin(positions), end(positions), vector<decltype(in.close_)::value_type>(),
                          [&in](auto &&acc, auto &&val) {
                              acc.emplace_back(in.close_[get<1>(val)]);
                              return acc;
                          });
    });
    auto &high = async(launch::async, [&in, &positions]() {
        return accumulate(
            begin(positions), end(positions), vector<decltype(in.high_)::value_type>(), [&in](auto &&acc, auto &&val) {
                const auto &max_elem = *max_element(begin(in.high_) + get<0>(val), begin(in.high_) + get<1>(val));
                acc.emplace_back(max_elem);
                return acc;
            });
    });
    auto &low = async(launch::async, [&in, &positions]() {
        return accumulate(
            begin(positions), end(positions), vector<decltype(in.low_)::value_type>(), [&in](auto &&acc, auto &&val) {
                const auto &min_elem = *min_element(begin(in.high_) + get<0>(val), begin(in.high_) + get<1>(val));
                acc.emplace_back(min_elem);
                return acc;
            });
    });
    // sync point here
    return TSData(in.asset_, ts.get(), open.get(), high.get(), low.get(), close.get());
}

auto AARC::TA::smooth_outliers(const TSData &in, const float tolerance) -> const TSData {
    using namespace std;
    if (in.ts_.empty() || tolerance == 0.0f) return in;
    const auto &pos = rand() % (in.ts_.size() - 10) + 10;
    const auto &avg = [ sum = 0.0f, &in, &pos ]() mutable {
        for (auto i = pos; i < pos + 10; i++) { sum += (in.close_[i] - in.close_[i - 1]); }
        return sum / 10.0f;
    }
    ();

    auto &&open = async(launch::async, [&in, &tolerance, &avg]() {
        const auto &sz   = in.open_.size();
        const auto  vout = make_unique<float[]>(sz);
        ispc::smooth_outliers(in.open_.data(), vout.get(), sz, tolerance, avg);
        return vector<float>(vout.get(), &vout.get()[sz]);
    });

    auto &&high = async(launch::async, [&in, &tolerance, &avg]() {
        const auto &&vout = make_unique<float[]>(in.high_.size());
        ispc::smooth_outliers(in.high_.data(), vout.get(), in.high_.size(), tolerance, avg);
        return vector<float>(vout.get(), &vout.get()[in.high_.size()]);
    });

    auto &&low = async(launch::async, [&in, &tolerance, &avg]() {
        const auto vout = make_unique<float[]>(in.high_.size());
        ispc::smooth_outliers(in.low_.data(), vout.get(), in.low_.size(), tolerance, avg);
        return vector<float>(vout.get(), &vout.get()[in.low_.size()]);
    });

    auto &&close = async(launch::async, [&in, &tolerance, &avg]() {
        const auto vout = make_unique<float[]>(in.close_.size());
        ispc::smooth_outliers(in.close_.data(), vout.get(), in.close_.size(), tolerance, avg);
        return vector<float>(vout.get(), &vout.get()[in.close_.size()]);
    });
    return TSData(in.asset_, in.ts_, open.get(), high.get(), low.get(), close.get());
}

auto AARC::TA::period_returns(const TSData &in, const size_t look_ahead_period, const PeriodReturnType pr_type,
                              const size_t start, const size_t fin) -> std::vector<float> {
    using namespace std;
    if (in.ts_.size() < look_ahead_period) return std::vector<float>(); // RVO
    // Find the boundaries
    const auto &&lb = lower_bound(begin(in.ts_), end(in.ts_), start), ub = upper_bound(begin(in.ts_), end(in.ts_), fin);
    if (lb == ub) return std::vector<float>(); // RVO
    const auto &&min_idx = static_cast<size_t>(distance(begin(in.ts_), lb));
    const auto &&max_idx = distance(begin(in.ts_), ub) - look_ahead_period;

    const auto &&p_returns = make_unique<float[]>(max_idx - min_idx);
    switch (pr_type) {
    case AARC::TA::PeriodReturnType::CLOSECLOSE:
        ispc::period_return(static_cast<const float *>(in.close_.data()), static_cast<const float *>(in.close_.data()),
                            p_returns.get(), min_idx, max_idx, look_ahead_period);
        break;
    case AARC::TA::PeriodReturnType::CLOSELOW:
        ispc::period_return(static_cast<const float *>(in.close_.data()), static_cast<const float *>(in.low_.data()),
                            p_returns.get(), min_idx, max_idx, look_ahead_period);
        break;
    case AARC::TA::PeriodReturnType::CLOSEHIGH:
        ispc::period_return(static_cast<const float *>(in.close_.data()), static_cast<const float *>(in.high_.data()),
                            p_returns.get(), min_idx, max_idx, look_ahead_period);
        break;
    }
    return vector<float>(p_returns.get(), &p_returns[max_idx - min_idx]);
}

auto AARC::TA::histogram(const std::vector<float> &in, const float bucket_size) -> std::vector<size_t> {
    using namespace std;
    if (in.empty() || bucket_size <= 0) return vector<size_t>();
    // Find range for buckets
    const auto &&min_max_elem = minmax_element(begin(in), end(in));
    const auto   min_e = *get<0>(min_max_elem), max_e = *get<1>(min_max_elem);
    const auto &&buckets = static_cast<size_t>((max_e - min_e) / bucket_size);
    if (buckets <= 1) return vector<size_t>();
    auto bins = vector<size_t>(buckets + 1);
    // Each element is independent which is implicit when using stl algorithms
    // so we can easily parallelise it
    for_each(begin(in), end(in), [&buckets, &bins, max_e, min_e](const auto &val) {
        const auto &&bucket_idx = static_cast<size_t>(((val - min_e) / (max_e - min_e)) * buckets);
        bins[bucket_idx]++;
    });
    return bins;
}

/* RSI is typically a n2 algorithm if implemented naively but by sacrificing a couple of array traversals to calculate
the prefix sum you can then calculate the Up/Down ratio as a 1 add + 1 divide linearly. It also then means you can
parallelise it
*/
auto AARC::TA::rsi(const std::vector<float> &in, const size_t period) -> std::vector<float> {
    using namespace std;
    if (in.empty() || period == 0) return vector<float>();
    if (in.size() <= period) return in;

    const auto &sz    = in.size() - period;
    auto        rs_up = make_unique<float[]>(sz);
    ispc::rs_sum(in.data(), rs_up.get(), in.size(), period, true);
    auto rs_down = make_unique<float[]>(sz);
    ispc::rs_sum(in.data(), rs_down.get(), in.size(), period, false);
    auto        ema_up = make_unique<float[]>(sz - period - 1);
    auto        ema_down = make_unique<float[]>(sz - period - 1);
    ispc::ema(rs_up.get(), static_cast<float*>(ema_up.get()), sz, period);
    ispc::ema(rs_down.get(), static_cast<float*>(ema_down.get()), sz, period);

    auto rsi_array = make_unique<float[]>(sz - period - 1);
    ispc::rsi(ema_up.get(), ema_down.get(), rsi_array.get(), sz - period - 1);
    return vector<float>(rsi_array.get(), &rsi_array.get()[sz - period - 1]);
}

auto AARC::TA::ema(const std::vector<float> &in, const size_t period) -> std::vector<float> {
    if (in.empty() || period == 0) return std::vector<float>();
    if (in.size() <= period) return in;
    auto ema_ = vector<float>(in.size() - 1);
    ispc::ema(in.data(), const_cast<float *>(ema_.data()), in.size(), static_cast<float>(period));
    return ema_;
}

auto AARC::TA::sma(const std::vector<float> &in, const size_t period) -> std::vector<float> {
    using namespace std;
    const auto &p_sum = [&in]() {
        std::vector<float> sum;
        std::partial_sum(begin(in), end(in), begin(sum));
        return sum;
    }();
    const auto &sma = accumulate(begin(p_sum), end(p_sum), vector<float>(),
                                 [&p_sum, &period, i = 0 ](auto &&acc, auto &&val) mutable {
                                     acc.emplace_back(p_sum[i + period] - val / static_cast<float>(period));
                                     ++i;
                                     return acc;
                                 });
    return sma;
}

auto AARC::TA::wma(const std::vector<float> &in, const size_t period) -> std::vector<float> {

    return std::vector<float>();
}

auto AARC::TA::macd(const std::vector<float> &in, const size_t upper_period, const size_t lower_period,
                    const size_t crossover_period) -> std::vector<float> {
    if (in.empty() || crossover_period == 0) return std::vector<float>();
    if (in.size() <= crossover_period) return in;
    auto ema_ = vector<float>(in.size() - 1);
    ispc::macd_sub(in.data(), const_cast<float *>(ema_.data()), in.size(), static_cast<float>(lower_period),
                   static_cast<float>(upper_period));
    const auto crossover_ema = ema(ema_, crossover_period);
    return crossover_ema;
}

auto AARC::TA::scale(const std::vector<float> &in, const float a, const float b) noexcept -> std::vector<float> {
    using namespace std;
    auto out = make_unique<float[]>(in.size());
    ispc::scale(in.data(), out.get(), in.size(), b - a);
    return vector<float>(out.get(), &out[in.size() - 1]);
}

#ifdef _TEST
#include "TimeSeriesCSVFactory.h"
#endif

TEST_CASE("Resample timeseries") {
    static auto const filename =
        "H:\\Users\\Mushfaque.Cradle\\Downloads\\HISTDATA_COM_ASCII_EURUSD_M1201703\\data2.csv";
    const auto input = AARC::TimeSeries_CSV::read_csv_file(filename);
    const auto out   = AARC::TA::resample(AARC::TSData(), 0);
    CHECK(out.ts_.empty());
    const auto out1 = AARC::TA::resample(input, 5);
    CHECK(out1.ts_.size() >= (input.ts_.size() / 5));
    const auto out2 = AARC::TA::resample(input, 60 * 24);
    CHECK(out2.ts_.size() >= (input.ts_.size() / (60 * 24)));
    const auto out3 = AARC::TA::resample(input, 60 * 24 * 3);
    CHECK(out3.ts_.size() >= (input.ts_.size() / (60 * 24 * 3)));
}

TEST_CASE("Period returns") {
    static auto const filename =
        "H:\\Users\\Mushfaque.Cradle\\Downloads\\HISTDATA_COM_ASCII_EURUSD_M1201703\\data2.csv";
    const auto input = AARC::TimeSeries_CSV::read_csv_file(filename);
    AARC::TA::period_returns(AARC::TSData(), 3, AARC::TA::PeriodReturnType::CLOSECLOSE, 0,
                             std::numeric_limits<size_t>::max());
    const auto out = AARC::TA::period_returns(input, 3, AARC::TA::PeriodReturnType::CLOSECLOSE, input.ts_[0],
                                              std::numeric_limits<size_t>::max());
    CHECK(out.back() - (input.close_.back() / input.close_[input.close_.size() - 3]) < 0.000001);
    // Invalid index
    const auto out1 = AARC::TA::period_returns(input, 3, AARC::TA::PeriodReturnType::CLOSECLOSE, input.ts_.size() / 3,
                                               input.ts_.size() / 2);
    CHECK(out1.empty());
    const auto out2 = AARC::TA::period_returns(input, 3, AARC::TA::PeriodReturnType::CLOSECLOSE,
                                               input.ts_[input.ts_.size() / 3], input.ts_[input.ts_.size() / 2]);
    CHECK(!out2.empty());

    const auto out3 = AARC::TA::period_returns(input, 3, AARC::TA::PeriodReturnType::CLOSELOW,
                                               input.ts_[input.ts_.size() / 3], input.ts_[input.ts_.size() / 2]);
    CHECK(!out2.empty());

    const auto out4 = AARC::TA::period_returns(input, 3, AARC::TA::PeriodReturnType::CLOSEHIGH,
                                               input.ts_[input.ts_.size() / 3], input.ts_[input.ts_.size() / 2]);
    CHECK(!out2.empty());
}

#include <iostream>

TEST_CASE("Technical analysis") {
    static auto const filename =
        "H:\\Users\\Mushfaque.Cradle\\Downloads\\HISTDATA_COM_ASCII_EURUSD_M1201703\\data2.csv";
    const auto input  = AARC::TimeSeries_CSV::read_csv_file(filename);
    const auto smooth = AARC::TA::smooth_outliers(input, 0.03f);
    CHECK(!smooth.ts_.empty());
    const auto p_returns = AARC::TA::period_returns(input, 3, AARC::TA::PeriodReturnType::CLOSECLOSE, input.ts_[0],
                                                    std::numeric_limits<size_t>::max());
    CHECK(!p_returns.empty());
    const auto out2 = AARC::TA::histogram(p_returns, 0.001f);
    CHECK(!out2.empty());
    const auto ema = AARC::TA::ema(input.close_, 10);
    CHECK(!ema.empty());
    const auto rsi = AARC::TA::rsi(input.close_, 10);
    CHECK(!rsi.empty());
    const auto macd = AARC::TA::macd(input.close_, 26, 12, 9);
    CHECK(!macd.empty());
    const auto scale = AARC::TA::scale(input.close_);
    CHECK(!scale.empty());
}
