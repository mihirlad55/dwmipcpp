/**
 * @file connection.hpp
 */
#pragma once

#include <functional>
#include <json/json.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "packet.hpp"

namespace dwmipc {
struct Monitor;
struct Client;

// Definition of X11 Window from xlib
typedef unsigned long Window;

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

/**
 * The DWM IPC connection class used to initiate a connection with DWM's IPC
 * socket and send/receive messages.
 */
class Connection {
  public:
    /**
     * Create a Connection object and connect to the DWM IPC socket.
     *
     * @param socket_path Path to DWM's IPC socket
     *
     */
    Connection(const std::string &socket_path);

    /**
     * Destroy the Connection object and disconnect from the DWM IPC socket
     */
    ~Connection();

    /**
     * Get a list of monitors and their properties as defined by DWM
     *
     * @return A list of Monitor objects
     *
     * @throw ResultFailureError if DWM sends an error reply.
     */
    std::shared_ptr<std::vector<Monitor>> get_monitors() const;

    /**
     * Get the list of tags defined by DWM
     *
     * @return A list of Tag objects
     *
     * @throw ResultFailureError if DWM sends an error reply.
     */
    std::shared_ptr<std::vector<Tag>> get_tags() const;

    /**
     * Get the list of available layouts as defined by DWM
     *
     * @return A list of Layout objects
     *
     * @throw ResultFailureError if DWM sends an error reply.
     */
    std::shared_ptr<std::vector<Layout>> get_layouts() const;

    /**
     * Get the properties of a DWM client
     *
     * @param win_id XID of the client window
     *
     * @return A Client object
     *
     * @throw ResultFailureError if DWM sends an error reply.
     */
    std::shared_ptr<Client> get_client(Window win_id) const;

    /**
     * Subscribe to the specified DWM event. After subscribing to an event, DWM
     * will send dwmipc::MESSAGE_TYPE_EVENT messages when the specified event is
     * raised. Subscribing to an already subscribed event will not throw an
     * error.
     *
     * @param ev The event to subscribe to
     *
     * @throw ResultFailureError if DWM sends an error reply.
     */
    void subscribe(const Event ev);

    /**
     * Unsubscribe to the specified DWM event. After unsubscribing to an event,
     * DWM will no longer send dwmipc::MESSAGE_TYPE_EVENT messages for the
     * specified event. Unsubscribing to an already unsubscribed event will not
     * throw an error.
     *
     * @param ev The event to subscribe to
     */
    void unsubscribe(const Event ev);

    /**
     * Try to read any received event messages and call event handlers.
     *
     * @throw NoMsgError if no messages were recieved
     * @throw IPCError if invalid message type received
     */
    void handle_event() const;

    /**
     * Run a DWM command
     *
     * @param name Name of the command
     * @param args Arguments for the command. The arguments must be either a
     *   string, bool, or number.
     *
     * @throw ResultFailureError if the command doesn't exist, the number of
     *   arguments are incorrect, the argument types are incorrect, or any other
     *   error reply from DWM.
     */
    template <typename... Types>
    void run_command(const std::string name, Types... args) const {
        Json::Value arr = Json::Value(Json::arrayValue);
        run_command_build(arr, args...);
        run_command(name, arr);
    }

    /**
     * Run a DWM command. Use this overload without specifying any arguments, if
     * this command takes no arguments.
     *
     * @param name Name of the command
     * @param arr JSON array of arguments for the command. If this parameter is
     *   not specified, the arguments default to an empty array.
     */
    void
    run_command(const std::string name,
                const Json::Value &arr = Json::Value(Json::arrayValue)) const;

    /**
     * Get the events that the connection is subscribed to. Use dwmipc::Event
     * values as a bitmask to determine which subscriptions are subscribed to.
     */
    uint8_t get_subscriptions() const;

    /**
     * The DWM IPC socket file descriptor
     */
    const int sockfd;

    /**
     * The path to the DWM IPC socket specified when Connection is constructed.
     */
    const std::string socket_path;

    /**
     * The dwmipc::EVENT_TAG_CHANGE handler. This will be called by handle_event
     * if an dwmipc::EVENT_TAG_CHANGE event message is received.
     */
    std::function<void(const TagChangeEvent &ev)> on_tag_change;

