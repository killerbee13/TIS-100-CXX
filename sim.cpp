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

#include "sim.hpp"
#include "field.hpp"
#include "levels.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "tests.hpp"
#include "tis100.h"
#include "utils.hpp"

#include <kblib/io.h>

#include <mutex>
#include <ranges>
#include <string>
#include <thread>

/// Configure the field with a test case, takes ownership of the test content
static void set_expected(field& f, single_test&& expected) {
	for (auto& i : f.regulars()) {
		auto p = i.get();
		p->reset();
		log_debug("reset node (", p->x, ',', p->y, ')');
	}
	using std::views::zip;
	for (const auto& [n, i] : zip(f.inputs(), expected.inputs)) {
		log_debug("reset input I", n->x);
		n->reset(std::move(i));
		auto debug = log_debug();
		debug << "set expected input I" << n->x << ":";
		write_list(debug, n->inputs);
	}
	for (const auto& [n, o] : zip(f.numerics(), expected.n_outputs)) {
		log_debug("reset output O", n->x);
		n->reset(std::move(o));
		auto debug = log_debug();
		debug << "set expected output O" << n->x << ":";
		write_list(debug, n->outputs_expected);
	}
	for (const auto& [n, i] : zip(f.images(), expected.i_outputs)) {
		log_debug("reset image O", n->x);
		n->reset(std::move(i));
		auto debug = log_debug();
		debug << "set expected image O" << n->x << ": {\n";
		debug.log_r([&] { return n->image_expected.write_text(color_logs); });
		debug << '}';
	}
}

static score run(field& f, size_t cycles_limit,
                 std::string* error_message = nullptr) {
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
	} catch (const hcf_exception& e) {
		log_info("Test aborted by HCF (node ", e.x, ',', e.y, ':', e.line, ')');
		sc.validated = false;
	}

	if (error_message and not sc.validated) {
		auto ss = std::ostringstream{};
		f.print_failed_test(ss, color_stdout);
		*error_message = std::move(ss).str();
	}
	return sc;
}

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wshadow=compatible-local"
score tis_sim::run_seed_ranges(field f) {
	assert(not seed_ranges.empty());
	score worst{};
	seed_range_iterator seed_it(seed_ranges);
	std::mutex it_m;
	std::mutex sc_m;
	bool failure_printed{};
	std::vector<uint> counters(num_threads);

	auto task = [](std::mutex& it_m, std::mutex& sc_m,
	               seed_range_iterator& seed_it, level& l, field f, tis_sim& sim,
	               score& worst, bool& failure_printed, uint& counter) static {
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
			score last = run(f, sim.random_cycles_limit);
			if (stop_requested) {
				return;
			}

			// none of this is hot, so it doesn't need to be parallelized
			// so it's simplest to just hold a lock the whole time
			std::unique_lock lock(sc_m);
			worst.random_test_ran++;
			worst.instructions = last.instructions;
			worst.nodes = last.nodes;
			sim.total_cycles += last.cycles;
			if (last.validated) {
				// for random tests, only one validation is needed
				worst.validated = true;
				worst.cycles = std::max(worst.cycles, last.cycles);
				worst.random_test_valid++;
			} else {
				std::string message = concat(
				    "Random test failed for seed: ", seed,
				    last.cycles == sim.random_cycles_limit ? " [timeout]" : "");
				if (std::exchange(failure_printed, true) == false) {
					log_info(message);
					f.print_failed_test(log_info(), color_logs);
				} else {
					log_debug(message);
				}
			}
			if (auto sig = info_requested.exchange(0)) {
				log_info("Random test progress: ", worst.random_test_valid,
				         " passed out of ", worst.random_test_ran, " total [sig ",
				         sig, "]");
#ifndef _WIN32
				if (sig == SIGTSTP) {
					raise(SIGSTOP);
				}
#endif
			}
			if (not sim.compute_stats) {
				// at least K passes and at least one fail
				if (worst.random_test_valid
				        >= sim.cheat_rate * sim.total_random_tests
				    and worst.random_test_valid < worst.random_test_ran) {
					return;
				}
			}
			if (sim.total_cycles >= sim.total_cycles_limit) {
				return;
			}
		}
	};
	if (f.inputs().empty()) {
		log_info("Secondary random tests skipped for invariant level");
		range_t r{0, 1};
		seed_range_iterator it2(std::span(&r, 1));
		task(it_m, sc_m, it2, *target_level, std::move(f), *this, worst,
		     failure_printed, counters[0]);
	} else if (num_threads > 1) {
		std::vector<std::thread> threads;
		// Using a separate vector avoids having the threads take ownership of the
		// levels, because that introduces an unnecessary clone() call in the
		// single-threaded cases
		std::vector<std::unique_ptr<level>> levels;
		for (auto i : range(num_threads)) {
			auto& l = levels.emplace_back(target_level->clone());
			threads.emplace_back(task, std::ref(it_m), std::ref(sc_m),
			                     std::ref(seed_it), std::ref(*l), f.clone(),
			                     std::ref(*this), std::ref(worst),
			                     std::ref(failure_printed), std::ref(counters[i]));
		}

		for (auto& t : threads) {
			t.join();
		}
		if (total_cycles >= total_cycles_limit) {
			log_info("Total cycles timeout reached, stopping tests at ",
			         worst.random_test_ran);
		}
		for (auto [x, i] : kblib::enumerate(counters)) {
			log_info("Thread ", i, " ran ", x, " tests");
		}
	} else {
		task(it_m, sc_m, seed_it, *target_level, std::move(f), *this, worst,
		     failure_printed, counters[0]);
	}

	if (stop_requested) {
		log_warn("Stop requested");
	}

	return worst;
}
#pragma GCC diagnostic pop

