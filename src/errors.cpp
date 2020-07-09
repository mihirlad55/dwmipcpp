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

header_error::header_error(const size_t read_bytes, const size_t to_read)
    : ipc_error(format_eof(read_bytes, to_read)) {}

header_error::header_error(const std::string &msg)
    : ipc_error(format_errno(msg)) {}

eof_error::eof_error(const size_t read_bytes, const size_t to_read)
    : ipc_error(format_eof(read_bytes, to_read)) {}

errno_error::errno_error() : ipc_error(format_errno()) {}

errno_error::errno_error(const std::string &msg)
    : ipc_error(format_errno(msg)) {}

} // namespace dwmipc
