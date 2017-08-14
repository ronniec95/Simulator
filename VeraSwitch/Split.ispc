export void ema(const uniform float vin[], uniform float vout[], const uniform int64 count,
                const uniform float period) {
    uniform float alpha = 1.0f / period;
    foreach(i = period+1 ... count) {
		float emav = 0.0;
		for(int j = i, k = period; j < i + (period + 1);j++,k--){
			emav += pow(1-alpha,k) * vin[j];	
		};
		vout[i-period-1] = emav;
	}
}

export void macd_sub(const uniform float vin[], uniform float vout[], const uniform int64 count,
                     const uniform float period, const uniform float period2) {
    uniform float alpha1 = 1.0f / period;
    uniform float alpha2 = 1.0f / period2;
    float         prev1  = vin[0];
    float         prev2  = vin[0];
    foreach (i = 1 ... count) {
        float v     = vin[i];
        prev1       = v + alpha1 * (v - prev1);
        prev2       = v + alpha2 * (v - prev2);
        vout[i - 1] = prev1 - prev2;
    }
}

uniform float minmax_array(const uniform float vin[], const uniform int64 count, uniform float &min_value,
                           uniform float &max_value) {
    min_value = vin[0];
    max_value = vin[0];
    foreach (i = 1 ... count) {
        uniform float min_v = reduce_min(vin[i]);
        uniform float max_v = reduce_max(vin[i]);
        min_value           = min(min_v, min_value);
        max_value           = max(max_v, max_value);
    }
}

export void scale(const uniform float vin[], uniform float vout[], const uniform int64 count,
                  const uniform float scaling) {
    uniform float min_value;
    uniform float max_value;
    minmax_array(vin, count, min_value, max_value);
    uniform float rng = max_value - min_value;
    foreach (i = 0 ... count) { vout[i] = min_value + scaling * ((vin[i] - min_value) / rng); }
}

export void rsi_summary(uniform float vinout[], const uniform int64 count) {
    for (uniform int i = 0; i < count; i++) { vinout[i] = 100.0f - 100.0f / (1.0f + vinout[i]); }
}

export void find_char(uniform const unsigned int8 arr[], const uniform int64 start, const uniform int64 end,
                      uniform const int8 delim, uniform int &pos) {
    for (uniform int index = start; index < end; index++) {
        if (arr[index] == delim) {
            pos = index;
            break;
        }
    }
}

export uniform int naive_atoi(uniform const unsigned int8 buf[], uniform const int sz) {
    static int  multiplier[] = {0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
    uniform int sum          = 0;
    foreach (i = 0 ... sz) {
        int value = buf[i] - 48;
        int mult  = multiplier[sz - i];
        int total = (value * mult);
        sum += reduce_add(total);
    }
    return sum;
}

// generate_n(back_inserter(results), max_idx - min_idx,
//           [ i = min_idx, closes = in.close_, &look_ahead_period ]() mutable {
//               const auto v0 = closes[i];
//               const auto v1 = closes[i + look_ahead_period];
//               i++;
//               return (static_cast<float>(v1) / static_cast<float>(v0)) - 1.0f;
//           });
export void period_return(uniform const float vin[], uniform const float vin2[], uniform float vout[],
                          const uniform int64 min_idx, const uniform int64 max_idx,
                          const uniform int64 look_ahead_period) {
    foreach (i = min_idx... max_idx) {
        const float v0    = vin[i];
        const float v1    = vin2[i + look_ahead_period];
        vout[i - min_idx] = (v1 / v0) - 1.0;
    }
}

export void smooth_outliers(uniform const float vin[], uniform float vout[], const uniform int64 count,
                            const uniform float tolerance, const uniform float avg) {
    // Copy everything across first
    foreach (i = 0 ... count) { vout[i] = vin[i]; }
    foreach (i = 1 ... count) {
        const float prev = vout[i - 1];
        const float val  = vin[i];
        const float diff = abs(val / prev - 1.0);
        const float move = (diff > tolerance) ? prev + avg : val;
        vout[i]          = move;
    }
}

/*
    auto &&up_sum =
        accumulate(begin(in)+1, end(in), vector<float>(), [prev = in.front()](auto &&acc, auto &&val) mutable {
            auto &&sum = (val > prev) ? val - prev : 0.0f;
            prev       = val;
            acc.emplace_back(sum);
            return acc;
        });
    partial_sum(begin(up_sum), end(up_sum), begin(up_sum));
    auto &&rs_up = accumulate(begin(up_sum) + period, end(up_sum), vector<float>(),
                              [ i = 0, &up_sum, &period ](auto &&acc, auto &&val) mutable {
                                  const auto &&p_sum = (val - up_sum[i]) / period;
                                  acc.emplace_back(p_sum);
                                  ++i;
                                  return acc;
                              });

*/
export void rs_sum(uniform const float vin[], uniform float vout[], const uniform int64 count,
                   uniform const int64 period, uniform const bool up) {

    float *uniform tmp = uniform new float[count - 1];
    // Sum of positives or negatives
	tmp[0] = 0.0;
    if (up) {
        foreach (i = 1 ... count) {
            const float prev = vin[i-1];
            const float val  = vin[i];
            const float sum  = val > prev ? val - prev : 0.0;
            tmp[i]           = sum;
        }
    } else {
        foreach (i = 1 ... count) {
            const float prev = vin[i-1];
            const float val  = vin[i];
            const float sum  = val < prev ? prev - val : 0.0;
            tmp[i]           = sum;
        }
    }
    // Prefix sum using sse
    for (uniform int i = 1; i < count; i++) { tmp[i] += tmp[i - 1];	 }

	// Gain
    foreach (i = period ... count) {
        const float prev    = tmp[i - period];
        const float current = tmp[i];
        const float gain    = ((current - prev) + 0.00000001) / ((float)period);
	    vout[i - period]     = gain;
    }
    delete[] tmp;
}

export void rsi(uniform const float rs_up[], uniform const float rs_down[], uniform float vout[],
                const uniform int64 count) {
    foreach (i = 0 ... count) {
        const float val = rs_up[i] / rs_down[i];
        vout[i]         = 100.0 - 100.f / (1.0 + val);
    }
}