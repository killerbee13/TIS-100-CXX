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

#include "logger.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "sim.hpp"
#include "utils.hpp"

#include <kblib/hash.h>
#include <kblib/stringops.h>

#include <csignal>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string_view>

#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>

#include <cxxabi.h>
#include <typeinfo>

#ifdef _WIN32
#	include <io.h>
#	define isatty(x) _isatty(x)
#endif

std::optional<std::string> demangle(const char* name) {
	int status{};
	auto type_name = abi::__cxa_demangle(name, nullptr, nullptr, &status);
	if (status != 0) {
		log_err("Unexpected error occured while demangling ",
		        kblib::quoted(std::string_view(name)));
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
			throw std::invalid_argument{
			    concat("range_constraint error, low: ", low, " > high: ", high)};
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
					throw std::invalid_argument{concat("Invalid character ",
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
					throw std::invalid_argument{concat("Invalid character ",
					                                   kblib::escapify(r[i]),
					                                   " in seed expr")};
				}
			}
			auto e = end->empty() ? kblib::max : parse_int<std::uint32_t>(*end);
			if (e < b) {
				throw std::invalid_argument{
				    concat("Seed ranges must be low..high, got: ", b, "..", e)};
			}
			seed_ranges.push_back({b, e + 1});
		}
	}
	return seed_ranges;
}

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

	TCLAP::CmdLine cmd(
	    "TIS-100 simulator and validator. For options --limit, --total-limit, "
	    "--random, --seed, --seeds, and --T30_size, integer arguments can be "
	    "specified with a scale suffix, either K, M, or B (case-insensitive) "
	    "for thousand, million, or billion respectively.");

	std::vector<std::string> ids_v;
	for (auto& l : builtin_layouts) {
		ids_v.emplace_back(l.segment);
		ids_v.emplace_back(l.name);
	}

	TCLAP::UnlabeledMultiArg<std::string> solutions(
	    "Solution", "Paths to solution files. ('-' for stdin)", true, "path",
	    cmd);

	TCLAP::ValuesConstraint<std::string> ids_c(ids_v);
	TCLAP::ValueArg<std::string> id_arg("l", "ID", "Level ID (Segment or name).",
	                                    false, "", &ids_c);
#if TIS_ENABLE_LUA
	TCLAP::ValueArg<std::string> custom_spec_arg(
	    "L", "custom-spec", "Custom Lua Spec file", false, "", "path");
	TCLAP::EitherOf level_args(cmd);
	level_args.add(id_arg).add(custom_spec_arg);
#else
	cmd.add(id_arg);
#endif

	TCLAP::ValueArg<human_readable_integer<size_t>> cycles_limit_arg(
	    "", "limit",
	    "Number of cycles to run test for before timeout. (Default 100500)",
	    false, defaults::cycles_limit, "integer", cmd);
	TCLAP::ValueArg<human_readable_integer<std::size_t>> total_cycles_limit_arg(
	    "", "total-limit",
	    "Max number of cycles to run between all tests before determining "
	    "cheating status. (Default no limit)",
	    false, defaults::total_cycles_limit, "integer", cmd);
	TCLAP::ValueArg<uint> threads("j", "threads",
	                              "Number of threads to use, or 0 for "
	                              "automatic. Log level must be info or "
	                              "lower.",
	                              false, defaults::num_threads, "integer", cmd);
	TCLAP::ValueArg<bool> fixed("", "fixed", "Run fixed tests. (Default 1)",
	                            false, defaults::run_fixed, "[0,1]", cmd);
	TCLAP::SwitchArg stats(
	    "S", "stats", "Run all random tests requested and calculate pass rate",
	    cmd);
	TCLAP::ValueArg<human_readable_integer<std::uint32_t>> random_arg(
	    "r", "random", "Random tests to run (upper bound)", false, 0, "integer",
	    cmd);
	TCLAP::ValueArg<human_readable_integer<std::uint32_t>> seed_arg(
	    "", "seed", "Seed to use for random tests", false, 0, "uint32_t", cmd);
	TCLAP::MultiArg<std::string> seed_exprs("", "seeds",
	                                        "A range of seed values to use",
	                                        false, "[range-expr...]", cmd);
	range_constraint percentage(0.0, 1.0);
	TCLAP::ValueArg<double> cheat_rate(
	    "", "cheat-rate",
	    concat("Threshold between /c and /h solutions, "
	           "as a fraction of total random tests. (Default ",
	           defaults::cheat_rate, ")"),
	    false, defaults::cheat_rate, &percentage, cmd);
	TCLAP::ValueArg<double> limit_multiplier(
	    "k", "limit-multiplier",
	    concat(
	        "Value to multiply cycle score by to determine random test timeout "
	        "limit. (Default ",
	        defaults::limit_multiplier, ")"),
	    false, defaults::limit_multiplier, "number", cmd);

	// Size constraint of word_max guarantees that JRO can reach every
	// instruction
	range_constraint<uint> size_constraint(0, word_max);
	TCLAP::ValueArg<uint> T21_size(
	    "", "T21-size",
	    concat("Number of instructions allowed per T21 node. (Default ",
	           defaults::T21_size, ")"),
	    false, defaults::T21_size, &size_constraint, cmd);
	// No technical reason to limit T30 capacity, as they are not addressable
	TCLAP::ValueArg<human_readable_integer<uint>> T30_size(
	    "", "T30-size",
	    concat("Memory capacity of T30 nodes. (Default ", defaults::T30_size,
	           ")"),
	    false, defaults::T30_size, "integer", cmd);

	std::vector<std::string> loglevels_allowed{
	    "none",  "err", "error", "warn", "notice", "info", "trace",
#if TIS_ENABLE_DEBUG
	    "debug",
#endif
	};
	TCLAP::ValuesConstraint<std::string> loglevels(loglevels_allowed);
	TCLAP::ValueArg<std::string> loglevel(
	    "", "loglevel", "Set the logging level. (Default 'notice')", false,
	    "notice", &loglevels);
