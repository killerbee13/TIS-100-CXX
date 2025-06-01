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
#ifndef T30_HPP
#define T30_HPP

#include "logger.hpp"
#include "node.hpp"
#include "utils.hpp"

struct T30 final : regular_node {
	T30(int x, int y, std::size_t max_size)
	    : regular_node(x, y, type_t::T30)
	    , max_size(max_size) {
		data.reserve(max_size);
	}
	void reset() noexcept {
		write_word = word_empty;
		write_port = port::any;
		data.clear();
		prev_end = data.end();
	}

	inline void step(logger&) {
		if (data.size() == max_size) {
			return;
		}
		for (auto p = port::dir_first; p <= port::dir_last; p++) {
			if (auto r = do_read(p); r != word_empty) {
				data.push_back(r);
				used = true;
				if (data.size() == max_size) {
					break;
				}
			}
		}
	}
	inline void finalize(logger&) {
		if (write_port != port::any) {
			data.erase(prev_end);
			write_port = port::any;
		}
		if (not data.empty()) {
			prev_end = data.end() - 1;
			write_word = data.back();
		}
	}
	std::unique_ptr<regular_node> clone() const override {
		return std::make_unique<T30>(x, y, max_size);
	}
	std::string state() const override {
		std::string ret = concat('(', x, ',', y, ") T30 {");
		for (auto w : data) {
			append(ret, w, ", ");
		}
		append(ret, '}');
		return ret;
	}

	bool used{}; // persistent among all tests

 private:
	word_vec data;
	word_vec::iterator prev_end;
	std::size_t max_size{def_T30_size};
};

#endif // T30_HPP
