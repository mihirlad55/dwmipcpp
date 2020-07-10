#pragma once

#include <memory>
#include <string>
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

enum Event {
    EVENT_TAG_CHANGE = 1,
    EVENT_SELECTED_CLIENT_CHANGE = 2,
    EVENT_LAYOUT_CHANGE = 4
};

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
    std::shared_ptr<Packet> dwm_msg(const MessageType type,
                                    const std::string &msg);
    std::shared_ptr<Packet> dwm_msg(const MessageType type);

};
} // namespace dwmipc
