#pragma once

#include "error.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace rdb {

class CmdParser {
 public:
  enum class ValueType {
    kBool,
    kFloat,
  };

  struct Command {
    std::string key;
    std::string value_text;
    ValueType type;
    std::variant<bool, float> value;
  };

  using Result = rdb::Result<Command>;

  explicit CmdParser(size_t max_line_size = 96);

  void RegisterCommand(std::string_view key, ValueType type);

  std::optional<Result> Feed(char byte);
  Result ParseLine(std::string_view input) const;
  void Reset();

 private:
  struct CommandSpec {
    std::string key;
    ValueType type;
  };

  const CommandSpec* FindCommand(std::string_view key) const;

  size_t max_line_size_;
  std::string line_;
  std::vector<CommandSpec> commands_;
  bool last_was_cr_ = false;
};

std::string_view ToString(CmdParser::ValueType type);
std::string_view ToString(ErrorCode code);

}  // namespace rdb
