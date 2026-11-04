#pragma once

#include <boost/system/api_config.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace core
{

#if defined(BOOST_WINDOWS_API)
typedef wchar_t char_type;
#elif defined(BOOST_POSIX_API)
typedef char char_type;
#else // well, let's do this big mistake
typedef char char_type;
#endif

using string_view = std::basic_string_view<char_type>;
using string = std::basic_string<char_type>;

using ostream = std::basic_ostream<char_type>;
using istream = std::basic_istream<char_type>;
using iostream = std::basic_iostream<char_type>;

#if defined(BOOST_WINDOWS_API)
inline ostream &cout = std::wcout;
#else
inline ostream &cout = std::cout;
#endif

} // namespace core
