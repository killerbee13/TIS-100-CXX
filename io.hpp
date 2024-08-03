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
#ifndef IO_HPP
#define IO_HPP

#include "logger.hpp"
#include "node.hpp"
#include <string>
#include <vector>

enum io_type_t { numeric, ascii, list };

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
			if (not wrt and idx != inputs.size()) {
				log_debug("I: ", not wrt, ',', idx != inputs.size());
				wrt.emplace(inputs[idx++]);
			}
		}
	}
	void reset() noexcept override {
		idx = 0;
		wrt.reset();
		s = activity::idle;
	}
	std::optional<word_t> emit(port) override {
		writing = bool(wrt);
		return std::exchange(wrt, std::nullopt);
	}
	std::string print() const override {
		return concat("I", x, " NUMERIC { ", state_name(s), " emitted:(", idx,
		              "/", inputs.size(), ") }");
	}

	word_vec inputs;
	std::size_t idx{};
	std::string filename{};
	io_type_t io_type{};
	std::optional<word_t> wrt;
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
		if (auto r = do_read(neighbors[port::up], port::down)) {
			log_debug("O", x, ": read");
			auto i = outputs_received.size();
			outputs_received.push_back(*r);
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
	std::optional<word_t> emit(port) override { return std::nullopt; }
	std::string print() const override {
		std::ostringstream ret;
		ret << concat("O", x, " NUMERIC {\nreceived:");
		write_list(ret, outputs_received, &outputs_expected,
		           use_color and log_is_tty);
		ret << '}';
		return std::move(ret).str();
	}
	word_vec outputs_expected;
	word_vec outputs_received;
	std::string filename;
	io_type_t io_type;
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
		if (auto r = do_read(neighbors[port::up], port::down)) {
			if (r < 0) {
				c_x.reset();
				c_y.reset();
			} else if (not c_x) {
				c_x = r;
			} else if (not c_y) {
				c_y = r;
			} else {
				if (r > 4) {
					r = 0;
				}
				poke(*r);
				++*c_x;
			}
		}
	}
	void finalize() override {}
	void reset() noexcept override {
		image_received.fill(tis_pixel::C_black);
		c_x.reset();
		c_y.reset();
	}
	std::optional<word_t> emit(port) override { return std::nullopt; }
	std::string print() const override {
		return concat("O", x, " IMAGE {\n", image_received.write_text(), "}");
	}

	void poke(tis_pixel p) {
		if (c_x and c_y and c_x < width and c_y < height) {
			image_received.at(*c_x, *c_y) = p;
		}
	}
	image_t image_expected;
	image_t image_received;
	std::string filename;
	std::ptrdiff_t width;
	std::ptrdiff_t height;
	std::optional<word_t> c_x;
	std::optional<word_t> c_y;
};

#endif // IO_HPP
