/* *****************************************************************************
 * TIS-100-CXX
 * Copyright (c) 2025 killerbee, Andrea Stacchiotti
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

#include "game.hpp"
#include "levels.hpp"
#include "logger.hpp"
#include "tis100.h"
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

/// Main simulator class
class tis_sim {
 private:
	// config
	std::vector<range_t> seed_ranges;
	std::unique_ptr<level> target_level;
	size_t cycles_limit = defaults::cycles_limit;
	size_t total_cycles_limit = defaults::total_cycles_limit;
	double cheat_rate = defaults::cheat_rate;
	double limit_multiplier = defaults::limit_multiplier;
	uint num_threads = defaults::num_threads;
	uint T21_size = defaults::T21_size;
	uint T30_size = defaults::T30_size;
	bool run_fixed = defaults::run_fixed;
	bool compute_stats = false;

 public:
	// runtime
	score sc;
	std::string error_message;
	size_t total_cycles{};
	size_t random_cycles_limit{};
	uint total_random_tests{};

 public:
	/// Adds a seed range [begin, end)
	void add_seed_range(uint32_t begin, uint32_t end) {
		seed_ranges.push_back({begin, end});
		total_random_tests += end - begin;
		log_debug("seeds: ", begin, "..", end - 1, " [", end - begin, "]");
	}

	void set_builtin_level_name(std::string_view builtin_level_name) {
		target_level
		    = std::make_unique<builtin_level>(find_level_id(builtin_level_name));
	}
#if TIS_ENABLE_LUA
	void set_custom_spec_path(const std::string& custom_spec_path) {
		target_level = std::make_unique<custom_level>(custom_spec_path);
	}
	void set_custom_spec_code(const std::string_view custom_spec_code,
	                          std::uint32_t base_seed) {
		target_level
		    = std::make_unique<custom_level>(custom_spec_code, base_seed);
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

	const score& simulate_code(std::string_view code);
	const score& simulate_file(const std::string& solution);

 private:
	score run_seed_ranges(level& l, field f);
};

#endif // SIM_HPP
