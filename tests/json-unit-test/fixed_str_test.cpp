#include <array>
#include <config/json/fixed_string.hpp>
#include <gtest/gtest.h>

namespace fixed_string_test
{

TEST(fixed_string_test, ctor_test)
{
  constexpr utils::basic_fixed_string<char, 3> str1('1', '2', '3');
  static_assert((str1.data_[0] == '1') && (str1.data_[1] == '2') && (str1.data_[2] == '3'));
  constexpr utils::basic_fixed_string<char, 4> str2("1234");
  static_assert((str2.data_[0] == '1') && (str2.data_[1] == '2') && (str2.data_[2] == '3') && (str2.data_[3] == '4'));
  constexpr std::array<char, 5> arr1 { '1', '2', '3', '4', '5' };
  constexpr utils::basic_fixed_string<char, 5> str3(arr1.begin(), arr1.end());
  static_assert((str3.data_[0] == '1') && (str3.data_[1] == '2') && //
      (str3.data_[2] == '3') && (str3.data_[3] == '4') && (str3.data_[4] == '5'));
  constexpr auto str4 = str2;
  static_assert((str4.data_[0] == '1') && (str4.data_[1] == '2') && (str4.data_[2] == '3') && (str4.data_[3] == '4'));
}

TEST(fixed_string_test, comparison_test)
{
  constexpr utils::fixed_string<5> str1("aboba");
  constexpr auto str2 = str1;
  static_assert(str1 == str2);
  static_assert(str1 == "aboba" && str2 == "aboba");
  static_assert(str1 != "aboba1" && str2 != "aboba2");
  constexpr utils::fixed_string<6> str3("friend");
  static_assert(str1 != str3);
}

} // namespace fixed_string_test
