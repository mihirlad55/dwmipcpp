/**
 * @file connection.hpp
 *
 * This is file contains the main class for interacting with DWM
 */

#pragma once

#include <functional>
#include <json/json.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "packet.hpp"
#include "types.hpp"

namespace dwmipc {
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
     * @param connect If true, the main socket and event socket will be
     *   connected to upon construction.
     */
    Connection(const std::string &socket_path, bool connect = true);

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
     * @throw SocketClosedError if the socket is disconnected
     */
    std::shared_ptr<std::vector<Monitor>> get_monitors();

    /**
     * Get the list of tags defined by DWM
     *
     * @return A list of Tag objects
     *
     * @throw ResultFailureError if DWM sends an error reply.
     * @throw SocketClosedError if the socket is disconnected
     */
    std::shared_ptr<std::vector<Tag>> get_tags();

    /**
     * Get the list of available layouts as defined by DWM
     *
     * @return A list of Layout objects
     *
     * @throw ResultFailureError if DWM sends an error reply.
     * @throw SocketClosedError if the socket is disconnected
     */
    std::shared_ptr<std::vector<Layout>> get_layouts();

    /**
     * Get the properties of a DWM client
     *
     * @param win_id XID of the client window
     *
     * @return A Client object
     *
     * @throw ResultFailureError if DWM sends an error reply.
     * @throw SocketClosedError if the socket is disconnected
     */
    std::shared_ptr<Client> get_client(Window win_id);

    /**
     * Subscribe to the specified DWM event. After subscribing to an event, DWM
     * will send dwmipc::MessageType::EVENT messages when the specified
     * event is raised. Subscribing to an already subscribed event will not
     * throw an error.
     *
     * @param ev The event to subscribe to
     *
     * @throw ResultFailureError if DWM sends an error reply.
     * @throw SocketClosedError if the socket is disconnected
     */
    void subscribe(const Event ev);

    /**
     * Unsubscribe to the specified DWM event. After unsubscribing to an event,
     * DWM will no longer send dwmipc::MessageType::EVENT messages for
     * the specified event. Unsubscribing to an already unsubscribed event will
     * not throw an error.
     *
     * @param ev The event to subscribe to
     *
     * @throw SocketClosedError if the socket is disconnected
     */
    void unsubscribe(const Event ev);

    /**
     * Try to read any received event messages and call event handlers.
     *
     * @return true if an event message was received and handled, false if no
     *   event messages were available.
     *
     * @throw SocketClosedError if the socket is disconnected
     * @throw IPCError if invalid message type received
     */
    bool handle_event();

