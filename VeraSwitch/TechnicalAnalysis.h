#pragma once
#include <vector>

namespace AARC {
    struct TSData;
    namespace TA {
        auto resample(const TSData &in, const int mins = 5 /* Resample to this time unit, in minutes */)
            -> const TSData;
        /*
        std::vector<float> sma(const TSData&in, const int start = 0, const int end = -1);
        Each TA has it's own set of parameters which we need to apply.
        Only apply for visible area to save on cpu cycles
        Note that TAs are variable independent so can be threaded
        */
    } // namespace TA
} // namespace AARC