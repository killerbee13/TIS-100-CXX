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
		detail::log(kblib::concat("WARN: ", strings...));
	}
}

template <typename... Strings>
auto log_err(Strings&&... strings) -> void {
	if (get_log_level() >= log_level::err) {
		detail::log(kblib::concat("ERROR: ", strings...));
	}
}

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

class stringrefbuf : public std::streambuf {
 public:
	stringrefbuf(std::string* s)
	    : str(s) {}
	stringrefbuf(std::nullptr_t)
	    : str(nullptr) {}

 protected:
	std::streamsize xsputn(const char_type* s, std::streamsize count) override {
		if (str) {
			str->append(s, kblib::to_unsigned(count));
		}
		return count;
	}
	int_type overflow(int_type ch = traits_type::eof()) override {
		if (not traits_type::eq_int_type(ch, traits_type::eof()) and str) {
			str->push_back(static_cast<char>(ch));
		}
		return 0;
	}

 private:
	std::string* str;
};

class logger : public std::ostream {
 public:
	logger(std::string prefix, bool do_log)
	    : std::ostream(&buf_)
	    , data_(std::move(prefix))
	    , do_log_(do_log)
	    , buf_((do_log) ? &data_ : nullptr)
	    , prefix_len_(data_.size()) {}

	auto log_r(std::invocable<> auto supplier) -> void {
		if (do_log_) {
			kblib::append(data_, supplier());
		}
	}
	auto log(const auto&... args) -> void {
		if (do_log_) {
			kblib::append(data_, args...);
		}
	}

	~logger() {
		if (do_log_) {
			data_.push_back('\n');
			detail::log(data_);
		}
	}

 private:
	std::string data_;
	bool do_log_;
	stringrefbuf buf_;
	// stores the length of the prefix, so that the logger can start a new line
	// after finalizing
	std::size_t prefix_len_;
};

inline auto log_debug() {
	return logger("DEBUG: ", get_log_level() >= log_level::debug);
}

inline auto log_info() {
	return logger("INFO: ", get_log_level() >= log_level::info);
}

inline auto log_notice() {
	return logger("INFO: ", get_log_level() >= log_level::notice);
}

inline auto log_warn() {
	return logger("WARNING: ", get_log_level() >= log_level::warn);
}

inline auto log_err() {
	return logger("ERROR: ", get_log_level() >= log_level::err);
}

#endif // LOGGER_HPP
