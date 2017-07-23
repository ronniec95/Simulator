#include "TechnicalAnalysis.h"
#include "TimeSeries.h"
#include <deque>
#include <doctest\doctest.h>
#include <future>
#include <numeric>
#include <ppl.h>
#include <spdlog\spdlog.h>
#include <vector>
#include "Split.h"

auto AARC::TA::resample(const AARC::TSData &in, const int mins) -> const AARC::TSData {
    using namespace std;
    if (in.ts_.empty() || mins == 0) return AARC::TSData();
    const auto start = in.ts_.front(), fin = in.ts_.back();
    if ((fin - start) < mins) { return TSData(); }

    // Find positions in one step
    const auto sz        = (fin - start) / mins + 1;
    auto       positions = vector<pair<size_t, size_t>>();
    positions.reserve(static_cast<size_t>(sz));
    auto current = start;
    generate_n(back_inserter(positions), sz, [&in, &current, &mins]() -> decltype(positions)::value_type {
        const auto lb = lower_bound(begin(in.ts_), end(in.ts_), current);
        const auto ub = upper_bound(begin(in.ts_), end(in.ts_), current + mins);
        current += mins;
        if (lb == ub) return {0, 0}; /* These are invalid positions, caused by gaps in data or weekends */
        return {distance(begin(in.ts_), lb), distance(begin(in.ts_), ub) - 1 /* need the index here so -1 */};
    });

    auto open = async(launch::async, [&positions, &in]() {
        return accumulate(begin(positions), end(positions), vector<float>(), [&in](auto &acc, const auto pos) {
            acc.emplace_back(in.open_[get<0>(pos)]);
            return acc;
        });
    });
    auto close = async(launch::async, [&positions, &in]() {
        return accumulate(begin(positions), end(positions), vector<float>(), [&in](auto &acc, const auto pos) {
            acc.emplace_back(in.close_[get<1>(pos)]);
            return acc;
        });

    });
    auto high = async(launch::async, [&positions, &in]() {
        return accumulate(begin(positions), end(positions), vector<float>(), [&in](auto &acc, const auto pos) {
            acc.emplace_back(*max_element(in.high_.begin() + get<0>(pos), in.high_.begin() + get<1>(pos)));
            return acc;
        });
    });
    auto low = async(launch::async, [&positions, &in]() {
        return accumulate(begin(positions), end(positions), vector<float>(), [&in](auto &acc, const auto pos) {
            acc.emplace_back(*min_element(in.high_.begin() + get<0>(pos), in.high_.begin() + get<1>(pos)));
            return acc;
        });
    });

    return TSData(in.ts_, open.get(), high.get(), low.get(), close.get());
}

auto AARC::TA::period_returns(const TSData &in, const size_t look_ahead_period, const size_t start, const size_t fin)
    -> std::vector<float> {
    using namespace std;
    if (in.ts_.size() < look_ahead_period) return std::vector<float>(); // RVO
    // Find the boundaries
    const auto lb = lower_bound(begin(in.ts_), end(in.ts_), start), ub = upper_bound(begin(in.ts_), end(in.ts_), fin);
    if (lb == ub) return std::vector<float>(); // RVO
    const auto min_idx = static_cast<size_t>(distance(begin(in.ts_), lb));
    const auto max_idx = distance(begin(in.ts_), ub) - look_ahead_period;

    vector<float> results;
    generate_n(back_inserter(results), max_idx - min_idx,
               [ i = min_idx, closes = in.close_, &look_ahead_period ]() mutable {
                   const auto v0 = closes[i];
                   const auto v1 = closes[i + look_ahead_period];
                   i++;
                   return (static_cast<float>(v1) / static_cast<float>(v0)) - 1.0f;
               });
    return results;
}

