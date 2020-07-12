#pragma once

#include <functional>
#include <json/json.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "packet.hpp"

namespace dwmipc {
struct Monitor;
struct Client;

typedef unsigned long Window;

struct Geometry {
    int x;
    int y;
    int width;
    int height;
};

struct Size {
    int width;
    int height;
};

struct Monitor {
    Geometry monitor_geom;
    Geometry window_geom;
    float master_factor;
    int num_master;
    int num;
    struct {
        unsigned int cur;
        unsigned int old;
    } tagset;
    struct {
        std::vector<Window> all;
        std::vector<Window> stack;
        Window selected;
    } clients;
    struct {
        struct {
            std::string cur;
            std::string old;
        } symbol;

        struct {
            uintptr_t cur;
            uintptr_t old;
        } address;
    } layout;
    struct {
        int y;
        bool is_shown;
        bool is_top;
        Window window_id;
    } bar;
};

struct Client {
    std::string name;
    Window window_id;
    int monitor_num;
    unsigned int tags;
    struct {
        int cur_width;
        int old_width;
    } border;
    struct {
        Geometry cur;
        Geometry old;
    } geom;
    struct {
        Size base;
        Size increment;
        Size max;
        Size min;
        struct {
            float min;
            float max;
        } aspect_ratio;
    } size_hints;
    struct {
        bool is_fixed;
        bool is_floating;
        bool is_urgent;
        bool never_focus;
        bool old_state;
        bool is_fullscreen;
    } states;
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
    uintptr_t old_address;
    uintptr_t new_address;
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

    std::shared_ptr<std::vector<Monitor>> get_monitors() const;
    std::shared_ptr<std::vector<Tag>> get_tags() const;
    std::shared_ptr<std::vector<Layout>> get_layouts() const;
    std::shared_ptr<Client> get_client(Window win_id) const;
    void subscribe(const Event ev);

    void unsubscribe(const Event ev);

    void handle_event() const;


    template <typename... Types>
    void run_command(const std::string name, Types... args) const {
        Json::Value arr = Json::Value(Json::arrayValue);
        run_command_build(arr, args...);
        run_command(name, arr);
    }

    // No command argument overload with default empty array
    void
    run_command(const std::string name,
                const Json::Value &arr = Json::Value(Json::arrayValue)) const;

    uint8_t get_subscriptions() const;

    const int sockfd;
    const std::string socket_path;

    std::function<void(const TagChangeEvent &ev)> on_tag_change;

    std::function<void(const SelectedClientChangeEvent &ev)>
        on_selected_client_change;

    std::function<void(const LayoutChangeEvent &ev)> on_layout_change;

  private:
    uint8_t subscriptions = 0;

    static int connect(const std::string &socket_path);

    void disconnect();

    void send_message(const std::shared_ptr<Packet> &packet) const;

    std::shared_ptr<Packet> recv_message() const;

    std::shared_ptr<Packet> dwm_msg(const MessageType type,
                                    const std::string &msg) const;

    std::shared_ptr<Packet> dwm_msg(const MessageType type) const;


    void subscribe(const Event ev, const bool sub);

    // Base case
    template <typename T>
    static void run_command_build(Json::Value &arr, T arg) {
        static_assert(std::is_arithmetic<T>::value ||             // number
                          std::is_same<T, std::string>::value ||  // string
                          std::is_same<T, const char *>::value || // string
                          std::is_same<T, bool>::value,           // bool
                      "The arguments to run_command must be a string, number, "
                      "bool, or NULL");
        arr.append(arg);
    }

    // Recursive
    template <typename T, typename... Ts>
    static void run_command_build(Json::Value &arr, T arg1, Ts... args) {
        run_command_build(arr, arg1);
        run_command_build(arr, args...);
    }
};
} // namespace dwmipc
