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

#include <string>

struct input_node : node {
	using node::node;
	type_t type() const noexcept override { return in; }
	void step() override {}
	void finalize() override {
		if (writing) {
			// writing this turn
			s = activity::write;
			writing = false;
		} else {
			s = activity::idle;
			// ready a value if we don't have one
			if (wrt == word_empty and idx != inputs.size()) {
				log_debug("I: ", wrt != word_empty, ',', idx != inputs.size());
				wrt = inputs[idx++];
			}
		}
	}
	void reset() noexcept override {
		idx = 0;
		wrt = word_empty;
		s = activity::idle;
	}
	std::unique_ptr<node> clone() const override {
		auto ret = std::make_unique<input_node>(x, y);
		ret->inputs = inputs;
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
	std::size_t idx{};
	optional_word wrt = word_empty;
	activity s{activity::idle};

 private:
	bool writing{};
};
struct output_node : node {
	using node::node;
	type_t type() const noexcept override { return out; }
	// Attempt to read from neighbor every step
	void step() override {
		if (complete) {
			return;
		}
		if (auto r = do_read(neighbors[port::up], port::down); r != word_empty) {
			log_debug("O", x, ": read");
			auto i = outputs_received.size();
			outputs_received.push_back(r);
			if (outputs_received[i] != outputs_expected[i]) {
				wrong = true;
				log_debug("incorrect value written");
			}
			complete = (outputs_expected.size() == outputs_received.size());
		}
	}
	void finalize() override {}
	void reset() noexcept override {
		outputs_received.clear();
		wrong = false;
		complete = false;
	}
	std::unique_ptr<node> clone() const override {
		auto ret = std::make_unique<output_node>(x, y);
		ret->outputs_expected = outputs_expected;
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
	char d{' '};
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

	type_t type() const noexcept override { return image; }
	void step() override {
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
	void finalize() override {}
	void reset() noexcept override {
		image_received.fill(tis_pixel::C_black);
		c_x = word_empty;
		c_y = word_empty;
	}
	std::unique_ptr<node> clone() const override {
		auto ret = std::make_unique<image_output>(x, y);
		ret->image_expected = image_expected;
		return ret;
	}
	optional_word emit(port) override { return word_empty; }
	std::string state() const override {
		return concat("O", x, " IMAGE {\n", image_received.write_text(), "}");
	}

	void poke(tis_pixel p) {
		if (c_x != word_empty and c_y != word_empty and c_x < width
		    and c_y < height) {
			image_received.at(c_x, c_y) = p;
		}
	}
	image_t image_expected;
	image_t image_received;
	std::ptrdiff_t width{};
	std::ptrdiff_t height{};
	optional_word c_x = word_empty;
	optional_word c_y = word_empty;
};

#endif // IO_HPP
