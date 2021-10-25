#pragma once

#include <server_lib/network/transport/connection_impl_i.h>

#include <server_lib/event_loop.h>

#include <cstdint>
#include <memory>

namespace server_lib {
namespace network {
    namespace transport_layer {

        /**
         * \ingroup network
         * \defgroup network_i Interfaces
         * Auxiliary network object interfaces
         * \defgroup network_utils Utilities
         * Helpers
         */

        /**
         * \ingroup network_i
         *
         * \brief Interface for transport implementation for async network server
         *
         * Implementations: server
         */
        class server_impl_i
        {
        public:
            virtual ~server_impl_i() = default;

        public:
            /**
            * Callback called whenever a network server has been started
            *
            */
            using start_callback_type = std::function<void()>;

            /**
             * Fail callback
             * Return error if server failed asynchronously
             *
             */
            using fail_callback_type = std::function<void(const std::string&)>;

            /**
            * Callback called whenever a new client is connecting to the network server
            *
            * Takes as parameter a shared pointer to the connection_impl_i that wishes to connect
            */
            using new_connection_callback_type = std::function<void(const std::shared_ptr<connection_impl_i>&)>;

            /**
            * Start the network server
            *
            * \param new_connection_callback
            * \param start_callback
            * \param fail_callback
            *
            */
            virtual bool start(const start_callback_type& start_callback,
                               const new_connection_callback_type& new_connection_callback,
                               const fail_callback_type& fail_callback)
                = 0;

            /**
            * Disconnect the network server if it was currently running.
            *
            */
            virtual void stop() = 0;

            /**
            * \return whether the network server is currently running or not
            */
            virtual bool is_running() const = 0;

            /**
            * \return thread loop
            */
            virtual event_loop& loop() = 0;
        };

    } // namespace transport_layer
} // namespace network
} // namespace server_lib
