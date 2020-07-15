/**
 * @file types.cpp
 *
 * This file implements the global event_map which maps the Event enum values
 * to their event names.
 */

#include "dwmipcpp/types.hpp"

namespace dwmipc {
const std::unordered_map<Event, std::string> event_map = {
    {Event::TAG_CHANGE, "tag_change_event"},
    {Event::SELECTED_CLIENT_CHANGE, "selected_client_change_event"},
    {Event::LAYOUT_CHANGE, "layout_change_event"},
    {Event::SELECTED_MONITOR_CHANGE, "selected_monitor_change_event"}};
} // namespace dwmipc
