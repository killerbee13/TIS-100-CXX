/* *****************************************************************************
 * TIX-100-CXX
 * Copyright (c) 2024 killerbee
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
#ifndef NODE_HPP
#define NODE_HPP

#include "image.hpp"
#include "logger.hpp"

#include <array>
#include <cstdint>
#include <vector>

using word_t = std::int16_t;
using index_t = std::int16_t;

constexpr inline int def_T21_size = 15;
constexpr inline int def_T30_size = 15;

enum port : std::int8_t {
	up,
	left,
	right,
	down,
	D5, // 3D expansion
	D6,
	nil,
	acc,
	any,
	last,
	immediate = -1
};

constexpr port invert(port p) {
	switch (p) {
	case up:
		return down;
	case left:
		return right;
	case right:
		return left;
	case down:
		return up;
	case D5:
		return D6;
	case D6:
		return D5;
	case nil:
	case acc:
	case any:
	case last:
	case immediate:
		throw std::invalid_argument{""};
	}
}

struct node {
 public:
	enum type_t { T21 = 1, T30, in, out, image, Damaged = -1 };
	enum class activity { idle, run, read, write };
	constexpr static std::string_view state_name(activity s) {
		switch (s) {
		case activity::idle:
			return "IDLE";
		case activity::run:
			return "RUN";
		case activity::read:
			return "READ";
		case activity::write:
			return "WRTE";
			break;
		}
	}
	constexpr static std::string_view port_name(port p) {
		switch (p) {
		case up:
			return "UP";
		case left:
			return "LEFT";
		case right:
			return "RIGHT";
		case down:
			return "DOWN";
		case D5:
			return "D5";
		case D6:
			return "D6";
		case nil:
			return "NIL";
		case acc:
			return "ACC";
		case any:
			return "ANY";
		case last:
			return "LAST";
		case immediate:
			return "VAL";
			break;
		}
	}

	virtual type_t type() const noexcept = 0;
	virtual bool step() = 0;
	virtual bool finalize() = 0;
	virtual void reset() noexcept = 0;

	virtual std::optional<word_t> read_(port p) = 0;
	virtual std::string print() const = 0;

	// damaged and stack nodes are always idle, so this default simplifies them
	virtual activity state() const noexcept { return activity::idle; };

	std::array<node*, 6> neighbors{};
	int x{};
	int y{};

	node() = default;
	node(int x, int y) noexcept
	    : x(x)
	    , y(y) {}
	virtual ~node() = default;

	friend node::type_t type(const node* n) {
		if (n) {
			return n->type();
		} else {
			return node::Damaged;
		}
	}

	friend bool valid(const node* n) { return n and n->type() != node::Damaged; }
	friend std::optional<word_t> do_read(node* n, port p) {
		assert(p >= port::up and p <= port::D6);
		if (not valid(n)) {
			return std::nullopt;
		} else {
			return n->read_(p);
		}
	}

 protected: // prevents most slicing
	node(const node&) = default;
	node(node&&) = default;
	node& operator=(const node&) = default;
	node& operator=(node&&) = default;
};

struct damaged : node {
	using node::node;
	type_t type() const noexcept override { return Damaged; }
	bool step() override { return false; }
	bool finalize() override { return false; }
	void reset() noexcept override {}
	std::optional<word_t> read_(port) override { return std::nullopt; }
	std::string print() const override {
		return kblib::concat("(", x, ',', y, ") {Damaged}");
	}
};

struct tis_pixel {
	enum color : unsigned char {
		C_black,
		C_dark_grey,
		C_light_grey,
		C_white,
		C_red
	} val{};
	static constexpr std::array<pnm::rgb_pixel, 5> colors{
	    pnm::color_from_hex("000000"), pnm::color_from_hex("464646"),
	    pnm::color_from_hex("9C9C9C"), pnm::color_from_hex("FDFDFD"),
	    pnm::color_from_hex("C10B0B"),
	};

	tis_pixel() = default;
	constexpr tis_pixel(color c) noexcept
	    : val(c) {}
	explicit(false) constexpr tis_pixel(std::integral auto c) noexcept
	    : val(static_cast<color>(c)) {}

	pnm::rgb_pixel as_rgb() const { return colors[val]; }
	constexpr operator pnm::rgb_pixel() const { return colors[val]; }
	constexpr operator pnm::rgba_pixel() const {
		return static_cast<pnm::rgba_pixel>(colors[val]);
	}

	constexpr std::uint8_t& operator[](std::size_t i) noexcept {
		return as_rgb()[i];
	}
	constexpr const std::uint8_t& operator[](std::size_t i) const noexcept {
		return as_rgb()[i];
	}

	constexpr std::uint8_t& red() noexcept { return as_rgb()[0]; }
	constexpr const std::uint8_t& red() const noexcept { return as_rgb()[0]; }
	constexpr std::uint8_t& green() noexcept { return as_rgb()[1]; }
	constexpr const std::uint8_t& green() const noexcept { return as_rgb()[1]; }
	constexpr std::uint8_t& blue() noexcept { return as_rgb()[2]; }
	constexpr const std::uint8_t& blue() const noexcept { return as_rgb()[2]; }

	// std::pow is not usable in constexpr, so this has to be precomputed
	static constexpr std::array<std::uint8_t, 5> values{0, 142, 205, 254, 122};
	constexpr std::uint8_t value() const noexcept { return values[val]; }

	static auto& header(std::ostream& os, std::ptrdiff_t width,
	                    std::ptrdiff_t height) noexcept {
		return os << "P6" << ' ' << width << ' ' << height << " 255\n";
	}
	static std::filesystem::path extension() { return ".ppm"; }
	friend std::ostream& operator<<(std::ostream& os, const tis_pixel& px) {
		return os << px.as_rgb();
	}
	constexpr static bool is_binary_writable = false;
	constexpr friend std::strong_ordering operator<=>(tis_pixel, tis_pixel)
	    = default;
};

struct image_t : pnm::image<tis_pixel> {
	using image::image;
	using enum tis_pixel::color;
	constexpr image_t() = default;
	image_t(std::ptrdiff_t width, std::ptrdiff_t height, tis_pixel value)
	    : image(width, height, value) {}
	image_t(std::ptrdiff_t width, std::ptrdiff_t height, int value)
	    : image(width, height, value) {}
	image_t(std::ptrdiff_t width, std::ptrdiff_t height,
	        std::initializer_list<tis_pixel> contents)
	    : image(width, height, contents) {}
	image_t(std::ptrdiff_t width, std::ptrdiff_t height,
	        std::initializer_list<int> contents)
	    : image(width, height) {
		assert(contents.size() == size());
		std::copy(contents.begin(), contents.end(), begin());
	}
	image_t(std::initializer_list<std::u16string_view> image) { assign(image); }

	constexpr static std::u16string_view key = u" ░▒█#";
	constexpr static std::array<std::string_view, 5> rkey
	    = {" ", "░", "▒", "█", "#"};

	void write_line(word_t x, word_t y, const std::vector<word_t>& vals) {
		x = std::max(x, word_t{});
		y = std::max(y, word_t{});
		auto l = std::min(kblib::to_signed(vals.size()), width() - x);
		std::copy_n(vals.begin(), l, begin() + y * width() + x);
	}

	using image::assign;
	constexpr void assign(std::u16string_view image,
	                      std::u16string_view key_ = key) {
		assert(image.size() == size());
		auto it = begin();
		for (auto px : image) {
			if (px == key_[0]) {
				*it++ = {C_black};
			} else if (px == key_[1]) {
				*it++ = {C_dark_grey};
			} else if (px == key_[2]) {
				*it++ = {C_light_grey};
			} else if (px == key_[3]) {
				*it++ = {C_white};
			} else if (px == key_[4]) {
				*it++ = {C_red};
			}
		}
	}

	constexpr void assign(std::initializer_list<std::u16string_view> image,
	                      std::u16string_view key_ = key) {
		if (image.size() == 0) {
			reshape(0, 0);
		}
		std::size_t w{image.begin()[0].size()};
		for (auto line : image) {
			assert(line.size() == w);
		}
		reshape(kblib::to_signed(w), kblib::to_signed(image.size()));
		auto it = begin();
		for (auto line : image) {
			for (auto px : line) {
				if (px == key_[0]) {
					*it++ = {C_black};
				} else if (px == key_[1]) {
					*it++ = {C_dark_grey};
				} else if (px == key_[2]) {
					*it++ = {C_light_grey};
				} else if (px == key_[3]) {
					*it++ = {C_white};
				} else if (px == key_[4]) {
					*it++ = {C_red};
				}
			}
		}
	}

	constexpr std::ostream& write_text(
	    std::ostream& os,
	    const std::array<std::string_view, 5>& rkey_ = rkey) const {
		for (const auto y : kblib::range(height())) {
			for (const auto x : kblib::range(width())) {
				os << rkey_[kblib::etoi(at(x, y).val)];
			}
			os << '\n';
		}
		return os;
	}

	constexpr std::string write_text(const std::array<std::string_view, 5>& rkey_
	                                 = rkey) const {
		std::string ret;
		for (const auto y : kblib::range(height())) {
			for (const auto x : kblib::range(width())) {
				ret += rkey_[kblib::etoi(at(x, y).val)];
			}
			ret += '\n';
		}
		return ret;
	}
};

struct single_test {
	std::vector<std::vector<word_t>> inputs{};
	std::vector<std::vector<word_t>> n_outputs{};
	image_t i_output{};
};
struct inputs_outputs {
	using list = std::array<word_t, 39>;
	std::array<single_test, 3> data;
};

template <typename T, typename U>
constexpr U sat_add(T a, T b, U l, U h) {
	using I = std::common_type_t<T, U>;
	return static_cast<U>(
	    std::clamp(static_cast<I>(a + b), static_cast<I>(l), static_cast<I>(h)));
}

template <auto l = word_t{-999}, auto h = word_t{999}, typename T>
constexpr auto sat_add(T a, T b) {
	return sat_add<T, std::common_type_t<decltype(l), decltype(h)>>(a, b, l, h);
}
template <auto l = word_t{-999}, auto h = word_t{999}, typename T>
constexpr auto sat_sub(T a, T b) {
	return sat_add<T, std::common_type_t<decltype(l), decltype(h)>>(a, -b, l, h);
}

inline std::ostream& write_list(std::ostream& os, const std::vector<word_t>& v,
                                const std::vector<word_t>* expected = nullptr,
                                bool colored = use_color) {

	if (colored and expected and v.size() != expected->size()) {
		os << print_color(bright_red);
	}
	os << '(';
	os << v.size() << ')';
	if (colored) {
		os << print_color(reset);
	}
	os << " [\n\t";
	for (std::size_t i = 0; i < v.size(); ++i) {
		if (colored and expected
		    and (i >= expected->size() or v[i] != (*expected)[i])) {
			os << print_color(bright_red);
		}
		os << v[i] << ' ';
		if (colored) {
			os << print_color(reset);
		}
	}
	os << "\n]";
	return os;
}

#endif // NODE_HPP
