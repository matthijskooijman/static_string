// Copyright (C) 2017 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef AK_TOOLKIT_STATIC_STRING_HEADER_GUARD_2018_12_27_HPP
#define AK_TOOLKIT_STATIC_STRING_HEADER_GUARD_2018_12_27_HPP

#include <type_traits>
#include <cassert>
#include <cstddef>

// # Based on value of macro AK_TOOLKIT_CONFIG_USING_STRING_VIEW we decide if and how
//   we want to handle a conversion to string_view

# if defined AK_TOOLKIT_CONFIG_USING_STRING_VIEW
#   if AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 0
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS()
#   elif AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 1
#     include <string_view>
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS() constexpr operator ::std::string_view () const { return ::std::string_view(c_str(), len()); }
#   elif AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 2
#     include <experimental/string_view>
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS() constexpr operator ::std::experimental::string_view () const { return ::std::experimental::string_view(c_str(), len()); }
#   elif AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 3
#     include <boost/utility/string_ref.hpp> 
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS() constexpr operator ::boost::string_ref () const { return ::boost::string_ref(c_str(), len()); }
#   elif AK_TOOLKIT_CONFIG_USING_STRING_VIEW == 4
#     include <string> 
#     define AK_TOOLKIT_STRING_VIEW_OPERATIONS() operator ::std::string () const { return ::std::string(c_str(), len()); }
#   endif
# else
#   define AK_TOOLKIT_STRING_VIEW_OPERATIONS()
# endif

namespace ak_toolkit { namespace static_str {
 
// # Implementation of a subset of C++14 std::integer_sequence and std::make_integer_sequence
 
namespace detail
{
  template <int... I>
  struct int_sequence
  {};
 
  template <int i, typename T>
  struct cat
  {
    static_assert (sizeof(T) < 0, "bad use of cat");
  };
 
  template <int i, int... I>
  struct cat<i, int_sequence<I...>>
  {
    using type = int_sequence<I..., i>;
  };
 
  template <int I>
  struct make_int_sequence_
  {
    static_assert (I >= 0, "bad use of make_int_sequence: negative size");
    using type = typename cat<I - 1, typename make_int_sequence_<I - 1>::type>::type;
  };
 
  template <>
  struct make_int_sequence_<0>
  {
    using type = int_sequence<>;
  };
 
  template <int I>
  using make_int_sequence = typename make_int_sequence_<I>::type;
}


// # Implementation of a constexpr-compatible assertion

#if defined NDEBUG
# define AK_TOOLKIT_ASSERT(CHECK) void(0)
#else
# define AK_TOOLKIT_ASSERT(CHECK) ((CHECK) ? void(0) : []{assert(!#CHECK);}())
#endif

/** Return the length of the given string */
constexpr size_t static_strlen(const char *str) {
  return *str == '\0' ? 0 : static_strlen(str + 1) + 1;
}

struct literal_ref {};
struct char_array {};

template <int N, typename Impl = literal_ref>
class string
{
	static_assert (N > 0 && N < 0, "Invalid specialization of string");
};

// # A wraper over a string literal with alternate interface. No ownership management

template <int N>
class string<N, literal_ref>
{
public:
    const char (&_lit)[N + 1];
    const size_t _len;
    const size_t _offset;
    explicit constexpr string(const char (&lit)[N + 1], size_t offset, size_t len)
      : _lit((AK_TOOLKIT_ASSERT(lit[N] == 0), lit)),
        _offset((AK_TOOLKIT_ASSERT(offset <= N), offset)),
        _len((AK_TOOLKIT_ASSERT(offset + len <= N), len))
    { }
public:
    constexpr string(const char (&lit)[N + 1]) : _lit((AK_TOOLKIT_ASSERT(lit[N] == 0), lit)), _offset(0), _len(N) {}
    constexpr char operator[](int i) const { return AK_TOOLKIT_ASSERT(i < _len), _lit[i+_offset]; }
    AK_TOOLKIT_STRING_VIEW_OPERATIONS()
    constexpr ::std::size_t size() const { return N; };
    constexpr ::std::size_t len() const { return _len; };
    constexpr const char* c_str() const { return _lit + _offset; }
    constexpr operator const char * () const { return c_str(); }

