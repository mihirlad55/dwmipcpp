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
     * @return true if an event message was received and handled, false if no
     *   event messages were available.
     *
     * @throw IPCError if invalid message type received
     */
    bool handle_event() const;

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
     * The main DWM IPC socket file descriptor for all non-event messages
     */
    const int main_sockfd;

    /**
     * The event DWM IPC socket file descriptor for all event messages. This is
     * kept separate to avoid conflicts between replies for sent messages and
     * event messages. For example, if a message were to be sent, and and an
     * event message was received before the reply, it could cause issues.
     */
    const int event_sockfd;

    /**
     * The path to the DWM IPC socket specified when Connection is constructed.
     */
    const std::string socket_path;

    /**
     * The Event::TAG_CHANGE handler. This will be called by handle_event
     * if an Event::TAG_CHANGE event message is received.
     */
    std::function<void(const TagChangeEvent &ev)> on_tag_change;

    /**
     * The Event::SELECTED_CLIENT_CHANGE handler. This will be called by
     * handle_event if an Event::SELECTED_CLIENT_CHANGE event message is
     * received.
     */
    std::function<void(const SelectedClientChangeEvent &ev)>
        on_selected_client_change;

    /**
     * The Event::LAYOUT_CHANGE handler. This will be called by
     * handle_event if an Event::LAYOUT_CHANGE event message is received.
     */
    std::function<void(const LayoutChangeEvent &ev)> on_layout_change;

    /**
     * The Event::SELECTED_MONITOR_CHANGE handler. This will be called by
     * handle_event if an Event::SELECTED_MONITOR_CHANGE event message is
     * received.
     */
    std::function<void(const SelectedMonitorChangeEvent &ev)>
        on_selected_monitor_change;

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
     * @param is_blocking If false, the socket will have the O_NONBLOCK flag set
     *
     * @return The file descriptor of the socket
     *
     * @throw IPCError if failed to connect to socket
     */
    static int connect(const std::string &socket_path, const bool is_blocking);

    /**
     * Disconnect from the DWM IPC socket.
     */
    void disconnect();

    /**
     * Send a packet to DWM
     *
     * @param sockfd The file descriptor of the socket to write the message to
     * @param packet The packet to send
     *
     * @throw ReplyError if invalid reply received. This could be caused by
     *   receiving an event message when expecting a reply to the newly sent
     *   message.
     */
    void send_message(int sockfd, const std::shared_ptr<Packet> &packet) const;

    /**
     * Receive any incoming messages from DWM. This is the main helper function
     * for attempting to read a message from DWM and validate the structure of
     * the message. It absorbs EINTR, EAGAIN, and EWOULDBLOCK errors.
     *
     * @param sockfd The file descriptor of the socket to receive the message
     *   from
     * @param wait Should it wait for a message to be received if a message is
     *   not already received? If this is false, the file descriptor should have
     *   the O_NONBLOCK flag set.
     *
     * @return A received packet from DWM
     *
     * @throw NoMsgError if no messages were received
     * @throw HeaderError if packet with invalid header received
     * @throw EOFError if unexpected EOF while reading message
     */
    std::shared_ptr<Packet> recv_message(int sockfd, const bool wait) const;

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
