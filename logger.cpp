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
#include <unistd.h>

static log_level current = log_level::notice;
static std::ostream* output = &std::clog;
static bool flush;

auto set_log_level(log_level new_level) -> void { current = new_level; }

auto get_log_level() -> log_level { return current; }

auto set_log_output(std::ostream& os) -> void {
	output = &os;
	// This is kinda hacky but should generally work.
	// This function is never used anyway.
	if (&os == &std::cout) {
		log_is_tty = isatty(STDOUT_FILENO);
	} else if (&os == &std::cerr or &os == &std::clog) {
		log_is_tty = isatty(STDERR_FILENO);
	}
}

auto log_flush() -> void { output->flush(); }

auto log_flush(bool do_flush) -> void { flush = do_flush; }

namespace detail {

auto log(std::string_view str) -> void {
	(*output) << str << '\n';
	if (flush) {
		output->flush();
	}
}

} // namespace detail

logger::logger(std::string_view prefix)
    : formatter_{std::make_unique<std::ostringstream>()}
    , log_{output} {
	(*formatter_) << prefix;
}
