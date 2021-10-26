#pragma once

#include <server_lib/network/connection.h>
#include <server_lib/network/server_config.h>

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

namespace server_lib {
namespace network {
    namespace transport_layer {
        struct server_impl_i;
        struct connection_impl_i;
    } // namespace transport_layer

    /**
     * \ingroup network
     *
     * \brief Simple async server with application connection
     */
    class server
    {
    public:
        server() = default;

        server(const server&) = delete;

        ~server();

        /**
         * Configurate
         *
         */
        static tcp_server_config configurate_tcp();

        /**
         * Common callback
         *
         */
        using common_callback_type = std::function<void()>;

        /**
         * Fail callback
         * Return error if connection failed asynchronously
         *
         */
        using fail_callback_type = std::function<void(const std::string&)>;

        /**
         * Inbound connection callback
         * Return connection object
         *
         */
        using new_connection_callback_type = std::function<void(const std::shared_ptr<connection>&)>;

        /**
         * Start the TCP server
         *
         */
        server& start(const tcp_server_config&);

        /**
         * Waiting for Web server starting or stopping
         *
         * \param wait_until_stop if 'false' it waits only
         * start process finishing
         *
         * \result if success
         */
        bool wait(bool wait_until_stop = false);

        server& on_start(common_callback_type&& callback);
        server& on_new_connection(new_connection_callback_type&& callback);
        server& on_fail(fail_callback_type&& callback);

        void stop(bool wait_for_removal = false);

        bool is_running(void) const;

        void post(common_callback_type&& callback);

    private:
        void on_new_client(const std::shared_ptr<transport_layer::connection_impl_i>&);
        void on_client_disconnected(size_t);

        std::shared_ptr<transport_layer::server_impl_i> _transport_layer;
        std::shared_ptr<unit_builder_i> _protocol;

        std::unordered_map<size_t, std::shared_ptr<connection>> _connections;
        std::mutex _connections_mutex;

        common_callback_type _start_callback = nullptr;
        new_connection_callback_type _new_connection_callback = nullptr;
        fail_callback_type _fail_callback = nullptr;
    };

} // namespace network
} // namespace server_lib