    /**
     * Run a DWM command
     *
     * @param name Name of the command
     * @param args Arguments for the command. The arguments must be either a
     *   string, bool, or number.
     *
     * @warning On particular versions of jsoncpp (i.e. 1.7.2), you may get
     *   compilation errors about unambigous function overloads. To fix this,
     *   you must cast the arguments to a Json type definition i.e.
     *   Json::UInt64, Json::UInt.
     *
     * @throw ResultFailureError if the command doesn't exist, the number of
     *   arguments are incorrect, the argument types are incorrect, or any other
     *   error reply from DWM.
     * @throw SocketClosedError if the socket is disconnected
     */
    template <typename... Types>
    void run_command(const std::string name, Types... args) {
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
     * @throw SocketClosedError if the socket is disconnected
     */
    void run_command(const std::string name,
                     const Json::Value &arr = Json::Value(Json::arrayValue));

    /**
     * Check if main socket is connected
     *
     * @return true if connected, false otherwise
     */
    bool is_main_socket_connected() const;

    /**
     * Check if event socket is connected
     *
     * @return true if connected, false otherwise
     */
    bool is_event_socket_connected() const;

    /**
     * Connect to the main socket
     *
     * @throw InvalidOperationError if socket is already connected
     */
    void connect_main_socket();

    /**
     * Connect to the event socket
     *
     * @throw InvalidOperationError if socket is already connected
     */
    void connect_event_socket();

    /**
     * Disconnect from the main socket
     *
     * @throw InvalidOperationError if socket is already disconnected
     */
    void disconnect_main_socket();

    /**
     * Disconnect from the event socket
     *
     * @throw InvalidOperationError if socket is already disconnected
     */
    void disconnect_event_socket();

    /**
     * Get the events that the connection is subscribed to. Use dwmipc::Event
     * values as a bitmask to determine which subscriptions are subscribed to.
     */
    uint8_t get_subscriptions() const;

    /**
     * Get the file descriptor of the main socket
     *
     * @return File descriptor of the main socket, -1 if disconnected
     */
    int get_main_socket_fd() const;

    /**
     * Get the file descriptor of the event socket
     *
     * @return File descriptor of the event socket, -1 if disconnected
     */
    int get_event_socket_fd() const;

    /**
     * The path to the DWM IPC socket specified when Connection is constructed.
     */
    const std::string socket_path;

    /**
     * The dwmipc::Event::TAG_CHANGE handler. This will be called by
     * handle_event if an dwmipc::Event::TAG_CHANGE event message is received.
     */
    std::function<void(const TagChangeEvent &ev)> on_tag_change;

    /**
     * The dwmipc::Event::CLIENT_FOCUS_CHANGE handler. This will be called by
     * handle_event if an dwmipc::Event::CLIENT_FOCUS_CHANGE event message is
     * received.
     */
    std::function<void(const ClientFocusChangeEvent &ev)>
        on_client_focus_change;

    /**
     * The dwmipc::Event::LAYOUT_CHANGE handler. This will be called by
     * handle_event if an dwmipc::Event::LAYOUT_CHANGE event message is
     * received.
     */
    std::function<void(const LayoutChangeEvent &ev)> on_layout_change;

    /**
     * The dwmipc::Event::MONITOR_FOCUS_CHANGE handler. This will be called
     * by handle_event if an dwmipc::Event::MONITOR_FOCUS_CHANGE event
     * message is received.
     */
    std::function<void(const MonitorFocusChangeEvent &ev)>
        on_monitor_focus_change;

    /**
     * The dwmipc::Event::FOCUSED_TITLE_CHANGE handler. This will be called
     * by handle_event if an dwmipc::Event::FOCUSED_TITLE_CHANGE event
     * message is received.
     */
    std::function<void(const FocusedTitleChangeEvent &ev)>
        on_focused_title_change;

    /**
     * The dwmipc::Event::FOCUSED_STATE_CHANGE handler. This will be called
     * by handle_event if an dwmipc::Event::FOCUSED_STATE_CHANGE event
     * message is received.
     */
    std::function<void(const FocusedStateChangeEvent &ev)>
        on_focused_state_change;

  private:
    /**
     * The main DWM IPC socket file descriptor for all non-event messages
     */
    int main_sockfd = -1;

    /**
     * The event DWM IPC socket file descriptor for all event messages. This is
     * kept separate to avoid conflicts between replies for sent messages and
     * event messages. For example, if a message were to be sent, and and an
     * event message was received before the reply, it could cause issues.
     */
    int event_sockfd = -1;

    /**
     * The events that the connection is subscribed to. Use dwmipc::Event values
     * as a bitmask to determine which subscriptions are subscribed to.
     */
    uint8_t subscriptions = 0;

    /**
     * Subscribe to all events specified in subscriptions. This is used to
     * resubscribe to events after a reconnection.
     */
    void resubscribe();

    /**
     * Obtain the appropriate file descriptor to use based on the type of
     * message
     *
     * @param type The type of message to be received/sent
     */
    int get_socket_fd(const MessageType type) const;

    /**
     * Require that the appropriate socket is connected to write/receive the
     * specified message type. This function throws a SocketClosedError if the
     * socket is disconnected.
     *
     * @throw SocketClosedError if the required socket is disconnected
     */
    void assert_socket_connected(const MessageType type) const;

    /**
     * Send a message to DWM with the specified payload and message type
     *
     * @param type IPC message type
     * @param msg The payload. This can be left empty for message types where
     *   the payload is not needed/used.
     * @param reconnect If true and the main socket is disconnected, an attempt
     *   to re-establish the connection will be made
     *
     * @return The reply packet from DWM
     *
     * @throw ReplyError if reply message type doesn't match sent message type
     */
    std::shared_ptr<Packet> dwm_msg(const MessageType type,
                                    const std::string &msg = "");

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
