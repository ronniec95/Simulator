#pragma once

#include <vector>

__declspec(dllexport) std::vector<int> uniform(const int a, const int b, const int seed, const int count);
__declspec(dllexport) std::vector<float> sobol_gen(const int dim_num, const int n, const int skip);