    /**
     * The dwmipc::EVENT_SELECTED_CLIENT_CHANGE handler. This will be called by
     * handle_event if an dwmipc::EVENT_SELECTED_CLIENT_CHANGE event message is
     * received.
     */
    std::function<void(const SelectedClientChangeEvent &ev)>
        on_selected_client_change;

    /**
     * The dwmipc::EVENT_LAYOUT_CHANGE handler. This will be called by
     * handle_event if an dwmipc::EVENT_LAYOUT_CHANGE event message is received.
     */
    std::function<void(const LayoutChangeEvent &ev)> on_layout_change;

  private:
    /**
     * The events that the connection is subscribed to. Use dwmipc::Event values
     * as a bitmask to determine which subscriptions are subscribed to.
     */
    uint8_t subscriptions = 0;

    /**
     * Connect to the DWM IPC socket at the specified path and get the file
     * descriptor to the socket. This is used to initialize the const sockfd
     * class member in the constructor initializer list.
     *
     * @param socket_path Path to the DWM IPC socket
     *
     * @return The file descriptor of the socket
     *
     * @throw IPCError if failed to connect to socket
     */
    static int connect(const std::string &socket_path);

    /**
     * Disconnect from the DWM IPC socket.
     */
    void disconnect();

    /**
     * Send a packet to DWM
     *
     * @param packet The packet to send
     *
     * @throw ReplyError if invalid reply received. This could be caused by
     *   receiving an event message when expecting a reply to the newly sent
     *   message.
     */
    void send_message(const std::shared_ptr<Packet> &packet) const;

    /**
     * Receive any incoming messages from DWM.
     *
     * @return A received packet from DWM
     *
     * @throw NoMsgError if no messages were received
     * @throw HeaderError if packet with invalid header received
     * @throw EOFError if unexpected EOF while reading message
     */
    std::shared_ptr<Packet> recv_message() const;

    /**
     * Send a message to DWM with the specified payload and message type
     *
     * @param type IPC message type
     * @param msg The payload
     *
     * @return The reply packet from DWM
     *
     * @throw ReplyError if reply message type doesn't match sent message type
     */
    std::shared_ptr<Packet> dwm_msg(const MessageType type,
                                    const std::string &msg) const;

    /**
     * Send a message to DWM of the specified message type with an empty payload
     *
     * @param type IPC message type
     *
     * @return The reply packet from DWM
     *
     * @throw ReplyError if reply message type doesn't match sent message type
     */
    std::shared_ptr<Packet> dwm_msg(const MessageType type) const;

    /**
     * Subscribe or unsubscribe to the specified event
     *
     * @param ev The event to subscribe/unsubcribe to
     * @param sub true to subscribe, false to unsubscribe
     *
     * @throw ResultFailureError if DWM sends an error reply.
     */
    void subscribe(const Event ev, const bool sub);

    /**
     * Base case of run_command_build which checks if a valid argument type was
     * provided and then appends the argument to the specified JSON array.
     *
     * @param arr The JSON array to append the argument to
     * @param arg The argument to append to the array
     */
    template <typename T>
    static void run_command_build(Json::Value &arr, T arg) {
        static_assert(std::is_arithmetic<T>::value ||             // number
                          std::is_same<T, std::string>::value ||  // string
                          std::is_same<T, const char *>::value || // string
                          std::is_same<T, bool>::value,           // bool
                      "The arguments to run_command must be a string, number, "
                      "bool, or NULL");
        arr.append(arg);
    }

    // Recursive
    /**
     * The recursive case of the run_command_build function which calls the base
     * case of run_command_build to append the first argument to the specified
     * JSON array and then calls itself with the remaining arguments.
     *
     * @param arr The JSON array to append the arg1 to
     * @param arg1 The argument to append to the array
     * @param args The rest of the arguments
     */
    template <typename T, typename... Ts>
    static void run_command_build(Json::Value &arr, T arg1, Ts... args) {
        run_command_build(arr, arg1);
        run_command_build(arr, args...);
    }
};
} // namespace dwmipc
