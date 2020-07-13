/**
 * @file connection.cpp
 *
 * This file contains the implementation details for the Connection class as
 * well as some helper functions for writing/reading sockets.
 */

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

/**
 * Helper function keep attempting to write a buffer to a file descriptor,
 * continuing on EINTR, EAGAIN, or EWOULDBLOCK errors.
 */
static ssize_t swrite(const int fd, const void *buf, const uint32_t count) {
    size_t written = 0;

    while (written < count) {
        const ssize_t n = write(fd, buf, count);

        if (n == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            throw ErrnoError("Error writing buffer to dwm socket");
        }
        written += n;
    }
    return written;
}

/**
 * Parse a reply packet from DWM into a Json::Value. Throw a ResultFailureError
 * if DWM sends an error reply.
 */
static void pre_parse_reply(Json::Value &root,
                            const std::shared_ptr<Packet> &reply) {
    const char *start = reply->payload;
    const char *end = start + reply->header->size - 1;

    std::string errs;

    const Json::CharReaderBuilder builder;
    const auto reader = builder.newCharReader();
    reader->parse(start, end, &root, &errs);

    // Not properly documented, but if the reply is an array any type of
    // function that checks for the existance of a key throws a Json::LogicError
    if (!root.isArray() && root.get("result", "") == "error")
        throw ResultFailureError(root["reason"].asString());

    delete reader;
}

/**
 * Parse a dwmipc::EVENT_TAG_CHANGE message
 */
static void parse_tag_change_event(const Json::Value &root,
                                   TagChangeEvent &event) {
    const std::string ev_name = event_map.at(Event::TAG_CHANGE);
    auto v_event = root[ev_name];
    auto v_old_state = v_event["old_state"];
    auto v_new_state = v_event["new_state"];

    event.monitor_num = v_event["monitor_number"].asUInt();
    event.old_state.selected = v_old_state["selected"].asUInt();
    event.old_state.occupied = v_old_state["occupied"].asUInt();
    event.old_state.urgent = v_old_state["urgent"].asUInt();
    event.new_state.selected = v_new_state["selected"].asUInt();
    event.new_state.occupied = v_new_state["occupied"].asUInt();
    event.new_state.urgent = v_new_state["urgent"].asUInt();
}

/**
 * Parse a dwmipc::EVENT_LAYOUT_CHANGE message
 */
static void parse_layout_change_event(const Json::Value &root,
                                      LayoutChangeEvent &event) {
    const std::string ev_name = event_map.at(Event::LAYOUT_CHANGE);
    auto v_event = root[ev_name];

    event.monitor_num = v_event["monitor_number"].asUInt();
    event.old_symbol = v_event["old_symbol"].asString();
    event.old_address = v_event["old_address"].asUInt64();
    event.new_symbol = v_event["new_symbol"].asString();
    event.new_address = v_event["new_address"].asUInt64();
}

/**
 * Parse a dwmipc::EVENT_SELECTED_CLIENT_CHANGE message
 */
static void
parse_selected_client_change_event(const Json::Value &root,
                                   SelectedClientChangeEvent &event) {
    const std::string ev_name = event_map.at(Event::SELECTED_CLIENT_CHANGE);
    auto v_event = root[ev_name];

    event.monitor_num = v_event["monitor_number"].asUInt();
    event.old_win_id = v_event["old_win_id"].asUInt();
    event.new_win_id = v_event["new_win_id"].asUInt();
}

