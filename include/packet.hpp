#pragma once

#include <cerrno>
#include <cstdint>
#include <string>

namespace dwmipc {

#define DWM_MAGIC "DWM-IPC"
#define DWM_MAGIC_LEN 7

typedef struct Header {
    uint8_t magic[DWM_MAGIC_LEN];
    uint32_t size;
    uint8_t type;
} __attribute((packed)) Header;

enum MessageType {
    MESSAGE_TYPE_RUN_COMMAND = 0,
    MESSAGE_TYPE_GET_MONITORS = 1,
    MESSAGE_TYPE_GET_TAGS = 2,
    MESSAGE_TYPE_GET_LAYOUTS = 3,
    MESSAGE_TYPE_GET_DWM_CLIENT = 4,
    MESSAGE_TYPE_SUBSCRIBE = 5,
    MESSAGE_TYPE_EVENT = 6
};

class Packet {
  public:
    Packet(const uint32_t size);
    Packet(const MessageType type, const std::string &msg);

    ~Packet();

    uint8_t *data;
    Header *header;
    uint32_t size;
    char *payload;

    void realloc_to_header_size();
};

} // namespace dwmipc
