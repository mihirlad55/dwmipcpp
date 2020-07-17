/**
 * @file util.cpp
 *
 * This file contains definitions of some utility/genric functions as declared
 * in util.hpp
 */

#include "dwmipcpp/util.hpp"

#include <cstring>

#include "dwmipcpp/errors.hpp"

namespace dwmipc {

int connect(const std::string &socket_path, bool is_blocking) {
    struct sockaddr_un addr;

    int flags = SOCK_STREAM | SOCK_CLOEXEC;
    if (!is_blocking)
        flags |= SOCK_NONBLOCK;

    int sockfd = socket(AF_UNIX, flags, 0);
    if (sockfd < 0)
        throw IPCError("Failed to create dwm socket");

    // Initialize struct to 0
    memset(&addr, 0, sizeof(struct sockaddr_un));

    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(sockfd, reinterpret_cast<struct sockaddr *>(&addr),
                  sizeof(struct sockaddr_un)) < 0) {
        throw IPCError("Failed to connect to dwm ipc socket");
    }

    return sockfd;
}

void disconnect(int fd) {
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

ssize_t swrite(const int fd, const void *buf, const uint32_t count) {
    size_t written = 0;

    while (written < count) {
        const ssize_t n = send(fd, buf, count, MSG_NOSIGNAL);

        if (n == -1) {
            // This may block, but shouldn't for long
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else if (errno == EPIPE)
                throw SocketClosedError(fd);

            throw ErrnoError("Error writing buffer to dwm socket");
        }
        written += n;
    }
    return written;
}

std::shared_ptr<Packet> recv_message(int sockfd, bool wait) {
    uint32_t read_bytes = 0;
    size_t to_read = Packet::HEADER_SIZE;
    auto packet = std::make_shared<Packet>(0);
    char *header = reinterpret_cast<char *>(packet->header);
    char *walk = reinterpret_cast<char *>(packet->data);

    // Read packet header
    while (read_bytes < to_read) {
        const ssize_t n =
            read(sockfd, header + read_bytes, to_read - read_bytes);

        if (n == 0) {
            if (read_bytes == 0) {
                // If no bytes returned and no bytes read, socket is closed
                throw SocketClosedError(sockfd);
            }
            throw HeaderError(read_bytes, to_read);
        } else if (n == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                // If no bytes read yet and we shouldn't wait for a message,
                // throw a NoMsgError. If the fd is non-blocking, this will most
                // likely occur if there is nothing to be read.
                if (read_bytes == 0 && !wait)
                    throw NoMsgError();
                continue;
            }
            throw ErrnoError("Error reading header");
        }
        read_bytes += n;
    }

    // Check if magic string is correct
    if (memcmp(header, DWM_MAGIC, DWM_MAGIC_LEN) != 0)
        throw HeaderError("Invalid magic string: " +
                          std::string(header, DWM_MAGIC_LEN));

    // Reallocate payload size based on message size in header
    packet->realloc_to_header_size();

    // Reinitialize addresses to header and walk
    header = reinterpret_cast<char *>(packet->header);
    walk = reinterpret_cast<char *>(packet->payload);

    // Extract payload
    read_bytes = 0;
    to_read = packet->header->size;
    while (read_bytes < to_read) {
        const ssize_t n = read(sockfd, walk + read_bytes, to_read - read_bytes);

        if (n == 0)
            throw EOFError(read_bytes, to_read);
        else if (n == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            throw ErrnoError("Error reading payload");
        }
        read_bytes += n;
    }
    return packet;
}

void send_message(int sockfd, const std::shared_ptr<Packet> &packet) {
    swrite(sockfd, packet->data, packet->size);
}

} // namespace dwmipc
