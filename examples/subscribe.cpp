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

    con.on_selected_client_change =
        [](const dwmipc::SelectedClientChangeEvent &event) {
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

    con.on_selected_monitor_change =
        [](const dwmipc::SelectedMonitorChangeEvent &event) {
            std::cout << "selected_monitor_change_event:" << std::endl;
            std::cout << "  old_monitor_number: " << event.old_mon_num
                      << std::endl;
            std::cout << "  new_monitor_number: " << event.new_mon_num
                      << std::endl;
        };

    con.subscribe(dwmipc::Event::LAYOUT_CHANGE);
    con.subscribe(dwmipc::Event::SELECTED_CLIENT_CHANGE);
    con.subscribe(dwmipc::Event::TAG_CHANGE);
    con.subscribe(dwmipc::Event::SELECTED_MONITOR_CHANGE);

    while (true) {
        try {
            con.handle_event();
        } catch (const dwmipc::IPCError &err) {
            std::cout << "Error handling event" << err.what() << std::endl;
        }
        msleep(100);
    }
}
