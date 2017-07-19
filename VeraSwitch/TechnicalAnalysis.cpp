#include "TechnicalAnalysis.h"
#include "TimeSeries.h"
#include <doctest\doctest.h>
#include <future>
#include <numeric>
#include <ppl.h>
#include <spdlog\spdlog.h>

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
