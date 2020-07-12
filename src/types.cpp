/**
 * @file types.cpp
 *
 * This file implements the global event_map which maps the Event enum values
 * to their event names.
 */

#include "types.hpp"

namespace dwmipc {
const std::unordered_map<Event, std::string> event_map = {
    {EVENT_TAG_CHANGE, "tag_change_event"},
    {EVENT_SELECTED_CLIENT_CHANGE, "selected_client_change_event"},
    {EVENT_LAYOUT_CHANGE, "layout_change_event"}};
} // namespace dwmipc
