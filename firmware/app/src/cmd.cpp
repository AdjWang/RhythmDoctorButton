#include "cmd.h"

#include <charconv>
#include <cmath>
#include <system_error>
#include <utility>

namespace rdb {
namespace {

bool IsSpace(const char c) {
  return c == ' ' || c == '\t';
}

bool IsAcceptedInputChar(const char c) {
  return c == '\t' || (c >= ' ' && c <= '~');
}

std::string_view Trim(std::string_view text) {
  while (!text.empty() && IsSpace(text.front())) {
    text.remove_prefix(1);
  }
  while (!text.empty() && IsSpace(text.back())) {
    text.remove_suffix(1);
  }
  return text;
}

std::string FormatMessage(const std::string_view message,
                          const std::string_view input,
                          const size_t position) {
  return std::string(message) + " at position " + std::to_string(position) +
         " in '" + std::string(input) + "'";
}

Error MakeError(const ErrorCode code, const std::string_view message,
                const std::string_view input, const size_t position = 0) {
  return Error{
      .code = code,
      .message = FormatMessage(message, input, position),
  };
}

std::optional<bool> ParseBool(const std::string_view value) {
  if (value == "true") {
    return true;
  }
  if (value == "false") {
    return false;
  }
  return std::nullopt;
}

std::optional<float> ParseFloat(const std::string_view value) {
  float parsed = 0.0f;
  const auto* const begin = value.data();
  const auto* const end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(parsed)) {
    return std::nullopt;
  }
  return parsed;
}

}  // namespace

CmdParser::CmdParser(const size_t max_line_size)
    : max_line_size_(max_line_size) {
  line_.reserve(max_line_size_);
}

void CmdParser::RegisterCommand(const std::string_view key,
                                const ValueType type) {
  commands_.push_back(CommandSpec{
      .key = std::string(key),
      .type = type,
  });
}

std::optional<CmdParser::Result> CmdParser::Feed(const char byte) {
  if (byte == '\n' && last_was_cr_) {
    last_was_cr_ = false;
    return std::nullopt;
  }
  if (byte == '\r' || byte == '\n') {
    last_was_cr_ = byte == '\r';
    Result result = ParseLine(line_);
    line_.clear();
    return result;
  }
  last_was_cr_ = false;
  if (byte == '\b' || byte == 0x7f) {
    if (!line_.empty()) {
      line_.pop_back();
    }
    return std::nullopt;
  }
  if (!IsAcceptedInputChar(byte)) {
    Error error = MakeError(ErrorCode::kCmdInvalidCharacter,
                            "invalid control character", line_, line_.size());
    Reset();
    return std::unexpected(std::move(error));
  }
  if (line_.size() >= max_line_size_ - 1) {
    Error error = MakeError(ErrorCode::kCmdLineTooLong, "line is too long",
                            line_, line_.size());
    Reset();
    return std::unexpected(std::move(error));
  }
  line_.push_back(byte);
  return std::nullopt;
}

CmdParser::Result CmdParser::ParseLine(const std::string_view input) const {
  const std::string_view trimmed_input = Trim(input);
  if (trimmed_input.empty()) {
    return std::unexpected(
        MakeError(ErrorCode::kCmdEmptyLine, "empty command", input));
  }
  const size_t trim_offset =
      static_cast<size_t>(trimmed_input.data() - input.data());
  const size_t equal_pos = trimmed_input.find('=');
  if (equal_pos == std::string_view::npos) {
    return std::unexpected(
        MakeError(ErrorCode::kCmdMissingEquals, "missing '='", input));
  }
  const size_t extra_equal_pos = trimmed_input.find('=', equal_pos + 1);
  if (extra_equal_pos != std::string_view::npos) {
    return std::unexpected(MakeError(ErrorCode::kCmdExtraEquals, "extra '='",
                                     input, trim_offset + extra_equal_pos));
  }
  const std::string_view key_text = Trim(trimmed_input.substr(0, equal_pos));
  const std::string_view value_text = Trim(trimmed_input.substr(equal_pos + 1));
  if (key_text.empty()) {
    return std::unexpected(
        MakeError(ErrorCode::kCmdEmptyKey, "empty key", input, trim_offset));
  }
  if (value_text.empty()) {
    return std::unexpected(MakeError(ErrorCode::kCmdEmptyValue, "empty value",
                                     input, trim_offset + equal_pos + 1));
  }
  const CommandSpec* const spec = FindCommand(key_text);
  if (spec == nullptr) {
    return std::unexpected(
        MakeError(ErrorCode::kCmdUnknownKey, "unknown key", input,
                  trim_offset));
  }
  if (spec->type == ValueType::kBool) {
    const std::optional<bool> value = ParseBool(value_text);
    if (!value.has_value()) {
      return std::unexpected(MakeError(ErrorCode::kCmdInvalidBool,
                                       "value must be true or false", input,
                                       trim_offset + equal_pos + 1));
    }
    return Command{
        .key = std::string(key_text),
        .value_text = std::string(value_text),
        .type = spec->type,
        .value = *value,
    };
  }
  const std::optional<float> value = ParseFloat(value_text);
  if (!value.has_value()) {
    return std::unexpected(MakeError(ErrorCode::kCmdInvalidFloat,
                                     "value must be a finite float", input,
                                     trim_offset + equal_pos + 1));
  }
  return Command{
      .key = std::string(key_text),
      .value_text = std::string(value_text),
      .type = spec->type,
      .value = *value,
  };
}

void CmdParser::Reset() {
  line_.clear();
  last_was_cr_ = false;
}

const CmdParser::CommandSpec* CmdParser::FindCommand(
    const std::string_view key) const {
  for (const CommandSpec& command : commands_) {
    if (command.key == key) {
      return &command;
    }
  }
  return nullptr;
}

std::string_view ToString(const CmdParser::ValueType type) {
  switch (type) {
    case CmdParser::ValueType::kBool:
      return "bool";
    case CmdParser::ValueType::kFloat:
      return "float";
  }
  return "unknown";
}

std::string_view ToString(const ErrorCode code) {
  switch (code) {
    case ErrorCode::kCmdEmptyLine:
      return "cmd_empty_line";
    case ErrorCode::kCmdLineTooLong:
      return "cmd_line_too_long";
    case ErrorCode::kCmdInvalidCharacter:
      return "cmd_invalid_character";
    case ErrorCode::kCmdMissingEquals:
      return "cmd_missing_equals";
    case ErrorCode::kCmdExtraEquals:
      return "cmd_extra_equals";
    case ErrorCode::kCmdEmptyKey:
      return "cmd_empty_key";
    case ErrorCode::kCmdEmptyValue:
      return "cmd_empty_value";
    case ErrorCode::kCmdUnknownKey:
      return "cmd_unknown_key";
    case ErrorCode::kCmdInvalidBool:
      return "cmd_invalid_bool";
    case ErrorCode::kCmdInvalidFloat:
      return "cmd_invalid_float";
  }
  return "unknown";
}

}  // namespace rdb
