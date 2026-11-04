#include <type_traits>

namespace core
{

/// \brief Conversion passed instance of an enum class type to the instance of an underlying type.
template <typename Enum> //
[[nodiscard]] constexpr inline std::underlying_type_t<Enum> to_underlying(Enum value) noexcept
{
  return static_cast<std::underlying_type_t<Enum>>(value);
}

/// \brief Conversion passed instance of an underlying_type_t to the instance of an enum class type.
template <typename Enum> //
[[nodiscard]] constexpr inline Enum from_underlying(std::underlying_type_t<Enum> value) noexcept
{
  return static_cast<Enum>(value);
}

} // namespace core
