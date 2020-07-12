/**
 * @file packet.hpp
 *
 * This file contains the declarations for the Packet class.
 */
#pragma once

#include <cerrno>
#include <cstdint>
#include <string>

namespace dwmipc {
/**
 * Enum of supported DWM message types
 */
enum MessageType {
    MESSAGE_TYPE_RUN_COMMAND = 0,
    MESSAGE_TYPE_GET_MONITORS = 1,
    MESSAGE_TYPE_GET_TAGS = 2,
    MESSAGE_TYPE_GET_LAYOUTS = 3,
    MESSAGE_TYPE_GET_DWM_CLIENT = 4,
    MESSAGE_TYPE_SUBSCRIBE = 5,
    MESSAGE_TYPE_EVENT = 6
};

/**
 * The magic string that correctly formed DWM message should start with
 */
static constexpr char DWM_MAGIC[] = "DWM-IPC";

/**
 * The length of the magic string excluding the null character
 */
static constexpr int DWM_MAGIC_LEN = 7;

/**
 * This class defines the structure of a basic message that can be sent to DWM
 * or received by DWM. The data allocated by this packet should not be
 * reallocated or freed manually by the user.
 */
class Packet {
  public:
    /**
     * Construct a packet and allocate the specified payload size
     *
     * @param size The size of the message payload not including the header
     */
    Packet(const uint32_t size);

    /**
     * Construct a packet with the specified message type and payload
     *
     * @param type The type of message to send
     * @param msg The payload of the packet. This will normally be a JSON
     * document.
     */
    Packet(const MessageType type, const std::string &msg);

    /**
     * Deconstruct a packet and free the allocated payload.
     */
    ~Packet();

    /**
     * A struct representing the header of an IPC packet.
     */
    struct Header {
        uint8_t magic[DWM_MAGIC_LEN]; ///< Header begins with the magic string
        uint32_t size;            ///< Size of the payload
        uint8_t type;             ///< Type of message sent
    } __attribute((packed));

    /**
     * Size of Header in bytes
     */
    static constexpr int HEADER_SIZE = sizeof(Header);

    uint8_t *data;  ///< Pointer to the start of the packet
    Header *header; ///< Pointer to the start of the header
    uint32_t size;  ///< Size of the entire packet including the header
    char *payload;  ///< Pointer to the start of the payload

    /**
     * Reallocate memory for the packet based on the size specified in the
     * header.
     */
    void realloc_to_header_size();
};

} // namespace dwmipc
