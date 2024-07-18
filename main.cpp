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
#include <cassert>
#include <iostream>

#include <kblib/stringops.h>

using namespace std::literals;

score run(field& l) {
	score sc{};
	sc.instructions = l.instructions();
	sc.nodes = l.nodes_used();
	do {
		++sc.cycles;
		log_debug("step ", sc.cycles);
		l.step();
	} while ((l.active()) and sc.cycles < 1'000'000);

	sc.validated = true;

	std::clog << std::flush;

	for (auto it = l.begin() + l.nodes_total(); it != l.end(); ++it) {
		auto n = it->get();
		if (type(n) == node::out) {
			auto p = static_cast<output_node*>(n);
			if (p->outputs_expected != p->outputs_received) {
				std::cout << "validation failure for output " << p->x << '\n';
				sc.validated = false;
			}
		} else if (type(n) == node::image) {
			auto p = static_cast<image_output*>(n);
			if (p->image_expected != p->image_received) {
				std::cout << "validation failure for output " << p->x << '\n';
				sc.validated = false;
			}
		}
	}

	return sc;
}

int main(int argc, char** argv) {
	std::ios_base::sync_with_stdio(false);
	set_log_level(log_level::info);

	auto f = parse(
	    "2 3 CCSCDC I0 NUMERIC numbers.txt O0 NUMERIC - 10 O2 ASCII -", "", "");
	// std::clog << f.layout();
	assert(f.layout() == R"(2 3
CCS
CDC
I0 NUMERIC numbers.txt
O0 NUMERIC - 10
O2 ASCII -
)");
	// can parse the 'BSC' code used in my spreadsheet as well as the 'CSMD' code
	// used by Phlarx/tis
	// in fact S and M are always interchangeable
	auto f2 = parse(
	    "2 3 BBSBCB I0 NUMERIC numbers.txt O0 NUMERIC - 10 O2 ASCII -", "", "");
	assert(f.layout() == f2.layout());

	// not confused by Bs in IO specs
	auto f3 = parse(
	    "2 3 CCSCDC I0 NUMERIC numbers.txt O0 NUMERIC - 10 O2 ASCII B", "", "");

	assert(f3.layout() == R"(2 3
CCS
CDC
I0 NUMERIC numbers.txt
O0 NUMERIC - 10
O2 ASCII B
)");

	auto code = assemble(R"(B:MOV 0,DOWN
  MOV ACC,DOWN
  SWP#C
  SUB 15
#COM M EN T:

  C:
L: MOV LEFT,DOWN
  MOV LEFT DOWN
  ADD 1#
  JLZ + #
N:+:MOV -1,DOWN
  JRO -999
  JMP C
  NEG)");

	for (const auto& l : layouts) {
		// parse(l[1], "", "");
	}

	if (0) {
		image_t im{30, 18};
		im.write_line(
		    20, 10, {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3});
		std::ofstream ftest("test.pnm");
		ftest << im;
		// pnm::finalize_to_png(im, "test0.png", "", "");
		tis_pixel::color c{tis_pixel::C_dark_grey};
		for (auto& p : im) {
			p = c;
			c = static_cast<tis_pixel::color>(kblib::etoi(c) + 1);
			if (c > tis_pixel::C_red) {
				c = {tis_pixel::C_dark_grey};
			}
		}
		// pnm::finalize_to_png(im, "test1.png", "", "");
	}

	if (argc > 2) {
		auto filename = std::string(argv[2]);
		auto s = kblib::get_file_contents(filename);
		if (not s) {
			std::cerr << "no such file: " << kblib::quoted(filename);
			return -1;
		} else {
			log_debug("source: ", kblib::quoted(*s), "\n");
		}
		auto id = level_id(argv[1]);
		auto test = [&] {
			if (argc > 3 and argv[3] == "random"sv) {
				return random_test(id);
			} else if (argc > 3) {
				return random_test(std::stoi(argv[3]));
			} else {
				return tests[id].data[0];
			}
		}();
		auto l = parse(layouts[id].layout, *s, test);
		auto sc = run(l);

		if (sc.validated) {
			std::cout << "validation successful\n";
			std::cout << "score: " << sc.cycles << '/' << sc.nodes << '/'
			          << sc.instructions << '\n';
		} else {
			std::cout << "validation failed after " << sc.cycles << " cycles\n";
		}
	} else if (argc == 2 and argv[1] == "generate"sv) {
		auto copy = [](auto x) { return x; };
		for (auto [lvl, i] : kblib::enumerate(layouts)) {
			std::clog << std::flush;

			auto base_path = std::filesystem::path(std::filesystem::current_path()
			                                       / lvl.segment);
			log_info("writing cfg: ", copy(base_path).concat(".tiscfg").native());
			std::ofstream layout_f(copy(base_path).concat(".tiscfg"));
			auto l = parse(lvl.layout, "", "");
			std::string layout;
			kblib::search_replace_copy(l.layout(), "@"s,
			                           std::string(lvl.segment) + "@",
			                           std::back_inserter(layout));
			layout_f << layout;
			std::clog << std::flush;

			auto r = random_test(i);
			std::size_t i_idx = 0;
			std::size_t o_idx = 0;
			for (auto it = l.begin() + l.nodes_total(); it != l.end(); ++it) {
				auto n = it->get();
				if (type(n) == node::in) {
					auto p = static_cast<input_node*>(n);
					auto o_path = copy(base_path).concat(p->filename);
					log_info("writing in ", i_idx, ": ", o_path.native());
					std::ofstream o_f(o_path);
					for (auto w : r.inputs[i_idx]) {
						o_f << w << '\n';
					}
					++i_idx;
				} else if (type(n) == node::out) {
					auto p = static_cast<output_node*>(n);
					auto o_path = copy(base_path).concat(p->filename);
					log_info("writing out ", o_idx, ": ", o_path.native());
					std::ofstream o_f(o_path);
					for (auto w : r.n_outputs[o_idx]) {
						o_f << w << '\n';
					}
					++o_idx;
				}
			}
		}
	} else {
		// tiny self-test
		auto l = parse(
		    "1 1 B I0 NUMERIC @0 O0 NUMERIC @5", "@0\nMOV UP,DOWN",
		    single_test{.inputs
		                = {{51, 62, 16, 83, 61, 14, 35, 17, 63, 48, 22, 40, 29,
		                    50, 77, 32, 31, 49, 89, 89, 12, 59, 53, 75, 37, 78,
		                    57, 38, 44, 98, 85, 25, 80, 39, 20, 16, 91, 81, 84}},
		                .n_outputs
		                = {{51, 62, 16, 83, 61, 14, 35, 17, 63, 48, 22, 40, 29,
		                    50, 77, 32, 31, 49, 89, 89, 12, 59, 53, 75, 37, 78,
		                    57, 38, 44, 98, 85, 25, 80, 39, 20, 16, 91, 81, 84}},
		                .i_output = {}});
		auto sc = run(l);

		if (sc.validated) {
			std::cout << "validation successful\n";
			std::cout << "score: " << sc.cycles << '/' << sc.nodes << '/'
			          << sc.instructions << '\n';
		} else {
			std::cout << "validation failed after " << sc.cycles << " cycles\n";
		}
	}

	return 0;
}
