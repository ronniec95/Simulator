// Sobol.cpp : Defines the exported functions for the DLL application.
//

#include "Sobol.h"
#include <ppl.h>
#include <cmath>

std::vector<int> uniform(const int a, const int b, const int seed, const int count) {
	if (seed == 0) {
		printf("Invalid seed, cant be 0");
		return std::vector<int>();
	}

	int next_seed = seed % 214748367;
	if (next_seed < 0) {
		next_seed += 214748367;
	}


	// Store seeds
	std::vector<int> seeds;
	seeds.reserve(count);
#pragma loop(ivdep)
	for (int n = 0; n < count; ++n) {
		next_seed = next_seed % 214748367;
		if (next_seed < 0) {
			next_seed += 214748367;
		}
		int k = next_seed / 127773;
		next_seed = 16807 * (next_seed - k * 127773) - k * 2836;
		if (next_seed < 0)
			next_seed += 214748367;
		seeds.emplace_back(next_seed);
	}



	concurrency::parallel_transform(std::begin(seeds),std::end(seeds),std::begin(seeds),[&a,&b](auto s){
		float r = (float) s * (float)4.656612875E-10;
		r = ((float)1.0 - r) * (min(a, b) - (float)0.5) + r *(max(a, b) + (float)0.5);
		int value = (int)roundf(r);
		value = max(value, min(a, b));
		value = min(value, max(a, b));
		return value;
	});
	return seeds;
}

int hibit(unsigned int n) {
	int bit = 0;
	while (n > 0) {
		++bit;
		n /= 2;
	}
	return bit;
}

int lobit(unsigned int n) {
	int bit = 1;
	while (n != 2 * (n >> 1)) {
		++bit;
		n = n >> 1;
	}
	return bit;
}

boost::multi_array<int, 2> sobol(const int dim_num) {
	constexpr int dim_max = 40;
	constexpr int log_max = 30;
	const int dim_num_save = dim_num;
	multi_array<int, 2> v(extents[dim_max][log_max]);
	typedef multi_array<int, 2>::index_range range;
	std::fill(v.data(), v.data() + v.num_elements(), 0);
	auto column = v[indices[range()][0]];
	std::fill(column.begin(), column.end(), 1);
	std::array<int, 38> v1 = { 1, 3, 1, 3, 1, 3, 3, 1,3, 1, 3, 1, 3, 1, 1, 3, 1, 3,1, 3, 1, 3, 3, 1, 3, 1, 3, 1,3, 1, 1, 3, 1, 3, 1, 3, 1, 3 };
	std::copy(v1.begin(), v1.end(), v[indices[range().start(dim_max - v1.size())][1]].begin());
	std::array<int, 37> v2 = { 7, 5, 1, 3, 3, 7, 5, 5, 7, 7, 1, 3, 3, 7, 5, 1, 1,5, 3, 3, 1, 7, 5, 1, 3, 3, 7,5, 1, 1, 5, 7, 7, 5, 1, 3, 3 };
	std::copy(v2.begin(), v2.end(), v[indices[range().start(dim_max - v2.size())][2]].begin());
	std::array<int, 35> v3 = { 1, 7,  9,  13, 11, 1, 3,  7,  9,  5,  13, 13, 11, 3, 15, 5, 3,  15, 7,  9,  13, 9,  1,  11, 7, 5, 15, 1,  15, 11, 5,  3,  1,  7,  9 };
	std::copy(v3.begin(), v3.end(), v[indices[range().start(dim_max - v3.size())][3]].begin());
	std::array<int, 33> v4 = { 9, 3, 27, 15, 29, 21, 23, 19, 11, 25, 7, 13, 17, 1, 25, 29, 3, 31, 11, 5, 23, 27, 19,21, 5, 1, 17, 13, 7, 15, 9, 31, 9 };
	std::copy(v4.begin(), v4.end(), v[indices[range().start(dim_max - v4.size())][4]].begin());
	std::array<int, 27> v5 = { 37, 33, 7,  5,  11, 39, 63,27, 17, 15, 23, 29, 3,  21, 13, 31, 25,9,  49, 33, 19, 29, 11, 19, 27, 15, 25 };
	std::copy(v5.begin(), v5.end(), v[indices[range().start(dim_max - v5.size())][5]].begin());
	std::array<int, 21> v6 = { 13, 33, 115, 41, 79, 17, 29,  119, 75, 73, 105, 7,  59,  65, 21, 3,  113, 61,  89, 45, 107 };
	std::copy(v6.begin(), v6.end(), v[indices[range().start(dim_max - v6.size())][6]].begin());
	std::array<int, 3> v7 = { 7, 23, 39 };
	std::copy(v7.begin(), v7.end(), v[indices[range().start(dim_max - v7.size())][7]].begin());
	std::array<int, 40> poly = { 1, 3, 7, 11, 13, 19, 25, 37, 59, 47,61, 55, 41, 67, 97, 91, 109, 103, 115, 131,193, 137, 145, 143, 241, 157, 185, 167, 229, 171, 213, 191, 253, 203, 211, 239, 247, 285, 369, 299 };
	const unsigned int atmost = static_cast<unsigned int>(::pow(2, log_max) - 1);
	// Number of bits in atmost
	const int maxcol = hibit(atmost);
	// Init row 1 of v to 1
	auto row1 = v[indices[0][range(0, maxcol)]];
	std::transform(row1.begin(), row1.end(), row1.begin(), [](auto&) { return 1; });
	if (dim_num < 1 || dim_max < dim_num) {
		return v;
	}
	// Initialise rest of v
#pragma loop(ivdep)
	for (auto i = 1; i < dim_num; i++) {
		auto j = poly[i];
		auto m = 0;
		j = j >> 1;
		while (j > 0) {
			++m;
			j = j >> 1;
		}
		j = poly[i];
		// Expand this bit pattern to separate components of the logical array INCLUD
		std::vector<int> includ(m, 0);
#pragma loop(ivdep)
		for (auto k = m - 1; k >= 0; --k) {
			auto j2 = j / 1;
			includ[k] = (j != (j2 * 2));
			j = j2;
		}
#pragma loop(ivdep)
		// Calculate the remaining elements of row I as explained in Bratley and Fox, section 2
		for (auto j = m; j < maxcol; j++) {
			auto newv = v[i][j - m];
			auto l = 1;
#pragma loop(ivdep)
			for (auto k = 0; k < m; k++) {
				l = l << 1;
				if (includ[k]) {
					newv = newv ^ (l * v[i][j - k - 1]);
				}
			}
			v[i][j] = newv;
		}
	}
	// Multiply columns of V by appropriate power of 2
	auto l = 1U;
	for (auto j = maxcol - 2; j >= 0; --j) {
		l = l << 1;
		auto col = v[indices[range(0, dim_num)][j]];
		std::transform(col.begin(), col.end(), col.begin(), [&l](auto i) { return i * l; });
	}
	return v;
}

