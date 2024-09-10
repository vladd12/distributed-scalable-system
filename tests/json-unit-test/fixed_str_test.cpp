#include <array>
#include <config/json/fixed_string.hpp>
#include <gtest/gtest.h>

namespace fixed_string_test
{

TEST(fixed_string_test, ctor_test)
{
  constexpr utils::basic_fixed_string str1('1', '2', '3');
  static_assert((str1.data_[0] == '1') && (str1.data_[1] == '2') && (str1.data_[2] == '3'));
  constexpr utils::basic_fixed_string str2("1234");
  static_assert((str2.data_[0] == '1') && (str2.data_[1] == '2') && (str2.data_[2] == '3') && (str2.data_[3] == '4'));
  constexpr std::array<char, 5> arr1 { '1', '2', '3', '4', '5' };
  constexpr utils::basic_fixed_string<char, arr1.size()> str3(arr1.begin(), arr1.end());
  static_assert((str3.data_[0] == '1') && (str3.data_[1] == '2') && //
      (str3.data_[2] == '3') && (str3.data_[3] == '4') && (str3.data_[4] == '5'));
  constexpr auto str4 = str2;
  static_assert((str4.data_[0] == '1') && (str4.data_[1] == '2') && (str4.data_[2] == '3') && (str4.data_[3] == '4'));
}

TEST(fixed_string_test, comparison_test)
{
  constexpr utils::fixed_string str1("aboba");
  constexpr auto str2 = str1;
  static_assert(str1 == str2);
  static_assert(str1 == "aboba" && str2 == "aboba");
  static_assert(str1 != "aboba1" && str2 != "aboba2");
  constexpr utils::fixed_string str3("friend");
  static_assert(str1 != str3);
  constexpr utils::fixed_string foo = "foo";
}

TEST(fixed_string_test, capacity_test)
{
  constexpr utils::fixed_string str("aboba");
  static_assert(str.size() == 5);
  static_assert((str.size() == str.length()) && (str.size() == str.max_size()) && !str.empty());
  constexpr utils::fixed_string empty("");
  static_assert((empty.size() == 0) && empty.empty());
}

TEST(fixed_string_test, iterators_test)
{
  constexpr utils::fixed_string str("This is a longer text for some test cases");
  for (auto iter = str.begin(), citer = str.cbegin(); //
       iter != str.end() || citer != str.cend();      //
       ++iter, ++citer)                               //
  {
    ASSERT_EQ(*iter, *citer);
  }
  for (auto riter = str.rbegin(), criter = str.crbegin(); //
       riter != str.rend() || criter != str.crend();      //
       ++riter, ++criter)                                 //
  {
    ASSERT_EQ(*riter, *criter);
  }
  auto riter = str.rend() - 1;
  for (auto iter = str.begin(); iter != str.end(); ++iter, --riter)
  {
    ASSERT_EQ(*iter, *riter);
  }
  auto criter = str.crbegin();
  for (auto citer = str.cend() - 1; criter != str.crend(); ++criter, --citer)
  {
    ASSERT_EQ(*citer, *criter);
  }
}

TEST(fixed_string_test, elem_access_test)
{
  constexpr utils::fixed_string str("012345");
  static_assert((str[0] == str.at(0)) && (str[0] == '0'));
  static_assert((str[1] == str.at(1)) && (str[1] == '1'));
  static_assert((str[2] == str.at(2)) && (str[2] == '2'));
  static_assert((str[3] == str.at(3)) && (str[3] == '3'));
  static_assert((str[4] == str.at(4)) && (str[4] == '4'));
  static_assert((str[5] == str.at(5)) && (str[5] == '5'));
  static_assert((str.front() == str[0]) && (str.front() == '0'));
  static_assert((str.back() == str[5]) && (str.back() == '5'));
}

TEST(fixed_string_test, swap_test)
{
  utils::fixed_string str1("123");
  utils::fixed_string str2("456");
  ASSERT_TRUE(str1 == utils::fixed_string("123"));
  ASSERT_TRUE(str2 == utils::fixed_string("456"));
  str1.swap(str2);
  ASSERT_TRUE(str1 == utils::fixed_string("456"));
  ASSERT_TRUE(str2 == utils::fixed_string("123"));
  utils::fixed_string str3("111");
  utils::fixed_string str4 = str3;
  ASSERT_EQ(str3, str4);
  str3.swap(str4);
  ASSERT_TRUE(str3 == utils::fixed_string("111"));
  ASSERT_TRUE(str4 == utils::fixed_string("111"));
  ASSERT_EQ(str3, str4);
}

TEST(fixed_string_test, string_view_test)
{
  constexpr utils::fixed_string str("This is a longer text for some test cases");
  std::string_view sw1 = str;
  std::string_view sw2 = str.view();
  std::string_view sw3 { str.c_str() };
  std::string_view sw4 { str.data() };
  ASSERT_EQ(sw1, sw2);
  ASSERT_EQ(sw1, sw3);
  ASSERT_EQ(sw1, sw4);
}

} // namespace fixed_string_test
