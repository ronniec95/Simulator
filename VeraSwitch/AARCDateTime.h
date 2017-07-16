#pragma once
#include <chrono>

namespace AARC {
    class AARCDateTime {
      public:
        explicit AARCDateTime(std::tm &tm) {
            tp = std::chrono::system_clock::from_time_t(_mktime64(&tm));
            // Convert to minutes from epoch
            minutes = std::chrono::duration_cast<std::chrono::minutes>(tp - epoch).count();
        }
        AARCDateTime(const int year, const int month, const int day, const int hour = 0, const int minute = 0,
                     const int second = 0) {
            std::tm tm;
            tm.tm_year = year - 1900;
            tm.tm_mday = day;
            tm.tm_mon  = month - 1;
            tm.tm_hour = hour;
            tm.tm_min  = minute;
            tm.tm_sec  = second;
            tp         = std::chrono::system_clock::from_time_t(_mktime64(&tm));
            minutes    = std::chrono::duration_cast<std::chrono::minutes>(tp - epoch).count();
        }
        explicit AARCDateTime(const uint64_t mins) {
            tp      = (epoch + std::chrono::minutes(mins));
            minutes = mins;
        }

        auto to_tm() const noexcept {
            std::tm    tm;
            const auto tt = std::chrono::system_clock::to_time_t(tp);
            _gmtime64_s(&tm, &tt);
            return tm;
        }

        std::chrono::system_clock::time_point              tp;
        const static std::chrono::system_clock::time_point epoch;
        uint64_t                                           minutes;
        static std::tm                                     epoch_tm;

      private:
    };
} // namespace AARC