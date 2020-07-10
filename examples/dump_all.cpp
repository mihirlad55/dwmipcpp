#include <iostream>

#include "../include/connection.hpp"

void dump_monitor(const std::shared_ptr<dwmipc::Monitor> &m) {
    std::cout << "layout_symbol: " << m->layout_symbol << std::endl;
    std::cout << "mfact: " << m->mfact << std::endl;
    std::cout << "nmaster: " << m->nmaster << std::endl;
    std::cout << "num: " << m->num << std::endl;
    std::cout << "bar_y: " << m->bar_y << std::endl;
    std::cout << "show_bar: " << m->show_bar << std::endl;
    std::cout << "top_bar: " << m->top_bar << std::endl;
    std::cout << "bar_window_id: " << m->win_bar << std::endl;
    std::cout << "screen:" << std::endl;
    std::cout << "  x: " << m->mx << std::endl;
    std::cout << "  y: " << m->my << std::endl;
    std::cout << "  width: " << m->mw << std::endl;
    std::cout << "  height: " << m->mh << std::endl;
    std::cout << "window:" << std::endl;
    std::cout << "  x: " << m->wx << std::endl;
    std::cout << "  y: " << m->wy << std::endl;
    std::cout << "  width: " << m->ww << std::endl;
    std::cout << "  height: " << m->wh << std::endl;
    std::cout << "tag_set:" << std::endl;
    std::cout << "  old: " << m->old_tagset << std::endl;
    std::cout << "  current: " << m->tagset << std::endl;
    std::cout << "layout:" << std::endl;
    std::cout << "  old: " << m->old_layout << std::endl;
    std::cout << "  current: " << m->current_layout << std::endl;
    std::cout << "selected_client: " << m->win_sel << std::endl;

    std::cout << "stack:" << std::endl;
    for (auto c : m->win_stack)
        std::cout << "  " << c << std::endl;

    std::cout << "clients:" << std::endl;
    for (auto c : m->win_clients)
        std::cout << "  " << c << std::endl;
}

void dump_tag(const std::shared_ptr<dwmipc::Tag> &tag) {
    std::cout << "bit_mask: " << tag->bit_mask << std::endl;
    std::cout << "name: " << tag->tag_name << std::endl;
}

void dump_layout(const std::shared_ptr<dwmipc::Layout> &layout) {
    std::cout << "layout_symbol: " << layout->symbol << std::endl;
    std::cout << "layout_address: " << layout->address << std::endl;
}

void dump_client(const std::shared_ptr<dwmipc::Client> &c) {
    std::cout << "name: " << c->name << std::endl;
    std::cout << "maxa: " << c->maxa << std::endl;
    std::cout << "tags: " << c->tags << std::endl;
    std::cout << "window_id: " << c->win << std::endl;
    std::cout << "monitor_number: " << c->monitor_num << std::endl;
    std::cout << "size:" << std::endl;
    std::cout << "  current:" << std::endl;
    std::cout << "    x: " << c->x << std::endl;
    std::cout << "    y: " << c->y << std::endl;
    std::cout << "    width: " << c->w << std::endl;
    std::cout << "    height: " << c->h << std::endl;
    std::cout << "  old " << std::endl;
    std::cout << "    x: " << c->oldx << std::endl;
    std::cout << "    y: " << c->oldy << std::endl;
    std::cout << "    width: " << c->oldw << std::endl;
    std::cout << "    height: " << c->oldh << std::endl;
    std::cout << "size_hints:" << std::endl;
    std::cout << "  base_width: " << c->basew << std::endl;
    std::cout << "  base_height: " << c->baseh << std::endl;
    std::cout << "  increase_width: " << c->incw << std::endl;
    std::cout << "  increase_height: " << c->inch << std::endl;
    std::cout << "  max_width: " << c->maxw << std::endl;
    std::cout << "  max_height: " << c->maxh << std::endl;
    std::cout << "  min_width: " << c->minw << std::endl;
    std::cout << "  min_height: " << c->minh << std::endl;
    std::cout << "border:" << std::endl;
    std::cout << "  current_width: " << c->bw << std::endl;
    std::cout << "  old_width: " << c->oldbw << std::endl;
    std::cout << "states:" << std::endl;
    std::cout << "  is_fixed: " << c->isfixed << std::endl;
    std::cout << "  is_floating: " << c->isfloating << std::endl;
    std::cout << "  is_urgent: " << c->isurgent << std::endl;
    std::cout << "  is_fullscreen: " << c->isfullscreen << std::endl;
    std::cout << "  never_focus: " << c->neverfocus << std::endl;
    std::cout << "  old_state: " << c->oldstate << std::endl;
}

int main() {
    dwmipc::Connection connection("/tmp/dwm.sock");

    auto monitors = connection.get_monitors();

    for (auto m : *monitors)
        dump_monitor(std::make_shared<dwmipc::Monitor>(m));

    auto tags = connection.get_tags();
    for (auto t: *tags)
        dump_tag(std::make_shared<dwmipc::Tag>(t));

    auto layouts = connection.get_layouts();
    for (auto l: *layouts)
        dump_layout(std::make_shared<dwmipc::Layout>(l));

    auto c = connection.get_client((*monitors)[0].win_sel);
    dump_client(c);
}

