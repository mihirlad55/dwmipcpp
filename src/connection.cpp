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

#include "dwmipcpp/connection.hpp"
#include "dwmipcpp/errors.hpp"
#include "dwmipcpp/util.hpp"

namespace dwmipc {
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
 * Parse a Event::TAG_CHANGE message
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
 * Parse a Event::LAYOUT_CHANGE message
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
 * Parse a Event::SELECTED_CLIENT_CHANGE message
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

/**
 * Parse a Event::SELECTED_MONITOR_CHANGE message
 */
static void parse_selected_monitor_change(const Json::Value &root,
                                          SelectedMonitorChangeEvent &event) {
    const std::string ev_name = event_map.at(Event::SELECTED_MONITOR_CHANGE);
    auto v_event = root[ev_name];

    event.old_mon_num = v_event["old_monitor_number"].asUInt();
    event.new_mon_num = v_event["new_monitor_number"].asUInt();
}

Connection::Connection(const std::string &socket_path, bool connect)
    : socket_path(socket_path) {
    if (connect) {
        connect_main_socket();
        connect_event_socket();
    }
}

Connection::~Connection() {
    if (is_main_socket_connected())
        disconnect_main_socket();

    if (is_event_socket_connected())
        disconnect_event_socket();
}

bool Connection::is_main_socket_connected() const {
    return this->main_sockfd != -1;
}

bool Connection::is_event_socket_connected() const {
    return this->event_sockfd != -1;
}

void Connection::connect_main_socket() {
    if (is_main_socket_connected())
        throw InvalidOperationError(
            "Cannot connect to main socket. Already connected.");
    this->main_sockfd = dwmipc::connect(socket_path, true);
}

void Connection::connect_event_socket() {
    if (is_event_socket_connected())
        throw InvalidOperationError(
            "Cannot connect to event socket. Already connected.");
    this->event_sockfd = dwmipc::connect(socket_path, false);
}

void Connection::disconnect_main_socket() {
    if (!is_main_socket_connected())
        throw InvalidOperationError(
            "Cannot disconnect from main socket. Already disconnected.");
    dwmipc::disconnect(this->main_sockfd);
    this->main_sockfd = -1;
}

void Connection::disconnect_event_socket() {
    if (!is_event_socket_connected())
        throw InvalidOperationError(
            "Cannot disconnect from event socket. Already disconnected.");
    dwmipc::disconnect(this->event_sockfd);
    this->event_sockfd = -1;
}

std::shared_ptr<Packet> Connection::dwm_msg(const MessageType type,
                                            const std::string &msg) {
    auto packet = std::make_shared<Packet>(type, msg);

    // If receiving an event message, use the event_sockfd
    int sockfd = (type == MessageType::SUBSCRIBE ? event_sockfd : main_sockfd);

    // Throw error if disconnected socket
    if (type == MessageType::SUBSCRIBE && !is_event_socket_connected()) {
        throw SocketClosedError("Cannot write to disconnected event socket");
    } else if (type != MessageType::SUBSCRIBE && !is_main_socket_connected()) {
        throw SocketClosedError("Cannot write to disconnected main socket");
    }

    try {
        send_message(sockfd, packet);
    } catch (const SocketClosedError &err) {
        if (type == MessageType::SUBSCRIBE)
            disconnect_event_socket();
        else
            disconnect_main_socket();
        throw;
    }

    std::shared_ptr<Packet> reply;
    try {
        reply = recv_message(sockfd, true);
    } catch (const SocketClosedError &err) {
        if (type == MessageType::SUBSCRIBE)
            disconnect_event_socket();
        else
            disconnect_main_socket();
        throw;
    }

    // Check if message type matches
    if (packet->header->type != static_cast<uint8_t>(type))
        throw ReplyError(packet->header->type, static_cast<uint8_t>(type));

    return reply;
}

std::shared_ptr<std::vector<Monitor>> Connection::get_monitors() {
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

        auto v_tag_state = v_mon["tag_state"];
        mon.tag_state.selected = v_tag_state["selected"].asUInt();
        mon.tag_state.occupied = v_tag_state["occupied"].asUInt();
        mon.tag_state.urgent = v_tag_state["urgent"].asUInt();

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

std::shared_ptr<std::vector<Tag>> Connection::get_tags() {
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

std::shared_ptr<std::vector<Layout>> Connection::get_layouts() {
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

std::shared_ptr<Client> Connection::get_client(Window win_id) {
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

bool Connection::handle_event() {
    std::shared_ptr<Packet> reply;
    if (!is_event_socket_connected())
        throw SocketClosedError(
            "Cannot handle event on disconnected event socket");
    try {
        reply = recv_message(event_sockfd, false);
    } catch (NoMsgError &) {
        return false;
    } catch (const SocketClosedError &err) {
        disconnect_event_socket();
        throw;
    }

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
    } else if (root.get(event_map.at(Event::SELECTED_MONITOR_CHANGE),
                        Json::nullValue) != Json::nullValue) {
        if (on_selected_monitor_change) {
            SelectedMonitorChangeEvent event;
            parse_selected_monitor_change(root, event);
            on_selected_monitor_change(event);
        }
    } else
        throw IPCError("Invalid event type received" +
                       std::string(reply->payload, reply->header->size));

    return true;
}

uint8_t Connection::get_subscriptions() const { return this->subscriptions; }

void Connection::run_command(const std::string name, const Json::Value &arr) {
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
