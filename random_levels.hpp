/* *****************************************************************************
 * %{QMAKE_PROJECT_NAME}
 * Copyright (c) %YEAR% killerbee
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
#ifndef RANDOM_LEVELS_HPP
#define RANDOM_LEVELS_HPP

#include "node.hpp"

std::array<single_test, 3> static_suite(int id);
std::optional<single_test> random_test(int id, std::uint32_t seed);

#if HISTOGRAM
inline std::array<std::vector<std::size_t>, 9> histogram;
#endif

#endif // RANDOM_LEVELS_HPP
