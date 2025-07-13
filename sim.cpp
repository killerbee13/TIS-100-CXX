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

#include "sim.hpp"
#include "field.hpp"
#include "levels.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "utils.hpp"

#include <kblib/io.h>

#include <mutex>
#include <string>
#include <thread>

template <typename T>
static void print_failed_test(const field& f, T&& os, bool color) {
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

static score run(field& f, size_t cycles_limit, bool print_err) {
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

	if (print_err and not sc.validated) {
		log_flush();
		print_failed_test(f, std::cout, color_stdout);
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
	    : v_end(ranges.cend())
	    , it(ranges.cbegin())
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
	seed_range_t::const_iterator v_end{};
	seed_range_t::const_iterator it{};
	std::uint32_t cur{};
};

static_assert(std::input_iterator<seed_range_iterator>);
static_assert(
    std::sentinel_for<seed_range_iterator::sentinel, seed_range_iterator>);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wshadow=compatible-local"
score tis_sim::run_seed_ranges(level& l, field f) {
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
			score last = run(f, sim.random_cycles_limit, false);
			if (stop_requested) {
				return;
			}

			// none of this is hot, so it doesn't need to be parallelized
			// so it's simplest to just hold a lock the whole time
			std::unique_lock lock(sc_m);
			++sim.random_test_ran;
			worst.instructions = last.instructions;
			worst.nodes = last.nodes;
			sim.total_cycles += last.cycles;
			if (last.validated) {
				// for random tests, only one validation is needed
				worst.validated = true;
				worst.cycles = std::max(worst.cycles, last.cycles);
				sim.random_test_valid++;
			} else {
				std::string message = concat(
				    "Random test failed for seed: ", seed,
				    last.cycles == sim.random_cycles_limit ? " [timeout]" : "");
				if (std::exchange(failure_printed, true) == false) {
					log_info(message);
					print_failed_test(f, log_info(), color_logs);
				} else {
					log_debug(message);
				}
			}
			if (not sim.compute_stats) {
				// at least K passes and at least one fail
				if (sim.random_test_valid >= sim.cheat_rate * sim.total_random_tests
				    and sim.random_test_valid < sim.random_test_ran) {
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
		task(it_m, sc_m, it2, l, std::move(f), *this, worst, failure_printed,
		     counters[0]);
	} else if (num_threads > 1) {
		std::vector<std::thread> threads;
		for (auto i : range(num_threads)) {
			threads.emplace_back(task, std::ref(it_m), std::ref(sc_m),
			                     std::ref(seed_it), std::ref(l), f.clone(),
			                     std::ref(*this), std::ref(worst),
			                     std::ref(failure_printed), std::ref(counters[i]));
		}

		for (auto& t : threads) {
			t.join();
		}
		if (total_cycles >= total_cycles_limit) {
			log_info("Total cycles timeout reached, stopping tests at ",
			         random_test_ran);
		}
		for (auto [x, i] : kblib::enumerate(counters)) {
			log_info("Thread ", i, " ran ", x, " tests");
		}
	} else {
		task(it_m, sc_m, seed_it, l, std::move(f), *this, worst, failure_printed,
		     counters[0]);
	}

	if (stop_requested) {
		log_warn("Stop requested");
	}

	return worst;
}
#pragma GCC diagnostic pop

score tis_sim::simulate(const std::string& solution) {
	total_cycles = 0;
	random_cycles_limit = cycles_limit;
	random_test_ran = 0;
	random_test_valid = 0;
	failed_test = 0;

	level* l;
	std::unique_ptr<level> level_from_name;
	if (global_level) {
		l = global_level.get();
	} else if (auto filename
	           = std::filesystem::path(solution).filename().string();
	           auto maybe_id = guess_level_id(filename)) {
		level_from_name = std::make_unique<builtin_level>(*maybe_id);
		l = level_from_name.get();
		log_debug("Deduced level ", builtin_layouts[*maybe_id].segment,
		          " from filename ", kblib::quoted(filename));
	} else {
		throw std::invalid_argument{
		    concat("Impossible to determine the level ID for ",
		           kblib::quoted(filename))};
	}
	field f = l->new_field(T30_size);

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

	parse_code(f, code, T21_size);
	log_debug_r([&] { return "Layout:\n" + f.layout(); });

	score sc{};
	if (run_fixed) {
		sc.validated = true;
		for (uint id = 0; id < 3; ++id) {
			set_expected(f, l->static_test(id));
			score last = run(f, cycles_limit, true);
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
				// store the cycles of the failed test to print them later
				sc.cycles = last.cycles;
				failed_test = id + 1;
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
		sc.achievement = sc.validated and l->has_achievement(f, sc);
	}

	if ((sc.validated or not run_fixed or compute_stats) and not stop_requested
	    and not seed_ranges.empty()) {
		if (sc.validated) {
			auto effective_limit = static_cast<size_t>(
			    static_cast<double>(sc.cycles) * limit_multiplier);
			random_cycles_limit = std::min(cycles_limit, effective_limit);
			log_info("Setting random test timeout to ", random_cycles_limit);
		}
		auto worst = run_seed_ranges(*l, std::move(f));

		log_info("Random test results: ", random_test_valid, " passed out of ",
		         random_test_ran, " total");

		if (not run_fixed) {
			sc = std::move(worst);
		}
		sc.cheat = (random_test_ran == 0 or random_test_ran != random_test_valid);
		sc.hardcoded = (random_test_valid
		                <= static_cast<uint>(random_test_ran * cheat_rate));
	}
	return sc;
}
