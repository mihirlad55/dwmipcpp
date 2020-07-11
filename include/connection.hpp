#pragma once

#include <functional>
#include <json/json.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "packet.hpp"

namespace dwmipc {
typedef unsigned long Window;
struct Monitor;
struct Client;

struct Client {
    std::string name;
    float mina, maxa;
    int x, y, w, h;
    int oldx, oldy, oldw, oldh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int bw, oldbw;
    unsigned int tags;
    bool isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
    int monitor_num;
    Window win;
};

struct Layout {
    std::string symbol;
    uintptr_t address;
};

struct TagState {
    unsigned int selected;
    unsigned int occupied;
    unsigned urgent;
};

struct Tag {
    unsigned int bit_mask;
    std::string tag_name;
};

struct Monitor {
    std::string layout_symbol;
    std::string old_layout;
    std::string current_layout;
    float mfact;
    int nmaster;
    int num;
    int bar_y;          // bar geometry
    int mx, my, mw, mh; // screen size
    int wx, wy, ww, wh; // window area
    unsigned int tagset;
    unsigned int old_tagset;
    bool show_bar;
    bool top_bar;
    std::vector<Window> win_clients;
    std::vector<Window> win_stack;
    Window win_sel;
    Window win_bar;
};

struct TagChangeEvent {
    TagState old_state;
    TagState new_state;
    unsigned int monitor_num;
};

struct SelectedClientChangeEvent {
    Window old_client_win;
    Window new_client_win;
    unsigned int monitor_num;
};

struct LayoutChangeEvent {
    std::string old_symbol;
    std::string new_symbol;
    unsigned int monitor_num;
};

enum Event {
    EVENT_TAG_CHANGE = 1,
    EVENT_SELECTED_CLIENT_CHANGE = 2,
    EVENT_LAYOUT_CHANGE = 4
};

extern const std::unordered_map<Event, std::string> event_map;

class Connection {
  public:
    Connection(const std::string &socket_path);
    ~Connection();

    std::shared_ptr<std::vector<Monitor>> get_monitors();
    std::shared_ptr<std::vector<Tag>> get_tags();
    std::shared_ptr<std::vector<Layout>> get_layouts();
    std::shared_ptr<Client> get_client(Window win_id);
    void subscribe(const Event ev);
    void unsubscribe(const Event ev);

    std::function<void(const TagChangeEvent &ev)> on_tag_change;
    std::function<void(const SelectedClientChangeEvent &ev)>
        on_selected_client_change;
    std::function<void(const LayoutChangeEvent &ev)> on_layout_change;

    void handle_event();

    template <typename... Types>
    void run_command(const std::string name, Types... args) {
        Json::Value root;
        root["command"] = name;
        root["args"] = Json::Value(Json::arrayValue);

        run_command_build(root["args"], args...);

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        const std::string msg = Json::writeString(builder, root);

        auto reply = dwm_msg(MESSAGE_TYPE_RUN_COMMAND, msg);

        // Dummy value
        Json::Value dummy;
        // Throws exception on failure result
        pre_parse_reply(dummy, reply);
    }

  private:
    const int sockfd;
    const std::string socket_path;
    unsigned int subscriptions = 0;

    static int connect(const std::string &socket_path);

    static void pre_parse_reply(Json::Value &root,
                                const std::shared_ptr<Packet> &reply);

    void disconnect();
    void send_message(const std::shared_ptr<Packet> &packet);

    std::shared_ptr<Packet> recv_message();
    std::shared_ptr<Packet> dwm_msg(const MessageType type,
                                    const std::string &msg);
    std::shared_ptr<Packet> dwm_msg(const MessageType type);

    void subscribe(const Event ev, const bool sub);

    // Base case
    template <typename T>
    static void run_command_build(Json::Value &arr, T arg) {
        arr.append(arg);
    }

    // Recursive
    template <typename... Ts>
    static void run_command_build(Json::Value &arr, Ts... args) {
        run_command_build(arr, args...);
    }
};
} // namespace dwmipc
