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
 * A DWM monitor
 */
struct Monitor {
    float master_factor;   ///< Percentage of space master clients should occupy
    int num_master;        ///< Number of clients that should be masters
    unsigned int num;      ///< Index of monitor (according to DWM)
    bool is_selected;      ///< Is the monitor selected (in focus)
    Geometry monitor_geom; ///< Monitor geometry
    Geometry window_geom;  ///< Window area geometry
    struct {
        unsigned int cur; ///< Current tags in view, each bit is a tag
        unsigned int old; ///< Last tags in view, each bit is a tag
    } tagset;             ///< Tags in view
    TagState tag_state;   ///< Current tag state
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
    std::string name;         ///< Name of window
    Window window_id;         ///< Window XID
    unsigned int monitor_num; ///< Index of monitor that client belongs to
    unsigned int tags; ///< Tags the client belongs to represented by bits
    struct {
        int cur;    ///< Current border width
        int old;    ///< Border width before border width change
    } border_width; ///< Properties about border width
    struct {
        Geometry cur; ///< Current window geometry
        Geometry old; ///< Window geometry before it was changed
    } geom;           ///< Window geometry
    struct {
        Size base; ///< Preferred window size
        Size step; ///< Preferred window size change increments
        Size max;  ///< Preferred maximum window size
        Size min;  ///< Preferred minimum window size
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
 * Struct describing a tag_change_event
 */
struct TagChangeEvent {
    TagState old_state;       ///< The old TagState
    TagState new_state;       ///< The new TagState
    unsigned int monitor_num; ///< Index of monitor that this event occured on
};

/**
 * Struct describing a client_focus_change_event
 */
struct ClientFocusChangeEvent {
    Window old_win_id;        ///< Window XID of the last focused client
    Window new_win_id;        ///< Window XID of the newly focused client
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
 * Struct describing a monitor_focus_change_event
 */
struct MonitorFocusChangeEvent {
    unsigned int old_mon_num; ///< Index of previously focused monitor
    unsigned int new_mon_num; ///< Index of newly focused monitor
};

/**
 * Enum of supported DWM message types
 */
enum class MessageType : uint8_t {
    RUN_COMMAND = 0,
    GET_MONITORS = 1,
    GET_TAGS = 2,
    GET_LAYOUTS = 3,
    GET_DWM_CLIENT = 4,
    SUBSCRIBE = 5,
    EVENT = 6
};

/**
 * IPC events that can be subscribed to. Each event value represents a bit.
 */
enum class Event : uint8_t {
    TAG_CHANGE = 1,
    CLIENT_FOCUS_CHANGE = 2,
    LAYOUT_CHANGE = 4,
    MONITOR_FOCUS_CHANGE = 8
};

/**
 * A hash map that associates Event enum values with their names
 */
extern const std::unordered_map<Event, std::string> event_map;

} // namespace dwmipc
