#pragma once
#include "TimeSeries.h"
#include <chrono>
#include <memory>

namespace AARC {
    namespace TimeSeriesFactory {
        auto create(const std::string &db, const AARC::TSData &data, const int units) -> void;
        auto remove(const std::string &db, const uint64_t asset_id, const uint64_t start, const uint64_t end,
                    const int units) -> void;
        auto select(const std::string &db, const uint64_t asset_id, const uint64_t start, const uint64_t end,
                    const int units) -> std::unique_ptr<TSData>;
    }; // namespace TimeSeriesFactory
} // namespace AARC