auto AARC::TA::histogram(const std::vector<float> &in, const float bucket_size) -> std::vector<size_t> {
    using namespace std;
    if (in.empty() || bucket_size == 0) return vector<size_t>();
    // Find range for buckets
    const auto min_max_elem = minmax_element(begin(in), end(in));
    const auto min_e = *get<0>(min_max_elem), max_e = *get<1>(min_max_elem);
    const auto buckets = static_cast<size_t>((max_e - min_e) / bucket_size);
    if (buckets <= 1) return vector<size_t>();
    auto bins = vector<size_t>(buckets + 1);
    // Each element is independent which is implicit when using stl algorithms
    // so we can easily parallelise it
    concurrency::parallel_for_each(begin(in), end(in), [&buckets, &bins, max_e, min_e](const auto val) {
        const auto bucket_idx = static_cast<size_t>(((val - min_e) / (max_e - min_e)) * buckets);
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

    auto rsi_out       = vector<float>(in.size() - period);
    auto up_prefix_sum = async(launch::async, [&in]() {
        vector<float> up_prefix_sum;
        partial_sum(begin(in), end(in), back_inserter(up_prefix_sum), [prev = in.front()](auto sum, auto val) mutable {
            const auto p_sum = (val > prev) ? sum + val : sum;
            prev             = val;
            return p_sum;
        });
        return up_prefix_sum;
    });

    auto down_prefix_sum = async(launch::async, [&in]() {
        vector<float> down_prefix_sum;
        partial_sum(begin(in), end(in),
                    back_inserter(down_prefix_sum), [prev = in.front()](auto sum, auto val) mutable {
                        const auto p_sum = (val < prev) ? sum + val : sum;
                        prev             = val;
                        return p_sum;
                    });
        return down_prefix_sum;
    });

    vector<float> rs;
    generate_n(back_inserter(rs), in.size() - period,
               [ i = 0, up_ps = up_prefix_sum.get(), down_ps = down_prefix_sum.get(), &period ]() mutable {
                   const auto up   = up_ps[i + period] - up_ps[i];
                   const auto down = down_ps[i + period] - down_ps[i];
                   i++;
                   return up / (down == 0.0f ? 0.0000001f : down);
               });
    rs = ema(rs, period / 5);
    ispc::rsi_summary(const_cast<float*>(rs.data()),rs.size());
    return rs;
}

auto AARC::TA::ema(const std::vector<float> &in, const size_t period) -> std::vector<float> {
    if (in.empty() || period == 0) return std::vector<float>();
    if (in.size() <= period) return in;   
    auto ema_ = vector<float>(in.size() - 1);
    ispc::ema(in.data(), const_cast<float*>(ema_.data()), ema_.size(), period);
    return ema_;
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
    AARC::TA::period_returns(AARC::TSData(), 3, 0, std::numeric_limits<size_t>::max());
    const auto out = AARC::TA::period_returns(input, 3, input.ts_[0], std::numeric_limits<size_t>::max());
    CHECK(out.back() - (input.close_.back() / input.close_[input.close_.size() - 3]) < 0.000001);
    // Invalid index
    const auto out1 = AARC::TA::period_returns(input, 3, input.ts_.size() / 3, input.ts_.size() / 2);
    CHECK(out1.empty());
    const auto out2 =
        AARC::TA::period_returns(input, 3, input.ts_[input.ts_.size() / 3], input.ts_[input.ts_.size() / 2]);
    CHECK(!out2.empty());
}

TEST_CASE("Histogram") {
    static auto const filename =
        "H:\\Users\\Mushfaque.Cradle\\Downloads\\HISTDATA_COM_ASCII_EURUSD_M1201703\\data2.csv";
    const auto input     = AARC::TimeSeries_CSV::read_csv_file(filename);
    const auto p_returns = AARC::TA::period_returns(input, 3, input.ts_[0], std::numeric_limits<size_t>::max());
    CHECK(!p_returns.empty());
    const auto out2 = AARC::TA::histogram(p_returns, 0.001f);
    CHECK(!out2.empty());
    const auto ema = AARC::TA::ema(input.close_, 10);
    CHECK(!ema.empty());
    const auto rsi = AARC::TA::rsi(input.close_, 10);
    CHECK(!rsi.empty());
}