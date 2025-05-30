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
#ifndef RUNNER_HPP
#define RUNNER_HPP

#include "field.hpp"
#include "levels.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "utils.hpp"

#include <atomic>
#include <csignal>
#include <mutex>
#include <thread>

inline std::atomic<std::sig_atomic_t> stop_requested;

extern "C" inline void sigterm_handler(int signal) { stop_requested = signal; }

template <typename T>
void print_validation_failure(const field& f, T&& os, bool color) {
	for (auto& i : f.inputs()) {
		os << "input " << i->x << ": ";
		write_list(os, i->inputs, nullptr, color) << '\n';
	}
	for (auto& p : f.numerics()) {
		if (not p->valid()) {
			os << "validation failure for output " << p->x;
			os << "\noutput: ";
			write_list(os, p->outputs_received, &p->outputs_expected, color);
			os << "\nexpected: ";
			write_list(os, p->outputs_expected, nullptr, color);
			os << "\n";
		}
	}
	for (auto& p : f.images()) {
		if (not p->valid()) {
			os << "validation failure for output " << p->x << "\noutput: ("
			   << p->width << ',' << p->height << ")\n"
			   << p->image_received.write_text(color) //
			   << "expected:\n"
			   << p->image_expected.write_text(color);
		}
	}
}

inline score run(field& f, size_t cycles_limit, bool print_err) {
	score sc{};
	sc.instructions = f.instructions();
	sc.nodes = f.nodes_used();
	try {
		bool active;
		do {
			++sc.cycles;
			log_trace("step ", sc.cycles);
			log_trace_r([&] { return "Current state:\n" + f.state(); });
			active = f.step();
		} while (
		    active and sc.cycles < cycles_limit
		    and not stop_requested // testing the atomic sighandler last is
		                           // equivalent to relaxed memory order in my
		                           // tests, testing it sooner loses performance
		);

		sc.validated = true;
		for (auto& p : f.numerics()) {
			if (not p->valid()) {
				sc.validated = false;
				break;
			}
		}
		for (auto& p : f.images()) {
			if (not p->valid()) {
				sc.validated = false;
				break;
			}
		}

		log_flush();
		if (print_err and not sc.validated) {
			print_validation_failure(f, std::cout, color_stdout);
		}
	} catch (hcf_exception& e) {
		log_info("Test aborted by HCF (node ", e.x, ',', e.y, ':', e.line, ')');
		sc.validated = false;
	}

	return sc;
}

/// numbers in [begin, end)
struct range_t {
	std::uint32_t begin{};
	std::uint32_t end{};
};

class seed_range_iterator {
 public:
	using seed_range_t = std::span<const range_t>;
	using value_type = std::uint32_t;
	using difference_type = std::ptrdiff_t;
	using iterator_concept = std::input_iterator_tag;

	seed_range_iterator() = default;
	explicit seed_range_iterator(const seed_range_t& ranges) noexcept
	    : v_end(ranges.end())
	    , it(ranges.begin())
	    , cur(it->begin) {}

	std::uint32_t operator*() const noexcept { return cur; }
	seed_range_iterator& operator++() noexcept {
		++cur;
		if (cur == it->end) {
			++it;
			if (it != v_end) {
				cur = it->begin;
			}
		}
		return *this;
	}
	seed_range_iterator operator++(int) noexcept {
		auto tmp = *this;
		++*this;
		return tmp;
	}

	struct sentinel {};
	bool operator==(sentinel) const noexcept { return it == v_end; }

	static sentinel end() noexcept { return {}; }

 private:
	seed_range_t::iterator v_end{};
	seed_range_t::iterator it{};
	std::uint32_t cur{};
};

static_assert(std::input_iterator<seed_range_iterator>);
static_assert(
    std::sentinel_for<seed_range_iterator::sentinel, seed_range_iterator>);

struct run_params {
	std::size_t& total_cycles;
	bool& failure_printed;
	uint& count;
	uint& valid_count;
	std::size_t total_cycles_limit;
	std::size_t cycles_limit;
	uint cheating_success_threshold;
	std::uint8_t quiet;
	bool stats;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wshadow=compatible-local"
inline score run_seed_ranges(level& l, field& f,
                             const std::vector<range_t> seed_ranges,
                             run_params params, unsigned num_threads) {
	assert(not seed_ranges.empty());
	score worst{};
	seed_range_iterator seed_it(seed_ranges);
	std::mutex it_m;
	std::mutex sc_m;
	std::vector<std::thread> threads;
	std::vector<int> counters(num_threads);

	auto task = [](std::mutex& it_m, std::mutex& sc_m,
	               seed_range_iterator& seed_it, level& l, field f,
	               run_params params, score& worst, int& counter) static {
		while (true) {
			std::uint32_t seed;
			{
				std::unique_lock lock(it_m);
				if (seed_it == seed_it.end()) {
					return;
				} else {
					seed = *seed_it++;
				}
			}

			auto test = l.random_test(seed);
			if (not test) {
				continue;
			}
			++counter;
			set_expected(f, std::move(*test));
			score last = run(f, params.cycles_limit, false);
			if (stop_requested) {
				return;
			}

			// none of this is hot, so it doesn't need to be parallelized
			// so it's simplest to just hold a lock the whole time
			std::unique_lock lock(sc_m);
			++params.count;
			worst.instructions = last.instructions;
			worst.nodes = last.nodes;
			params.total_cycles += last.cycles;
			if (last.validated) {
				// for random tests, only one validation is needed
				worst.validated = true;
				worst.cycles = std::max(worst.cycles, last.cycles);
				params.valid_count++;
			} else {
				if (std::exchange(params.failure_printed, true) == false) {
					log_info("Random test failed for seed: ", seed,
					         last.cycles == params.cycles_limit ? " [timeout]" : "");
					print_validation_failure(f, log_info(), color_logs);
				} else {
					log_debug("Random test failed for seed: ", seed);
				}
			}
			if (not params.stats) {
				// at least K passes and at least one fail
				if (params.valid_count >= params.cheating_success_threshold
				    and params.valid_count < params.count) {
					return;
				}
			}
			if (params.total_cycles >= params.total_cycles_limit) {
				return;
			}
		}
	};
	if (f.inputs().empty()) {
		log_info("Secondary random tests skipped for invariant level");
		range_t r{0, 1};
		seed_range_iterator it2(std::span(&r, 1));
		task(it_m, sc_m, it2, l, std::move(f), params, worst, counters[0]);
	} else if (num_threads > 1) {
		for (auto i : range(num_threads)) {
			threads.emplace_back(task, std::ref(it_m), std::ref(sc_m),
			                     std::ref(seed_it), std::ref(l), f.clone(), params,
			                     std::ref(worst), std::ref(counters[i]));
		}

		for (auto& t : threads) {
			t.join();
		}
		if (params.total_cycles >= params.total_cycles_limit) {
			log_info("Total cycles timeout reached, stopping tests at ",
			         params.count);
		}
		for (auto [x, i] : kblib::enumerate(counters)) {
			log_info("Thread ", i, " ran ", x, " tests");
		}
	} else {
		task(it_m, sc_m, seed_it, l, std::move(f), params, worst, counters[0]);
	}

	if (stop_requested) {
		log_warn("Stop requested");
	}

	return worst;
}
#pragma GCC diagnostic pop

#endif // RUNNER_HPP
