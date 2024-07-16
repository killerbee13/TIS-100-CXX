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
#include "node.hpp"
#include "parser.hpp"
#include <cassert>
#include <iostream>

#include <kblib/stringops.h>

using namespace std::literals;
int main() {
	auto f = parse(
	    "2 3 CCSCDC I0 NUMERIC numbers.txt O0 NUMERIC - 10 O2 ASCII -", "", "");
	std::clog << f.layout();
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
		parse(l[1], "", "");
	}

	image_t im{30, 18};
	im.write_line(20, 10,
	              {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3});
	std::ofstream ftest("test.pnm");
	ftest << im;
	pnm::finalize_to_png(im, "test0.png", "", "");
	tis_pixel::color c{tis_pixel::C_dark_grey};
	for (auto& p : im) {
		p = c;
		c = static_cast<tis_pixel::color>(kblib::etoi(c) + 1);
		if (c > tis_pixel::C_red) {
			c = {tis_pixel::C_dark_grey};
		}
	}
	pnm::finalize_to_png(im, "test1.png", "", "");

	return 0;
}
