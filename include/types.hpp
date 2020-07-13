/**
 * @file types.hpp
 *
 * This file contains important type definitions for objects that are commonly
 * used to interact with the Connection class.
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace dwmipc {
/**
 * The magic string that correctly formed DWM packets should start with
 */
static constexpr char DWM_MAGIC[] = "DWM-IPC";

/**
 * The length of the magic string excluding the null character
 */
static constexpr int DWM_MAGIC_LEN = 7;

typedef unsigned long Window; ///< Definition of X11 Window from xlib

/**
 * Struct that defines a generic rectangle geometry
 */
struct Geometry {
    int x;      ///< The x coordinate
    int y;      ///< The y coordinate
    int width;  ///< The width
    int height; ///< The height
};

/**
 * Struct that defines a generic size
 */
struct Size {
    int width;  ///< The width
    int height; ///< The height
};

/**
 * A DWM monitor
 */
struct Monitor {
    Geometry monitor_geom; ///< Monitor geometry
    Geometry window_geom;  ///< Window area geometry
    float master_factor;   ///< Percentage of space master clients should occupy
    int num_master;        ///< Number of clients that should be masters
    int num;               ///< Index of monitor (according to DWM)
    struct {
        unsigned int cur; ///< Current tags in view, each bit is a tag
        unsigned int old; ///< Last tags in view, each bit is a tag
    } tagset;             ///< Tags in view
    struct {
        std::vector<Window> all;   ///< Vector of all client window XIDs
        std::vector<Window> stack; ///< Vector of client window XIDs in stack
        Window selected;           ///< Window XID of currently selected client
    } clients; ///< Properties about the clients on this monitor
    struct {
        struct {
            std::string cur; ///< Current layout symbol
            std::string old; ///< Layout symbol before layout change.
        } symbol; ///< Layout symbol. This may be different from the symbol
                  ///< given by get_layouts
        struct {
            uintptr_t cur; ///< Current layout address
            uintptr_t old; ///< Layout address before layout change
        } address;         ///< Layout address
    } layout;              ///< Layout properties
    struct {
        int y;            ///< Y coordinate of DWM status bar
        bool is_shown;    ///< Is the DWM status bar shown
        bool is_top;      ///< Is the DWM status bar on top
        Window window_id; ///< Window XID of DWM status bar
    } bar;                ///< DWM status bar properties
};

/**
 * DWM client describing a window
 */
struct Client {
    std::string name;  ///< Name of window
    Window window_id;  ///< Window XID
    int monitor_num;   ///< Index of monitor that client belongs to
    unsigned int tags; ///< Tags the client belongs to represented by bits
    struct {
        int cur_width; ///< Current border width
        int old_width; ///< Border width before border width change
    } border;          ///< Properties about border width
    struct {
        Geometry cur; ///< Current window geometry
        Geometry old; ///< Window geometry before it was changed
    } geom;           ///< Window geometry
    struct {
        Size base;      ///< Preferred window size
        Size increment; ///< Preferred window size change increments
        Size max;       ///< Preferred maximum window size
        Size min;       ///< Preferred minimum window size
        struct {
            float min;  ///< Preferred minimum aspect ratio
            float max;  ///< Preferred maximum aspect raito
        } aspect_ratio; ///< Preferred aspect ratio
    } size_hints;       ///< Size hints from client
    struct {
        bool is_fixed;    ///< Is the client position fixed
        bool is_floating; ///< Is the client floating
        bool is_urgent;   ///< Does the client have the urgent flag set
        bool never_focus; ///< Does the client handle its own inputs
        bool old_state;   ///< Stores the floating state if client is fullscreen
        bool is_fullscreen; ///< Is the client fullscreen (not monocle layout)
    } states;               ///< Client states
};

/**
 * DWM layout.
 */
struct Layout {
    std::string
        symbol; ///< Symbol that represents the layout. Note that the symbol
                ///< given here is the defining symbol of the layout. The layout
                ///< symbol given in Monitor or LayoutChangeEvent may be a
                ///< variation of this symbol. This symbol should not be assumed
                ///< to be the same symbol found in Monitor or in a
                ///< LayoutChangeEvent.
    uintptr_t address; ///< Address of layout in dwm's memory, used in
                       ///< setlayoutsafe command
};

/**
 * A struct representing states of each tag
 */
struct TagState {
    unsigned int selected; ///< Each bit is a tag, tags in view are 1
    unsigned int occupied; ///< Each bit is a tag, tags with clients are 1
    unsigned int urgent; ///< Each bit is a tag, tags with urgent clients are 1
};

/**
 * A tag
 */
struct Tag {
    unsigned int bit_mask; ///< The bit mask of this tag
    std::string tag_name;  ///< The name of the tag
};

/**
 * Struct describing a tag_change_event
 */
struct TagChangeEvent {
    TagState old_state;       ///< The old TagState
    TagState new_state;       ///< The new TagState
    unsigned int monitor_num; ///< Index of monitor that this event occured on
};

/**
 * Struct describing a selected_client_change_event
 */
struct SelectedClientChangeEvent {
    Window old_client_win;    ///< Window XID of the last selected client
    Window new_client_win;    ///< Window XID of the currently selected client
    unsigned int monitor_num; ///< Index of monitor that this event occured on
};

/**
 * Struct describing a layout_change_event. Note that the layout symbol may
 * change without the layout changing. The monocle layout is a good example of
 * this behavior.
 */
struct LayoutChangeEvent {
    std::string old_symbol;   ///< Last layout symbol. A layout may not always
                              ///< have the same symbol.
    std::string new_symbol;   ///< New layout symbol. A layout may not always
                              ///< have the same symbol.
    uintptr_t old_address;    ///< Address of old layout
    uintptr_t new_address;    ///< Address of new layout
    unsigned int monitor_num; ///< Index of monitor that this event occured on
};

/**
 * Enum of supported DWM message types
 */
enum MessageType {
    MESSAGE_TYPE_RUN_COMMAND = 0,
    MESSAGE_TYPE_GET_MONITORS = 1,
    MESSAGE_TYPE_GET_TAGS = 2,
    MESSAGE_TYPE_GET_LAYOUTS = 3,
    MESSAGE_TYPE_GET_DWM_CLIENT = 4,
    MESSAGE_TYPE_SUBSCRIBE = 5,
    MESSAGE_TYPE_EVENT = 6
};

/**
 * IPC events that can be subscribed to. Each event value represents a bit.
 */
enum Event : uint8_t {
    EVENT_TAG_CHANGE = 1,
    EVENT_SELECTED_CLIENT_CHANGE = 2,
    EVENT_LAYOUT_CHANGE = 4
};

/**
 * A hash map that associates Event enum values with their names
 */
extern const std::unordered_map<Event, std::string> event_map;

} // namespace dwmipc