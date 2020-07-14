#include <iostream>

#include "dwmipcpp/connection.hpp"

void dump_monitor(const std::shared_ptr<dwmipc::Monitor> &m) {
    std::cout << "master_factor: " << m->master_factor << std::endl;
    std::cout << "num_master: " << m->num_master << std::endl;
    std::cout << "num: " << m->num << std::endl;

    std::cout << "monitor_geometry:" << std::endl;
    std::cout << "  x: " << m->monitor_geom.x << std::endl;
    std::cout << "  y: " << m->monitor_geom.y << std::endl;
    std::cout << "  width: " << m->monitor_geom.width << std::endl;
    std::cout << "  height: " << m->monitor_geom.height << std::endl;

    std::cout << "window_geometry:" << std::endl;
    std::cout << "  x: " << m->window_geom.x << std::endl;
    std::cout << "  y: " << m->window_geom.y << std::endl;
    std::cout << "  width: " << m->window_geom.width << std::endl;
    std::cout << "  height: " << m->window_geom.height << std::endl;

    std::cout << "tagset:" << std::endl;
    std::cout << "  current: " << m->tagset.cur << std::endl;
    std::cout << "  old: " << m->tagset.old << std::endl;

    std::cout << "clients:" << std::endl;
    std::cout << "  selected: " << m->clients.selected << std::endl;
    std::cout << "  stack:" << std::endl;
    for (auto c : m->clients.stack)
        std::cout << "  - " << c << std::endl;
    std::cout << "  all:" << std::endl;
    for (auto c : m->clients.all)
        std::cout << "  - " << c << std::endl;

    std::cout << "layout:" << std::endl;
    std::cout << "  symbol:" << std::endl;
    std::cout << "    current: " << m->layout.symbol.cur << std::endl;
    std::cout << "    old: " << m->layout.symbol.old << std::endl;
    std::cout << "  address:" << std::endl;
    std::cout << "    current: " << m->layout.address.cur << std::endl;
    std::cout << "    old: " << m->layout.address.old << std::endl;

    std::cout << "bar:" << std::endl;
    std::cout << "  y: " << m->bar.y << std::endl;
    std::cout << "  is_shown: " << m->bar.is_shown << std::endl;
    std::cout << "  is_top: " << m->bar.is_top << std::endl;
    std::cout << "  window_id: " << m->bar.window_id << std::endl;
}

void dump_tag(const std::shared_ptr<dwmipc::Tag> &tag) {
    std::cout << "bit_mask: " << tag->bit_mask << std::endl;
    std::cout << "name: " << tag->tag_name << std::endl;
}

void dump_layout(const std::shared_ptr<dwmipc::Layout> &layout) {
    std::cout << "symbol: " << layout->symbol << std::endl;
    std::cout << "address: " << layout->address << std::endl;
}

void dump_client(const std::shared_ptr<dwmipc::Client> &c) {
    std::cout << "name: " << c->name << std::endl;
    std::cout << "tags: " << c->tags << std::endl;
    std::cout << "window_id: " << c->window_id << std::endl;
    std::cout << "monitor_number: " << c->monitor_num << std::endl;

    std::cout << "geometry:" << std::endl;
    std::cout << "  current:" << std::endl;
    std::cout << "    x: " << c->geom.cur.x << std::endl;
    std::cout << "    y: " << c->geom.cur.y << std::endl;
    std::cout << "    width: " << c->geom.cur.width << std::endl;
    std::cout << "    height: " << c->geom.cur.height << std::endl;
    std::cout << "  old " << std::endl;
    std::cout << "    x: " << c->geom.old.x << std::endl;
    std::cout << "    y: " << c->geom.old.y << std::endl;
    std::cout << "    width: " << c->geom.old.width << std::endl;
    std::cout << "    height: " << c->geom.old.height << std::endl;

    std::cout << "size_hints:" << std::endl;
    std::cout << "  base:" << std::endl;
    std::cout << "    width: " << c->size_hints.base.width << std::endl;
    std::cout << "    height: " << c->size_hints.base.height << std::endl;
    std::cout << "  step:" << std::endl;
    std::cout << "    width: " << c->size_hints.step.width << std::endl;
    std::cout << "    height: " << c->size_hints.step.height << std::endl;
    std::cout << "  max:" << std::endl;
    std::cout << "    width: " << c->size_hints.max.width << std::endl;
    std::cout << "    height: " << c->size_hints.max.height << std::endl;
    std::cout << "  min:" << std::endl;
    std::cout << "    width: " << c->size_hints.min.width << std::endl;
    std::cout << "    height: " << c->size_hints.min.height << std::endl;
    std::cout << "  aspect_ratio:" << std::endl;
    std::cout << "    min: " << c->size_hints.aspect_ratio.min << std::endl;
    std::cout << "    max: " << c->size_hints.aspect_ratio.max << std::endl;

    std::cout << "border_width:" << std::endl;
    std::cout << "  current: " << c->border_width.cur << std::endl;
    std::cout << "  old: " << c->border_width.old << std::endl;

    std::cout << "states:" << std::endl;
    std::cout << "  is_fixed: " << c->states.is_fixed << std::endl;
    std::cout << "  is_floating: " << c->states.is_floating << std::endl;
    std::cout << "  is_urgent: " << c->states.is_urgent << std::endl;
    std::cout << "  is_fullscreen: " << c->states.is_fullscreen << std::endl;
    std::cout << "  never_focus: " << c->states.never_focus << std::endl;
    std::cout << "  old_state: " << c->states.old_state << std::endl;
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

    auto c = connection.get_client((*monitors)[0].clients.selected);
    dump_client(c);
}

