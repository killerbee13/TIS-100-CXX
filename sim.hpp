/* *****************************************************************************
 * TIS-100-CXX
 * Copyright (c) 2024 killerbee, Andrea Stacchiotti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ****************************************************************************/
#ifndef SIM_HPP
#define SIM_HPP

#include "levels.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "utils.hpp"

#include <atomic>
#include <csignal>
#include <thread>

inline std::atomic<std::sig_atomic_t> stop_requested;

extern "C" inline void sigterm_handler(int signal) { stop_requested = signal; }

/// numbers in [begin, end)
struct range_t {
	std::uint32_t begin{};
	std::uint32_t end{};
};

namespace defaults {
constexpr inline size_t cycles_limit = 100'500;
constexpr inline size_t total_cycles_limit = kblib::max;
constexpr inline bool run_fixed = true;
constexpr inline uint num_threads = 1;
constexpr inline double cheat_rate = 0.05;
constexpr inline double limit_multiplier = 5.0;
} // namespace defaults

/// Main simulator class
class tis_sim {
 private:
	// config
	std::vector<range_t> seed_ranges;
	std::unique_ptr<level> global_level;
	size_t cycles_limit = defaults::cycles_limit;
	size_t total_cycles_limit = defaults::total_cycles_limit;
	uint total_random_tests{};
	uint num_threads = defaults::num_threads;
	double cheat_rate = defaults::cheat_rate;
	double limit_multiplier = defaults::limit_multiplier;
	uint T21_size = defaults::T21_size;
	uint T30_size = defaults::T30_size;
	bool run_fixed = defaults::run_fixed;
	bool compute_stats = false;

 public:
	// runtime
	size_t total_cycles{};
	size_t random_cycles_limit{};
	uint random_test_ran{};
	uint random_test_valid{};
	uint failed_test{};

 public:
	void set_seed_ranges(std::vector<range_t> seed_ranges_) {
		seed_ranges = std::move(seed_ranges_);
		auto debug = log_debug();
		debug << "Seed ranges parsed: {\n";
		for (auto r : seed_ranges) {
			debug << r.begin << ".." << r.end - 1 << " [" << r.end - r.begin
			      << "]; ";
			total_random_tests += r.end - r.begin;
		}
		debug << "\n} sum: " << total_random_tests << " tests";
	}

	void set_builtin_level_name(const std::string& builtin_level_name) {
		global_level
		    = std::make_unique<builtin_level>(find_level_id(builtin_level_name));
	}
#if TIS_ENABLE_LUA
	void set_custom_spec_path(const std::string& custom_spec_path) {
		global_level = std::make_unique<custom_level>(custom_spec_path);
	}
#endif

	void set_num_threads(uint num_threads_) {
		if (num_threads_ == 0) {
			num_threads_ = std::thread::hardware_concurrency();
		}
		log_info("Using ", num_threads_, " threads");
		num_threads = num_threads_;
	}

	void set_cycles_limit(size_t l) { cycles_limit = l; }
	void set_total_cycles_limit(size_t l) { total_cycles_limit = l; }
	void set_cheat_rate(double cheat_rate_) { cheat_rate = cheat_rate_; }
	void set_limit_multiplier(double v) { limit_multiplier = v; }
	void set_T21_size(uint size_) { T21_size = size_; }
	void set_T30_size(uint size_) { T30_size = size_; }
	void set_run_fixed(bool v) { run_fixed = v; }
	void set_compute_stats(bool v) { compute_stats = v; }

	score simulate(const std::string& solution);

 private:
	score run_seed_ranges(level& l, field f);
};

#endif // SIM_HPP
