/* *****************************************************************************
 * TIX-100-CXX
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
#ifndef NODE_HPP
#define NODE_HPP

#include "image.hpp"
#include "logger.hpp"

#if NDEBUG
#	define RELEASE 1
#else
#	define RELEASE 0
#endif

#include <array>
#include <cstdint>
#include <vector>

using word_t = std::int16_t;
constexpr inline word_t word_min = -999;
constexpr inline word_t word_max = 999;
static_assert(word_min == -word_max);

/// we don't use a whole 16bit for a word, roll out a faster optional<word_t>
using optional_word = word_t;
constexpr inline optional_word word_empty = kblib::min;
static_assert(word_empty < word_min);

using word_vec = std::vector<word_t>;

constexpr inline int def_T21_size = 15;
constexpr inline int def_T30_size = 15;

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
	default:
		throw std::invalid_argument{""};
	}
}

enum port : std::int8_t {
	// The directions are values 0-5, so < and > can be used to compare them
	left,
	right,
	up,
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
	case left:
		return right;
	case right:
		return left;
	case up:
		return down;
	case down:
		return up;
	case D5:
		return D6;
	case D6:
		return D5;
	default:
		std::unreachable();
	}
}

struct node {
 public:
	enum type_t { T21 = 1, T30, in, out, image, Damaged = -1, null = -2 };

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
		default:
			throw std::invalid_argument{concat("Illegal port ", etoi(p))};
		}
	}

	/// Returns the type of the node
	virtual type_t type() const noexcept = 0;
	/// Begin processing a cycle. Do not perform writes.
	virtual void step() = 0;
	/// Finish processing a cycle. Writes are completed here.
	virtual void finalize() = 0;
	/// Reset the node to its default (startup) configuration. Do not erase code
	/// or expected values.
	virtual void reset() noexcept = 0;

	/// Attempt to answer a read from this node, coming from direction p
	virtual optional_word emit(port p) = 0;
	/// Generate a string representation of the current state of the node
	virtual std::string state() const = 0;

	// In theory this may be expanded to a "TIS-3D" simulator which will have 6
	// neighbors for each node.
	std::array<node*, 6> neighbors{};
	int x{};
	int y{};

	node() = default;
	node(int x, int y) noexcept
	    : x(x)
	    , y(y) {}
	virtual ~node() = default;

	/// null and Damaged are negative. Any positive value is a valid node
	// (0 is unallocated)
	friend bool valid(const node* n) { return n and etoi(n->type()) > 0; }
	/// Attempt to read a value from n, coming from direction p
	friend optional_word do_read(node* n, port p) {
		assert(p >= port::left and p <= port::D6);
		if (not n) {
			return word_empty;
		} else {
			return n->emit(p);
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
	void step() override {}
	void finalize() override {}
	void reset() noexcept override {}
	optional_word emit(port) override { return word_empty; }
	std::string state() const override {
		return concat("(", x, ',', y, ") {Damaged}");
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
	    // The game uses floats, {
	    //    { 0, new Color(0f, 0f, 0f) },
	    //    { 1, new Color(0.273f, 0.273f, 0.273f) },
	    //    { 2, new Color(0.547f, 0.547f, 0.547f) },
	    //    { 3, new Color(0.808f, 0.808f, 0.808f) },
	    //    { 4, new Color(0.648f, 0.043f, 0.043f) }
	    // };
	    // Which I think convert to
	    // 000000, 464646, 8B8B8B, CECECE, AE0B0B
	    // but the above colors are picked from screenshots so the game must have
	    // a shader that darkens them
	};

	static color normalize(auto c) {
		if (c > C_red or c < 0) {
			return C_black;
		}
		return static_cast<color>(c);
	}

	tis_pixel() = default;
	constexpr tis_pixel(color c) noexcept
	    : val(normalize(c)) {}
	explicit(false) constexpr tis_pixel(std::integral auto c) noexcept
	    : val(normalize(c)) {}

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

	// PNM doesn't have an indexed version so full RGB is the only option
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
	image_t(std::initializer_list<std::u16string_view> image,
	        std::u16string_view key_ = key) {
		assign(image, key_);
	}
	// char16_t needed to have random access
	constexpr static std::u16string_view key = u" ░▒█#";
	// the reverse key needs to separate them instead
	constexpr static std::array<std::string_view, 5> rkey
	    = {" ", "░", "▒", "█", "#"};

	using image::assign;
	// It's a lot more convenient to use single characters for pixels instead of
	// using the enum values or comma-separated integers
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
#if not RELEASE
		for (auto line : image) {
			assert(line.size() == w);
		}
#endif
		reshape(to_signed(w), to_signed(image.size()));
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
		for (const auto y : range(height())) {
			for (const auto x : range(width())) {
				os << rkey_[etoi(at(x, y).val)];
			}
			os << '\n';
		}
		return os;
	}

	constexpr std::string write_text(const std::array<std::string_view, 5>& rkey_
	                                 = rkey) const {
		std::string ret;
		for (const auto y : range(height())) {
			for (const auto x : range(width())) {
				ret += rkey_[etoi(at(x, y).val)];
			}
			ret += '\n';
		}
		return ret;
	}

	constexpr std::string write_text(bool colored) const {
		return write_text(
		    {" ", "░", "▒", "█",
		     colored ? concat(print_escape(red), "▓", print_escape(reset_color))
		             : "#"});
	}
};

struct single_test {
	std::vector<word_vec> inputs{};
	std::vector<word_vec> n_outputs{};
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

template <auto l = word_min, auto h = word_max, typename T>
constexpr auto sat_add(T a, T b) {
	return sat_add<T, std::common_type_t<decltype(l), decltype(h)>>(a, b, l, h);
}
template <auto l = word_min, auto h = word_max, typename T>
constexpr auto sat_sub(T a, T b) {
	return sat_add<T, std::common_type_t<decltype(l), decltype(h)>>(a, -b, l, h);
}

// returns a new string, padded
inline std::string pad_right(std::string input, std::size_t size,
                             char padding = ' ') {
	input.resize(std::max(input.size(), size), padding);
	return input;
}
inline std::string pad_right(std::string_view input, std::size_t size,
                             char padding = ' ') {
	return pad_right(std::string(input), size, padding);
}
inline std::string pad_right(std::integral auto input, std::size_t size,
                             char padding = ' ') {
	return pad_right(std::to_string(input), size, padding);
}

inline std::string pad_left(std::string input, std::size_t size,
                            char padding = ' ') {
	input.insert(0, std::max(0uz, size - input.size()), padding);
	return input;
}
inline std::string pad_left(std::string_view input, std::size_t size,
                            char padding = ' ') {
	return pad_left(std::string(input), size, padding);
}
inline std::string pad_left(std::integral auto input, std::size_t size,
                            char padding = ' ') {
	return pad_left(std::to_string(input), size, padding);
}

template <typename Stream>
   requires requires(Stream s) {
	   { s << "" << 1 };
   }
inline Stream&& write_list(Stream&& os, const word_vec& v,
                           const word_vec* expected = nullptr,
                           bool colored = use_color) {
	if (colored and expected and v.size() != expected->size()) {
		os << print_escape(bright_red);
	}
	os << '(';
	os << v.size();
	if (expected) {
		os << '/' << expected->size();
	}
	os << ')';
	if (colored) {
		os << print_escape(none);
	}
	os << " [\n\t";
	for (std::size_t i = 0; i < v.size(); ++i) {
		if (colored and expected
		    and (i >= expected->size() or v[i] != (*expected)[i])) {
			os << print_escape(bright_red);
		}
		os << v[i] << ' ';
		if (colored) {
			os << print_escape(none);
		}
	}
	os << "\n]";
	return std::forward<Stream>(os);
}

struct hcf_exception {
	int x{};
	int y{};
	int line{};
};

#endif // NODE_HPP
