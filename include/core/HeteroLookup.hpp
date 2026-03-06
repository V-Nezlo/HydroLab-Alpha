#ifndef HETEROLOOKUP_HPP
#define HETEROLOOKUP_HPP

#include <cstddef>
#include <string_view>
#include <string>

struct StrHash {
	using is_transparent = void;

	size_t operator()(std::string_view s) const noexcept {
		return std::hash<std::string_view>{}(s);
	}
	size_t operator()(const std::string& s) const noexcept {
		return (*this)(std::string_view{s});
	}
	size_t operator()(const char* s) const noexcept {
		return (*this)(std::string_view{s});
	}
};

struct StrEq {
	using is_transparent = void;

	bool operator()(const std::string& a, const std::string& b) const noexcept {
		return a == b;
	}

	bool operator()(std::string_view a, std::string_view b) const noexcept {
		return a == b;
	}
	bool operator()(const std::string& a, std::string_view b) const noexcept {
		return std::string_view{a} == b;
	}
	bool operator()(std::string_view a, const std::string& b) const noexcept {
		return a == std::string_view{b};
	}
	bool operator()(const char* a, std::string_view b) const noexcept {
		return std::string_view{a} == b;
	}
	bool operator()(std::string_view a, const char* b) const noexcept {
		return a == std::string_view{b};
	}
};

#endif // HETEROLOOKUP_HPP
