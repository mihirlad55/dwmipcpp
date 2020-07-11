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
}
