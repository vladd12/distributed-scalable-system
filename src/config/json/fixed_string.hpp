#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace utils
{

template <typename T, typename... Ts>
concept one_of = (false || ... || std::same_as<T, Ts>);

template <class charT, std::size_t N, class traits = std::char_traits<charT>> //
class basic_fixed_string
{
public:
  charT data_[N + 1] = {}; // exposition only

  // types
  using traits_type = traits;
  using value_type = charT;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using const_iterator = const_pointer;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  static_assert(std::is_same_v<charT, typename traits::char_type>, "Bad char_traits for basic_fixed_string");
  static_assert(!std::is_array_v<charT> && std::is_trivial_v<charT> && std::is_standard_layout_v<charT>,
      "The character type of basic_fixed_string must be a non-array trivial standard-layout type. See N4861 ");

  // capacity
  static constexpr std::integral_constant<size_type, N> size {};
  static constexpr std::integral_constant<size_type, N> length {};
  static constexpr std::integral_constant<size_type, N> max_size {};
  static constexpr std::bool_constant<N == 0> empty {};

  // [fixed.string.cons], construction and assignment

  template <std::convertible_to<charT>... Chars>                          //
  requires((sizeof...(Chars) == N) && (... && !std::is_pointer_v<Chars>)) //
      constexpr explicit basic_fixed_string(Chars... chars) noexcept
      : data_ { static_cast<charT>(chars)..., charT() }
  {
  }

  consteval basic_fixed_string(const charT (&txt)[N + 1]) noexcept
  {
    for (size_type index = 0; index < N + 1; ++index)
      data_[index] = txt[index];
  }

  template <std::input_iterator It, std::sentinel_for<It> S> //
  requires std::convertible_to<std::iter_value_t<It>, charT> //
  constexpr basic_fixed_string(It begin, S end)
  {
    assert(std::distance(begin, end) == N);
    std::copy(begin, end, &data_[0]);
    data_[N] = charT();
  }

  constexpr basic_fixed_string(const basic_fixed_string &) noexcept = default;
  constexpr basic_fixed_string &operator=(const basic_fixed_string &) noexcept = default;

  // iterator support

  constexpr const_iterator begin() const noexcept
  {
    return &data_[0];
  }

  constexpr const_iterator end() const noexcept
  {
    return begin() + N;
  }

  constexpr const_iterator cbegin() const noexcept
  {
    return begin();
  }

  constexpr const_iterator cend() const noexcept
  {
    return end();
  }

  constexpr const_reverse_iterator rbegin() const noexcept
  {
    return std::make_reverse_iterator(end());
  }

  constexpr const_reverse_iterator rend() const noexcept
  {
    return std::make_reverse_iterator(begin());
  }

  constexpr const_reverse_iterator crbegin() const noexcept
  {
    return std::make_reverse_iterator(cend());
  }

  constexpr const_reverse_iterator crend() const noexcept
  {
    return std::make_reverse_iterator(cbegin());
  }

  // element access

  constexpr const_reference operator[](size_type pos) const
  {
    return data_[pos];
  }

  constexpr const_reference at(size_type pos) const
  {
    return data_[pos];
  }

  constexpr const_reference front() const
  {
    return data_[0];
  }

  constexpr const_reference back() const
  {
    return data_[N - 1];
  }

  // [fixed.string.modifiers], modifiers

  constexpr void swap(basic_fixed_string &s) noexcept
  {
    std::swap(data_, s.data_);
  }

  // [fixed.string.ops], string operations

  constexpr const_pointer c_str() const noexcept
  {
    return cbegin();
  }

  constexpr const_pointer data() const noexcept
  {
    return cbegin();
  }

  constexpr std::basic_string_view<charT, traits> view() const noexcept
  {
    return std::basic_string_view<charT, traits>(c_str(), N);
  }

  constexpr operator std::basic_string_view<charT, traits>() const noexcept
  {
    return view();
  }

  // [fixed.string.comparison]

  template <std::size_t N2> //
  friend constexpr bool operator==(const basic_fixed_string &lhs, const basic_fixed_string<charT, N2, traits> &rhs)
  {
    if constexpr (N == N2)
    {
      for (auto il = lhs.cbegin(), ir = rhs.cbegin(); il != lhs.cend() || ir != rhs.cend(); ++il, ++ir)
        if (*il != *ir)
          return false;
      return true;
    }
    return false;
  }

  template <std::size_t N2> //
  friend consteval bool operator==(const basic_fixed_string &lhs, const charT (&rhs)[N2])
  {
    return lhs == basic_fixed_string<charT, N2 - 1, traits>(rhs);
  }

  template <std::size_t N2> //
  friend constexpr bool operator!=(const basic_fixed_string &lhs, const basic_fixed_string<charT, N2, traits> &rhs)
  {
    return !(lhs == rhs);
  }

  template <std::size_t N2> //
  friend consteval bool operator!=(const basic_fixed_string &lhs, const charT (&rhs)[N2])
  {
    return !(lhs == rhs);
  }
};

// [fixed.string.deduct], deduction guides

template <one_of<char, char8_t, char16_t, char32_t, wchar_t> CharT, std::convertible_to<CharT>... Rest>
basic_fixed_string(CharT, Rest...) -> basic_fixed_string<CharT, 1 + sizeof...(Rest)>;

template <typename charT, std::size_t N> basic_fixed_string(const charT (&str)[N]) -> basic_fixed_string<charT, N - 1>;

template <std::size_t N> //
class fixed_string : public basic_fixed_string<char, N>
{
  using basic_fixed_string<char, N>::basic_fixed_string;
};
template <std::size_t N> fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

template <std::size_t N> //
class wfixed_string : public basic_fixed_string<wchar_t, N>
{
  using basic_fixed_string<wchar_t, N>::basic_fixed_string;
};
template <std::size_t N> wfixed_string(const wchar_t (&str)[N]) -> wfixed_string<N - 1>;

template <std::size_t N> //
class u8fixed_string : public basic_fixed_string<char8_t, N>
{
  using basic_fixed_string<char8_t, N>::basic_fixed_string;
};
template <std::size_t N> u8fixed_string(const char8_t (&str)[N]) -> u8fixed_string<N - 1>;

template <std::size_t N> //
class u16fixed_string : public basic_fixed_string<char16_t, N>
{
  using basic_fixed_string<char16_t, N>::basic_fixed_string;
};
template <std::size_t N> u16fixed_string(const char16_t (&str)[N]) -> u16fixed_string<N - 1>;

template <std::size_t N> //
class u32fixed_string : public basic_fixed_string<char32_t, N>
{
  using basic_fixed_string<char32_t, N>::basic_fixed_string;
};
template <std::size_t N> u32fixed_string(const char32_t (&str)[N]) -> u32fixed_string<N - 1>;

}
