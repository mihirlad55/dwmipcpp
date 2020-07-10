#include <cstring>
#include <sstream>
#include <string>

#include "errors.hpp"

namespace dwmipc {
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

static std::string format_failure(const std::string &reason) {
    std::stringstream out;
    out << "[dwmipcpp] " << reason;
    return out.str();
}

header_error::header_error(const size_t read_bytes, const size_t to_read)
    : ipc_error(format_eof(read_bytes, to_read)) {}

header_error::header_error(const std::string &msg)
    : ipc_error(format_errno(msg)) {}

eof_error::eof_error(const size_t read_bytes, const size_t to_read)
    : ipc_error(format_eof(read_bytes, to_read)) {}

reply_error::reply_error(const int expected, const int got)
    : ipc_error(format_reply(expected, got)) {}

result_failure_error::result_failure_error(const std::string &reason)
    : ipc_error(format_failure(reason)) {}

errno_error::errno_error() : ipc_error(format_errno()) {}

errno_error::errno_error(const std::string &msg)
    : ipc_error(format_errno(msg)) {}

} // namespace dwmipc
