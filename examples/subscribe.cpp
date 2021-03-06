#include <iostream>
#include <unistd.h>

#include "dwmipcpp/connection.hpp"
#include "dwmipcpp/errors.hpp"

bool msleep(const long milliseconds) {
    struct timespec ts;
    int res;

    // Invalid argument
    if (milliseconds < 0)
        return false;

    time_t sec = milliseconds / 1000;
    long ns = (milliseconds % 1000) * 1000 * 1000;

    ts.tv_sec = sec;
    ts.tv_nsec = ns;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return true;
}

int main() {
    dwmipc::Connection con("/tmp/dwm.sock");

    con.on_layout_change = [](const dwmipc::LayoutChangeEvent &event) {
        std::cout << "layout_change_event:" << std::endl;
        std::cout << "  monitor_number: " << event.monitor_num << std::endl;
        std::cout << "  old_symbol: " << event.old_symbol << std::endl;
        std::cout << "  new_symbol: " << event.new_symbol << std::endl;
    };

    con.on_client_focus_change =
        [](const dwmipc::ClientFocusChangeEvent &event) {
            std::cout << "selected_client_change_event:" << std::endl;
            std::cout << "  monitor_num: " << event.monitor_num << std::endl;
            std::cout << "  old: " << event.old_win_id << std::endl;
            std::cout << "  new: " << event.new_win_id << std::endl;
        };

    con.on_tag_change = [](const dwmipc::TagChangeEvent &event) {
        std::cout << "tag_change_event:" << std::endl;
        std::cout << "  old: " << std::endl;
        std::cout << "    selected: " << event.old_state.selected << std::endl;
        std::cout << "    occupied: " << event.old_state.occupied << std::endl;
        std::cout << "    urgent: " << event.old_state.urgent << std::endl;
        std::cout << "  new: " << std::endl;
        std::cout << "    selected: " << event.new_state.selected << std::endl;
        std::cout << "    occupied: " << event.new_state.occupied << std::endl;
        std::cout << "    urgent: " << event.new_state.urgent << std::endl;
    };

    con.on_monitor_focus_change =
        [](const dwmipc::MonitorFocusChangeEvent &event) {
            std::cout << "selected_monitor_change_event:" << std::endl;
            std::cout << "  old_monitor_number: " << event.old_mon_num
                      << std::endl;
            std::cout << "  new_monitor_number: " << event.new_mon_num
                      << std::endl;
        };

    con.on_focused_title_change =
        [](const dwmipc::FocusedTitleChangeEvent &event) {
            std::cout << "focused_title_change_event:" << std::endl;
            std::cout << "  monitor_number: " << event.monitor_num << std::endl;
            std::cout << "  client_window_id: " << event.client_window_id
                      << std::endl;
            std::cout << "  old_name: " << event.old_name << std::endl;
            std::cout << "  new_name: " << event.new_name << std::endl;
        };

    con.on_focused_state_change =
        [](const dwmipc::FocusedStateChangeEvent &event) {
            std::cout << "focused_state_change_event:" << std::endl;
            std::cout << "  monitor_number: " << event.monitor_num << std::endl;
            std::cout << "  client_window_id: " << event.client_window_id
                      << std::endl;
            std::cout << "  old_state:" << std::endl;
            std::cout << "    old_state: " << event.old_state.old_state
                      << std::endl;
            std::cout << "    is_fixed: " << event.old_state.is_fixed
                      << std::endl;
            std::cout << "    is_floating: " << event.old_state.is_floating
                      << std::endl;
            std::cout << "    is_fullscreen: " << event.old_state.is_fullscreen
                      << std::endl;
            std::cout << "    is_urgent: " << event.old_state.is_urgent
                      << std::endl;
            std::cout << "    never_focus: " << event.old_state.never_focus
                      << std::endl;
            std::cout << "  new_state:" << std::endl;
            std::cout << "    old_state: " << event.new_state.old_state
                      << std::endl;
            std::cout << "    is_fixed: " << event.new_state.is_fixed
                      << std::endl;
            std::cout << "    is_floating: " << event.new_state.is_floating
                      << std::endl;
            std::cout << "    is_fullscreen: " << event.new_state.is_fullscreen
                      << std::endl;
            std::cout << "    is_urgent: " << event.new_state.is_urgent
                      << std::endl;
            std::cout << "    never_focus: " << event.new_state.never_focus
                      << std::endl;
        };

    con.subscribe(dwmipc::Event::LAYOUT_CHANGE);
    con.subscribe(dwmipc::Event::CLIENT_FOCUS_CHANGE);
    con.subscribe(dwmipc::Event::TAG_CHANGE);
    con.subscribe(dwmipc::Event::MONITOR_FOCUS_CHANGE);
    con.subscribe(dwmipc::Event::FOCUSED_TITLE_CHANGE);
    con.subscribe(dwmipc::Event::FOCUSED_STATE_CHANGE);

    while (true) {
        try {
            con.handle_event();
        } catch (const dwmipc::SocketClosedError &err) {
            std::cerr << err.what() << std::endl;
            std::cout << "Attempting to reconnect" << std::endl;
            msleep(500);
            con.connect_event_socket();
        } catch (const dwmipc::IPCError &err) {
            std::cerr << "Error handling event" << err.what() << std::endl;
        }
        msleep(100);
    }
}
