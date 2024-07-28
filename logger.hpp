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
#include <sstream>

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
		detail::log(kblib::concat("WARN: ", strings...));
	}
}

template <typename... Strings>
auto log_err(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::err) {
		detail::log(kblib::concat("ERROR: ", strings...));
	}
}

// Only these are provided because higher priority log messages are rare enough
// not to impact performance
auto log_debug_r(std::invocable<> auto supplier) -> void {
	if (get_log_level() >= log_level::debug) {
		detail::log(kblib::concat("DEBUG: ", supplier()));
	}
}

auto log_info_r(std::invocable<> auto supplier) -> void {
	if (get_log_level() >= log_level::info) {
		detail::log(kblib::concat("INFO: ", supplier()));
	}
}

template <typename T>
concept streamable_out = requires(std::ostream& os, const T& v) {
	os << v;
};

class logger {
 public:
	logger(std::string_view prefix);
	logger(std::nullptr_t)
	    : formatter_{}
	    , log_(nullptr) {}
	logger(logger&& o)
	    : formatter_{std::move(o.formatter_)}
	    , log_(o.log_) {}

	auto log_r(std::invocable<> auto supplier) -> void {
		if (formatter_) {
			auto s = supplier();
			(*formatter_) << s;
		}
	}
	auto log(auto&&... args) -> void {
		if (formatter_) {
			((*formatter_) << ... << args);
		}
	}
	auto operator<<(const streamable_out auto& v) -> logger& {
		if (formatter_) {
			(*formatter_) << v;
		}
		return *this;
	}

	~logger() {
		if (formatter_) {
			(*log_) << formatter_->view() << '\n';
		}
	}

 private:
	std::unique_ptr<std::ostringstream> formatter_;
	std::ostream* log_;
};

inline auto log_debug() {
	return (get_log_level() >= log_level::debug) ? logger("DEBUG: ")
	                                             : logger(nullptr);
}

inline auto log_info() {
	return (get_log_level() >= log_level::info) ? logger("INFO: ")
	                                            : logger(nullptr);
}

inline auto log_notice() {
	return (get_log_level() >= log_level::notice) ? logger("INFO: ")
	                                              : logger(nullptr);
}

inline auto log_warn() {
	return (get_log_level() >= log_level::warn) ? logger("WARNING: ")
	                                            : logger(nullptr);
}

inline auto log_err() {
	return (get_log_level() >= log_level::err) ? logger("ERROR: ")
	                                           : logger(nullptr);
}

#endif // LOGGER_HPP
