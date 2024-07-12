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

#include "node.hpp"
#include "parser.hpp"
#include <cassert>
#include <iostream>

#include <kblib/stringops.h>

using namespace std::literals;
int main() {
	auto f = parse(
	    "2 3 CCSCCC I0 NUMERIC numbers.txt O0 NUMERIC - 10 O2 ASCII -", "", "");
	std::cout << f.layout();
	assert(f.layout() == R"(2 3
CCS
CCC
I0 NUMERIC numbers.txt
O0 NUMERIC - 10
O2 ASCII -
)");

	auto code = assemble(R"(B:MOV 0,DOWN
  MOV ACC,DOWN
  SWP
  SUB 15
#COM M EN T:

C:
L: MOV LEFT,DOWN
  MOV LEFT,DOWN
  ADD 1#
  JLZ L #
N:A:MOV -1,DOWN
  SWP
  ADD 1
  NEG)");

	return 0;
}