    constexpr string substr(size_t start, size_t len) const {
        return string(_lit, start + _offset, len);
    }

    constexpr string substr(size_t start) const {
        return string(_lit, start + _offset, len() - start);
    }
};

template <int N>
  using string_literal = string<N, literal_ref>;


// # A function that converts raw string literal into string_literal and deduces the size.

template <int N_PLUS_1>
constexpr string_literal<N_PLUS_1 - 1> literal(const char (&lit)[N_PLUS_1])
{
    return string_literal<N_PLUS_1 - 1>(lit);
}


// # This implements a null-terminated array that stores elements on stack.

template <int N>
class string<N, char_array>

{
public:
    char _array[N + 1];
    struct private_ctor {};
    
    template <int NL, int NR, int... Il, int... Ir, typename TL, typename TR>
    constexpr explicit string(private_ctor, string<NL, TL> const& l, string<NR, TR> const& r, detail::int_sequence<Il...>)
      : _array{concat_elem(l, r, Il)..., 0}
    {
    }

    template <int NL, int NR, typename TL, typename TR>
    constexpr char concat_elem(string<NL, TL> const& l, string<NR, TR> const& r, size_t i) {
        return i < l.len()
               ? l[i]
               : i - l.len() < r.len()
                 ? r[i - l.len()]
                 : 0;
    }
   
public:
    template <int M, typename TL, typename TR, typename std::enable_if<(M <= N), bool>::type = true>
    constexpr explicit string(string<M, TL> l, string<N - M, TR> r)
    : string(private_ctor{}, l, r, detail::make_int_sequence<N>{})
    {
    }

    constexpr string(string_literal<N> l) // converting
    : string(private_ctor{}, l, literal(""), detail::make_int_sequence<N>{})
    {
    }

    // Expanding is always ok, shrinking only down to l.len()
    template <int OldN, typename T>
    constexpr string(string<OldN, T> l)
    : string((AK_TOOLKIT_ASSERT(N >= OldN || N >= l.len()), private_ctor{}), l, literal(""), detail::make_int_sequence<N>{})
    {
    }

    constexpr ::std::size_t size() const { return N; }
    constexpr ::std::size_t len() const { return static_strlen(_array); }
  
    constexpr const char* c_str() const { return _array; }
    constexpr operator const char * () const { return c_str(); }
    AK_TOOLKIT_STRING_VIEW_OPERATIONS()
    constexpr char operator[] (int i) const { return AK_TOOLKIT_ASSERT(i >= 0 && i < N), _array[i]; }

    constexpr string substr(size_t start, size_t len) const {
        // Return implicitly converts back to an array_string, which is
        // needed because the _array referenced might be a temporary, so
        // we shouldn't return a pointer to it
        return literal(_array).substr(start, len);
    }

    constexpr string substr(size_t start) const {
        return literal(_array).substr(start);
    }

};

template <int N>
  using array_string = string<N, char_array>;
  
// # A set of concatenating operators, for different combinations of raw literals, string_literal<>, and array_string<>

template <int N1, int N2, typename TL, typename TR>
constexpr string<N1 + N2, char_array> operator+(string<N1, TL> const& l, string<N2, TR> const& r)
{
    return string<N1 + N2, char_array>(l, r);
}

template <int N1_1, int N2, typename TR>
constexpr string<N1_1 - 1 + N2, char_array> operator+(const char (&l)[N1_1], string<N2, TR> const& r)
{
    return string<N1_1 - 1 + N2, char_array>(string_literal<N1_1 - 1>(l), r);
}

template <int N1, int N2_1, typename TL>
constexpr string<N1 + N2_1 - 1, char_array> operator+(string<N1, TL> const& l, const char (&r)[N2_1])
{
    return string<N1 + N2_1 - 1, char_array>(l, string_literal<N2_1 - 1>(r));
}

}} // namespace ak_toolkit::static_str

#endif // header guard
