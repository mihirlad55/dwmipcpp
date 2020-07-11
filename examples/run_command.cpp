#include <unistd.h>
#include <iostream>

#include "connection.hpp"

int main() {
    dwmipc::Connection con("/tmp/dwm.sock");

    con.run_command("togglefloating");
    sleep(1);
    con.run_command("view", 0);
    sleep(1);
    con.run_command("view", 8);
    sleep(1);
    con.run_command("toggletag", 16);
    sleep(1);

    // Set layout to monocle
    auto layouts = con.get_layouts();
    for (dwmipc::Layout lt : *layouts) {
        if (lt.symbol == "[M]")
            con.run_command("setlayout", lt.address);
    }
}