/// @param solution can be a file path or "-" for stdin
const score& tis_sim::simulate_file(const std::string& solution) {
	bool deduced;
	if (target_level) {
		deduced = false;
	} else {
		auto filename = std::filesystem::path(solution).filename().string();
		for (auto& l : builtin_levels) {
			if (filename.starts_with(l.segment)) {
				log_debug("Deduced level ", l.segment, " from filename ",
				          kblib::quoted(filename));
				target_level = std::make_unique<builtin_level>(l);
				break;
			}
		}
		if (not target_level) {
			throw std::invalid_argument{
			    concat("Impossible to determine the level for ",
			           kblib::quoted(filename))};
		}
		deduced = true;
	}

	std::string code;
	if (solution == "-") {
		std::ostringstream in;
		in << std::cin.rdbuf();
		code = std::move(in).str();
	} else if (std::filesystem::is_regular_file(solution)) {
		code = kblib::try_get_file_contents(solution, std::ios::in);
	} else {
		throw std::invalid_argument{
		    concat("invalid file: ", kblib::quoted(solution))};
	}

	simulate_code(code);
	if (deduced) {
		target_level.reset();
	}
	return sc;
}

const score& tis_sim::simulate_code(std::string_view code) {
	sc = score{};
	error_message.clear();
	total_cycles = 0;
	random_cycles_limit = cycles_limit;

	if (not target_level) {
		throw std::logic_error("No target level set");
	}
	field f = target_level->new_field(T30_size);
	f.parse_code(code, T21_size);
	log_debug_r([&] { return "Layout:\n" + f.layout(); });

	if (run_fixed) {
		sc.validated = true;
		for (uint id = 0; id < 3; ++id) {
			set_expected(f, target_level->static_test(id));
			score last = run(f, cycles_limit, &error_message);
			sc.instructions = last.instructions;
			sc.nodes = last.nodes;
			total_cycles += last.cycles;
			log_info("fixed test ", id + 1, ' ',
			         last.validated ? "validated"sv : "failed"sv, " in ",
			         last.cycles, " cycles");
			if (last.validated) {
				sc.cycles = std::max(sc.cycles, last.cycles);
			} else {
				sc.validated = false;
				append(error_message, "for fixed test ", id + 1, //
				       " after ", last.cycles, " cycles");
				if (last.cycles == cycles_limit) {
					error_message += " [timeout]";
				}
				error_message += '\n';
				break;
			}
			// optimization: skip running the 2nd and 3rd rounds for invariant
			// levels (specifically, the image test patterns)
			if (f.inputs().empty()) {
				log_info("Secondary tests skipped for invariant level");
				break;
			}
			if (stop_requested) {
				log_notice("Stop requested");
				break;
			}
		}
		sc.achievement = sc.validated and target_level->has_achievement(f, sc);
	}

	if ((sc.validated or not run_fixed or compute_stats) and not stop_requested
	    and not seed_ranges.empty()) {
		if (sc.validated) {
			auto effective_limit = static_cast<size_t>(
			    static_cast<double>(sc.cycles) * limit_multiplier);
			random_cycles_limit = std::min(cycles_limit, effective_limit);
			log_info("Setting random test timeout to ", random_cycles_limit);
		}
		auto worst = run_seed_ranges(std::move(f));

		if (not run_fixed) {
			sc = std::move(worst);
		} else {
			sc.random_test_ran = worst.random_test_ran;
			sc.random_test_valid = worst.random_test_valid;
		}
		sc.cheat = (sc.random_test_ran == 0
		            or sc.random_test_ran != sc.random_test_valid);
		sc.hardcoded = (sc.random_test_valid
		                <= static_cast<uint>(sc.random_test_ran * cheat_rate));

		log_info("Random test results: ", sc.random_test_valid, " passed out of ",
		         sc.random_test_ran, " total");
	}
	return sc;
}