int Connection::connect(const std::string &socket_path) {
    struct sockaddr_un addr;

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        throw IPCError("Failed to create dwm socket");
    fcntl(sockfd, F_SETFD, FD_CLOEXEC);

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

void Connection::disconnect() { close(this->sockfd); }

std::shared_ptr<Packet> Connection::recv_message() const {
    uint32_t read_bytes = 0;
    size_t to_read = Packet::HEADER_SIZE;
    auto packet = std::make_shared<Packet>(0);
    char *header = reinterpret_cast<char *>(packet->header);
    char *walk = reinterpret_cast<char *>(packet->data);

    // Read packet header
    while (read_bytes < to_read) {
        const ssize_t n =
            read(this->sockfd, header + read_bytes, to_read - read_bytes);

        if (n == 0) {
            if (read_bytes == 0)
                throw NoMsgError();
            else
                throw HeaderError(read_bytes, to_read);
        } else if (n == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
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

void Connection::send_message(const std::shared_ptr<Packet> &packet) const {
    swrite(this->sockfd, packet->data, packet->size);
}

std::shared_ptr<Packet> Connection::dwm_msg(const MessageType type,
                                            const std::string &msg) const {
    auto packet = std::make_shared<Packet>(type, msg);
    send_message(packet);
    auto reply = recv_message();

    // Sent message type should match reply message type
    if (packet->header->type != reply->header->type)
        throw ReplyError(packet->header->type, reply->header->type);
    return reply;
}

std::shared_ptr<Packet> Connection::dwm_msg(const MessageType type) const {
    return dwm_msg(type, "");
}

std::shared_ptr<std::vector<Monitor>> Connection::get_monitors() const {
    auto reply = dwm_msg(MessageType::GET_MONITORS);
    Json::Value root;
    pre_parse_reply(root, reply);
    auto monitors = std::make_shared<std::vector<Monitor>>();

    for (Json::Value v_mon : root) {
        Monitor mon;

        mon.master_factor = v_mon["master_factor"].asFloat();
        mon.num_master = v_mon["num_master"].asInt();
        mon.num = v_mon["num"].asUInt();

        auto v_monitor_geom = v_mon["monitor_geometry"];
        mon.monitor_geom.x = v_monitor_geom["x"].asInt();
        mon.monitor_geom.y = v_monitor_geom["y"].asInt();
        mon.monitor_geom.width = v_monitor_geom["width"].asInt();
        mon.monitor_geom.height = v_monitor_geom["height"].asInt();

        auto v_window_geom = v_mon["window_geometry"];
        mon.window_geom.x = v_window_geom["x"].asInt();
        mon.window_geom.y = v_window_geom["y"].asInt();
        mon.window_geom.width = v_window_geom["width"].asInt();
        mon.window_geom.height = v_window_geom["height"].asInt();

        auto v_layout = v_mon["layout"];
        auto v_symbol = v_layout["symbol"];
        mon.layout.symbol.cur = v_symbol["current"].asString();
        mon.layout.symbol.old = v_symbol["old"].asString();

        auto v_address = v_layout["address"];
        mon.layout.address.cur = v_address["current"].asUInt64();
        mon.layout.address.old = v_address["old"].asUInt64();

        auto v_bar = v_mon["bar"];
        mon.bar.y = v_bar["y"].asInt();
        mon.bar.is_shown = v_bar["is_shown"].asBool();
        mon.bar.is_top = v_bar["is_top"].asBool();
        mon.bar.window_id = v_bar["window_id"].asUInt();

        auto v_tagset = v_mon["tagset"];
        mon.tagset.cur = v_tagset["current"].asUInt();
        mon.tagset.old = v_tagset["old"].asUInt();

        auto v_clients = v_mon["clients"];
        mon.clients.selected = v_clients["selected"].asUInt();

        for (Json::Value v : v_clients["stack"])
            mon.clients.stack.push_back(v.asUInt());

        for (Json::Value v : v_clients["all"])
            mon.clients.all.push_back(v.asUInt());

        monitors->push_back(mon);
    }
    return monitors;
}

std::shared_ptr<std::vector<Tag>> Connection::get_tags() const {
    auto reply = dwm_msg(MessageType::GET_TAGS);
    Json::Value root;
    pre_parse_reply(root, reply);
    auto tags = std::make_shared<std::vector<Tag>>();

    for (Json::Value v_tag : root) {
        Tag tag;

        tag.bit_mask = v_tag["bit_mask"].asUInt();
        tag.tag_name = v_tag["name"].asString();
        tags->push_back(tag);
    }
    return tags;
}

std::shared_ptr<std::vector<Layout>> Connection::get_layouts() const {
    auto reply = dwm_msg(MessageType::GET_LAYOUTS);
    Json::Value root;
    pre_parse_reply(root, reply);
    auto layouts = std::make_shared<std::vector<Layout>>();

    for (Json::Value v_lt : root) {
        Layout lt;

        lt.symbol = v_lt["symbol"].asString();
        lt.address = v_lt["address"].asUInt64();

        layouts->push_back(lt);
    }
    return layouts;
}

std::shared_ptr<Client> Connection::get_client(Window win_id) const {
    // No need to generate the JSON using library since it is so simple
    // Format: { "client_window_id": <window id> }
    const std::string msg =
        "{\"client_window_id\":" + std::to_string(win_id) + "}";
    auto reply = dwm_msg(MessageType::GET_DWM_CLIENT, msg);
    Json::Value root;
    pre_parse_reply(root, reply);
    auto client = std::make_shared<Client>();

    client->name = root["name"].asString();
    client->tags = root["tags"].asUInt();
    client->window_id = root["window_id"].asUInt();
    client->monitor_num = root["monitor_num"].asUInt();

    auto v_geom = root["geometry"];
    auto v_geom_cur = v_geom["current"];
    client->geom.cur.x = v_geom_cur["x"].asInt();
    client->geom.cur.y = v_geom_cur["y"].asInt();
    client->geom.cur.width = v_geom_cur["width"].asInt();
    client->geom.cur.height = v_geom_cur["height"].asInt();

    auto v_geom_old = v_geom["old"];
    client->geom.old.x = v_geom_old["x"].asInt();
    client->geom.old.y = v_geom_old["y"].asInt();
    client->geom.old.width = v_geom_old["width"].asInt();
    client->geom.old.height = v_geom_old["height"].asInt();

    auto v_size_hints = root["size_hints"];
    auto v_base = v_size_hints["base"];
    client->size_hints.base.width = v_base["width"].asInt();
    client->size_hints.base.height = v_base["height"].asInt();

    auto v_step = v_size_hints["step"];
    client->size_hints.step.width = v_step["width"].asInt();
    client->size_hints.step.height = v_step["height"].asInt();

    auto v_max = v_size_hints["max"];
    client->size_hints.max.width = v_max["width"].asInt();
    client->size_hints.max.height = v_max["height"].asInt();

    auto v_min = v_size_hints["min"];
    client->size_hints.min.width = v_min["width"].asInt();
    client->size_hints.min.height = v_min["height"].asInt();

    auto v_aspect_ratio = v_size_hints["aspect_ratio"];
    client->size_hints.aspect_ratio.min = v_aspect_ratio["min"].asInt();
    client->size_hints.aspect_ratio.max = v_aspect_ratio["max"].asInt();

    auto v_border_width = root["border_width"];
    client->border_width.cur = v_border_width["current"].asInt();
    client->border_width.old = v_border_width["old"].asInt();

    auto v_states = root["states"];
    client->states.is_fixed = v_states["is_fixed"].asBool();
    client->states.is_floating = v_states["is_floating"].asBool();
    client->states.is_urgent = v_states["is_urgent"].asBool();
    client->states.is_fullscreen = v_states["is_fullscreen"].asBool();
    client->states.never_focus = v_states["never_focus"].asBool();
    client->states.old_state = v_states["old_state"].asBool();

    return client;
}

void Connection::subscribe(const Event ev, const bool sub) {
    // Get string representation of event
    const std::string ev_name = event_map.at(ev);

    Json::StreamWriterBuilder builder;
    // No need to waste bytes on pretty JSON
    builder["indentation"] = "";

    Json::Value root;
    root["event"] = ev_name;
    root["action"] = (sub ? "subscribe" : "unsubscribe");

    const std::string msg = Json::writeString(builder, root);
    auto reply = dwm_msg(MessageType::SUBSCRIBE, msg);

    // Throws error on failure result, we don't care about success result
    Json::Value dummy;
    pre_parse_reply(dummy, reply);
}

void Connection::subscribe(const Event ev) {
    subscribe(ev, true);
    this->subscriptions |= static_cast<uint8_t>(ev);
}

void Connection::unsubscribe(const Event ev) {
    subscribe(ev, false);
    if (this->subscriptions & static_cast<uint8_t>(ev))
        this->subscriptions -= static_cast<uint8_t>(ev);
}

void Connection::handle_event() const {
    auto reply = recv_message();
    if (reply->header->type != static_cast<uint8_t>(MessageType::EVENT))
        throw IPCError("Invalid message type received");

    Json::Value root;
    pre_parse_reply(root, reply);

    // First key of JSON will be event name
    if (root.get(event_map.at(Event::TAG_CHANGE), Json::nullValue) !=
        Json::nullValue) {
        if (on_tag_change) {
            TagChangeEvent event;
            parse_tag_change_event(root, event);
            on_tag_change(event);
        }
    } else if (root.get(event_map.at(Event::LAYOUT_CHANGE), Json::nullValue) !=
               Json::nullValue) {
        if (on_layout_change) {
            LayoutChangeEvent event;
            parse_layout_change_event(root, event);
            on_layout_change(event);
        }
    } else if (root.get(event_map.at(Event::SELECTED_CLIENT_CHANGE),
                        Json::nullValue) != Json::nullValue) {
        if (on_selected_client_change) {
            SelectedClientChangeEvent event;
            parse_selected_client_change_event(root, event);
            on_selected_client_change(event);
        }
    } else
        throw IPCError("Invalid event type received");
}

uint8_t Connection::get_subscriptions() const { return this->subscriptions; }

void Connection::run_command(const std::string name,
                             const Json::Value &arr) const {
    Json::Value root;
    root["command"] = name;
    root["args"] = Json::Value(arr);

    Json::StreamWriterBuilder builder;
    // No need to waste bytes on pretty JSON
    builder["indentation"] = "";
    const std::string msg = Json::writeString(builder, root);

    auto reply = dwm_msg(MessageType::RUN_COMMAND, msg);

    // Dummy value
    Json::Value dummy;
    // Throws exception on failure result
    pre_parse_reply(dummy, reply);
}

} // namespace dwmipc
