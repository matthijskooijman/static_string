#include <ak_toolkit/static_string.hpp>
#include <iostream>
namespace sstr = ak_toolkit::static_str;

#if __cplusplus >= 201703L
constexpr sstr::string CD = sstr::literal("CD");
constexpr sstr::string NAME = "AB" + CD + sstr::literal("EF") + "GH";
#else
constexpr auto CD = sstr::literal("CD");
constexpr auto NAME = "AB" + CD + sstr::literal("EF") + "GH";
#endif

static_assert(NAME.size() == 8, "***");
static_assert(NAME[0] == 'A', "***");

constexpr auto CD_ = CD + "";
static_assert(CD_.size() == CD.size(), "***");

constexpr auto CDE = NAME.substr(2, 2) + "E";

/**
 * Helper to figure out the strrchr result. A helper is needed so we can
 * put the result of the recursive call in a variable, which is not
 * otherwise allowed in C++11. The alternative, to do the recursive call
 * twice, makes the algorithm exponential, which breaks for a string
 * length of more than a few characters. */
template <typename T1, typename T2>
constexpr auto static_strrchrnul_helper(T1 const& s, T2 const& recursive, char c)
-> decltype(s.substr(0)) {
	return (recursive.len() == 0 && s[0] == c) ? s : recursive;
}

/**
 * Return the last occurence of c in the given string, or an empty
 * string if the character does not occur. This should behave just like
 * the regular strrchrnul function.
 */
template <int N, typename T>
constexpr auto static_strrchrnul(sstr::string<N, T> const& s, char c)
-> decltype(s.substr(0))
{
  /* C++14 version
    // If we reach the end of the string, return an empty string
    if (s.len() == 0) return s;
    // Otherwise there is a remainder string, check that
    auto recursive = static_strrchrnul(s.substr(1), c);
    // If c was found in the remainder, return that
    if (recursive.len() != 0)
      return recursive;
    // If c was not found in the remainder, but we find it here, return
    // the current string.
    if (s[0] == c)
	return s;
    // Otherwise, return an empty string (which is conveniently what
    // recursive contains).
    return recursive;
  */
  return s.len() == 0 ? s : static_strrchrnul_helper(s, static_strrchrnul(s.substr(1), c), c);
}

/**
 * Return one past the last separator in the given path, or the start of
 * the path if it contains no separator.
 * Unlike the regular basename, this does not handle trailing separators
 * specially (so it returns an empty string if the path ends in a
 * separator).
 */
template <int N, typename T>
constexpr auto static_basename(sstr::string<N, T> const& path)
-> decltype(static_strrchrnul(path, '/'))
{
  return (static_strrchrnul(path, '/').len() > 0
      ? static_strrchrnul(path, '/').substr(1)
      : path.substr(0)
     );
}

#include <sstream>
#include <cassert>

int main ()
{
	std::ostringstream os;
	os << NAME;
	std::cout << os.str() << std::endl;
	assert(os.str() == "ABCDEFGH");

	os.str("");
	os << CDE;
	std::cout << os.str() << std::endl;
	assert(os.str() == "CDE");

	std::cout << "original: " << __FILE__ << std::endl;
	constexpr auto file = static_basename(sstr::literal(__FILE__) + "") + "";
	std::cout << "file: " << file << std::endl;
	std::cout << "sizeof(file): " << sizeof(file) << std::endl;
	constexpr auto small_file = sstr::array_string<file.len()>(file);
	std::cout << "small_file: " << small_file << std::endl;
	std::cout << "sizeof(small_file): " << sizeof(small_file) << std::endl;
}
