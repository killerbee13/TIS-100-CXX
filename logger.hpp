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
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "kblib/stringops.h"

namespace detail {
auto log(std::string_view str) -> void;
} // namespace detail

enum class log_level {
	silent = 0,
	err = 1,
	warn = 2,
	notice = 3,
	info = 4,
	debug = 5,
};

auto set_log_level(log_level) -> void;
auto get_log_level() -> log_level;

auto set_log_output(std::ostream&) -> void;
auto set_log_output(const std::ostream&&) -> void = delete;

auto log_flush() -> void;
auto log_flush(bool do_flush) -> void;

inline bool use_color{false};

enum color {
	none,
	black = 30,
	red,
	green,
	yellow,
	blue,
	magenta,
	cyan,
	white,
	reset = 39,
	bright_black = 90,
	bright_red,
	bright_green,
	bright_yellow,
	bright_blue,
	bright_magenta,
	bright_cyan,
	bright_white,
};

inline std::string print_color(color fg, color bg = none) {
	if (not use_color) {
		return {};
	}
	std::string ret = "\033[";
	switch (fg) {
	case none:
		break;
	case black:
	case red:
	case green:
	case yellow:
	case blue:
	case magenta:
	case cyan:
	case white:
	case reset:
	case bright_black:
	case bright_red:
	case bright_green:
	case bright_yellow:
	case bright_blue:
	case bright_magenta:
	case bright_cyan:
	case bright_white:
		ret += std::to_string(static_cast<int>(fg));
		break;
	}
	switch (bg) {
	case none:
		break;
	case black:
	case red:
	case green:
	case yellow:
	case blue:
	case magenta:
	case cyan:
	case white:
	case reset:
	case bright_black:
	case bright_red:
	case bright_green:
	case bright_yellow:
	case bright_blue:
	case bright_magenta:
	case bright_cyan:
	case bright_white:
		ret += ';';
		ret += std::to_string(static_cast<int>(fg) + 10);
		break;
	}
	ret += 'm';
	return ret;
}

template <typename... Strings>
auto log_debug(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::debug) {
		detail::log(kblib::concat("DEBUG: ", strings...));
	}
}

template <typename... Strings>
auto log_info(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::info) {
		detail::log(kblib::concat("INFO: ", strings...));
	}
}

template <typename... Strings>
auto log_notice(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::notice) {
		detail::log(kblib::concat("NOTICE: ", strings...));
	}
}

template <typename... Strings>
auto log_warn(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::warn) {
		detail::log(kblib::concat(print_color(yellow),
		                          "WARN: ", print_color(reset), strings...));
	}
}

template <typename... Strings>
auto log_err(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::err) {
		detail::log(kblib::concat(print_color(red), "ERROR: ", print_color(reset),
		                          strings...));
	}
}

auto log_debug(std::invocable<> auto supplier) -> void {
	if (get_log_level() >= log_level::debug) {
		detail::log(kblib::concat("DEBUG: ", supplier()));
	}
}

auto log_info(std::invocable<> auto supplier) -> void {
	if (get_log_level() >= log_level::info) {
		detail::log(kblib::concat("INFO: ", supplier()));
	}
}

#endif // LOGGER_HPP
