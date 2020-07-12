#pragma once

#include <stdexcept>

namespace dwmipc {
class IPCError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class HeaderError : public IPCError {
  public:
    HeaderError(const size_t read, const size_t expected);
    HeaderError(const std::string &msg);
};

class EOFError : public IPCError {
  public:
    EOFError(const size_t read, const size_t expected);
};

class NoMsgError : public IPCError {
  public:
    NoMsgError();
};

class ReplyError : public IPCError {
  public:
    ReplyError(const int expected, const int got);
};

class ResultFailureError : public IPCError {
  public:
    ResultFailureError(const std::string &reason);
};

class ErrnoError : public IPCError {
  public:
    ErrnoError();
    ErrnoError(const std::string &msg);
};

} // namespace dwmipc
