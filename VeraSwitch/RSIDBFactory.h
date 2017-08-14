#pragma once
#include "TimeSeries.h"
#include <chrono>
#include <memory>
#include <string>

namespace AARC {
    namespace RSIDBFactory {
        // Crud operations for a technical indicator
        // Rather than have a database/factory interface for each and every technical analysis indicator, we have a
        // single interface that takes the original OLHC timeseries and the TA output as inputs, along with the TA name
        // and resolution and period. As you can see it ends up with a combinatorial set of outputs
        auto create(const std::string &db, const std::string &name, const AARC::TSData &ts, const vector<float> &in,
                    const int units) -> void;
        auto remove(const std::string &db, const uint64_t asset_id, const uint64_t start, const uint64_t end,
                    const int units) -> void;
        auto select(const std::string &db, const uint64_t asset_id, const uint64_t start, const uint64_t end,
                    const int units) -> std::unique_ptr<TSData>;
    }; // namespace RSIDBFactory
} // namespace AARC