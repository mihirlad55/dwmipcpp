#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "util.hpp"

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

static ssize_t swrite(const int fd, const void *buf, const uint32_t count) {
    size_t written = 0;

    while (written < count) {
        const ssize_t n = write(fd, buf, count);

        if (n == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            throw errno_error("Error writing buffer to dwm socket");
        }
        written += n;
    }
    return written;
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

Packet::Packet(const uint32_t payload_size)
    : size(payload_size + sizeof(Header)) {
    // Use malloc since primitive type, and to allow realloc
    this->data = (uint8_t *)malloc(sizeof(uint8_t) * size);
    this->header = (Header *)data;
    this->payload = (char *)(data + sizeof(Header));
    this->header->size = payload_size;
    std::memcpy(this->header->magic, DWM_MAGIC, DWM_MAGIC_LEN);
    this->header->type = 0;
}

Packet::Packet(const MessageType type, const std::string &msg)
    : Packet(msg.size()) {
    this->header->type = type;
    std::strncpy(this->payload, msg.c_str(), msg.size());
}

Packet::~Packet() { free(this->data); }

void Packet::realloc_to_header_size() {
    this->size = this->header->size + sizeof(Header);
    this->data = (uint8_t *)realloc(this->data, this->size);
    this->header = (Header *)this->data;
    this->payload = (char *)(this->data + sizeof(Header));
}

int connect(std::string &socket_path) {
    struct sockaddr_un addr;

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        throw ipc_error("Failed to create dwm socket");
    fcntl(sockfd, F_SETFD, FD_CLOEXEC);

    // Initialize struct to 0
    memset(&addr, 0, sizeof(struct sockaddr_un));

    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (const struct sockaddr *)&addr,
                sizeof(struct sockaddr_un)) < 0) {
        throw ipc_error("Failed to connect to dwm ipc socket");
    }

    return sockfd;
}

void disconnect(const int sockfd) { close(sockfd); }

std::shared_ptr<Packet> recv_message(const int sockfd) {
    uint32_t read_bytes = 0;
    size_t to_read = sizeof(Header);
    auto packet = std::make_shared<Packet>(0);
    char *header = (char *)packet->header;
    char *walk = (char *)packet->data;

    while (read_bytes < to_read) {
        const ssize_t n =
            read(sockfd, header + read_bytes, to_read - read_bytes);

        if (n == 0) {
            if (read_bytes == 0) {
                throw eof_error(read_bytes, to_read);
            } else {
                throw header_error(read_bytes, to_read);
            }
        } else if (n == -1) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            throw errno_error("Error reading header");
        }
        read_bytes += n;
    }

    if (memcmp(header, DWM_MAGIC, DWM_MAGIC_LEN) != 0)
        throw header_error("Invalid magic string: " +
                           std::string(header, DWM_MAGIC_LEN));

    packet->realloc_to_header_size();
    header = (char *)packet->header;
    walk = (char *)packet->payload;

    // Extract payload
    read_bytes = 0;
    to_read = packet->header->size;
    while (read_bytes < to_read) {
        const ssize_t n = read(sockfd, walk + read_bytes, to_read - read_bytes);

        if (n == 0)
            throw eof_error(read_bytes, to_read);
        else if (n == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            throw errno_error("Error reading payload");
        }
        read_bytes += n;
    }
    return packet;
}

void send_message(const int sockfd, const std::shared_ptr<Packet> &packet) {
    swrite(sockfd, packet->data, packet->size);
}

std::shared_ptr<Packet> dwm_msg(const int sockfd, const MessageType type,
                                const std::string &msg) {
    auto packet = std::make_shared<Packet>(type, msg);
    send_message(sockfd, packet);
    auto reply = recv_message(sockfd);
    if (packet->header->type != reply->header->type)
        throw header_error("Unexpected reply message type: " +
                           std::to_string(reply->header->type));
    return reply;
}

} // namespace dwmipc
