/**
 * @file errors.cpp
 *
 * This file contains the implementation details for errors.hpp.
 */
#include <cstring>
#include <sstream>
#include <string>

#include "dwmipcpp/errors.hpp"

namespace dwmipc {
static std::string format_generic(const std::string &msg) {
    std::stringstream out;
    out << "[dwmipcpp] " << msg;
    return out.str();
}

static std::string format_errno(const std::string &msg = std::string()) {
    std::stringstream out;
    out << "[dwmipcpp] errno " << errno << ": " << msg << "(" << strerror(errno)
        << ")";
    return out.str();
}

static std::string format_eof(const size_t read_bytes, const size_t to_read) {
    std::stringstream out;
    out << "[dwmipcpp] Unexpected EOF (" << read_bytes << " bytes read, "
        << to_read << " bytes expected)";
    return out.str();
}

static std::string format_reply(const int expected, const int got) {
    std::stringstream out;
    out << "[dwmipcpp] Unexpected reply type (Got " << got << " type, wanted "
        << expected << " type)";
    return out.str();
}

static std::string format_closed_socket(const int fd) {
    std::stringstream out;
    out << "[dwmipcpp] "
        << " Socket with file descriptor " << fd << " closed";
    return out.str();
}

HeaderError::HeaderError(const size_t read_bytes, const size_t to_read)
    : IPCError(format_eof(read_bytes, to_read)) {}

HeaderError::HeaderError(const std::string &msg)
    : IPCError(format_errno(msg)) {}

EOFError::EOFError(const size_t read_bytes, const size_t to_read)
    : IPCError(format_eof(read_bytes, to_read)) {}

NoMsgError::NoMsgError() : IPCError(format_generic("No messages available")) {}

ReplyError::ReplyError(const int expected, const int got)
    : IPCError(format_reply(expected, got)) {}

ResultFailureError::ResultFailureError(const std::string &reason)
    : IPCError(format_generic(reason)) {}

ErrnoError::ErrnoError() : IPCError(format_errno()) {}

ErrnoError::ErrnoError(const std::string &msg) : IPCError(format_errno(msg)) {}

SocketClosedError::SocketClosedError(const int fd)
    : IPCError(format_closed_socket(fd)) {}

SocketClosedError::SocketClosedError(const std::string &msg)
    : IPCError(format_generic(msg)) {}

} // namespace dwmipc
