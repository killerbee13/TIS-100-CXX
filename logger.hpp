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
inline bool log_is_tty{false};

enum SGR_code {
	none,
	bold = 1,
	underline = 4,
	reverse = 7,
	normal = 22,
	black = 30,
	red,
	green,
	yellow,
	blue,
	magenta,
	cyan,
	white,
	reset_color = 39,
	bg_black = 40,
	bg_red,
	bg_green,
	bg_yellow,
	bg_blue,
	bg_magenta,
	bg_cyan,
	bg_white,
	reset_bg_color = 49,
	bright_black = 90,
	bright_red,
	bright_green,
	bright_yellow,
	bright_blue,
	bright_magenta,
	bright_cyan,
	bright_white,
	// bright black, also known as grey
	bg_bright_black = 90,
	bg_bright_red,
	bg_bright_green,
	bg_bright_yellow,
	bg_bright_blue,
	bg_bright_magenta,
	bg_bright_cyan,
	bg_bright_white,
};

std::string print_escape(SGR_code first,
                         std::same_as<SGR_code> auto... colors) {
	if (not use_color) {
		return {};
	}
	if (first == none and sizeof...(colors) == 0) {
		return "\033[m";
	}
	std::string ret = "\033["
	                  + (std::to_string(static_cast<int>(first)) + ...
	                     + (';' + std::to_string(static_cast<int>(colors))));
	ret += 'm';
	return ret;
}
std::string log_print_escape(SGR_code first,
                             std::same_as<SGR_code> auto... colors) {
	if (log_is_tty) {
		return print_escape(first, colors...);
	} else {
		return {};
	}
}

template <typename... Strings>
auto log_debug(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::debug) {
		detail::log(concat("DEBUG: ", strings...));
	}
}

template <typename... Strings>
auto log_info(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::info) {
		detail::log(concat("INFO: ", strings...));
	}
}

template <typename... Strings>
auto log_notice(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::notice) {
		detail::log(concat("NOTICE: ", strings...));
	}
}

template <typename... Strings>
auto log_warn(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::warn) {
		detail::log(concat(log_print_escape(yellow),
		                   "WARN: ", log_print_escape(none), strings...));
	}
}

template <typename... Strings>
auto log_err(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::err) {
		detail::log(concat(log_print_escape(red),
		                   "ERROR: ", log_print_escape(none), strings...));
	}
}

// Only these are provided because higher priority log messages are rare enough
// not to impact performance
auto log_debug_r(std::invocable<> auto supplier) -> void {
	if (get_log_level() >= log_level::debug) {
		detail::log(concat("DEBUG: ", supplier()));
	}
}

auto log_info_r(std::invocable<> auto supplier) -> void {
	if (get_log_level() >= log_level::info) {
		detail::log(concat("INFO: ", supplier()));
	}
}

template <typename T>
concept streamable_out = requires(std::ostream& os, const T& v) { os << v; };

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
	return (get_log_level() >= log_level::warn)
	           ? logger(log_print_escape(yellow)
	                    + "WARNING: " + log_print_escape(none))
	           : logger(nullptr);
}

inline auto log_err() {
	return (get_log_level() >= log_level::err)
	           ? logger(log_print_escape(yellow)
	                    + "ERROR: " + log_print_escape(none))
	           : logger(nullptr);
}

#endif // LOGGER_HPP
