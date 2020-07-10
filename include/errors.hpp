#pragma once

#include <stdexcept>

namespace dwmipc {
class ipc_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class header_error : public ipc_error {
  public:
    header_error(const size_t read, const size_t expected);
    header_error(const std::string &msg);
};

class eof_error : public ipc_error {
  public:
    eof_error(const size_t read, const size_t expected);
};

class reply_error : public ipc_error {
  public:
    reply_error(const int expected, const int got);
};

class result_failure_error : public ipc_error {
  public:
    result_failure_error(const std::string &reason);
};

class errno_error : public ipc_error {
  public:
    errno_error();
    errno_error(const std::string &msg);
};

} // namespace dwmipc