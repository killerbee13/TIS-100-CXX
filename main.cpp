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
void print_validation_failure(field& f, T&& os, bool color) {
	for (auto& i : f.inputs()) {
		os << "input " << i->x << ": ";
		write_list(os, i->inputs, nullptr, color) << '\n';
	}
	for (auto& p : f.numerics()) {
		if (p->outputs_expected != p->outputs_received) {
			os << "validation failure for output " << p->x;
			os << "\noutput: ";
			write_list(os, p->outputs_received, &p->outputs_expected, color);
			os << "\nexpected: ";
			write_list(os, p->outputs_expected, nullptr, color);
			os << "\n";
		}
	}
	for (auto& p : f.images()) {
		if (p->wrong_pixels) {
			os << "validation failure for output " << p->x << "\noutput: ("
			   << p->width << ',' << p->height << ")\n"
			   << p->image_received.write_text(color) //
			   << "expected:\n"
			   << p->image_expected.write_text(color);
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
		for (auto& p : l.numerics()) {
			if (p->wrong || ! p->complete) {
				sc.validated = false;
				break;
			}
		}
		for (auto& p : l.images()) {
			if (p->wrong_pixels) {
				sc.validated = false;
				break;
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

std::optional<std::string> demangle(const char* name) {
	int status{};
	auto type_name = abi::__cxa_demangle(name, nullptr, nullptr, &status);
	if (status != 0) {
		log_err("Unexpected error occured while demangling ",
		        kblib::quoted(std::string_view(name)));
		log_flush();
		return {};
	}
	std::string ret = type_name;
	std::free(type_name);
	return ret;
}

template <typename T>
std::string demangle() {
	return demangle(typeid(T).name()).value();
}

template <typename Int>
constexpr Int parse_int(std::string_view str_) {
	auto str = str_;
	Int multiplier{1};
	if (str.ends_with('k') or str.ends_with('K')) {
		multiplier = 1'000;
		str.remove_suffix(1);
	} else if (str.ends_with('m') or str.ends_with('M')) {
		multiplier = 1'000'000;
		str.remove_suffix(1);
	} else if (str.ends_with('b') or str.ends_with('B')) {
		multiplier = 1'000'000'000;
		str.remove_suffix(1);
	}
	auto base = kblib::parse_integer<Int>(str);
	if (kblib::max.of<Int>() / multiplier < base) {
		throw std::out_of_range{concat("Number ", str, str_.back(),
		                               " is too large for type ",
		                               demangle<Int>())};
	}
	return multiplier * base;
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

/// numbers in [begin, end)
struct range_t {
	std::uint32_t begin{};
	std::uint32_t end{};
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
				} else if ("0123456789kKmM"sv.contains(r[i])) {
					begin.push_back(r[i]);
				} else {
					throw std::invalid_argument{kblib::concat("Invalid character ",
					                                          kblib::escapify(r[i]),
					                                          " in seed expr")};
				}
			}
			auto b = parse_int<std::uint32_t>(begin);
			if (not end) {
				seed_ranges.push_back({b, b + 1});
				continue;
			}
			for (; i != r.size(); ++i) {
				if ("0123456789kKmM"sv.contains(r[i])) {
					end->push_back(r[i]);
				} else {
					throw std::invalid_argument{kblib::concat("Invalid character ",
					                                          kblib::escapify(r[i]),
					                                          " in seed expr")};
				}
			}
			auto e = end->empty() ? kblib::max : parse_int<std::uint32_t>(*end);
			if (e < b) {
				throw std::invalid_argument{kblib::concat(
				    "Seed ranges must be low..high, got: ", b, "..", e)};
			}
			seed_ranges.push_back({b, e + 1});
		}
	}
	return seed_ranges;
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

void failure_summary(score sc, int fixed, int quiet, int cycles_limit) {
	// we use stdout here, so flush logs to avoid mangled messages in the shell
	log_flush();
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
		std::cout << " after " << sc.cycles << " cycles";
		if (std::cmp_equal(sc.cycles, cycles_limit)) {
			std::cout << " [timeout]";
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
	int cheating_success_threshold;
	std::uint8_t quiet;
	bool stats;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wshadow=compatible-local"
score run_seed_ranges(field& f, uint level_id,
                      const std::vector<range_t> seed_ranges, run_params params,
                      unsigned num_threads, std::size_t cycles,
                      std::vector<std::ptrdiff_t>& histogram) {
	assert(not seed_ranges.empty());
	score worst{};
	seed_range_iterator seed_it(seed_ranges);
	std::mutex it_m;
	std::mutex sc_m;
	std::vector<std::thread> threads;
	std::vector<int> counters(num_threads);

	auto task =
	    [](std::mutex& it_m, std::mutex& sc_m, seed_range_iterator& seed_it,
	       field f, uint level_id, run_params params, score& worst, int& counter,
	       std::size_t cycles, std::vector<std::ptrdiff_t>& histogram) static {
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
			    last = run(f, params.cycles_limit, false);

			    if (stop_requested) {
				    return;
			    }

			    // none of this is hot, so it doesn't need to be parallelized
			    // so it's simplest to just hold a lock the whole time
			    std::unique_lock l(sc_m);
			    ++params.count;
			    auto ratio = static_cast<double>(last.cycles) / cycles;
			    if (last.validated and std::floor(ratio) < histogram.size() - 1) {
				    ++histogram[std::floor(ratio)];
			    } else if (last.cycles == params.cycles_limit) {
				    ++histogram.back();
			    }
			    worst.instructions = last.instructions;
			    worst.nodes = last.nodes;
			    params.total_cycles += last.cycles;
			    if (last.validated) {
				    // for random tests, only one validation is needed
				    worst.validated = true;
				    params.valid_count++;
				    // don't count timeouts in max cycles
				    worst.cycles = std::max(worst.cycles, last.cycles);
			    } else {
				    if (std::exchange(params.failure_printed, true) == false) {
					    log_info("Random test failed for seed: ", seed,
					             std::cmp_equal(last.cycles, params.cycles_limit)
					                 ? " [timeout]"
					                 : "");
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
				    log_info("Total cycles timeout reached, stopping tests at ",
				             params.count);
				    return;
			    }
		    }
	    };
	if (f.inputs().empty()) {
		log_info("Secondary random tests skipped for invariant level");
		range_t r{0, 1};
		seed_range_iterator it2(std::span(&r, 1));
		task(it_m, sc_m, it2, std::move(f), level_id, params, worst, counters[0],
		     cycles, histogram);
	} else if (num_threads > 1) {
		for (auto i : range(num_threads)) {
			threads.emplace_back(task, std::ref(it_m), std::ref(sc_m),
			                     std::ref(seed_it), f.clone(), level_id, params,
			                     std::ref(worst), std::ref(counters[i]), cycles,
			                     std::ref(histogram));
		}

		for (auto& t : threads) {
			t.join();
		}
		for (auto [x, i] : kblib::enumerate(counters)) {
			log_info("Thread ", i, " ran ", x, " tests");
		}
	} else {
		task(it_m, sc_m, seed_it, std::move(f), level_id, params, worst,
		     counters[0], cycles, histogram);
	}

	if (stop_requested) {
		log_warn("Stop requested");
		log_flush();
	}

	return worst;
}
#pragma GCC diagnostic pop

template <typename Int>
struct human_readable_integer {
	using ValueCategory = TCLAP::StringLike;
	constexpr human_readable_integer() = default;
	constexpr human_readable_integer(const human_readable_integer&) = default;
	constexpr human_readable_integer(Int v)
	    : val(v) {}
	constexpr human_readable_integer(std::string_view str) {
		val = parse_int<Int>(str);
	}

	constexpr human_readable_integer& operator=(const human_readable_integer&)
	    = default;
	constexpr human_readable_integer& operator=(std::string_view str) {
		val = parse_int<Int>(str);
		return *this;
	}
	constexpr human_readable_integer& operator=(Int v) {
		val = v;
		return *this;
	}

	operator Int() const noexcept { return val; }
	Int val;
};

static_assert(human_readable_integer<int>("10").val == 10);
static_assert(human_readable_integer<int>("10k").val == 10'000);
static_assert(human_readable_integer<int>("10M").val == 10'000'000);

enum exit_code : int { SUCCESS = 0, FAILURE = 1, EXCEPTION = 2 };

int main(int argc, char** argv) try {
	std::ios_base::sync_with_stdio(false);
	set_log_flush(not RELEASE);

	TCLAP::CmdLine cmd(
	    "TIS-100 simulator and validator. For options --limit, --total-limit, "
	    "--random, --seed, --seeds, and --T30_size, integer arguments can be "
	    "specified with a scale suffix, either K, M, or B (case-insensitive) "
	    "for thousand, million, or billion respectively.");

	std::vector<std::string> ids_v;
	for (auto l : layouts) {
		ids_v.emplace_back(l.segment);
		ids_v.emplace_back(l.name);
	}

	TCLAP::UnlabeledMultiArg<std::string> solutions(
	    "Solution", "Path to solution file. ('-' for stdin)", true, "path", cmd);

	TCLAP::ValuesConstraint<std::string> ids_c(ids_v);
	TCLAP::ValueArg<std::string> id_arg("l", "ID", "Level ID (Segment or name).",
	                                    false, "", &ids_c);
	range_constraint<uint> level_nums_constraint(0, layouts.size() - 1);
	TCLAP::ValueArg<uint> level_num("L", "level", "Numeric level ID", false, 0,
	                                &level_nums_constraint);
	TCLAP::EitherOf level_args(cmd);
	level_args.add(id_arg).add(level_num);

	TCLAP::ValueArg<human_readable_integer<int>> cycles_limit_arg(
	    "", "limit",
	    "Number of cycles to run test for before timeout. (Default 100500)",
	    false, 100'500, "integer", cmd);
	TCLAP::ValueArg<human_readable_integer<std::size_t>> total_cycles_limit_arg(
	    "", "total-limit",
	    "Max number of cycles to run between all tests before determining "
	    "cheating status. (Default no limit)",
	    false, kblib::max.of<std::size_t>(), "integer", cmd);
	TCLAP::ValueArg<unsigned> threads("j", "threads",
	                                  "Number of threads to use, or 0 for "
	                                  "automatic. Log level must be info or "
	                                  "lower.",
	                                  false, 1, "integer", cmd);

	TCLAP::ValueArg<bool> fixed("", "fixed", "Run fixed tests. (Default 1)",
	                            false, true, "[0,1]", cmd);
	// unused because the test case parser is not implemented
	TCLAP::ValueArg<std::string> set_test("t", "test", "Manually set test cases",
	                                      false, "", "test case");
	TCLAP::ValueArg<human_readable_integer<std::uint32_t>> random_arg(
	    "r", "random", "Random tests to run (upper bound)", false, 0, "integer");
	TCLAP::ValueArg<human_readable_integer<std::uint32_t>> seed_arg(
	    "", "seed", "Seed to use for random tests", false, 0, "uint32_t");
	TCLAP::SwitchArg stats(
	    "S", "stats", "Run all random tests requested and calculate pass rate",
	    cmd);
	TCLAP::MultiArg<std::string> seed_exprs("", "seeds",
	                                        "A range of seed values to use",
	                                        false, "[range-expr...]", cmd);
	TCLAP::AnyOf implicit_random(cmd);
	implicit_random.add(random_arg).add(seed_arg);
	range_constraint percentage(0.0, 1.0);
	TCLAP::ValueArg<double> cheat_rate(
	    "", "cheat-rate",
	    "Threshold between /c and /h solutions, "
	    "as a fraction of total random tests. (Default 0.05)",
	    false, 0.05, &percentage, cmd);
	TCLAP::SwitchArg hist(
	    "", "histogram",
	    "Print histograms of the random score ratios (cycles/fixed cycles)",
	    cmd);

	// Size constraint of word_max guarantees that JRO can reach every
	// instruction
	range_constraint<unsigned> size_constraint(0, word_max);
	TCLAP::ValueArg<unsigned> T21_size(
	    "", "T21_size",
	    concat("Number of instructions allowed per T21 node. (Default ",
	           def_T21_size, ")"),
	    false, def_T21_size, &size_constraint, cmd);
	// No technical reason to limit T30 capacity, as they are not addressable
	TCLAP::ValueArg<human_readable_integer<unsigned>> T30_size(
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

	auto cycles_limit = cycles_limit_arg.getValue().val;
	auto total_cycles_limit = total_cycles_limit_arg.getValue().val;
	auto random_count = random_arg.getValue().val;
	auto base_seed = seed_arg.getValue().val;

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

	std::vector<range_t> seed_ranges;

	if (seed_exprs.isSet()) {
		if (random_arg.isSet() or seed_arg.isSet()) {
			throw std::invalid_argument{
			    "Cannot set --seeds in combination with -r or --seed"};
		}
		seed_ranges = parse_ranges(seed_exprs.getValue());
	} else if (random_arg.isSet()) {
		std::uint32_t seed;
		if (seed_arg.isSet()) {
			seed = base_seed;
		} else {
			seed = std::random_device{}();
			log_info("random seed: ", seed);
		}
		seed_ranges.push_back({seed, seed + random_count});
	}

	std::uint32_t total_random_tests{};
	{
		auto log = log_debug();
		log << "Seed ranges parsed: {\n";
		for (auto r : seed_ranges) {
			log << r.begin << ".." << r.end - 1 << " [" << r.end - r.begin
			    << "]; ";
			total_random_tests += r.end - r.begin;
		}
		log << "\n} sum: " << total_random_tests << " tests";
	}
	if (dry_run.getValue()) {
		return exit_code::SUCCESS;
	}

	exit_code return_code = exit_code::SUCCESS;
	for (auto& solution : solutions.getValue()) {
		uint level_id;
		if (id_arg.isSet()) {
			level_id = find_level_id(id_arg.getValue());
		} else if (level_num.isSet()) {
			level_id = level_num.getValue();
		} else if (auto filename
		           = std::filesystem::path(solution).filename().string();
		           auto maybeId = guess_level_id(filename)) {
			level_id = *maybeId;
			log_debug("Deduced level ", layouts.at(level_id).segment,
			          " from filename \"", filename, "\"");
		} else {
			log_err("Impossible to determine the level ID for \"", filename, "\"");
			return_code = exit_code::EXCEPTION;
			continue;
		}
		field f(layouts.at(level_id).layout, T30_size.getValue());

		if (solution == "-") {
			std::ostringstream in;
			in << std::cin.rdbuf();
			std::string code = std::move(in).str();

			parse_code(f, code, T21_size.getValue());
		} else {
			if (std::filesystem::is_regular_file(solution)) {
				auto code = kblib::try_get_file_contents(solution);
				try {
					parse_code(f, code, T21_size.getValue());
				} catch (const std::invalid_argument& e) {
					log_err(e.what());
					return_code = exit_code::EXCEPTION;
					continue;
				}
			} else {
				log_err("invalid file: ", kblib::quoted(solution));
				return_code = exit_code::EXCEPTION;
				continue;
			}
		}
		log_debug_r([&] { return "Layout:\n" + f.layout(); });

		score sc{};
		std::size_t total_cycles{};
		sc.validated = true;
		if (fixed.getValue()) {
			int succeeded{1};
			for (auto test : static_suite(level_id)) {
				set_expected(f, test);
				score last = run(f, cycles_limit, true);
				sc.cycles = std::max(sc.cycles, last.cycles);
				sc.instructions = last.instructions;
				sc.nodes = last.nodes;
				sc.validated = sc.validated and last.validated;
				if (stop_requested) {
					log_notice("Stop requested");
					break;
				}
				total_cycles += last.cycles;
				log_info("fixed test ", succeeded, ' ',
				         last.validated ? "validated"sv : "failed"sv,
				         " with score ", to_string(last, false));
				if (not last.validated) {
					break;
				}
				++succeeded;
				// optimization: skip running the 2nd and 3rd rounds for invariant
				// levels (specifically, the image test patterns)
				if (f.inputs().empty()) {
					log_info("Secondary tests skipped for invariant level");
					break;
				}
			}
			sc.achievement = check_achievement(level_id, f, sc);
			failure_summary(sc, succeeded, quiet.getValue(), cycles_limit);
		}

		std::vector<std::ptrdiff_t> histogram(27);
		int count = 0;
		int valid_count = 0;
		score worst{};
		if ((fixed.getValue() == 0 or sc.validated) and not stop_requested
		    and not seed_ranges.empty()) {
			bool failure_printed{};
			run_params params{total_cycles,
			                  failure_printed,
			                  count,
			                  valid_count,
			                  total_cycles_limit,
			                  cycles_limit,
			                  static_cast<int>(cheat_rate * total_random_tests),
			                  static_cast<uint8_t>(quiet.getValue()),
			                  stats.getValue()};
			worst = run_seed_ranges(f, level_id, seed_ranges, params, num_threads,
			                        sc.cycles, histogram);

			log_info("Random test results: ", valid_count, " passed out of ",
			         count, " total");

			if (not fixed.getValue()) {
				sc = worst;
				failure_summary(sc, -1, quiet.getValue(), cycles_limit);
			}
			sc.cheat = (count != valid_count);
			sc.hardcoded = (valid_count <= static_cast<int>(count * cheat_rate));
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

			if (valid_count != 0) {
				auto ratio = static_cast<double>(worst.cycles) / sc.cycles;
				auto color = (ratio < 2) ? ""s : print_escape(yellow);
				std::cout << " random max_cycles: " << worst.cycles << " (" << color
				          << ratio << print_escape(none) << ')';
			} else {
				std::cout << " random max_cycles: - (0)";
			}
			if (hist) {
				if (std::ranges::any_of(histogram, std::identity{})) {
					auto scale
					    = std::min(130. / *std::ranges::max_element(histogram), 1.0);
					auto count = 0uz;
					for (auto i : range(histogram.size() - 1)) {
						if (histogram[i]) {
							count = i + 1;
						}
					}
					std::cout << "\nhistogram: (█ = " << 1 / scale << '\n';
					for (auto i : range(count)) {
						std::cout
						    << std::setw(3) << i << ": " << std::setw(6)
						    << histogram[i] << ' '
						    << kblib::repeat("█"s, std::ceil(histogram[i] * scale))
						    << '\n';
					}
					std::cout << "inf: " << std::setw(6) << histogram.back() << ' '
					          << kblib::repeat("█"s,
					                           std::ceil(histogram.back() * scale));
				} else {
					std::cout << "\nNo tests passed--no histogram to display";
				}
			}
		}
		std::cout << std::endl;
		if (not sc.validated) {
			return_code = std::max(return_code, exit_code::FAILURE);
		}
		if (stop_requested) {
			break;
		}
	}

	return return_code;
} catch (const std::exception& e) {
	auto type_name = demangle(typeid(e).name());
	if (not type_name) {
		return exit_code::EXCEPTION;
	}

	log_err("Failed with exception of type ", *type_name, ": ", e.what());
	log_flush();
	return exit_code::EXCEPTION;
}
