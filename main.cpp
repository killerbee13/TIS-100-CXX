/* *****************************************************************************
 * TIX-100-CXX
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

#include "builtin_levels.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "random_levels.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <kblib/hash.h>
#include <kblib/io.h>
#include <kblib/stringops.h>
#include <mutex>
#include <random>
#include <thread>

#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>

#include <cxxabi.h>
#include <typeinfo>

using namespace std::literals;

std::atomic<std::sig_atomic_t> stop_requested;

extern "C" void sigterm_handler(int signal) { stop_requested = signal; }

template <typename T>
void print_validation_failure(field& l, T&& os, bool color) {
	for (auto it = l.begin_io(); it != l.end(); ++it) {
		auto n = it->get();
		if (n->type() == node::in) {
			auto p = static_cast<input_node*>(n);
			os << "input " << p->x << ": ";
			write_list(os, p->inputs, nullptr, color) << '\n';
		} else if (n->type() == node::out) {
			auto p = static_cast<output_node*>(n);
			if (p->outputs_expected != p->outputs_received) {
				os << "validation failure for output " << p->x;
				os << "\noutput: ";
				write_list(os, p->outputs_received, &p->outputs_expected, color);
				os << "\nexpected: ";
				write_list(os, p->outputs_expected, nullptr, color);
				os << "\n";
			}
		} else if (n->type() == node::image) {
			auto p = static_cast<image_output*>(n);
			if (p->wrong_pixels) {
				os << "validation failure for output " << p->x << "\noutput:\n"
				   << p->image_received.write_text(color) //
				   << "expected:\n"
				   << p->image_expected.write_text(color);
			}
		}
	}
}

score run(field& l, int cycles_limit, bool print_err) {
	score sc{};
	sc.instructions = l.instructions();
	sc.nodes = l.nodes_used();
	try {
		do {
			++sc.cycles;
			log_trace("step ", sc.cycles);
			log_trace_r([&] { return "Current state:\n" + l.state(); });
			l.step();
		} while (stop_requested == 0 and l.active()
		         and std::cmp_less(sc.cycles, cycles_limit));
		sc.validated = true;

		log_flush();
		for (auto it = l.begin_output(); it != l.end(); ++it) {
			auto n = it->get();
			if (n->type() == node::out) {
				auto p = static_cast<output_node*>(n);
				if (p->wrong || ! p->complete) {
					sc.validated = false;
					break;
				}
			} else if (n->type() == node::image) {
				auto p = static_cast<image_output*>(n);
				if (p->wrong_pixels) {
					sc.validated = false;
					break;
				}
			}
		}

		if (print_err and not sc.validated) {
			print_validation_failure(l, std::cout, color_stdout);
		}
	} catch (hcf_exception& e) {
		log_info("Test aborted by HCF (node ", e.x, ',', e.y, ':', e.line, ')');
		sc.validated = false;
	}

	return sc;
}

template <typename T>
class range_constraint : public TCLAP::Constraint<T> {
 public:
	range_constraint(T low, T high)
	    : low(low)
	    , high(high) {
		if (low > high) {
			throw std::invalid_argument{""};
		}
	}
	bool check(const T& value) const override {
		return value >= low and value <= high;
	}

	std::string description() const override {
		return concat('[', low, '-', high, ']');
	}

	std::string shortID() const override { return description(); }

 private:
	T low{};
	T high{};
};

struct range_t {
	std::uint32_t begin{};
	std::uint32_t size{};
};

std::vector<range_t> parse_ranges(const std::vector<std::string>& seed_exprs) {
	std::vector<range_t> seed_ranges;
	for (auto& ex : seed_exprs) {
		for (auto& r : kblib::split_dsv(ex, ',')) {
			std::string begin;
			std::optional<std::string> end;
			std::size_t i{};
			for (; i != r.size(); ++i) {
				if (r[i] == '.') {
					if (i + 1 < r.size() and r[i + 1] == '.') {
						end.emplace();
						i += 2;
						break;
					} else {
						throw std::invalid_argument{
						    "Decimals not allowed in seed exprs"};
					}
				} else if (r[i] >= '0' and r[i] <= '9') {
					begin.push_back(r[i]);
				} else {
					throw std::invalid_argument{kblib::concat("Invalid character ",
					                                          kblib::escapify(r[i]),
					                                          " in seed expr")};
				}
			}
			auto b = kblib::parse_integer<std::uint32_t>(begin);
			if (not end) {
				seed_ranges.push_back({b, 1});
				continue;
			}
			for (; i != r.size(); ++i) {
				if (r[i] >= '0' and r[i] <= '9') {
					end->push_back(r[i]);
				} else {
					throw std::invalid_argument{kblib::concat("Invalid character ",
					                                          kblib::escapify(r[i]),
					                                          " in seed expr")};
				}
			}
			auto e = end->empty() ? kblib::max
			                      : kblib::parse_integer<std::uint32_t>(*end);
			if (e < b) {
				throw std::invalid_argument{kblib::concat(
				    "Seed ranges must be low..high, got: ", b, "..", e)};
			}
			seed_ranges.push_back({b, e - b + 1});
		}
	}
	return seed_ranges;
}

class seed_range_iterator {
 public:
	using seed_range_t = std::vector<range_t>;
	using value_type = std::uint32_t;
	using difference_type = std::ptrdiff_t;
	using iterator_concept = std::input_iterator_tag;

	seed_range_iterator() = default;
	explicit seed_range_iterator(const std::vector<range_t>& ranges) noexcept
	    : v(&ranges)
	    , it(ranges.begin())
	    , cur(it->begin) {}

	std::uint32_t operator*() const noexcept { return cur; }
	seed_range_iterator& operator++() noexcept {
		++cur;
		if (cur == it->begin + it->size) {
			++it;
			if (it != v->end()) {
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
	bool operator==(sentinel) const noexcept {
		return (not v) or it == v->end();
	}

	static sentinel end() noexcept { return {}; }

 private:
	const std::vector<range_t>* v{};
	seed_range_t::const_iterator it{};
	std::uint32_t cur{};
};

static_assert(std::input_iterator<seed_range_iterator>);
static_assert(
    std::sentinel_for<seed_range_iterator::sentinel, seed_range_iterator>);

void score_summary(score sc, int fixed, int quiet, int cycles_limit) {
	// we use stdout here, so flush logs to avoid mangled messages in the shell
	std::clog << std::flush;
	if (sc.validated) {
		if (not quiet) {
			std::cout << print_escape(bright_blue, bold) << "validation successful"
			          << print_escape(none) << "\n";
		}
	} else if (quiet < 2) {
		std::cout << print_escape(red) << "validation failed"
		          << print_escape(none);
		if (fixed != -1) {
			std::cout << " for fixed test " << fixed;
		}
		std::cout << " after " << sc.cycles << " cycles ";
		if (std::cmp_equal(sc.cycles, cycles_limit)) {
			std::cout << "[timeout]";
		}
		std::cout << '\n';
	}
}

struct run_params {
	std::size_t& total_cycles;
	bool& failure_printed;
	int& count;
	int& valid_count;
	std::size_t total_cycles_limit;
	int cycles_limit;
	std::uint8_t quiet;
	bool stats;
	double cheat_rate;
	std::uint32_t total_random_tests;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow=compatible-local"
score run_seed_ranges(field& f, uint level_id,
                      const std::vector<range_t> seed_ranges, run_params params,
                      unsigned num_threads) {
	score worst{};
	seed_range_iterator seed_it(seed_ranges);
	std::mutex it_m;
	std::mutex sc_m;
	std::vector<std::thread> threads;
	std::vector<int> counters(num_threads);

	auto task = [](std::mutex& it_m, std::mutex& sc_m,
	               seed_range_iterator& seed_it, field f, uint level_id,
	               run_params params, score& worst, int& counter) static {
		while (true) {
			std::uint32_t seed;
			score last{};
			{
				std::unique_lock l(it_m);
				if (seed_it == seed_it.end()) {
					return;
				} else {
					seed = *seed_it++;
				}
			}

			auto test = random_test(level_id, seed);
			if (not test) {
				continue;
			}
			++counter;
			set_expected(f, *test);
			if (stop_requested) {
				log_notice("Stop requested");
				return;
			}
			last = run(f, params.cycles_limit, false);

			// none of this is hot, so it doesn't need to be parallelized
			// so it's simplest to just hold a lock the whole time
			std::unique_lock l(sc_m);
			++params.count;
			worst.cycles = std::max(worst.cycles, last.cycles);
			worst.instructions = last.instructions;
			worst.nodes = last.nodes;
			// for random tests, only one validation is needed
			worst.validated = worst.validated or last.validated;
			params.valid_count += last.validated ? 1 : 0;
			params.total_cycles += last.cycles;
			if (not last.validated) {
				if (std::exchange(params.failure_printed, true) == false) {
					log_info("Random test failed for seed: ", seed,
					         std::cmp_equal(last.cycles, params.cycles_limit)
					             ? "[timeout]"
					             : "");
					print_validation_failure(f, log_info(), color_logs);
				} else {
					log_debug("Random test failed for seed: ", seed);
				}
			}
			if (not params.stats) {
				// at least K passes and at least one fail
				if (params.valid_count >= static_cast<int>(
				        params.cheat_rate * params.total_random_tests)
				    and params.valid_count < params.count) {
					return;
				}
			}
			if (not f.has_inputs()) {
				log_info("Secondary random tests skipped for invariant level");
				return;
			}
			if (params.total_cycles >= params.total_cycles_limit) {
				log_info("Total cycles timeout reached, stopping tests at ",
				         params.count);
				return;
			}
		}
	};

	if (num_threads > 1) {
		for (auto i : range(num_threads)) {
			threads.emplace_back(task, std::ref(it_m), std::ref(sc_m),
			                     std::ref(seed_it), f.clone(), level_id, params,
			                     std::ref(worst), std::ref(counters[i]));
		}

		for (auto& t : threads) {
			t.join();
		}
		for (auto [x, i] : kblib::enumerate(counters)) {
			log_info("Thread ", i, " ran ", x, " tests");
		}
	} else {
		task(it_m, sc_m, seed_it, std::move(f), level_id, params, worst,
		     counters[0]);
	}

	return worst;
}
#pragma GCC diagnostic pop

int main(int argc, char** argv) try {
	std::ios_base::sync_with_stdio(false);
	set_log_flush(not RELEASE);

	TCLAP::CmdLine cmd("TIS-100 simulator and validator");

	std::vector<std::string> ids_v;
	for (auto l : layouts) {
		ids_v.emplace_back(l.segment);
		ids_v.emplace_back(l.name);
	}

	TCLAP::UnlabeledValueArg<std::string> solution(
	    "Solution", "Path to solution file. ('-' for stdin)", true, "-", "path",
	    cmd);
	TCLAP::ValuesConstraint<std::string> ids_c(ids_v);
	TCLAP::UnlabeledValueArg<std::string> id_arg(
	    "ID", "Level ID (Segment or name).", false, "", &ids_c);

	range_constraint<int> level_nums_constraint(0, layouts.size() - 1);
	// unused because the test case parser is not implemented
	TCLAP::ValueArg<std::string> layout_s("l", "layout", "Layout string", false,
	                                      "", "layout");
	TCLAP::ValueArg<int> level_num("L", "level", "Numeric level ID", false, 0,
	                               &level_nums_constraint);
	TCLAP::EitherOf layout(cmd);
	layout.add(id_arg).add(level_num); //.add(layout_s);

	TCLAP::ValueArg<int> cycles_limit(
	    "", "limit",
	    "Number of cycles to run test for before timeout. (Default 100500)",
	    false, 100'500, "integer", cmd);
	TCLAP::ValueArg<std::size_t> total_cycles_limit(
	    "", "total-limit",
	    "Max number of cycles to run between all tests before determining "
	    "cheating status. (Default no limit)",
	    false, kblib::max, "integer", cmd);
	TCLAP::ValueArg<unsigned> threads(
	    "j", "threads",
	    "Number of threads to use. Log level must be info or lower.", false, 1,
	    "integer", cmd);

	TCLAP::ValueArg<bool> fixed("", "fixed", "Run fixed tests. (Default 1)",
	                            false, true, "[0,1]", cmd);
	// unused because the test case parser is not implemented
	TCLAP::ValueArg<std::string> set_test("t", "test", "Manually set test cases",
	                                      false, "", "test case");
	TCLAP::ValueArg<std::uint32_t> random(
	    "r", "random", "Random tests to run (upper bound)", false, 0, "integer");
	TCLAP::ValueArg<std::uint32_t> seed_arg(
	    "", "seed", "Seed to use for random tests", false, 0, "uint32_t");
	TCLAP::SwitchArg stats(
	    "S", "stats", "Run all random tests requested and calculate pass rate",
	    cmd);
	TCLAP::MultiArg<std::string> seed_exprs("", "seeds",
	                                        "A range of seed values to use",
	                                        false, "[range-expr...]", cmd);
	TCLAP::AnyOf implicit_random(cmd);
	implicit_random.add(random).add(seed_arg);
	range_constraint percentage(0.0, 1.0);
	TCLAP::ValueArg<double> cheat_rate(
	    "", "cheat-rate",
	    "Threshold between /c and /h solutions, "
	    "as a fraction of total random tests. (Default 0.05)",
	    false, 0.05, &percentage, cmd);

	// Size constraint of word_max guarantees that JRO can reach every
	// instruction
	range_constraint<unsigned> size_constraint(0, word_max);
	TCLAP::ValueArg<unsigned> T21_size(
	    "", "T21_size",
	    concat("Number of instructions allowed per T21 node. (Default ",
	           def_T21_size, ")"),
	    false, def_T21_size, &size_constraint, cmd);
	// No technical reason to limit T30 capacity, as they are not addressable
	TCLAP::ValueArg<unsigned> T30_size(
	    "", "T30_size",
	    concat("Memory capacity of T30 nodes. (Default ", def_T30_size, ")"),
	    false, def_T30_size, "integer", cmd);

	std::vector<std::string> loglevels_allowed{
	    "none", "err", "error", "warn", "notice", "info", "trace", "debug"};
	TCLAP::ValuesConstraint<std::string> loglevels(loglevels_allowed);
	TCLAP::ValueArg<std::string> loglevel(
	    "", "loglevel", "Set the logging level. (Default 'notice')", false,
	    "notice", &loglevels);
	TCLAP::SwitchArg debug_loglevel("", "debug",
	                                "Equivalent to --loglevel debug");
	TCLAP::SwitchArg trace_loglevel("", "trace",
	                                "Equivalent to --loglevel trace");
	TCLAP::SwitchArg info_loglevel("", "info", "Equivalent to --loglevel info");
	TCLAP::EitherOf log_arg(cmd);
	log_arg.add(loglevel)
	    .add(info_loglevel)
	    .add(trace_loglevel)
	    .add(debug_loglevel);

	TCLAP::MultiSwitchArg quiet("q", "quiet",
	                            "Suppress printing anything but score and "
	                            "errors. A second flag suppresses errors.",
	                            cmd);
	TCLAP::SwitchArg color("c", "color",
	                       "Print in color. "
	                       "(Defaults on if STDOUT is a tty.)",
	                       cmd);
	// TODO: implement log coloring detection
	TCLAP::SwitchArg log_color("C", "log-color",
	                           "Enable colors in the log. "
	                           "(Defaults on if STDERR is a tty.)",
	                           cmd);

	TCLAP::SwitchArg dry_run(
	    "", "dry-run", "Parse the command line, but don't run any tests", cmd);

	cmd.parse(argc, argv);
	// Color output on interactive shells or when explicitely requested
	color_stdout = color.isSet() or isatty(STDOUT_FILENO);
	//  Color logs on interactive shells or when explicitely requested
	color_logs = log_color.isSet() or isatty(STDERR_FILENO);
	signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigterm_handler);
	signal(SIGUSR1, sigterm_handler);
	signal(SIGUSR2, sigterm_handler);

	set_log_level([&] {
		using namespace kblib::literals;
		if (debug_loglevel.isSet()) {
			return log_level::debug;
		} else if (trace_loglevel.isSet()) {
			return log_level::trace;
		} else if (info_loglevel.isSet()) {
			return log_level::info;
		}
		switch (kblib::FNV32a(loglevel.getValue())) {
		case "none"_fnv32:
			return log_level::silent;
		case "err"_fnv32:
		case "error"_fnv32:
			return log_level::err;
		case "warn"_fnv32:
			return log_level::warn;
		case "notice"_fnv32:
			return log_level::notice;
		case "info"_fnv32:
			return log_level::info;
		case "trace"_fnv32:
			return log_level::trace;
		case "debug"_fnv32:
			return log_level::debug;
		default:
			log_warn("Unknown log level ", kblib::quoted(loglevel.getValue()),
			         " ignored. Validation bug?");
			return get_log_level();
		}
	}());

	unsigned num_threads = threads.getValue();
	if (threads.getValue() != 1) {
		if (get_log_level() > log_level::info) {
			throw std::invalid_argument(
			    "log_level cannot be higher than info with -j");
		}
		if (threads.getValue() == 0) {
			num_threads = std::thread::hardware_concurrency();
		}
	}
	log_info("Using ", num_threads, " threads");

	uint level_id{};
	field f = [&] {
		if (id_arg.isSet()) {
			level_id = find_level_id(id_arg.getValue());
			return field(layouts.at(to_unsigned(level_id)).layout,
			             T30_size.getValue());
		} else if (level_num.isSet()) {
			level_id = level_num.getValue();
			return field(layouts.at(to_unsigned(level_id)).layout,
			             T30_size.getValue());
		} else if (auto filename = std::filesystem::path(solution.getValue())
		                               .filename()
		                               .string();
		           auto maybeId = guess_level_id(filename)) {
			level_id = *maybeId;
			log_debug("Deduced level ", layouts.at(to_unsigned(level_id)).segment,
			          " from filename \"", filename, "\"");
			return field(layouts.at(to_unsigned(level_id)).layout,
			             T30_size.getValue());
		} else {
			throw std::invalid_argument{
			    "Impossible to determine the level ID from information given"};
		}
	}();

	std::vector<range_t> seed_ranges;

	if (seed_exprs.isSet()) {
		if (random.isSet() or seed_arg.isSet()) {
			throw std::invalid_argument{
			    "Cannot set --seeds in combination with -r or --seed"};
		}
		seed_ranges = parse_ranges(seed_exprs.getValue());
	} else if (random.isSet()) {
		std::uint32_t seed;
		if (seed_arg.isSet()) {
			seed = seed_arg.getValue();
		} else {
			seed = std::random_device{}();
			log_info("random seed: ", seed);
		}
		seed_ranges.push_back({seed, random.getValue()});
	}

	std::uint32_t total_random_tests{};
	{
		auto log = log_debug();
		log << "Seed ranges parsed: {\n";
		for (auto r : seed_ranges) {
			log << r.begin << ".." << r.begin + r.size - 1 << " [" << r.size
			    << "]; ";
			total_random_tests += r.size;
		}
		log << "\n} sum: " << total_random_tests << " tests";
	}
	if (dry_run.getValue()) {
		return 0;
	}

	if (solution.getValue() == "-") {
		std::ostringstream in;
		in << std::cin.rdbuf();
		std::string code = std::move(in).str();

		parse_code(f, code, T21_size.getValue());
	} else {
		if (std::filesystem::is_regular_file(solution.getValue())) {
			auto code = kblib::try_get_file_contents(solution.getValue());
			parse_code(f, code, T21_size.getValue());
		} else {
			log_err("invalid file: ", kblib::quoted(solution.getValue()));
			throw std::invalid_argument{"Solution must name a regular file"};
		}
	}

	log_debug_r([&] { return f.layout(); });

	score sc{};
	std::size_t total_cycles{};
	sc.validated = true;
	if (fixed.getValue()) {
		score last{};
		int succeeded{1};
		for (auto test : static_suite(level_id)) {
			set_expected(f, test);
			last = run(f, cycles_limit.getValue(), true);
			if (stop_requested) {
				log_notice("Stop requested");
				break;
			}
			sc.cycles = std::max(sc.cycles, last.cycles);
			sc.instructions = last.instructions;
			sc.nodes = last.nodes;
			sc.validated = sc.validated and last.validated;
			total_cycles += last.cycles;
			log_info("fixed test ", succeeded, ' ',
			         last.validated ? "validated"sv : "failed"sv, " with score ",
			         to_string(last, false));
			if (not last.validated) {
				break;
			}
			++succeeded;
			// optimization: skip running the 2nd and 3rd rounds for invariant
			// levels (specifically, the image test patterns)
			if (not f.has_inputs()) {
				log_info("Secondary tests skipped for invariant level");
				break;
			}
		}
		sc.achievement = check_achievement(level_id, f, sc);
		score_summary(sc, succeeded, quiet.getValue(), cycles_limit.getValue());
	}

	int count = 0;
	int valid_count = 0;
	if (sc.validated and not seed_ranges.empty()) {
		bool failure_printed{};
		run_params params{total_cycles,
		                  failure_printed,
		                  count,
		                  valid_count,
		                  total_cycles_limit,
		                  cycles_limit,
		                  static_cast<uint8_t>(quiet.getValue()),
		                  stats.getValue(),
		                  cheat_rate,
		                  total_random_tests};
		auto worst
		    = run_seed_ranges(f, level_id, seed_ranges, params, num_threads);

		log_info("Random test results: ", valid_count, " passed out of ", count,
		         " total");

		sc.cheat = (count != valid_count);
		sc.hardcoded = (valid_count <= static_cast<int>(count * cheat_rate));
		if (not fixed.getValue()) {
			sc = worst;
			score_summary(sc, -1, quiet.getValue(), cycles_limit.getValue());
		}
	}

	if (not quiet.getValue()) {
		std::cout << "score: ";
	}
	std::cout << sc;
	if (count > 0 and stats.isSet()) {
		const auto rate = 100. * valid_count / count;
		std::cout << " PR: ";
		if (valid_count == count) {
			std::cout << print_escape(bright_blue, bold);
		} else if (rate >= 100 * cheat_rate) {
			std::cout << print_escape(yellow);
		} else {
			std::cout << print_escape(bright_red);
		}
		std::cout << rate << '%' << print_escape(none) << " (" << valid_count
		          << '/' << count << ")";
	}
	std::cout << '\n';

	return (sc.validated) ? 0 : 1;
} catch (const std::exception& e) {
	int status{};
	auto type_name
	    = abi::__cxa_demangle(typeid(e).name(), nullptr, nullptr, &status);
	if (status != 0) {
		log_err("Unexpected error occured while handling exception: ", e.what());
		log_flush();
		return 2;
	}

	log_err("Failed with exception of type ", type_name, ": ", e.what());
	log_flush();
	return 2;
}
