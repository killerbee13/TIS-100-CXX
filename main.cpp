/* *****************************************************************************
 * TIX-100-CXX
 * Copyright (c) 2024 killerbee
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
#include "io.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "random_levels.hpp"
#include <iostream>

#include <kblib/hash.h>
#include <kblib/stringops.h>
#include <random>
#define TCLAP_SETBASE_ZERO 1
#include <tclap/CmdLine.h>

using namespace std::literals;

score run(field& l, int cycles_limit) {
	score sc{};
	sc.instructions = l.instructions();
	sc.nodes = l.nodes_used();
	do {
		++sc.cycles;
		log_debug("step ", sc.cycles);
		log_debug_r([&] { return "Current state:\n" + l.state(); });
		l.step();
	} while ((l.active()) and std::cmp_less(sc.cycles, cycles_limit));
	sc.validated = true;

	log_flush();
	for (auto it = l.end_regular(); it != l.end(); ++it) {
		auto n = it->get();
		if (type(n) == node::out) {
			auto p = static_cast<output_node*>(n);
			if (p->outputs_expected != p->outputs_received) {
				sc.validated = false;
			}
		} else if (type(n) == node::image) {
			auto p = static_cast<image_output*>(n);
			if (p->image_expected != p->image_received) {
				sc.validated = false;
			}
		}
	}

	if (not sc.validated) {
		for (auto it = l.end_regular(); it != l.end(); ++it) {
			auto n = it->get();
			if (type(n) == node::in) {
				auto p = static_cast<input_node*>(n);
				std::cout << "input " << p->x << ": ";
				write_list(std::cout, p->inputs) << '\n';
			} else if (type(n) == node::out) {
				auto p = static_cast<output_node*>(n);
				if (p->outputs_expected != p->outputs_received) {
					std::cout << "validation failure for output " << p->x;
					std::cout << "\noutput: ";
					write_list(std::cout, p->outputs_received, &p->outputs_expected);
					std::cout << "\nexpected: ";
					write_list(std::cout, p->outputs_expected);
					std::cout << "\n";
				}
			} else if (type(n) == node::image) {
				auto p = static_cast<image_output*>(n);
				if (p->image_expected != p->image_received) {
					std::cout << "validation failure for output " << p->x
					          << "\noutput:\n"
					          << p->image_received.write_text() //
					          << "expected:\n"
					          << p->image_expected.write_text();
				}
			}
		}
	}

	return sc;
}

template <std::integral T>
class range_int_constraint : public TCLAP::Constraint<T> {
 public:
	range_int_constraint(T low, T high)
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
		return kblib::concat('[', low, '-', high, ']');
	}

	std::string shortID() const override { return description(); }

 private:
	T low{};
	T high{};
};

namespace TCLAP {
template <>
struct ArgTraits<field> {
	using ValueCategory = StringLike;
};
} // namespace TCLAP

int generate(uint32_t seed);

int main(int argc, char** argv) try {
	std::ios_base::sync_with_stdio(false);
	log_flush(not RELEASE);

	TCLAP::CmdLine cmd("TIS-100 simulator and validator");

	std::vector<std::string> ids_v;
	for (auto l : layouts) {
		ids_v.emplace_back(l.segment);
		ids_v.emplace_back(l.name);
	}

	TCLAP::UnlabeledValueArg<std::string> solution(
	    "Solution", "path to solution file", true, "-", "path", cmd);
	TCLAP::ValuesConstraint<std::string> ids_c(ids_v);
	TCLAP::UnlabeledValueArg<std::string> id_arg("ID", "Level ID", false, "",
	                                             &ids_c);

	range_int_constraint<int> level_nums_constraint(0, layouts.size() - 1);
	// unused because the test case parser is not implemented
	TCLAP::ValueArg<std::string> layout_s("l", "layout", "Layout string", false,
	                                      "", "layout");
	TCLAP::ValueArg<int> level_num("L", "level", "Numeric level ID", false, 0,
	                               &level_nums_constraint);
	TCLAP::OneOf layout(cmd);
	layout.add(id_arg).add(level_num); //.add(layout_s);

	TCLAP::ValueArg<bool> fixed("", "fixed", "Run fixed tests", false, true,
	                            "bool", cmd);
	TCLAP::ValueArg<int> random("r", "random", "Random tests to run", false, 0,
	                            "integer", cmd);
	TCLAP::ValueArg<std::uint32_t> seed_arg(
	    "", "seed", "Seed to use for random tests", false, 0, "uint32_t", cmd);
	TCLAP::ValueArg<int> cycles_limit(
	    "", "limit", "Number of cycles to run test for before timeout", false,
	    10'000, "integer", cmd);
	// unused because the test case parser is not implemented
	TCLAP::ValueArg<std::string> set_test("t", "test", "Manually set test cases",
	                                      false, "", "test case");

	// Size constraint of 999 guarantees that JRO can reach every instruction
	range_int_constraint<unsigned> size_constraint(0, 999);
	TCLAP::ValueArg<unsigned> T21_size(
	    "", "T21_size", "Number of instructions allowed per T21 node", false,
	    def_T21_size, &size_constraint, cmd);
	TCLAP::ValueArg<unsigned> T30_size("", "T30_size",
	                                   "Memory capacity of T30 nodes", false,
	                                   def_T30_size, "integer", cmd);

	std::vector<std::string> loglevels_allowed{"none",   "err",  "error", "warn",
	                                           "notice", "info", "debug"};
	TCLAP::ValuesConstraint<std::string> loglevels(loglevels_allowed);
	const TCLAP::ValueArg<std::string> loglevel(
	    "", "loglevel", "Set the logging level. Defaults to 'notice'.", false,
	    "notice", &loglevels, cmd);
	TCLAP::MultiSwitchArg quiet("q", "quiet",
	                            "Suppress printing anything but score and "
	                            "errors. A second flag suppresses errors.",
	                            cmd);
	TCLAP::SwitchArg color("c", "color", "Print in color", cmd);
	// TODO: implement log coloring detection
	TCLAP::SwitchArg log_color("C", "log-color",
	                           "Enable colors in the log. (Defaults on if -c is "
	                           "set and STDERR is a tty.)");

	TCLAP::ValueArg<std::string> write_machine_layout(
	    "", "write-layouts", "write the C++-parsable layouts to [filename]",
	    false, "", "filename", cmd);

	cmd.parse(argc, argv);
	use_color = color.getValue();

	set_log_level([&] {
		using namespace kblib::literals;
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
		case "debug"_fnv32:
			return log_level::debug;
		default:
			log_warn("Unknown log level ", kblib::quoted(loglevel.getValue()),
			         " ignored. Validation bug?");
			return get_log_level();
		}
	}());

	int id{};
	field f = [&] {
		if (id_arg.isSet()) {
			id = level_id(id_arg.getValue());
			return field(layouts1.at(static_cast<std::size_t>(id)).layout,
			             T30_size.getValue());
		} else if (level_num.isSet()) {
			id = level_num.getValue();
			return field(layouts1.at(static_cast<std::size_t>(id)).layout,
			             T30_size.getValue());
		} else if (layout_s.isSet()) {
			id = -1;
			return parse_layout_guess(layout_s.getValue(), T30_size.getValue());
		} else {
			abort();
		}
	}();

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

	if (write_machine_layout.isSet()) {
		std::ofstream out(write_machine_layout.getValue());

		out << R"(#ifndef LAYOUTSPECS_HPP
#define LAYOUTSPECS_HPP

#include "node.hpp"

struct builtin_layout_spec {
	struct io_node_spec {
		node::type_t type;
		std::optional<std::pair<word_t, word_t>> image_size;
	};

	std::array<std::array<node::type_t, 4>, 3> nodes;
	std::array<std::array<io_node_spec, 4>, 2> io;
};

struct level_layout1 {
	std::string_view segment;
	std::string_view name;
	builtin_layout_spec layout;
};

constexpr std::array<level_layout1, 51> gen_layouts() {
	using enum node::type_t;
	return {{
)";
		for (auto& l : layouts) {
			out << "\t{" << std::quoted(l.segment) << ", " << std::quoted(l.name)
			    << ",\n\t\t";
			out << parse_layout_guess(l.layout, def_T30_size).machine_layout()
			    << "},\n";
		}
		out << R"(}};
}

inline constexpr auto layouts1 = gen_layouts();

#endif
)";
	}

	std::uint32_t seed;
	if (random.getValue() > 0) {
		if (seed_arg.isSet()) {
			seed = seed_arg.getValue();
		} else {
			seed = std::random_device{}();
			log_info("random seed: ", seed);
		}
	}

	log_debug_r([&] { return f.layout(); });

	score sc;
	if (fixed.getValue()) {
		sc.validated = true;
		score last{};
		int succeeded{1};
		for (auto test : static_suite(id)) {
			set_expected(f, test);
			last = run(f, cycles_limit.getValue());
			sc.cycles = std::max(sc.cycles, last.cycles);
			sc.instructions = last.instructions;
			sc.nodes = last.nodes;
			sc.validated = sc.validated and last.validated;
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
		sc.achievement = check_achievement(id, f, sc);

		if (sc.validated) {
			if (not quiet.getValue()) {
				std::cout << print_color(bright_blue) << "validation successful"
				          << print_color(reset) << "\n";
			}
		} else {
			std::cout << print_color(red) << "validation failed"
			          << print_color(reset) << " for fixed test " << succeeded
			          << " after " << last.cycles << " cycles ";
			if (std::cmp_equal(sc.cycles, cycles_limit.getValue())) {
				std::cout << "[timeout]";
			}
			std::cout << '\n';
		}
	}
	if (random.getValue() > 0) {
		score last{};
		score worst{};
		int count = 0;
		for (; count < random.getValue(); ++count) {
			auto test = random_test(id, seed++);
			set_expected(f, test);
			last = run(f, cycles_limit.getValue());
			if (not last.validated) {
				worst.cheat = true;
			}
			worst.cycles = std::max(worst.cycles, last.cycles);
			worst.instructions = last.instructions;
			worst.nodes = last.nodes;
			// for random tests, only one validation is needed
			worst.validated = worst.validated or last.validated;
		}
		sc.cheat = worst.cheat;
		if (not fixed.getValue()) {
			sc = worst;

			if (sc.validated and not quiet.getValue()) {
				if (not quiet.getValue()) {
					std::cout << print_color(bright_blue) << "validation successful"
					          << print_color(reset) << "\n";
				}
			} else if (quiet.getValue() < 2) {
				std::cout << print_color(red) << "validation failed"
				          << print_color(reset);
				if (std::cmp_equal(sc.cycles, cycles_limit.getValue())) {
					std::cout << " [timeout]";
				}
				std::cout << '\n';
			}
		}
	}
	if (not quiet.getValue()) {
		std::cout << "score: ";
	}
	std::cout << sc << '\n';
	return (sc.validated) ? 0 : 1;

#if 0
	// tiny self-test
	auto l = parse(
		 "1 1 B I0 NUMERIC @0 O0 NUMERIC @5",
		 "@0\nMOV UP,ACC\nADD ACC\nMOV ACC,DOWN",
		 single_test{
			  .inputs = {{51, 62, 16, 83, 61, 14, 35, 17, 63, 48, 22, 40, 29,
							  50, 77, 32, 31, 49, 89, 89, 12, 59, 53, 75, 37, 78,
							  57, 38, 44, 98, 85, 25, 80, 39, 20, 16, 91, 81, 84}},
			  .n_outputs = {{102, 124, 32,  166, 122, 28,  70,  34,  126, 96,
								  44,  80,  58,  100, 154, 64,  62,  98,  178, 178,
								  24,  118, 106, 150, 74,  156, 114, 76,  88,  196,
								  170, 50,  160, 78,  40,  32,  182, 162, 168}},
			  .i_output = {}});
	sc = run(l, cycles_limit.getValue());

	if (sc.validated) {
		std::cout << "validation successful\n";
		std::cout << "score: " << sc << '\n';
	} else {
		std::cout << "validation failed after " << sc.cycles << " cycles ";
		if (sc.cycles == cycles_limit) {
			std::cout << "[timeout]";
		}
		std::cout << '\n';
	}
	return (sc.validated) ? 0 : 1;
#endif

} catch (const std::exception& e) {
	log_err("failed with exception: ", e.what());
	throw;
}

// this was mostly to check that the parser and serializer (layout())
// round-tripped correctly
int generate(std::uint32_t seed) {
	auto copy = [](auto x) { return x; };
	for (auto [lvl, i] : kblib::enumerate(layouts)) {
		auto base_path = std::filesystem::path(std::filesystem::current_path()
		                                       / lvl.segment);
		log_info("writing cfg: ", copy(base_path).concat(".tiscfg").native());
		log_flush();
		std::ofstream layout_f(copy(base_path).concat(".tiscfg"),
		                       std::ios_base::trunc);
		auto l = parse_layout_guess(lvl.layout, def_T30_size);
		std::string layout;
		kblib::search_replace_copy(l.layout(), "@"s,
		                           std::string(lvl.segment) + "@",
		                           std::back_inserter(layout));
		layout_f << layout;

		auto r = random_test(static_cast<int>(i), seed);
		std::size_t i_idx = 0;
		std::size_t o_idx = 0;
		for (auto it = l.end_regular(); it != l.end(); ++it) {
			auto n = it->get();
			if (type(n) == node::in) {
				auto p = static_cast<input_node*>(n);
				auto o_path = copy(base_path).concat(p->filename);
				log_info("writing in ", i_idx, ": ", o_path.native());
				std::ofstream o_f(o_path, std::ios_base::trunc);
				for (auto w : r.inputs[i_idx]) {
					o_f << w << '\n';
				}
				++i_idx;
			} else if (type(n) == node::out) {
				auto p = static_cast<output_node*>(n);
				auto o_path = copy(base_path).concat(p->filename);
				log_info("writing out ", o_idx, ": ", o_path.native());
				std::ofstream o_f(o_path, std::ios_base::trunc);
				for (auto w : r.n_outputs[o_idx]) {
					o_f << w << '\n';
				}
				++o_idx;
			} else if (type(n) == node::image) {
				auto p = static_cast<image_output*>(n);
				auto o_path = copy(base_path).concat(p->filename);
				log_info("writing image: ", o_path.native());
				std::ofstream o_f(o_path, std::ios_base::trunc);
				o_f << r.i_output;
				o_f.close();

				o_path += ".pnm";
				log_info("writing scaled image: ", o_path.native());
				o_f.open(o_path, std::ios_base::trunc);
				r.i_output.write(o_f, 11, 17);
			}
		}
	}
	return 0;
}
