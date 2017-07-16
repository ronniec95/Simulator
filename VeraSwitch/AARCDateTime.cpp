#include "AARCDateTime.h"
#include <doctest\doctest.h>


std::tm AARC::AARCDateTime::epoch_tm = { 0,0,0,1,0,90,0,0,0 };
const std::chrono::system_clock::time_point AARC::AARCDateTime::epoch = std::chrono::system_clock::from_time_t(mktime(&AARC::AARCDateTime::epoch_tm));

TEST_CASE("Epoch") {
	// Create a time point 2/3/1992
	const auto tp = AARC::AARCDateTime(1992, 2, 24);
	// Number of seconds between
	CHECK(tp.minutes > 0);
	// Convert epoch + seconds back to datetime
	const auto tm = tp.to_tm();
	CHECK(tm.tm_year == 1992 - 1900);
	CHECK(tm.tm_mon == 1);
	CHECK(tm.tm_mday == 24);
}