#if TIS_ENABLE_DEBUG
	TCLAP::SwitchArg debug_loglevel("", "debug",
	                                "Equivalent to --loglevel debug");
#endif
	TCLAP::SwitchArg trace_loglevel("", "trace",
	                                "Equivalent to --loglevel trace");
	TCLAP::SwitchArg info_loglevel("", "info", "Equivalent to --loglevel info");
	TCLAP::EitherOf log_arg(cmd);
	log_arg
	    .add(loglevel)
#if TIS_ENABLE_DEBUG
	    .add(debug_loglevel)
#endif
	    .add(trace_loglevel)
	    .add(info_loglevel);

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

	// initialize globals
	{
		// Flush logs automatically for humans
		set_log_flush(isatty(STDERR_FILENO));
		// Color output on interactive shells or when explicitely requested
		color_stdout = color.isSet() or isatty(STDOUT_FILENO);
		// Color logs on interactive shells or when explicitely requested
		color_logs = log_color.isSet() or isatty(STDERR_FILENO);

		signal(SIGTERM, sigterm_handler);
		signal(SIGINT, sigterm_handler);
#ifndef _WIN32
		signal(SIGUSR1, sigterm_handler);
		signal(SIGUSR2, sigterm_handler);
#endif

		set_log_level([&] {
#if TIS_ENABLE_DEBUG
			if (debug_loglevel.isSet()) {
				return log_level::debug;
			} else
#endif
			    if (trace_loglevel.isSet()) {
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
#if TIS_ENABLE_DEBUG
			case "debug"_fnv32:
				return log_level::debug;
#endif
			default:
				log_warn("Unknown log level ", kblib::quoted(loglevel.getValue()),
				         " ignored. Validation bug?");
				return get_log_level();
			}
		}());
	}

	// initialize the sim
	tis_sim sim;
	{
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
				seed = seed_arg.getValue().val;
			} else {
				seed = std::random_device{}();
				log_info("random seed: ", seed);
			}
			auto random_count = random_arg.getValue().val;
			seed_ranges.push_back({seed, seed + random_count});
		}
		sim.set_seed_ranges(std::move(seed_ranges));

		if (id_arg.isSet()) {
			sim.set_builtin_level_name(id_arg.getValue());
		}
#if TIS_ENABLE_LUA
		if (custom_spec_arg.isSet()) {
			sim.set_custom_spec_path(custom_spec_arg.getValue());
		}
#endif
		sim.set_cycles_limit(cycles_limit_arg.getValue());
		sim.set_total_cycles_limit(total_cycles_limit_arg.getValue());
		if (threads.getValue() != 1 and get_log_level() > log_level::info) {
			throw std::invalid_argument(
			    "log_level cannot be higher than info with -j");
		} else {
			sim.set_num_threads(threads.getValue());
		}
		sim.set_cheat_rate(cheat_rate.getValue());
		sim.set_limit_multiplier(limit_multiplier.getValue());
		sim.set_T21_size(T21_size.getValue());
		sim.set_T30_size(T30_size.getValue());
		sim.set_run_fixed(fixed.getValue());
		sim.set_compute_stats(stats.getValue());
	}

	if (dry_run.isSet()) {
		return exit_code::SUCCESS;
	}

	exit_code return_code = exit_code::SUCCESS;
	bool break_filenames = false;
	for (auto& solution : solutions.getValue()) {
		if (solutions.getValue().size() > 1) {
			if (std::exchange(break_filenames, true)) {
				std::cout << '\n';
			}
			std::cout << kblib::escapify(solution) << ":" << std::endl;
		}

		try {
			score sc = sim.simulate(solution);
			if (not sc.validated) {
				return_code = std::max(return_code, exit_code::FAILURE);
			}

			log_flush();
			if (sc.validated) {
				if (not quiet.getValue()) {
					std::cout << print_escape(bright_blue, bold)
					          << "validation successful" << print_escape(none)
					          << "\n";
				}
			} else if (quiet.getValue() < 2) {
				std::cout << print_escape(red, bold) << "validation failed"
				          << print_escape(none);
				if (sim.failed_test != 0) {
					std::cout << " for fixed test " << fixed //
					          << " after " << sc.cycles << " cycles";
					if (sc.cycles == cycles_limit_arg.getValue()) {
						std::cout << " [timeout]";
					}
				}
				std::cout << '\n';
			}

			if (not quiet.getValue()) {
				std::cout << "score: ";
			}
			std::cout << to_string(sc);
			if (sim.random_test_ran > 0 and stats.getValue()) {
				std::cout << " PR: ";
				if (not sc.cheat) {
					std::cout << print_escape(bright_blue, bold);
				} else if (not sc.hardcoded) {
					std::cout << print_escape(yellow);
				} else {
					std::cout << print_escape(red);
				}
				const auto rate
				    = 100. * sim.random_test_valid / sim.random_test_ran;
				std::cout << rate << '%' << print_escape(none) << " ("
				          << sim.random_test_valid << '/' << sim.random_test_ran
				          << ")";
			}
			std::cout << std::endl;
		} catch (const std::exception& e) {
			log_err(e.what());
			return_code = exit_code::EXCEPTION;
		}

		if (stop_requested) {
			break;
		}
	}

	return return_code;
} catch (const std::exception& e) {
	auto type_name = demangle(typeid(e).name());
	if (type_name) {
		log_err("Failed with exception of type ", *type_name, ": ", e.what());
	}
	return exit_code::EXCEPTION;
}
