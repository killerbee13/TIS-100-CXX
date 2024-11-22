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
#include "logger.hpp"

#include <iostream>
#include <mutex>

static log_level current = log_level::notice;
static std::ostream* output = &std::clog;
std::recursive_mutex log_m;

auto set_log_level(log_level new_level) -> void { current = new_level; }

auto get_log_level() -> log_level { return current; }

namespace detail {

static bool flush;

auto log(std::string_view str) -> void {
	std::unique_lock l(log_m);
	(*output) << str << '\n';
	if (flush) {
		output->flush();
	}
}

} // namespace detail

auto log_flush() -> void {
	std::unique_lock l(log_m);
	output->flush();
}

auto set_log_flush(bool do_flush) -> void { detail::flush = do_flush; }

logger::logger(std::string_view prefix)
    : formatter_{std::make_unique<std::ostringstream>()} {
	(*formatter_) << prefix;
}
