#pragma once

#include <memory>
#include <string>
#include <vector>

#include "packet.hpp"

namespace dwmipc {

class Connection {
  public:
    Connection(const std::string &socket_path);
    ~Connection();

  private:
    const int sockfd;
    const std::string socket_path;

    static int connect(const std::string &socket_path);

    void disconnect();
    void send_message(const std::shared_ptr<Packet> &packet);

    std::shared_ptr<Packet> recv_message();
    std::shared_ptr<Packet> dwm_msg(const MessageType type, const std::string &msg);
};
} // namespace dwmipc
