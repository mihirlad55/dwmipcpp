/**
 * @file errors.hpp
 *
 * This file contains declarations for library related errors. This file is used
 * internally by dwmipcpp and these classes should not be instantiated outside
 * of the library.
 */

#pragma once

#include <stdexcept>

namespace dwmipc {
/**
 * Base DWM IPC error class.
 */
class IPCError : public std::runtime_error {
    // Inherit constructor that takes an std::string
    using std::runtime_error::runtime_error;
};

/**
 * This error is thrown when a message with a malformed header is read
 */
class HeaderError : public IPCError {
  public:
    /**
     * Construct a HeaderError in the case of an end of file while trying to
     * read the header.
     *
     * @param read The number of bytes read before EOF
     * @param expected The number of bytes expected to have been read before EOF
     */
    HeaderError(const size_t read, const size_t expected);

    /**
     * Construct a HeaderError with the specified message
     *
     * @param msg The details of why this error is being thrown
     */
    HeaderError(const std::string &msg);
};

/**
 * This error is thrown when an EOF is reached while trying to read the message
 * payload.
 */
class EOFError : public IPCError {
  public:
    /**
     * Construct a EOFError in the case of an end of file while trying to read
     * the message payload.
     *
     * @param read The number of bytes read before EOF
     * @param expected The number of bytes expected to have been read before EOF
     */
    EOFError(const size_t read, const size_t expected);
};

/**
 * This error is thrown when a read is attempted on the DWM socket when there
 * is no message to read.
 */
class NoMsgError : public IPCError {
  public:
    /**
     * Construct a NoMsgError with its default error message.
     */
    NoMsgError();
};

/**
 * This error is thrown when an invalid reply is received from DWM after sending
 * it a message. This is most commonly caused by the type of the reply message
 * not matching the type of the sent message.
 */
class ReplyError : public IPCError {
  public:
    /**
     * Construct a ReplyError when there was a type mismatch
     *
     * @param expected The message type expected in the reply
     * @param got The message type that was specified in the reply
     */
    ReplyError(const int expected, const int got);
};

/**
 * This error is thrown when DWM replies with a result: failure, which is caused
 * by a successful message send, but unsuccessful message processing by DWM.
 */
class ResultFailureError : public IPCError {
  public:
    /**
     * Construct a ResultFailureError specifying DWM's reason for replying with
     * a failure message.
     */
    ResultFailureError(const std::string &reason);
};

/**
 * This error is thrown when there is an error using a C function that sets
 * errno.
 */
class ErrnoError : public IPCError {
  public:
    /**
     * Construct an ErrnoError with the message defaulting to the error code and
     * message specified by errno and strerror(errno).
     */
    ErrnoError();

    /**
     * Construct an ErrnoError with a specific message to add on to the default
     * message.
     */
    ErrnoError(const std::string &msg);
};

/**
 * This error is thrown when a read/write operation is attempted on a
 * disconnected socket
 */
class SocketClosedError : public IPCError {
  public:
    /**
     * Construct a SocketClosedError with the specified message
     */
    SocketClosedError(const std::string &msg);

    /**
     * Construct a SocketClosedError with the specified socket file descriptor
     *
     * @param fd The file descriptor of the socket that has been closed
     */
    SocketClosedError(const int fd);
};

} // namespace dwmipc
