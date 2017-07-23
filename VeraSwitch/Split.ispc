//const auto alpha = 1.0f / period;
//return accumulate(begin(in), end(in), vector<float>(), [prev = in.front(),&alpha](auto& acc, const auto val) mutable {
//    prev = alpha * val + (1.0f - alpha) * prev;
//         acc.emplace_back(prev);
//         return acc;
//    });
export void ema(const uniform float vin[], uniform float vout[], const uniform int count, const uniform float period) {
    uniform float alpha = 1.0f / period;
	float prev = vin[0];
    for (uniform int i = 1 ; i < count; i++) {
        float       v    = vin[i];
        prev  = alpha * v + (1.0f - alpha) * prev;
    }
}

export void rsi_summary(uniform float vinout[], const uniform int count) {
    for(uniform int i = 0 ;i < count ;i++) { vinout[i] = 100.0f - 100.0f / (1.0f + vinout[i]); }
}

export void find_char(uniform const unsigned int8 arr[], const uniform int64 start, const uniform int64 end, uniform const int8 delim,
                      uniform int &pos) {
    for (uniform int index = start; index < end; index++) {
        if (arr[index] == delim) {
            pos = index;
            break;
        }
    }
}
