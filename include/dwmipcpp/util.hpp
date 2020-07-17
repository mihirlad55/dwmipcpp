/**
 * @file util.hpp
 *
 * This file defines some function declarations for generic/utility functions.
 */

#pragma once

#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "dwmipcpp/packet.hpp"

namespace dwmipc {
/**
 * Connect to the DWM IPC socket at the specified path and get the file
 * descriptor to the socket.
 *
 * @param socket_path Path to the DWM IPC socket
 * @param is_blocking If false, the socket will have the O_NONBLOCK flag set
 *
 * @return The file descriptor of the socket
 *
 * @throw IPCError if failed to connect to socket
 */
int connect(const std::string &socket_path, bool is_blocking);

/**
 * Disconnect from the DWM IPC socket and close the specified file descriptor
 *
 * @param fd The file descriptor of the socket to close
 */
void disconnect(int fd);

/**
 * Helper function keep attempting to write a buffer to a file descriptor,
 * continuing on EINTR, EAGAIN, or EWOULDBLOCK errors.
 *
 * @param fd File descriptor to write to
 * @param buf Address to a buffer to write to the file descriptor
 * @param count Number of bytes of buffer to write
 *
 * @return Number of bytes written if successful, -1 otherwise
 */
ssize_t swrite(const int fd, const void *buf, const uint32_t count);

/**
 * Receive any incoming messages from the specified socket. This is the main
 * helper function for attempting to read a message from DWM and validate the
 * structure of the message. It absorbs EINTR, EAGAIN, and EWOULDBLOCK errors.
 *
 * @param sockfd The file descriptor of the socket to receive the message
 *   from
 * @param wait Should it wait for a message to be received if a message is
 *   not already received? If this is false, the file descriptor should have
 *   the O_NONBLOCK flag set.
 *
 * @return A received packet from DWM
 *
 * @throw NoMsgError if no messages were received
 * @throw HeaderError if packet with invalid header received
 * @throw EOFError if unexpected EOF while reading message
 */
std::shared_ptr<Packet> recv_message(int sockfd, bool wait);

/**
 * Send a packet to the specified socket
 *
 * @param sockfd The file descriptor of the socket to write the message to
 * @param packet The packet to send
 *
 * @throw ReplyError if invalid reply received. This could be caused by
 *   receiving an event message when expecting a reply to the newly sent
 *   message.
 */
void send_message(int sockfd, const std::shared_ptr<Packet> &packet);

} // namespace dwmipc
