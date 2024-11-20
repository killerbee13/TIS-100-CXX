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

#include "utils.hpp"

#include "kblib/stringops.h"
#include <sstream>

using kblib::concat;

namespace detail {
auto log(std::string_view str) -> void;
} // namespace detail

enum class log_level {
	silent,
	err,
	warn,
	notice,
	info,
	trace,
	debug,
};

auto set_log_level(log_level) -> void;
auto get_log_level() -> log_level;

auto log_flush() -> void;
auto set_log_flush(bool do_flush) -> void;

template <typename... Strings>
auto log_debug([[maybe_unused]] Strings&&... strings) -> void {
#if TIS_ENABLE_DEBUG
	if (get_log_level() >= log_level::debug) {
		detail::log(concat("DEBUG: ", strings...));
	}
#endif
}
template <typename... Strings>
auto log_trace(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::trace) {
		detail::log(concat("TRACE: ", strings...));
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
auto log_debug_r([[maybe_unused]] std::invocable<> auto supplier) -> void {
#if TIS_ENABLE_DEBUG
	if (get_log_level() >= log_level::debug) {
		detail::log(concat("DEBUG: ", supplier()));
	}
#endif
}
auto log_trace_r(std::invocable<> auto supplier) -> void {
	if (get_log_level() >= log_level::trace) {
		detail::log(concat("TRACE: ", supplier()));
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
	    : formatter_{} {}
	logger(logger&& o)
	    : formatter_{std::move(o.formatter_)} {}

	auto log_r(std::invocable<> auto supplier) -> void {
		if (formatter_) [[unlikely]] {
			auto s = supplier();
			(*formatter_) << s;
		}
	}
	auto log(auto&&... args) -> void {
		if (formatter_) [[unlikely]] {
			((*formatter_) << ... << args);
		}
	}
	auto operator<<(const streamable_out auto& v) -> logger& {
		if (formatter_) [[unlikely]] {
			(*formatter_) << v;
		}
		return *this;
	}

	bool good() const { return bool(formatter_); }

	~logger() {
		if (formatter_) [[unlikely]] {
			detail::log(formatter_->view());
		}
	}

 private:
	std::unique_ptr<std::ostringstream> formatter_;
};

inline auto log_debug() {
#if TIS_ENABLE_DEBUG
	if (get_log_level() >= log_level::debug) {
		return logger("DEBUG: ");
	}
#endif
	return logger(nullptr);
}
inline auto log_trace() {
	return (get_log_level() >= log_level::trace) ? logger("TRACE: ")
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
