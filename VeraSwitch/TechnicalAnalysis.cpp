#include "TechnicalAnalysis.h"
#include "TimeSeries.h"
#include <doctest\doctest.h>
#include <future>
#include <numeric>
#include <ppl.h>
#include <spdlog\spdlog.h>
#include <vector>

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
    const auto  min_idx = static_cast<size_t>(distance(begin(in.ts_), lb));
    const auto  max_idx = distance(begin(in.ts_), ub) - look_ahead_period;
    const auto &closes  = in.close_;

    vector<float> results;
    results.reserve(max_idx - min_idx);
    for (auto i = min_idx; i < max_idx; i++) {
        const auto v0 = closes[i];
        const auto v1 = closes[i + look_ahead_period];
        results.emplace_back((static_cast<float>(v1) / static_cast<float>(v0)) - 1.0f);
    }
    return results;
}

#ifdef _TEST
#include "TimeSeriesCSVFactory.h"
#endif

TEST_CASE("Resample timeseries") {
    static auto const filename =
        "H:\\Users\\Mushfaque.Cradle\\Downloads\\HISTDATA_COM_ASCII_EURUSD_M1201703\\data2.csv";
    const auto input = AARC::TimeSeries_CSV::read_csv_file(filename);
    SUBCASE("No data") {
        const auto out = AARC::TA::resample(AARC::TSData(), 0);
        CHECK(out.ts_.empty());
    }
    SUBCASE("5 minute") {
        const auto out = AARC::TA::resample(input, 5);
        CHECK(out.ts_.size() >= (input.ts_.size() / 5));
    }
    SUBCASE("Daily") {
        const auto out = AARC::TA::resample(input, 60 * 24);
        CHECK(out.ts_.size() >= (input.ts_.size() / (60 * 24)));
    }
    SUBCASE("3 Day") {
        const auto out = AARC::TA::resample(input, 60 * 24 * 3);
        CHECK(out.ts_.size() >= (input.ts_.size() / (60 * 24 * 3)));
    }
}

TEST_CASE("Period returns") {
    static auto const filename =
        "H:\\Users\\Mushfaque.Cradle\\Downloads\\HISTDATA_COM_ASCII_EURUSD_M1201703\\data2.csv";
    const auto input = AARC::TimeSeries_CSV::read_csv_file(filename);

    SUBCASE("No data") { AARC::TA::period_returns(AARC::TSData(), 3, 0, std::numeric_limits<size_t>::max()); }
    SUBCASE("Full array") {
        const auto out = AARC::TA::period_returns(input, 3, input.ts_[0], std::numeric_limits<size_t>::max());
        CHECK(out.back() - (input.close_.back() / input.close_[input.close_.size() - 3]) < 0.000001);
    }
    SUBCASE("Partial array") {

        // Invalid index
        const auto out1 = AARC::TA::period_returns(input, 3, input.ts_.size() / 3, input.ts_.size() / 2);
        CHECK(out1.empty());

        const auto out =
            AARC::TA::period_returns(input, 3, input.ts_[input.ts_.size() / 3], input.ts_[input.ts_.size() / 2]);
        CHECK(!out.empty());
    }
}