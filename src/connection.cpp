#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <json/json.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "connection.hpp"
#include "errors.hpp"

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

static std::shared_ptr<Json::Value>
pre_parse_reply(const std::shared_ptr<Packet> &reply) {
    std::string payload(reply->payload, reply->header->size);
    const char *start = payload.c_str();
    const char *end = start + reply->header->size;

    Json::Value root;
    std::string errs;

    const Json::CharReaderBuilder builder;
    const auto reader = builder.newCharReader();
    reader->parse(start, end, &root, &errs);

    if (root.isObject() && root["result"])
        throw result_failure_error(root["reason"].asString());

    return std::make_shared<Json::Value>(root);
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
        throw reply_error(packet->header->type, reply->header->type);
    return reply;
}

std::shared_ptr<Packet> Connection::dwm_msg(const MessageType type) {
    return dwm_msg(type, "");
}

std::shared_ptr<std::vector<Monitor>> Connection::get_monitors() {
    auto reply = dwm_msg(MESSAGE_TYPE_GET_MONITORS);
    auto root = pre_parse_reply(reply);
    auto monitors = std::make_shared<std::vector<Monitor>>();

    for (Json::Value v_mon : *root) {
        Monitor mon;

        mon.layout_symbol = v_mon["layout_symbol"].asString();
        mon.old_layout = v_mon["layout"]["old"].asString();
        mon.current_layout = v_mon["layout"]["current"].asString();
        mon.mfact = v_mon["mfact"].asFloat();
        mon.nmaster = v_mon["nmaster"].asInt();
        mon.num = v_mon["num"].asInt();
        mon.bar_y = v_mon["bar_y"].asInt();
        mon.mx = v_mon["screen"]["x"].asInt();
        mon.mx = v_mon["screen"]["y"].asInt();
        mon.mw = v_mon["screen"]["width"].asInt();
        mon.mh = v_mon["screen"]["height"].asInt();
        mon.wx = v_mon["screen"]["x"].asInt();
        mon.wx = v_mon["screen"]["y"].asInt();
        mon.ww = v_mon["screen"]["width"].asInt();
        mon.wh = v_mon["screen"]["height"].asInt();
        mon.tagset = v_mon["tag_set"]["old"].asUInt();
        mon.old_tagset = v_mon["tag_set"]["new"].asUInt();
        mon.show_bar = v_mon["show_bar"].asBool();
        mon.top_bar = v_mon["top_bar"].asBool();
        mon.win_sel = v_mon["selected_client"].asUInt();
        mon.win_bar = v_mon["bar_window_id"].asUInt();

        for (Json::Value v : v_mon["clients"]) {
            mon.win_clients.push_back(v.asUInt());
        }

        for (Json::Value v : v_mon["stack"]) {
            mon.win_clients.push_back(v.asUInt());
        }
        monitors->push_back(mon);
    }
    return monitors;
}

std::shared_ptr<std::vector<Tag>> Connection::get_tags() {
    auto reply = dwm_msg(MESSAGE_TYPE_GET_TAGS);
    auto root = pre_parse_reply(reply);
    auto tags = std::make_shared<std::vector<Tag>>();

    for (Json::Value v_tag : *root) {
        Tag tag;

        tag.bit_mask = v_tag["bit_mask"].asUInt();
        tag.tag_name = v_tag["name"].asString();
        tags->push_back(tag);
    }
    return tags;
}

std::shared_ptr<std::vector<Layout>> Connection::get_layouts() {
    auto reply = dwm_msg(MESSAGE_TYPE_GET_LAYOUTS);
    auto root = pre_parse_reply(reply);
    auto layouts = std::make_shared<std::vector<Layout>>();

    for (Json::Value v_lt : *root) {
        Layout lt;

        lt.address = v_lt["layout_address"].asUInt64();
        lt.symbol = v_lt["layout_symbol"].asString();

        layouts->push_back(lt);
    }
    return layouts;
}

std::shared_ptr<Client> Connection::get_client(Window win_id) {
    // No need to generate the JSON using library since it is so simple
    const std::string msg =
        "{\"client_window_id\":" + std::to_string(win_id) + "}";
    auto reply = dwm_msg(MESSAGE_TYPE_GET_DWM_CLIENT, msg);
    auto root = pre_parse_reply(reply);
    auto client = std::make_shared<Client>();

    client->name = (*root)["name"].asString();
    client->mina = (*root)["mina"].asInt();
    client->maxa = (*root)["maxa"].asInt();
    client->tags = (*root)["tags"].asUInt();
    client->x = (*root)["size"]["current"]["x"].asInt();
    client->y = (*root)["size"]["current"]["y"].asInt();
    client->w = (*root)["size"]["current"]["width"].asInt();
    client->h = (*root)["size"]["current"]["height"].asInt();
    client->oldx = (*root)["size"]["old"]["x"].asInt();
    client->oldy = (*root)["size"]["old"]["y"].asInt();
    client->oldw = (*root)["size"]["old"]["width"].asInt();
    client->oldh = (*root)["size"]["old"]["height"].asInt();
    client->basew = (*root)["size_hints"]["base_width"].asInt();
    client->baseh = (*root)["size_hints"]["base_height"].asInt();
    client->incw = (*root)["size_hints"]["increase_width"].asInt();
    client->inch = (*root)["size_hints"]["increase_height"].asInt();
    client->maxw = (*root)["size_hints"]["max_width"].asInt();
    client->maxh = (*root)["size_hints"]["max_height"].asInt();
    client->minw = (*root)["size_hints"]["min_width"].asInt();
    client->minh = (*root)["size_hints"]["min_height"].asInt();
    client->bw = (*root)["border"]["current_width"].asInt();
    client->oldbw = (*root)["border"]["old_width"].asInt();
    client->isfixed = (*root)["states"]["is_fixed"].asBool();
    client->isfloating = (*root)["states"]["is_floating"].asBool();
    client->isurgent = (*root)["states"]["is_urgent"].asBool();
    client->isfullscreen = (*root)["states"]["is_fullscreen"].asBool();
    client->neverfocus = (*root)["states"]["never_focus"].asBool();
    client->oldstate = (*root)["states"]["old_state"].asBool();

    return client;
}

} // namespace dwmipc