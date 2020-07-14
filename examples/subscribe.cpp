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

    con.subscribe(dwmipc::Event::LAYOUT_CHANGE);
    con.subscribe(dwmipc::Event::SELECTED_CLIENT_CHANGE);
    con.subscribe(dwmipc::Event::TAG_CHANGE);

    while (true) {
        try {
            con.handle_event();
        } catch (const dwmipc::NoMsgError&) {
        }
        msleep(100);
    }
}
