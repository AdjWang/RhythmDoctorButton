#pragma once

#include <expected>
#include <string>

namespace rdb {

enum class ErrorCode : int {
  kCmdEmptyLine,
  kCmdLineTooLong,
  kCmdInvalidCharacter,
  kCmdMissingEquals,
  kCmdExtraEquals,
  kCmdEmptyKey,
  kCmdEmptyValue,
  kCmdUnknownKey,
  kCmdInvalidBool,
  kCmdInvalidFloat,
};

struct Error {
  ErrorCode code;
  std::string message;
};

template <typename T>
using Result = std::expected<T, Error>;

}  // namespace rdb
