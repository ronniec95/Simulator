#include "TimeSeriesCSVFactory.h"
#include "AARCDateTime.h"
#include "TimeSeries.h"
#include "Utilities.h"
#include <algorithm>
#include <chobo\small_vector.hpp>
#include <doctest\doctest.h>
#include <fstream>
#include <numeric>
#include <spdlog\spdlog.h>
#include <vector>

namespace {

    // Only works for positive numbers and really only tested with dates
    auto naive_atoi(const char *buf, const int pos, const int sz) {
        if (buf == nullptr) return 0;
        if ((pos + sz) > sizeof(buf) / sizeof(buf[0])) return 0;
        static int multiplier[] = {0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
        int        sum          = 0;
        for (int i = pos; i < pos + sz; i++) {
            const auto value = buf[i] - '0';
            const auto mult  = multiplier[sz - i + pos];
            sum += value * mult;
        }
        return sum;
    }
    auto naive_atof(const std::string &buf) {
        if (buf.empty()) return 0.0f;
        static int   multiplier[]  = {0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
        static float divider[]     = {0.0f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f};
        auto         sum           = float(0);
        const auto   sz            = buf.size();
        auto         decimal_point = 0;
        for (int i = 0; i < sz; i++) {
            if (buf[i] == '.') {
                decimal_point = i;
                break;
            }
        }
        for (auto i = sz - 1; i-- > 0;) {
            const auto lookup = buf[i] - '0';
            if (lookup == -2) continue;
            const auto value = lookup;
            const auto pos   = int64_t(i - decimal_point);
            sum += (pos > 0 ? divider[pos] : multiplier[-pos]) * value;
        }
        return sum;
    }

    auto scnstr(std::tm &tm, const std::string &fmt, const std::string &buffer) noexcept -> bool {
        tm.tm_year = -1;
        tm.tm_mday = -1;
        for (auto i = 0, pos = 0; i < fmt.size() - 1; i++) {
            if (pos > buffer.size()) return false;
            const char curr = fmt[i];
            const char next = fmt[i + 1];
            if (curr == '%') {
                switch (next) {
                case 'Y':
                    tm.tm_year = naive_atoi(buffer.data(), pos, 4) - 1900;
                    pos += 4;
                    break;
                case 'm':
                    tm.tm_mon = naive_atoi(buffer.data(), pos, 2);
                    pos += 2;
                    break;
                case 'd':
                    tm.tm_mday = naive_atoi(buffer.data(), pos, 2);
                    pos += 2;
                    break;
                case 'H':
                    tm.tm_hour = naive_atoi(buffer.data(), pos, 2);
                    pos += 2;
                    break;
                case 'M':
                    tm.tm_min = naive_atoi(buffer.data(), pos, 2);
                    pos += 2;
                    break;
                case 'S':
                    tm.tm_sec = naive_atoi(buffer.data(), pos, 2);
                    pos += 2;
                    break;
                case '%': pos++; break;
                }
                i++;
            } else {
                if (curr != buffer[pos]) return false;
                pos++;
            }
        }
        return (tm.tm_year > 50 && tm.tm_year < 200 && tm.tm_mday >= 0 && tm.tm_hour >= 0 && tm.tm_min >= 0 &&
                tm.tm_sec >= 0)
                   ? true
                   : false;
    }
    // Read line looking for separator chars based on probability of occurrence
    auto find_separator(const std::string &data) noexcept -> const char {
        using namespace std;
        MethodLogger mlog("find_separator");
        const auto   separators = array<char, 9>{',', ';', '\t', '|', ':', '~', '-', '+', '#'};
        const auto   counters =
            std::accumulate(begin(data), end(data), array<size_t, sizeof(separators)>{0}, [](auto acc, const auto ch) {
                switch (ch) {
                case ',': acc[0]++; break;
                case ';': acc[1]++; break;
                case '\t': acc[2]++; break;
                case '|': acc[3]++; break;
                case ':': acc[4]++; break;
                case '~': acc[5]++; break;
                case '-': acc[6]++; break;
                case '+': acc[7]++; break;
                case '#': acc[8]++; break;
                }
                return acc;
            });
        // Find the counter with the maximum
        const auto &it = std::max_element(std::begin(counters), std::end(counters));
        return all_of(begin(counters), end(counters), [](auto i) { return i == 0; })
                   ? 0
                   : separators[distance(begin(counters), it)];
    }
    template <typename Out> auto find_header(const std::string &name, const Out &cols) noexcept -> const int64_t {
        using namespace std;
        MethodLogger mlog("find_header");
        SPDLOG_DEBUG(mlog.logger(), "Looking for {} ", name);
        const auto &it = find_if(begin(cols), end(cols),
                                 [&name](const std::string &col) { return !(col.find(name) == string::npos); });
        return (it == cols.end()) ? -1 : distance(cols.begin(), it);
    }
    template <typename Out>
    auto find_headers(const Out &rows, std::unique_ptr<AARC::TimeSeries_CSV::details> &details) noexcept {
        MethodLogger mlog("find_details");
        mlog.logger()->error_if(!details, "Details is null");
        mlog.logger()->error_if(details->separator == 0, "No separator found");
        details->header_line = -1;
        for (const auto &row : rows) {
            const auto fields     = split<Out>(row, details->separator);
            details->open_column  = find_header("open", fields);
            details->high_column  = find_header("high", fields);
            details->low_column   = find_header("low", fields);
            details->close_column = find_header("close", fields);
            details->ts_column    = find_header("time", fields);
            details->header_line++;
            if (!(details->open_column == -1 && details->high_column == -1 && details->low_column == -1 &&
                  details->close_column == -1 && details->ts_column == -1))
                break;
        }
    }
    template <typename Out>
    auto find_timestamp_parser(const Out &rows, std::unique_ptr<AARC::TimeSeries_CSV::details> &details) noexcept {
        MethodLogger mlog("find_timestamp_parser");
        mlog.logger()->error_if(details->ts_column == -1, "Timestamp column not found");
        std::array<size_t, AARC::TimeSeries_CSV::ts_formats.size()> ts_vote{};
        for (auto row = std::begin(rows) + details->header_line; row != std::end(rows); row++) {
            const auto fields       = split<Out>(*row, details->separator);
            const auto ts_candidate = fields[details->ts_column];
            for (auto i = 0UL; i < AARC::TimeSeries_CSV::ts_formats.size(); i++) {
                std::tm tm{};
                if (scnstr(tm, AARC::TimeSeries_CSV::ts_formats[i], ts_candidate) == true) ts_vote[i]++;
            }
        }
        const auto it = max_element(std::begin(ts_vote), std::end(ts_vote));
        details->timeseries_format =
            (it == ts_vote.end()) ? std::string() : AARC::TimeSeries_CSV::ts_formats[distance(begin(ts_vote), it)];
    }

    // Just read 5K of data which should encompass at least a few dozen lines
    // and return as a string buffer
    auto csv_part_load(const std::string &filename) {
        using namespace std;
        MethodLogger mlog("csv_load_path");
        mlog.logger()->debug(" Filename {} Trying to load", filename);
        ifstream              csvfile(filename, std::ios::in | std::ios::binary);
        array<char, 5 * 1024> buf;
        csvfile.read(buf.data(), buf.size());
        string line;
        line.resize(csvfile.gcount());
        if (csvfile.gcount() > 0) { transform(begin(buf), end(buf), begin(line), ::tolower); }
        return line;
    }

    // Read a specified number of non-blank lines, starting from the header row
    // and return as a TSData object
    auto csv_read_line(const std::string &filename, const std::unique_ptr<AARC::TimeSeries_CSV::details> &details,
                       const size_t num_lines) {
        using namespace std;
        ifstream in(filename);
        string   line;
        auto row_num = size_t(0), end_row = num_lines == -1 ? UINTMAX_MAX : size_t(details->header_line + num_lines);
        auto fields = chobo::small_vector<string>(16, string());
        auto ts     = make_unique<AARC::TSData>();
        while (getline(in, line)) {
            if (row_num > details->header_line) {
                split(line, details->separator, fields.begin());
                std::tm tm = {};
                scnstr(tm, details->timeseries_format, fields[details->ts_column]);
                ts->ts_.emplace_back(AARC::AARCDateTime(tm).minutes);
                ts->open_.emplace_back(naive_atof(fields[details->open_column]));
                ts->high_.emplace_back(naive_atof(fields[details->high_column]));
                ts->low_.emplace_back(naive_atof(fields[details->low_column]));
                ts->close_.emplace_back(naive_atof(fields[details->close_column]));
            }

            if (row_num > num_lines) break;
            row_num++;
        }
        return ts;
    }
    auto find_details(const std::string &csv_part) -> std::unique_ptr<AARC::TimeSeries_CSV::details> {
        const auto rows    = split<chobo::small_vector<std::string>>(csv_part, '\n');
        auto       details = std::make_unique<AARC::TimeSeries_CSV::details>();
        details->separator = find_separator(csv_part);
        find_headers(rows, details);
        find_timestamp_parser(rows, details);
        return details;
    }
} // namespace

auto AARC::TimeSeries_CSV::read_csv_file(const std::string &filename) -> std::unique_ptr<AARC::TSData> {
    const auto csv_part = csv_part_load(filename);
    const auto details  = find_details(csv_part);
    return csv_read_line(filename, details, -1);
}

auto AARC::TimeSeries_CSV::read_csv_partial_file(const std::string &filename) -> std::unique_ptr<AARC::TSData> {
    const auto csv_part = csv_part_load(filename);
    const auto details  = find_details(csv_part);
    return csv_read_line(filename, details, 20);
}

TEST_SUITE("Timeseries parsing") {
    TEST_CASE("CSV File details finder") {
        const char *data = "foo;bar\nfoawuehfaw\nfdqwfe\ntime;open;high;low;close;;\n"
                           "20170301 023400; 1.053910; 1.054070; 1.053880; 1.053960; 0\n"
                           "20170301 023500; 1.053990; 1.054020; 1.053870; 1.053960; 0\n"
                           "20170301 023600; 1.053970; 1.054070; 1.053850; 1.054070; 0\n"
                           "20170301 023700; 1.054060; 1.054090; 1.053970; 1.054050; 0\n"
                           "20170301 023800; 1.054070; 1.054290; 1.054020; 1.054250; 0\n"
                           "20170301 023900; 1.054270; 1.054360; 1.054210; 1.054280; 0\n"
                           "20170301 024000; 1.054290; 1.054660; 1.054290; 1.054410; 0\n"
                           "20170301 024100; 1.054400; 1.054650; 1.054330; 1.054650; 0\n"
                           "20170301 024200; 1.054630; 1.054640; 1.054510; 1.054550; 0\n"
                           "20170301 024300; 1.054560; 1.054580; 1.054360; 1.054500; 0\n";
        SUBCASE("Split") {
            const auto rows = split<std::vector<std::string>>(data, '\n');
            CHECK(rows.size() > 0);
        }
        SUBCASE("Find separator") {
            const auto separator = find_separator(data);
            CHECK(separator == ';');
        }
        SUBCASE("Find seperator with empty data") {
            const auto separator = find_separator("");
            CHECK(separator == 0);
        }
        SUBCASE("Find header") {
            const std::array<std::string, 15> cols{"open", "high", "low", "fooclosebar", "timestamp"};
            SUBCASE("SUBSTRING") {
                CHECK(find_header("open", cols) == 0);
                CHECK(find_header("close", cols) == 3); // Substring matches
                CHECK(find_header("time", cols) == 4);
            }
            // Not found matches
            SUBCASE("No match") {
                const std::array<std::string, 15> cols{"open", "high", "low", "fooclosebar", "timestamp"};
                CHECK(find_header("baz", cols) == -1);
            }
            SUBCASE("No columns, no crash") {
                // Empty set of columns, shouldn't crash
                const std::array<std::string, 15> cols{};
                CHECK(find_header("bar", cols) == -1);
            }
        }
        SUBCASE("Find headers") {
            const auto rows    = split<std::vector<std::string>>(data, '\n');
            auto       details = std::make_unique<AARC::TimeSeries_CSV::details>();
            details->separator = ';';
            find_headers(rows, details);
            CHECK(details->open_column == 1);
            CHECK(details->high_column == 2);
            CHECK(details->low_column == 3);
            CHECK(details->close_column == 4);
            CHECK(details->ts_column == 0);
        }
        SUBCASE("Find timestamp parsers") {
            const auto rows    = split<std::vector<std::string>>(data, '\n');
            auto       details = std::make_unique<AARC::TimeSeries_CSV::details>();
            details->separator = ';';
            find_headers(rows, details);
            find_timestamp_parser(rows, details);
        }
    }

    TEST_CASE("naive_atoi") {
        const char data[] = "123456";
        SUBCASE("valid data") {
            const auto val = naive_atoi(data, 0, 3);
            CHECK(val == 123);
        }
        SUBCASE("null data") {
            const auto val = naive_atoi(nullptr, 0, 3);
            CHECK(val == 0);
        }
        SUBCASE("pointer outside bounds") {
            const auto val = naive_atoi(data, 6, 10);
            CHECK(val == 0);
        }
    }

    TEST_CASE("native_atof") {
        const char data[] = "123.456789";
        SUBCASE("valid data") {
            const auto val = naive_atof(data);
            CHECK(val - 123.456789 < 0.00001);
        }
        SUBCASE("null data") {
            const auto val = naive_atof("");
            CHECK(val == 0);
        }
    }
    static auto const filename =
        "H:\\Users\\Mushfaque.Cradle\\Downloads\\HISTDATA_COM_ASCII_EURUSD_M1201703\\data2.csv";
    TEST_CASE("CSV Partial load") {
        const auto tsdata = AARC::TimeSeries_CSV::read_csv_partial_file(filename);
        CHECK(tsdata->ts_.size() > 0);
    }
    TEST_CASE("CSV full load") {
        const auto tsdata = AARC::TimeSeries_CSV::read_csv_file(filename);
        CHECK(tsdata->ts_.size() > 0);
    }
}
