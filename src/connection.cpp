#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "errors.hpp"
#include "connection.hpp"

namespace dwmipc {
Connection::Connection(const std::string &socket_path)
    : sockfd(connect(socket_path)), socket_path(socket_path) {}

Connection::~Connection() { disconnect(); }

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

int Connection::connect(const std::string &socket_path) {
    struct sockaddr_un addr;

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        throw ipc_error("Failed to create dwm socket");
    fcntl(sockfd, F_SETFD, FD_CLOEXEC);

    // Initialize struct to 0
    memset(&addr, 0, sizeof(struct sockaddr_un));

    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(sockfd, (const struct sockaddr *)&addr,
                  sizeof(struct sockaddr_un)) < 0) {
        throw ipc_error("Failed to connect to dwm ipc socket");
    }

    return sockfd;
}

void Connection::disconnect() { close(this->sockfd); }

std::shared_ptr<Packet> Connection::recv_message() {
    uint32_t read_bytes = 0;
    size_t to_read = sizeof(Header);
    auto packet = std::make_shared<Packet>(0);
    char *header = (char *)packet->header;
    char *walk = (char *)packet->data;

    while (read_bytes < to_read) {
        const ssize_t n =
            read(this->sockfd, header + read_bytes, to_read - read_bytes);

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

void Connection::send_message(const std::shared_ptr<Packet> &packet) {
    swrite(this->sockfd, packet->data, packet->size);
}

std::shared_ptr<Packet> Connection::dwm_msg(const MessageType type,
                                            const std::string &msg) {
    auto packet = std::make_shared<Packet>(type, msg);
    send_message(packet);
    auto reply = recv_message();
    if (packet->header->type != reply->header->type)
        throw header_error("Unexpected reply message type: " +
                           std::to_string(reply->header->type));
    return reply;
}

} // namespace dwmipc