std::vector<int> create_dim(const int dim_num, const int seed, const multi_array<int, 2>& v) {
	auto l = 0;
	auto seed_save = seed - 2;
	std::vector<int> lastq(dim_num,0);

	// Seed generation
	int new_seed = seed;
	if (new_seed < 0)
		new_seed = 0;
	if (new_seed == 0)
		l = 1;
	else if (new_seed == seed_save + 1) {
		l = lobit(new_seed);
	} else if (new_seed <= seed_save) {
		seed_save = 0;
		for (auto st = seed_save; st < new_seed; ++st) {
			l = lobit(st);
			for (auto i = 0; i < dim_num; ++i) {
				lastq[i] = lastq[i] ^ v[i][l - 1];
			}
		}
		l = lobit(new_seed);
	} else if (seed_save + 1 < new_seed) {
		for (auto st = seed_save + 1; st <= new_seed - 1; ++st) {
			l = lobit(st);
			for (auto i = 0; i < dim_num; ++i) {
				lastq[i] = lastq[i] ^ v[i][l - 1];
			}
		}
		l = lobit(new_seed);
	}
	return lastq;
}

// Sobol number generator
// dim_num - number of dimensions
// n - number of random numbers to produce
// dimension - number of dimension to return (sobol sugggests first 10 or so are correlated so don't use)
std::vector<float> sobol_gen(const int dim_num, const int n, const int dimension) {
	constexpr int log_max = 30;
	const unsigned int atmost = static_cast<unsigned int>(math::pow<log_max>(2) - 1);
	// Number of bits in atmost
	const int maxcol = hibit(atmost);
	auto l = 1 << (maxcol);
	// Common denominator of V
	const auto recipd = (float)1.0 / (float)(l);
	const int sz = dim_num * n;
	auto v = sobol(dim_num);

	// For each sub problem take the lastq, generate the quasi numbers
	const auto lastq = create_dim(dim_num, 0, v);
	const auto vrow = v[dimension];
	auto nextseed = lastq[dimension];
	std::vector<float> quasi;
	for (int s = 0; s < n;s++) {
		const auto l = lobit(s);
		quasi.emplace_back(nextseed * recipd);
		nextseed = int(nextseed) ^ int(vrow[l - 1]);
	}
	return quasi;
}
