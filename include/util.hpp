#pragma once

#include <memory>
#include <stdexcept>
#include <string>

namespace dwmipc {
const char *DWM_MAGIC = "DWM-IPC";
constexpr int DWM_MAGIC_LEN = 7;

typedef struct Header {
    uint8_t magic[DWM_MAGIC_LEN];
    uint32_t size;
    uint8_t type;
} __attribute((packed)) Header;

enum MessageType {
    IPC_TYPE_RUN_COMMAND = 0,
    IPC_TYPE_GET_MONITORS = 1,
    IPC_TYPE_GET_TAGS = 2,
    IPC_TYPE_GET_LAYOUTS = 3,
    IPC_TYPE_GET_DWM_CLIENT = 4,
    IPC_TYPE_SUBSCRIBE = 5,
    IPC_TYPE_EVENT = 6
};

class ipc_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class header_error : public ipc_error {
  public:
    header_error(const size_t read, const size_t expected);
    header_error(const std::string &msg);
};

class eof_error : public ipc_error {
  public:
    eof_error(const size_t read, const size_t expected);
};

class errno_error : public ipc_error {
  public:
    errno_error();
    errno_error(const std::string &msg);
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

int connect(const std::string &socket_path);

void disconnect(const int sockfd);

std::shared_ptr<Packet> recv_message(const int sockfd);

void send_message(const int sockfd, const Packet &packet);

std::shared_ptr<Packet> dwm_msg(const int sockfd, const MessageType type,
                                const std::string &msg);

} // namespace dwmipc
