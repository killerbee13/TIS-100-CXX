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
#ifndef IO_HPP
#define IO_HPP

#include "image.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "utils.hpp"

#include <algorithm>
#include <string>

struct input_node final : node {
	using node::node;

	void reset(word_vec inputs_) noexcept {
		write_word = word_empty;
		write_port = port::down;
		inputs = std::move(inputs_);
		idx = 0;
		s = activity::idle;
	}

	type_t type() const noexcept override { return in; }
	[[gnu::always_inline]] inline void execute(logger& debug) {
		debug << "I" << x << ": ";
		if (write_port == port::nil) {
			// writing this turn
			s = activity::write;
			write_port = port::down;
			debug << "writing";
		} else {
			s = activity::idle;
			// ready a value if we don't have one
			if (write_word == word_empty and idx != inputs.size()) {
				debug << "reloading";
				write_word = inputs[idx++];
			} else {
				debug << "waiting";
			}
		}
		debug << '\n';
	}
	std::unique_ptr<input_node> clone() const {
		auto ret = std::make_unique<input_node>(x, y);
		ret->reset(inputs);
		return ret;
	}
	std::string state() const override {
		return concat("I", x, " NUMERIC { ", state_name(s), " emitted:(", idx,
		              "/", inputs.size(), ") }");
	}

	word_vec inputs;

 private:
	std::size_t idx{};
	activity s{activity::idle};
};

struct output_node : node {
	using node::node;
	/// Always not null if the node is simulated
	node* linked;
};

struct num_output final : output_node {
	using output_node::output_node;

	void reset(word_vec outputs_expected_) {
		outputs_expected = std::move(outputs_expected_);
		outputs_received.clear();
		wrong = false;
		complete = outputs_expected.empty();
	}

	type_t type() const noexcept override { return out; }
	/// Attempt to read from neighbor every step
	/// @returns is_active
	[[gnu::always_inline]] inline bool execute(logger& debug) {
		if (complete) {
			return false;
		}
		if (auto r = linked->emit(port::down); r != word_empty) {
			debug << "O" << x << ": read\n";
			auto i = outputs_received.size();
			outputs_received.push_back(r);
			complete = (outputs_expected.size() == outputs_received.size());
			if (outputs_received[i] != outputs_expected[i]) {
				wrong = true;
				debug << "incorrect value written\n";
// speed up simulator by failing early when an incorrect output is written
#if RELEASE
				return false;
#endif
			}
		}
		return not complete;
	}
	[[gnu::always_inline]] inline bool valid() const { return complete and not wrong; }
	/// Return a new node initialized in the same way as *this.
	/// (Not a copy constructor; new node is as if reset() and has no neighbors)
	std::unique_ptr<num_output> clone() const {
		auto ret = std::make_unique<num_output>(x, y);
		ret->reset(outputs_expected);
		return ret;
	}
	std::string state() const override {
		std::ostringstream ret;
		ret << concat("O", x, " NUMERIC {\nreceived:");
		write_list(ret, outputs_received, &outputs_expected);
		ret << '}';
		return std::move(ret).str();
	}

	word_vec outputs_expected;
	word_vec outputs_received;

 private:
	bool wrong{false};
	bool complete{false};
};

struct image_output final : output_node {
	using output_node::output_node;

	void reset(image_t image_expected_) {
		image_expected = std::move(image_expected_);
		width = image_expected.width();
		height = image_expected.height();
		image_received.reshape(width, height);
		image_received.fill(tis_pixel::C_black);
		wrong_pixels = std::ranges::count_if(
		    image_expected, [](auto pix) { return pix != tis_pixel::C_black; });
		c_x = word_empty;
		c_y = word_empty;
	}

	type_t type() const noexcept override { return image; }
	/// @returns is_active
	[[gnu::always_inline]] inline bool execute(logger&) {
		if (auto r = linked->emit(port::down); r != word_empty) {
			if (r < 0) {
				c_x = word_empty;
				c_y = word_empty;
			} else if (c_x == word_empty) {
				c_x = r;
			} else if (c_y == word_empty) {
				c_y = r;
			} else {
				poke(r);
				++c_x;
			}
		}
		return bool(wrong_pixels);
	}
	[[gnu::always_inline]] inline bool valid() const { return not wrong_pixels; }
	/// Return a new node initialized in the same way as *this.
	/// (Not a copy constructor; new node is as if reset() and has no neighbors)u
	std::unique_ptr<image_output> clone() const {
		auto ret = std::make_unique<image_output>(x, y);
		ret->reset(image_expected);
		return ret;
	}
	std::string state() const override {
		return concat("O", x, " IMAGE { wrong: ", wrong_pixels, "\n",
		              image_received.write_text(), "}");
	}

	image_t image_expected;
	image_t image_received;
	std::ptrdiff_t width{};
	std::ptrdiff_t height{};

 private:
	void poke(tis_pixel pix_new) {
		if (c_x < width and c_y < height) {
			auto& pix_rec = image_received[c_x, c_y];
			auto& pix_exp = image_expected[c_x, c_y];
			if (pix_rec == pix_exp) {
				wrong_pixels++;
			}
			if (pix_new == pix_exp) {
				wrong_pixels--;
			}
			pix_rec = pix_new;
		}
	}

	std::size_t wrong_pixels;
	optional_word c_x = word_empty;
	optional_word c_y = word_empty;
};

#endif // IO_HPP
