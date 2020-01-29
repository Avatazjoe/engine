/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <limits.h>
#include <algorithm>
#include <vector>
#include <ctype.h>
#include <stdarg.h>
#include <SDL_stdinc.h>
#include <string_view>
#include <math.h>

namespace core {
namespace string {

extern core::String format(SDL_PRINTF_FORMAT_STRING const char *msg, ...) SDL_PRINTF_VARARG_FUNC(1);
extern bool formatBuf(char *buf, size_t bufSize, SDL_PRINTF_FORMAT_STRING const char *msg, ...) SDL_PRINTF_VARARG_FUNC(3);

inline int toInt(const char* str) {
	return SDL_atoi(str);
}

inline int64_t toLong(const char* str) {
	return ::atol(str);
}

inline int toInt(const core::String& str) {
	return toInt(str.c_str());
}

inline int64_t toLong(const core::String& str) {
	return toLong(str.c_str());
}

inline bool toBool(const core::String& str) {
	return str == "1" || str == "true";
}

inline char toHex(char code) {
	static const char* _hex = "0123456789ABCDEF";
	return _hex[code & 15];
}

inline float toFloat(const core::String& str) {
	return (float)SDL_atof(str.c_str());
}

inline double toDouble(const core::String& str) {
	return (double)SDL_atof(str.c_str());
}

/**
 * @brief Modifies the string input buffer by looking for the given token, replace the start of the token with @c \0
 * and returns the position after the given token.
 *
 * @code
 * char *buf = "a b";
 * char *a = getBeforeToken(buf, " ", strlen(buf)); // "a"
 * char *b = buf;                                   // "b"
 * @endcode
 *
 * @return The function returns @c nullptr if the token wasn't found inside the given buffer boundaries.
 */
extern char* getBeforeToken(char **buffer, const char *token, size_t bufferSize);

extern void splitString(const core::String& string, std::vector<std::string>& tokens, const core::String& delimiters = " \t\r\n\f\v");
extern void splitString(const core::String& string, std::vector<std::string_view>& tokens, const core::String& delimiters = " \t\r\n\f\v");

extern core::String toLower(const core::String& string);
extern core::String toLower(const char* string);

inline char toUpper(char in) { return SDL_toupper((int)in); }
inline char toLower(char in) { return SDL_tolower((int)in); }

extern core::String toUpper(const core::String& string);
extern core::String toUpper(const char* string);

inline bool startsWith(const core::String& string, const core::String& token) {
	return !string.compare(0, token.size(), token);
}

inline bool startsWith(const std::string_view& string, const core::String& token) {
	return !string.compare(0, token.size(), token);
}

inline bool startsWith(const std::string_view& string, const char* token) {
	return !string.compare(0, strlen(token), token);
}

inline bool startsWith(const core::String& string, const char* token) {
	return !string.compare(0, strlen(token), token);
}

inline bool startsWith(const char* string, const char* token) {
	return !std::string_view(string).compare(0, strlen(token), token);
}

/**
 * @brief Locate the string after the last occurrence of the given character in the input string
 * @return nullptr if the character is not part of the input string. Otherwise the pointer to the character
 * followed by the last found match.
 */
inline const char* after(const char* input, int character) {
	const char *s = SDL_strrchr(input, character);
	if (s != nullptr) {
		++s;
	}
	return s;
}

/**
 * @return negative value if not found
 */
inline int indexOf(const char *a, const char *b) {
	char *offset = SDL_strstr(a, b);
	return (int)(intptr_t)(offset - a);
}

inline bool endsWith(const core::String& string, const core::String& end) {
	const std::size_t strLength = string.length();
	const std::size_t endLength = end.length();
	if (strLength >= endLength) {
		const std::size_t index = strLength - endLength;
		return string.compare(index, endLength, end) == 0;
	}
	return false;
}

extern core::String replaceAll(const core::String& str, const core::String& searchStr, const char* replaceStr, size_t replaceStrSize);

inline core::String replaceAll(const core::String& str, const core::String& searchStr, const char* replaceStr) {
	return replaceAll(str, searchStr, replaceStr, strlen(replaceStr));
}

inline core::String replaceAll(const core::String& str, const core::String& searchStr, const std::string_view& replaceStr) {
	return replaceAll(str, searchStr, replaceStr.data(), replaceStr.size());
}

extern void replaceAllChars(core::String& str, char in, char out);

inline core::String replaceAll(const core::String& str, const core::String& searchStr, const core::String& replaceStr) {
	return replaceAll(str, searchStr, replaceStr.data(), replaceStr.size());
}

extern bool isNumber(const core::String &in);
extern bool isInteger(const core::String& in);

/**
 * @brief Cuts everything (including the pattern) from the match
 */
inline std::string_view cutAfterFirstMatch(const std::string_view str, const core::String& pattern, size_t start = 0) {
	std::string_view::size_type pos = str.find_first_of(pattern, 0);
	return str.substr(start, pos);
}

/**
 * @brief extract path with trailing /
 * @note Assumed to be normalized (no \ , only /)
 */
inline std::string_view extractPath(const std::string_view str) {
	const size_t pos = str.rfind("/");
	if (pos == std::string::npos) {
		return "";
	}
	return str.substr(0, pos + 1) ;
}

inline std::string_view stripExtension(const std::string_view str) {
	const size_t pos = str.rfind(".");
	if (pos == std::string::npos) {
		return str;
	}
	return str.substr(0, pos) ;
}

inline std::string_view extractFilename(std::string_view str) {
	const size_t pathPos = str.rfind('/');
	if (pathPos != std::string::npos) {
		str = str.substr(pathPos + 1);
	}
	const size_t extPos = str.rfind('.');
	if (extPos != std::string::npos) {
		str = str.substr(0, extPos);
	}
	return str;
}

inline std::string_view extractFilenameWithExtension(std::string_view str) {
	const size_t pathPos = str.rfind('/');
	if (pathPos != std::string::npos) {
		str = str.substr(pathPos + 1);
	}
	return str;
}

inline core::String eraseAllSpaces(const core::String& str) {
	core::String tmp = str;
	tmp.erase(std::remove(tmp.begin(), tmp.end(), ' '), tmp.end());
	return tmp;
}

inline bool contains(const core::String& str, const core::String& search) {
	return str.rfind(search) != std::string::npos;
}

inline bool contains(const char *haystack, const char *needle) {
	char *pos = (char*)SDL_strstr(haystack, needle);
	return pos != nullptr;
}

inline bool icontains(const core::String& str, const core::String& search) {
	return toLower(str).rfind(toLower(search)) != std::string::npos;
}

inline std::string_view ltrim(const std::string_view str) {
	size_t startpos = str.find_first_not_of(" \t");
	if (std::string::npos != startpos) {
		return str.substr(startpos);
	}
	return str;
}

inline std::string_view rtrim(const std::string_view str) {
	size_t endpos = str.find_last_not_of(" \t");
	if (std::string::npos != endpos) {
		return str.substr(0, endpos + 1);
	}
	return str;
}

inline std::string_view trim(const std::string_view str) {
	return ltrim(rtrim(str));
}

inline bool iequals(const core::String& a, const core::String& b) {
	const size_t sz = a.size();
	if (b.size() != sz) {
		return false;
	}
	for (size_t i = 0u; i < sz; ++i) {
		if (tolower(a[i]) != tolower(b[i])) {
			return false;
		}
	}
	return true;
}

template<typename ITER>
core::String join(const ITER& begin, const ITER& end, const char *delimiter) {
	auto i = begin;
	if (i == end) {
		return "";
	}
	std::ostringstream ss;
	ss << *i;
	for (++i; i != end; ++i) {
		ss << delimiter;
		ss << *i;
	}
	return ss.str();
}

template<typename ITER, typename FUNC>
core::String join(const ITER& begin, const ITER& end, const char *delimiter, FUNC&& func) {
	auto i = begin;
	if (i == end) {
		return "";
	}
	std::ostringstream ss;
	ss << func(*i);
	for (++i; i != end; ++i) {
		ss << delimiter;
		ss << func(*i);
	}
	return ss.str();
}

/**
 * @brief Performs a pattern/wildcard based string match
 * @param[in] pattern The pattern can deal with wildcards like * and ?
 * @param[in] text The text to match against the pattern
 */
extern bool matches(const char* pattern, const char* text);
extern bool matches(const core::String& pattern, const char* text);
inline bool matches(const core::String& pattern, const core::String& text) {
	return matches(pattern, text.c_str());
}

// pass by copy to prevent aliasing
extern core::String concat(std::string_view first, std::string_view second);

/**
 * @param[in,out] str Converts a string into UpperCamelCase.
 * @note Underscores are removed and the following character is also converted to upper case.
 * Example: @c foo_bar will end as @c FooBar
 */
extern void upperCamelCase(core::String& str);
extern core::String upperCamelCase(const core::String& str);

extern void lowerCamelCase(core::String& str);
extern core::String lowerCamelCase(const core::String& str);

extern char* append(char* buf, size_t bufsize, const char* string);

extern int count(const char *buf, char chr);

/**
 * @note Call @c core_free() on the returned string
 */
extern char *urlEncode(const char *str);

}
}
