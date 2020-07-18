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
    {Event::CLIENT_FOCUS_CHANGE, "client_focus_change_event"},
    {Event::LAYOUT_CHANGE, "layout_change_event"},
    {Event::MONITOR_FOCUS_CHANGE, "monitor_focus_change_event"},
    {Event::FOCUSED_TITLE_CHANGE, "focused_title_change_event"}};
} // namespace dwmipc
