#pragma once
#include "TimeSeries.h"
#include <chrono>
#include <memory>

namespace AARC {
	namespace TimeSeriesFactory {
		auto create(const std::string& db, const AARC::TSData& data);
		auto remove(const std::string& db, const uint64_t asset_id,
					const uint64_t start,
					const uint64_t end);
		auto select(const std::string& db,
					const uint64_t asset_id,
					const uint64_t start,
					const uint64_t end)->std::unique_ptr<TSData>;
	};
}