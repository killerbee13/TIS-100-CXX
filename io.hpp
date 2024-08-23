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

struct input_node : node {
	using node::node;

	void reset(word_vec inputs_) noexcept {
		inputs = inputs_;
		idx = 0;
		wrt = word_empty;
		s = activity::idle;
	}

	type_t type() const noexcept override { return in; }
	void step(logger&) override {}
	void finalize(logger& debug) override {
		debug << "I" << x << ": ";
		if (writing) {
			// writing this turn
			s = activity::write;
			writing = false;
			debug << "writing";
		} else {
			s = activity::idle;
			// ready a value if we don't have one
			if (wrt == word_empty and idx != inputs.size()) {
				debug << "reloading";
				wrt = inputs[idx++];
			} else {
				debug << "waiting";
			}
		}
		debug << '\n';
	}
	std::unique_ptr<node> clone() const override {
		auto ret = std::make_unique<input_node>(x, y);
		ret->reset(inputs);
		return ret;
	}
	optional_word emit(port) override {
		writing = wrt != word_empty;
		return std::exchange(wrt, word_empty);
	}
	std::string state() const override {
		return concat("I", x, " NUMERIC { ", state_name(s), " emitted:(", idx,
		              "/", inputs.size(), ") }");
	}

	word_vec inputs;

 private:
	std::size_t idx{};
	optional_word wrt = word_empty;
	activity s{activity::idle};
	bool writing{};
};

struct output_node : node {
	using node::node;

	void reset(word_vec outputs_expected_) {
		outputs_expected = outputs_expected_;
		outputs_received.clear();
		wrong = false;
		complete = outputs_expected_.empty();
	}

	type_t type() const noexcept override { return out; }
	// Attempt to read from neighbor every step
	void step(logger& debug) override {
		if (complete) {
			return;
		}
		if (auto r = do_read(neighbors[port::up], port::down); r != word_empty) {
			debug << "O" << x << ": read";
			auto i = outputs_received.size();
			outputs_received.push_back(r);
			if (outputs_received[i] != outputs_expected[i]) {
				wrong = true;
				debug << "incorrect value written";
			}
			complete = (outputs_expected.size() == outputs_received.size());
			debug << '\n';
		}
	}
	void finalize(logger&) override {}
	std::unique_ptr<node> clone() const override {
		auto ret = std::make_unique<output_node>(x, y);
		ret->reset(outputs_expected);
		return ret;
	}
	optional_word emit(port) override { return word_empty; }
	std::string state() const override {
		std::ostringstream ret;
		ret << concat("O", x, " NUMERIC {\nreceived:");
		write_list(ret, outputs_received, &outputs_expected);
		ret << '}';
		return std::move(ret).str();
	}

	word_vec outputs_expected;
	word_vec outputs_received;
	bool wrong{false};
	bool complete{false};
};

struct image_output : node {
	using node::node;

	constexpr void reshape(std::ptrdiff_t w, std::ptrdiff_t h) {
		image_expected.reshape(w, h);
		image_received.reshape(w, h);
		width = w;
		height = h;
	}
	void reset(image_t image_expected_) {
		image_expected = image_expected_;
		image_received.fill(tis_pixel::C_black);
		wrong_pixels = std::ranges::count_if(
		    image_expected, [](auto pix) { return pix != tis_pixel::C_black; });
		c_x = word_empty;
		c_y = word_empty;
	}

	type_t type() const noexcept override { return image; }
	void step(logger&) override {
		if (auto r = do_read(neighbors[port::up], port::down); r != word_empty) {
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
	}
	void finalize(logger&) override {}
	std::unique_ptr<node> clone() const override {
		auto ret = std::make_unique<image_output>(x, y);
		ret->reshape(width, height);
		ret->reset(image_expected);
		return ret;
	}
	optional_word emit(port) override { return word_empty; }
	std::string state() const override {
		return concat("O", x, " IMAGE { wrong: ", wrong_pixels, "\n",
		              image_received.write_text(), "}");
	}

	image_t image_expected;
	image_t image_received;
	std::size_t wrong_pixels;
	std::ptrdiff_t width{};
	std::ptrdiff_t height{};

 private:
	void poke(tis_pixel pix_new) {
		if (c_x != word_empty and c_y != word_empty and c_x < width
		    and c_y < height) {
			auto& pix_rec = image_received.at(c_x, c_y);
			auto& pix_exp = image_expected.at(c_x, c_y);
			if (pix_rec == pix_exp) {
				wrong_pixels++;
			}
			if (pix_new == pix_exp) {
				wrong_pixels--;
			}
			pix_rec = pix_new;
		}
	}

	optional_word c_x = word_empty;
	optional_word c_y = word_empty;
};

#endif // IO_HPP
