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
#include "logger.hpp"

#include <iostream>

static log_level current = log_level::notice;
static std::ostream* output = &std::clog;
static bool flush;

auto set_log_level(log_level new_level) -> void { current = new_level; }

auto get_log_level() -> log_level { return current; }

auto set_log_output(std::ostream& os) -> void { output = &os; }

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
    : detail::stringbuf_container{std::make_unique<std::stringbuf>()}
    , std::ostream{data_.get()}
    , log_{output} {
	do_write(prefix);
}
