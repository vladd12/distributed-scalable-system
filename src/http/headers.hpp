#pragma once

#include <iosfwd> // std::istream
#include <string>
#include <unordered_map>

namespace http
{

/// @brief   HTTP headers storage with case-insensitive keys.
/// @details Values are trimmed before stored, only keys are full normalized.
///          This matches RFC 7230 §3.2: header field names are case-insensitive.
class headers_t final
{
private:
  using storage_t = std::unordered_map<std::string, std::string>;
  storage_t storage;

public:
  using const_iterator = storage_t::const_iterator;
  using iterator = storage_t::iterator;
  using value_type = storage_t::value_type;

  inline headers_t() = default;
  headers_t(std::initializer_list<value_type> init_list);

  // clang-format off
  /// @brief Iterator to the first header.
  [[nodiscard]] inline iterator begin() noexcept { return storage.begin(); }
  /// @brief Iterator past the last header.
  [[nodiscard]] inline iterator end() noexcept { return storage.end(); }
  /// @brief Const iterator to the first header.
  [[nodiscard]] inline const_iterator begin() const noexcept { return storage.begin(); }
  /// @brief Const iterator past the last header.
  [[nodiscard]] inline const_iterator end() const noexcept { return storage.end(); }
  /// @copydoc begin() const noexcept
  [[nodiscard]] inline const_iterator cbegin() const noexcept { return storage.cbegin(); }
  /// @copydoc end() const noexcept
  [[nodiscard]] inline const_iterator cend() const noexcept { return storage.cend(); }

  /// @brief True if no headers are stored.
  [[nodiscard]] inline bool empty() const noexcept { return storage.empty(); }
  /// @brief Number of stored headers.
  [[nodiscard]] inline std::size_t size() const noexcept { return storage.size(); }
  /// @brief True if a header with normalized `key` exists.
  [[nodiscard]] inline bool contains(const std::string &key) const { return find(key) != end(); }
  // clang-format on

  /// @brief   Returns reference to the value for `key` (inserting empty string if absent).
  /// @details `key` is normalized (trim + lowercase) before lookup.
  [[nodiscard]] std::string &operator[](const std::string &key);
  /// @copydoc operator[](const std::string &)
  [[nodiscard]] std::string &operator[](std::string &&key);

  /// @brief  Finds element with normalized `key`.
  /// @return Iterator to element, or end() if not found.
  [[nodiscard]] iterator find(const std::string &key);
  /// @copydoc find(const std::string &)
  [[nodiscard]] const_iterator find(const std::string &key) const;

  /// @brief   Parses headers from an input stream up to the first empty line.
  /// @details Stops at CRLF CRLF boundary or EOF. Keys are normalized via
  ///          `utils::normalize`; values are trimmed. Used by both request
  ///          and response parsing.
  /// @return  Parsed headers (empty if the first line is already empty).
  [[nodiscard]] static headers_t parse(std::istream &stream);

  /// @brief  Parses the "Content-Length" header.
  /// @return Parsed length as std::size_t, or 0 if the header is absent.
  /// @throws core::http_error if the value is empty, non-numeric, or has trailing garbage.
  [[nodiscard]] std::size_t content_length() const;

  /// @brief  Computes how many body bytes still need to be read to satisfy Content-Length.
  /// @param  body Currently-read body (used as the "actual" size).
  /// @return 0 if body.size() >= expected; otherwise expected - body.size().
  [[nodiscard]] std::size_t remaining(const std::string &body) const;
};

} // namespace http
