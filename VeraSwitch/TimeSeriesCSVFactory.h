#pragma once
#include "TimeSeries.h"
#include <memory>
#include <string>

namespace AARC {
    namespace TimeSeries_CSV {
        const std::array<std::string, 2> ts_formats = {"%Y%m%d %H%M%S", "%Y%m%d %H:%M:%S"};
        struct details {
            char        separator;
            uint64_t    header_line;
            uint64_t    open_column, high_column, low_column, close_column, ts_column;
            std::string timeseries_format;
        };

        auto read_csv_file(const std::string &filename) -> AARC::TSData;
        auto read_csv_partial_file(const std::string &filename) -> AARC::TSData;
    } // namespace TimeSeries_CSV
} // namespace AARC
