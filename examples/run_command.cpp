#include <unistd.h>
#include <iostream>

#include "connection.hpp"

int main() {
    dwmipc::Connection con("/tmp/dwm.sock");

    con.run_command("focusstack", 1);
    sleep(1);
    con.run_command("view", 1);
    sleep(1);
    con.run_command("view", 8);
}
