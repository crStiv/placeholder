//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2017-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#pragma once

#include <nil/actor/config.hpp>

#ifndef ACTOR_NO_EXCEPTIONS
#include <exception>
#endif    // ACTOR_NO_EXCEPTIONS

#include <forward_list>
#include <map>
#include <type_traits>
#include <unordered_map>

#include <nil/actor/actor_traits.hpp>
#include <nil/actor/broadcast_downstream_manager.hpp>
#include <nil/actor/default_downstream_manager.hpp>
#include <nil/actor/detail/behavior_stack.hpp>

#include <nil/actor/detail/stream_sink_driver_impl.hpp>
#include <nil/actor/detail/stream_sink_impl.hpp>
#include <nil/actor/detail/stream_source_driver_impl.hpp>
#include <nil/actor/detail/stream_source_impl.hpp>
#include <nil/actor/detail/stream_stage_driver_impl.hpp>
#include <nil/actor/detail/stream_stage_impl.hpp>
#include <nil/actor/detail/tick_emitter.hpp>
#include <nil/actor/detail/unordered_flat_map.hpp>
#include <nil/actor/error.hpp>
#include <nil/actor/extend.hpp>
#include <nil/actor/fwd.hpp>
#include <nil/actor/inbound_path.hpp>
#include <nil/actor/intrusive/drr_cached_queue.hpp>
#include <nil/actor/intrusive/drr_queue.hpp>
#include <nil/actor/intrusive/fifo_inbox.hpp>
#include <nil/actor/intrusive/wdrr_dynamic_multiplexed_queue.hpp>
#include <nil/actor/intrusive/wdrr_fixed_multiplexed_queue.hpp>
#include <nil/actor/invoke_message_result.hpp>
#include <nil/actor/is_actor_handle.hpp>
#include <nil/actor/local_actor.hpp>
#include <nil/actor/logger.hpp>
#include <nil/actor/make_sink_result.hpp>
#include <nil/actor/make_source_result.hpp>
#include <nil/actor/make_stage_result.hpp>
#include <nil/actor/mixin/behavior_changer.hpp>
#include <nil/actor/mixin/requester.hpp>
#include <nil/actor/mixin/sender.hpp>
#include <nil/actor/no_stages.hpp>
#include <nil/actor/policy/arg.hpp>
#include <nil/actor/policy/categorized.hpp>
#include <nil/actor/policy/downstream_messages.hpp>
#include <nil/actor/policy/normal_messages.hpp>
#include <nil/actor/policy/upstream_messages.hpp>
#include <nil/actor/policy/urgent_messages.hpp>
#include <nil/actor/response_handle.hpp>
#include <nil/actor/scheduled_actor.hpp>
#include <nil/actor/sec.hpp>
#include <nil/actor/stream.hpp>
#include <nil/actor/stream_manager.hpp>
#include <nil/actor/stream_sink_trait.hpp>
#include <nil/actor/stream_source_trait.hpp>
#include <nil/actor/stream_stage_trait.hpp>
#include <nil/actor/to_string.hpp>

namespace nil {
    namespace actor {

        // -- related free functions ---------------------------------------------------

        /// @relates scheduled_actor
        /// Default handler function that sends the message back to the sender.
        BOOST_SYMBOL_VISIBLE result<message> reflect(scheduled_actor *, message &);

        /// @relates scheduled_actor
        /// Default handler function that sends
        /// the message back to the sender and then quits.
        BOOST_SYMBOL_VISIBLE result<message> reflect_and_quit(scheduled_actor *, message &);

        /// @relates scheduled_actor
        /// Default handler function that prints messages
        /// message via `aout` and drops them afterwards.
        BOOST_SYMBOL_VISIBLE result<message> print_and_drop(scheduled_actor *, message &);

        /// @relates scheduled_actor
        /// Default handler function that simply drops messages.
        BOOST_SYMBOL_VISIBLE result<message> drop(scheduled_actor *, message &);

        /// A cooperatively scheduled, event-based actor implementation.
        class BOOST_SYMBOL_VISIBLE scheduled_actor : public local_actor,
                                                     public resumable,
                                                     public non_blocking_actor_base {
        public:
            // -- nested enums -----------------------------------------------------------

            /// Categorizes incoming messages.
            enum class message_category {
                /// Triggers the current behavior.
                ordinary,
                /// Triggers handlers for system messages such as `exit_msg` or `down_msg`.
                internal,
                /// Delays processing.
                skipped,
            };

            /// Result of one-shot activations.
            enum class activation_result {
                /// Actor is still alive and handled the activation message.
                success,
                /// Actor handled the activation message and terminated.
                terminated,
                /// Actor skipped the activation message.
                skipped,
                /// Actor dropped the activation message.
                dropped
            };

            // -- nested and member types ------------------------------------------------

            /// Base type.
            using super = local_actor;

            /// Maps slot IDs to stream managers.
            using stream_manager_map = std::map<stream_slot, stream_manager_ptr>;

            /// Stores asynchronous messages with default priority.
            using normal_queue = intrusive::drr_cached_queue<policy::normal_messages>;

            /// Stores asynchronous messages with hifh priority.
            using urgent_queue = intrusive::drr_cached_queue<policy::urgent_messages>;

            /// Stores upstream messages.
            using upstream_queue = intrusive::drr_queue<policy::upstream_messages>;

            /// Stores downstream messages.
            using downstream_queue = intrusive::wdrr_dynamic_multiplexed_queue<policy::downstream_messages>;

            /// Configures the FIFO inbox with four nested queues:
            ///
            ///   1. Default asynchronous messages
            ///   2. High-priority asynchronous messages
            ///   3. Upstream messages
            ///   4. Downstream messages
            ///
            /// The queue for downstream messages is in turn composed of a nested queues,
            /// one for each active input slot.
            struct mailbox_policy {
                using deficit_type = size_t;

                using mapped_type = mailbox_element;

                using unique_pointer = mailbox_element_ptr;

                using queue_type =
                    intrusive::wdrr_fixed_multiplexed_queue<policy::categorized, urgent_queue, normal_queue,
                                                            upstream_queue, downstream_queue>;
            };

            static constexpr size_t urgent_queue_index = 0;

            static constexpr size_t normal_queue_index = 1;

            static constexpr size_t upstream_queue_index = 2;

            static constexpr size_t downstream_queue_index = 3;

            /// A queue optimized for single-reader-many-writers.
            using mailbox_type = intrusive::fifo_inbox<mailbox_policy>;

            /// The message ID of an outstanding response with its callback.
            using pending_response = std::pair<const message_id, behavior>;

            /// A pointer to a scheduled actor.
            using pointer = scheduled_actor *;

            /// Function object for handling unmatched messages.
            using default_handler = std::function<result<message>(pointer, message &)>;

            /// Function object for handling error messages.
            using error_handler = std::function<void(pointer, error &)>;

            /// Function object for handling down messages.
            using down_handler = std::function<void(pointer, down_msg &)>;

            /// Function object for handling node down messages.
            using node_down_handler = std::function<void(pointer, node_down_msg &)>;

            /// Function object for handling exit messages.
            using exit_handler = std::function<void(pointer, exit_msg &)>;

#ifndef ACTOR_NO_EXCEPTIONS
            /// Function object for handling exit messages.
            using exception_handler = std::function<error(pointer, std::exception_ptr &)>;
#endif    // ACTOR_NO_EXCEPTIONS

            /// Consumes messages from the mailbox.
            struct mailbox_visitor {
                scheduled_actor *self;
                size_t &handled_msgs;
                size_t max_throughput;

                /// Consumes upstream messages.
                intrusive::task_result operator()(size_t, upstream_queue &, mailbox_element &);

                /// Consumes downstream messages.
                intrusive::task_result operator()(size_t, downstream_queue &, stream_slot slot,
                                                  policy::downstream_messages::nested_queue_type &, mailbox_element &);

                // Dispatches asynchronous messages with high and normal priority to the
                // same handler.
                template<class Queue>
                intrusive::task_result operator()(size_t, Queue &, mailbox_element &x) {
                    return (*this)(x);
                }

                // Consumes asynchronous messages.
                intrusive::task_result operator()(mailbox_element &x);
            };

            // -- static helper functions ------------------------------------------------

            static void default_error_handler(pointer ptr, error &x);

            static void default_down_handler(pointer ptr, down_msg &x);

            static void default_node_down_handler(pointer ptr, node_down_msg &x);

            static void default_exit_handler(pointer ptr, exit_msg &x);

#ifndef ACTOR_NO_EXCEPTIONS
            static error default_exception_handler(pointer ptr, std::exception_ptr &x);
#endif    // ACTOR_NO_EXCEPTIONS

            // -- constructors and destructors -------------------------------------------

            explicit scheduled_actor(actor_config &cfg);

            scheduled_actor(scheduled_actor &&) = delete;

            scheduled_actor(const scheduled_actor &) = delete;

            scheduled_actor &operator=(scheduled_actor &&) = delete;

            scheduled_actor &operator=(const scheduled_actor &) = delete;

            ~scheduled_actor() override;

            // -- overridden functions of abstract_actor ---------------------------------

            using abstract_actor::enqueue;

            void enqueue(mailbox_element_ptr ptr, execution_unit *eu) override;

            mailbox_element *peek_at_next_mailbox_element() override;

            // -- overridden functions of local_actor ------------------------------------

            const char *name() const override;

            void launch(execution_unit *eu, bool lazy, bool hide) override;

            bool cleanup(error &&fail_state, execution_unit *host) override;

            // -- overridden functions of resumable --------------------------------------

            subtype_t subtype() const override;

            void intrusive_ptr_add_ref_impl() override;

            void intrusive_ptr_release_impl() override;

            resume_result resume(execution_unit *, size_t) override;

            // -- scheduler callbacks ----------------------------------------------------

            /// Returns a factory for proxies created
            /// and managed by this actor or `nullptr`.
            virtual proxy_registry *proxy_registry_ptr();

            // -- state modifiers --------------------------------------------------------

            /// Finishes execution of this actor after any currently running message
            /// handler is done. This member function clears the behavior stack of the
            /// running actor and invokes `on_exit()`. The actors does not finish
            /// execution if the implementation of `on_exit()` sets a new behavior. When
            /// setting a new behavior in `on_exit()`, one has to make sure to not produce
            /// an infinite recursion.
            ///
            /// If `on_exit()` did not set a new behavior, the actor sends an exit message
            /// to all of its linked actors, sets its state to exited and finishes
            /// execution.
            void quit(error x = error {});

            // -- properties -------------------------------------------------------------

            /// Returns the queue for storing incoming messages.
            inline mailbox_type &mailbox() noexcept {
                return mailbox_;
            }

            /// Returns map for all active streams.
            inline stream_manager_map &stream_managers() noexcept {
                return stream_managers_;
            }

            /// Returns map for all pending streams.
            inline stream_manager_map &pending_stream_managers() noexcept {
                return pending_stream_managers_;
            }

            // -- event handlers ---------------------------------------------------------

            /// Sets a custom handler for unexpected messages.
            inline void set_default_handler(default_handler fun) {
                if (fun)
                    default_handler_ = std::move(fun);
                else
                    default_handler_ = print_and_drop;
            }

            /// Sets a custom handler for unexpected messages.
            template<class F>
            typename std::enable_if<std::is_convertible<F, std::function<result<message>(message &)>>::value>::type
                set_default_handler(F fun) {
                default_handler_ = [=](scheduled_actor *, message &xs) { return fun(xs); };
            }

            /// Sets a custom handler for error messages.
            inline void set_error_handler(error_handler fun) {
                if (fun)
                    error_handler_ = std::move(fun);
                else
                    error_handler_ = default_error_handler;
            }

            /// Sets a custom handler for error messages.
            template<class T>
            auto set_error_handler(T fun) -> decltype(fun(std::declval<error &>())) {
                set_error_handler([fun](scheduled_actor *, error &x) { fun(x); });
            }

            /// Sets a custom handler for down messages.
            inline void set_down_handler(down_handler fun) {
                if (fun)
                    down_handler_ = std::move(fun);
                else
                    down_handler_ = default_down_handler;
            }

            /// Sets a custom handler for down messages.
            template<class T>
            auto set_down_handler(T fun) -> decltype(fun(std::declval<down_msg &>())) {
                set_down_handler([fun](scheduled_actor *, down_msg &x) { fun(x); });
            }

            /// Sets a custom handler for node down messages.
            void set_node_down_handler(node_down_handler fun) {
                if (fun)
                    node_down_handler_ = std::move(fun);
                else
                    node_down_handler_ = default_node_down_handler;
            }

            /// Sets a custom handler for down messages.
            template<class T>
            auto set_node_down_handler(T fun) -> decltype(fun(std::declval<node_down_msg &>())) {
                set_node_down_handler([fun](scheduled_actor *, node_down_msg &x) { fun(x); });
            }

            /// Sets a custom handler for error messages.
            inline void set_exit_handler(exit_handler fun) {
                if (fun)
                    exit_handler_ = std::move(fun);
                else
                    exit_handler_ = default_exit_handler;
            }

            /// Sets a custom handler for exit messages.
            template<class T>
            auto set_exit_handler(T fun) -> decltype(fun(std::declval<exit_msg &>())) {
                set_exit_handler([fun](scheduled_actor *, exit_msg &x) { fun(x); });
            }

#ifndef ACTOR_NO_EXCEPTIONS
            /// Sets a custom exception handler for this actor. If multiple handlers are
            /// defined, only the functor that was added *last* is being executed.
            inline void set_exception_handler(exception_handler fun) {
                if (fun)
                    exception_handler_ = std::move(fun);
                else
                    exception_handler_ = default_exception_handler;
            }

            /// Sets a custom exception handler for this actor. If multiple handlers are
            /// defined, only the functor that was added *last* is being executed.
            template<class F>
            typename std::enable_if<std::is_convertible<F, std::function<error(std::exception_ptr &)>>::value>::type
                set_exception_handler(F f) {
                set_exception_handler([f](scheduled_actor *, std::exception_ptr &x) { return f(x); });
            }
#endif    // ACTOR_NO_EXCEPTIONS

            // -- stream management ------------------------------------------------------

            template<class Driver, class... Ts, class Init, class Pull, class Done, class Finalize = unit_t>
            [[deprecated("use attach_stream_source instead")]] make_source_result_t<
                typename Driver::downstream_manager_type, Ts...>
                make_source(std::tuple<Ts...> xs, Init init, Pull pull, Done done, Finalize fin = {}) {
                using detail::make_stream_source;
                auto mgr =
                    make_stream_source<Driver>(this, std::move(init), std::move(pull), std::move(done), std::move(fin));
                auto slot = mgr->add_outbound_path(std::move(xs));
                return {slot, std::move(mgr)};
            }

            template<
                class... Ts, class Init, class Pull, class Done, class Finalize = unit_t,
                class DownstreamManager = broadcast_downstream_manager<typename stream_source_trait_t<Pull>::output>>
            [[deprecated("use attach_stream_source instead")]] make_source_result_t<DownstreamManager, Ts...>
                make_source(std::tuple<Ts...> xs, Init init, Pull pull, Done done, Finalize fin = {},
                            policy::arg<DownstreamManager> = {}) {
                using driver = detail::stream_source_driver_impl<DownstreamManager, Pull, Done, Finalize>;
                return make_source<driver>(std::move(xs), std::move(init), std::move(pull), std::move(done),
                                           std::move(fin));
            }

            template<class Init, class Pull, class Done, class Finalize = unit_t,
                     class DownstreamManager = default_downstream_manager_t<Pull>,
                     class Trait = stream_source_trait_t<Pull>>
            [[deprecated("use attach_stream_source instead")]] detail::enable_if_t<
                !is_actor_handle<Init>::value && Trait::valid, make_source_result_t<DownstreamManager>>
                make_source(Init init, Pull pull, Done done, Finalize finalize = {},
                            policy::arg<DownstreamManager> token = {}) {
                return make_source(std::make_tuple(), init, pull, done, finalize, token);
            }

            template<class ActorHandle, class... Ts, class Init, class Pull, class Done, class Finalize = unit_t,
                     class DownstreamManager = default_downstream_manager_t<Pull>,
                     class Trait = stream_source_trait_t<Pull>>
            [[deprecated("use attach_stream_source instead")]] detail::enable_if_t<
                is_actor_handle<ActorHandle>::value, make_source_result_t<DownstreamManager>>
                make_source(const ActorHandle &dest, std::tuple<Ts...> xs, Init init, Pull pull, Done done,
                            Finalize fin = {}, policy::arg<DownstreamManager> = {}) {
                // TODO: type checking of dest
                using driver = detail::stream_source_driver_impl<DownstreamManager, Pull, Done, Finalize>;
                auto mgr = detail::make_stream_source<driver>(this, std::move(init), std::move(pull), std::move(done),
                                                              std::move(fin));
                auto slot = mgr->add_outbound_path(dest, std::move(xs));
                return {slot, std::move(mgr)};
            }

            template<class ActorHandle, class Init, class Pull, class Done, class Finalize = unit_t,
                     class DownstreamManager = default_downstream_manager_t<Pull>,
                     class Trait = stream_source_trait_t<Pull>>
            [[deprecated("use attach_stream_source instead")]] detail::enable_if_t<
                is_actor_handle<ActorHandle>::value && Trait::valid, make_source_result_t<DownstreamManager>>
                make_source(const ActorHandle &dest, Init init, Pull pull, Done done, Finalize fin = {},
                            policy::arg<DownstreamManager> token = {}) {
                return make_source(dest, std::make_tuple(), std::move(init), std::move(pull), std::move(done),
                                   std::move(fin), token);
            }

            template<class Driver, class Init, class Pull, class Done, class Finalize = unit_t>
            [[deprecated("use attach_continuous_stream_source instead")]] auto
                make_continuous_source(Init init, Pull pull, Done done, Finalize fin = {}) {
                using detail::make_stream_source;
                auto mgr =
                    make_stream_source<Driver>(this, std::move(init), std::move(pull), std::move(done), std::move(fin));
                mgr->continuous(true);
                return mgr;
            }

            template<
                class Init, class Pull, class Done, class Finalize = unit_t,
                class DownstreamManager = broadcast_downstream_manager<typename stream_source_trait_t<Pull>::output>>
            [[deprecated("use attach_continuous_stream_source instead")]] auto
                make_continuous_source(Init init, Pull pull, Done done, Finalize fin = {},
                                       policy::arg<DownstreamManager> = {}) {
                using driver = detail::stream_source_driver_impl<DownstreamManager, Pull, Done, Finalize>;
                return make_continuous_source<driver>(std::move(init), std::move(pull), std::move(done),
                                                      std::move(fin));
            }

            template<class Driver, class... Ts>
            [[deprecated("use attach_stream_sink instead")]] make_sink_result<typename Driver::input_type>
                make_sink(const stream<typename Driver::input_type> &src, Ts &&... xs) {
                auto mgr = detail::make_stream_sink<Driver>(this, std::forward<Ts>(xs)...);
                auto slot = mgr->add_inbound_path(src);
                return {slot, std::move(mgr)};
            }

            template<class In, class Init, class Fun, class Finalize = unit_t, class Trait = stream_sink_trait_t<Fun>>
            [[deprecated("use attach_stream_sink instead")]] make_sink_result<In>
                make_sink(const stream<In> &in, Init init, Fun fun, Finalize fin = {}) {
                using driver = detail::stream_sink_driver_impl<In, Fun, Finalize>;
                return make_sink<driver>(in, std::move(init), std::move(fun), std::move(fin));
            }

            template<class Driver, class In, class... Ts, class... Us>
            [[deprecated("use attach_stream_stage instead")]] make_stage_result_t<
                In, typename Driver::downstream_manager_type, Ts...>
                make_stage(const stream<In> &src, std::tuple<Ts...> xs, Us &&... ys) {
                using detail::make_stream_stage;
                auto mgr = make_stream_stage<Driver>(this, std::forward<Us>(ys)...);
                auto in = mgr->add_inbound_path(src);
                auto out = mgr->add_outbound_path(std::move(xs));
                return {in, out, std::move(mgr)};
            }

            template<class In, class... Ts, class Init, class Fun, class Finalize = unit_t,
                     class DownstreamManager = default_downstream_manager_t<Fun>,
                     class Trait = stream_stage_trait_t<Fun>>
            [[deprecated("use attach_stream_stage instead")]] make_stage_result_t<In, DownstreamManager, Ts...>
                make_stage(const stream<In> &in, std::tuple<Ts...> xs, Init init, Fun fun, Finalize fin = {},
                           policy::arg<DownstreamManager> token = {}) {
                ACTOR_IGNORE_UNUSED(token);
                ACTOR_ASSERT(current_mailbox_element() != nullptr);
                ACTOR_ASSERT(current_mailbox_element()->content().types() == make_type_id_list<open_stream_msg>());
                using output_type = typename stream_stage_trait_t<Fun>::output;
                using state_type = typename stream_stage_trait_t<Fun>::state;
                static_assert(
                    std::is_same<void(state_type &), typename detail::get_callable_trait<Init>::fun_sig>::value,
                    "Expected signature `void (State&)` for init function");
                static_assert(std::is_same<void(state_type &, downstream<output_type> &, In),
                                           typename detail::get_callable_trait<Fun>::fun_sig>::value,
                              "Expected signature `void (State&, downstream<Out>&, In)` "
                              "for consume function");
                using driver =
                    detail::stream_stage_driver_impl<typename Trait::input, DownstreamManager, Fun, Finalize>;
                return make_stage<driver>(in, std::move(xs), std::move(init), std::move(fun), std::move(fin));
            }

            template<class In, class Init, class Fun, class Finalize = unit_t,
                     class DownstreamManager = default_downstream_manager_t<Fun>,
                     class Trait = stream_stage_trait_t<Fun>>
            [[deprecated("use attach_stream_stage instead")]] make_stage_result_t<In, DownstreamManager>
                make_stage(const stream<In> &in, Init init, Fun fun, Finalize fin = {},
                           policy::arg<DownstreamManager> token = {}) {
                return make_stage(in, std::make_tuple(), std::move(init), std::move(fun), std::move(fin), token);
            }

            template<class Driver, class... Ts>
            [[deprecated("use attach_continuous_stream_stage instead")]] auto make_continuous_stage(Ts &&... xs) {
                auto ptr = detail::make_stream_stage<Driver>(this, std::forward<Ts>(xs)...);
                ptr->continuous(true);
                return ptr;
            }

            template<class Init, class Fun, class Cleanup, class DownstreamManager = default_downstream_manager_t<Fun>,
                     class Trait = stream_stage_trait_t<Fun>>
            [[deprecated("use attach_continuous_stream_stage instead")]] auto
                make_continuous_stage(Init init, Fun fun, Cleanup cleanup, policy::arg<DownstreamManager> token = {}) {
                ACTOR_IGNORE_UNUSED(token);
                using input_type = typename Trait::input;
                using output_type = typename Trait::output;
                using state_type = typename Trait::state;
                static_assert(
                    std::is_same<void(state_type &), typename detail::get_callable_trait<Init>::fun_sig>::value,
                    "Expected signature `void (State&)` for init function");
                static_assert(std::is_same<void(state_type &, downstream<output_type> &, input_type),
                                           typename detail::get_callable_trait<Fun>::fun_sig>::value,
                              "Expected signature `void (State&, downstream<Out>&, In)` "
                              "for consume function");
                using driver = detail::stream_stage_driver_impl<typename Trait::input, DownstreamManager, Fun, Cleanup>;
                return make_continuous_stage<driver>(std::move(init), std::move(fun), std::move(cleanup));
            }

            /// @cond PRIVATE

            // -- timeout management -----------------------------------------------------

            /// Requests a new timeout and returns its ID.
            uint64_t set_receive_timeout(actor_clock::time_point x);

            /// Requests a new timeout for the current behavior and returns its ID.
            uint64_t set_receive_timeout();

            /// Resets the timeout if `timeout_id` is the active timeout.
            void reset_receive_timeout(uint64_t timeout_id);

            /// Returns whether `timeout_id` is currently active.
            bool is_active_receive_timeout(uint64_t tid) const;

            /// Requests a new timeout and returns its ID.
            uint64_t set_stream_timeout(actor_clock::time_point x);

            // -- message processing -----------------------------------------------------

            /// Adds a callback for an awaited response.
            void add_awaited_response_handler(message_id response_id, behavior bhvr);

            /// Adds a callback for a multiplexed response.
            void add_multiplexed_response_handler(message_id response_id, behavior bhvr);

            /// Returns the category of `x`.
            message_category categorize(mailbox_element &x);

            /// Tries to consume `x`.
            virtual invoke_message_result consume(mailbox_element &x);

            /// Tries to consume `x`.
            void consume(mailbox_element_ptr x);

            /// Activates an actor and runs initialization code if necessary.
            /// @returns `true` if the actor is alive and ready for `reactivate`,
            ///          `false` otherwise.
            bool activate(execution_unit *ctx);

            /// One-shot interface for activating an actor for a single message.
            activation_result activate(execution_unit *ctx, mailbox_element &x);

            /// Interface for activating an actor any
            /// number of additional times after `activate`.
            activation_result reactivate(mailbox_element &x);

            // -- behavior management ----------------------------------------------------

            /// Returns `true` if the behavior stack is not empty.
            inline bool has_behavior() const noexcept {
                return !bhvr_stack_.empty();
            }

            inline behavior &current_behavior() {
                return !awaited_responses_.empty() ? awaited_responses_.front().second : bhvr_stack_.back();
            }

            /// Installs a new behavior without performing any type checks.
            void do_become(behavior bhvr, bool discard_old);

            /// Performs cleanup code for the actor if it has no active
            /// behavior or was explicitly terminated.
            /// @returns `true` if cleanup code was called, `false` otherwise.
            bool finalize();

            /// Returns the behavior stack.
            inline detail::behavior_stack &bhvr_stack() {
                return bhvr_stack_;
            }

            /// Pushes `ptr` to the cache of the default queue.
            void push_to_cache(mailbox_element_ptr ptr);

            /// Returns the default queue of the mailbox that stores ordinary messages.
            normal_queue &get_normal_queue();

            /// Returns the queue of the mailbox that stores `upstream_msg` messages.
            upstream_queue &get_upstream_queue();

            /// Returns the queue of the mailbox that stores `downstream_msg` messages.
            downstream_queue &get_downstream_queue();

            /// Returns the queue of the mailbox that stores high priority messages.
            urgent_queue &get_urgent_queue();

            // -- inbound_path management ------------------------------------------------

            /// Creates a new path for incoming stream traffic from `sender`.
            virtual inbound_path *make_inbound_path(stream_manager_ptr mgr, stream_slots slots, strong_actor_ptr sender,
                                                    type_id_t rtti);

            /// Silently closes incoming stream traffic on `slot`.
            virtual void erase_inbound_path_later(stream_slot slot);

            /// Closes incoming stream traffic on `slot`. Emits a drop message on the
            /// path if `reason == none` and a `forced_drop` message otherwise.
            virtual void erase_inbound_path_later(stream_slot slot, error reason);

            /// Silently closes all inbound paths for `mgr`.
            virtual void erase_inbound_paths_later(const stream_manager *mgr);

            /// Closes all incoming stream traffic for a manager. Emits a drop message on
            /// each path if `reason == none` and a `forced_drop` message on each path
            /// otherwise.
            virtual void erase_inbound_paths_later(const stream_manager *mgr, error reason);

            // -- handling of stream messages --------------------------------------------

            void handle_upstream_msg(stream_slots slots, actor_addr &sender, upstream_msg::ack_open &x);

            template<class T>
            void handle_upstream_msg(stream_slots slots, actor_addr &sender, T &x) {
                ACTOR_LOG_TRACE(ACTOR_ARG(slots) << ACTOR_ARG(sender) << ACTOR_ARG(x));
                ACTOR_IGNORE_UNUSED(sender);
                auto i = stream_managers_.find(slots.receiver);
                if (i == stream_managers_.end()) {
                    auto j = pending_stream_managers_.find(slots.receiver);
                    if (j != pending_stream_managers_.end()) {
                        j->second->stop(sec::stream_init_failed);
                        pending_stream_managers_.erase(j);
                        return;
                    }
                    ACTOR_LOG_INFO("no manager found:" << ACTOR_ARG(slots));
                    // TODO: replace with `if constexpr` when switching to C++17
                    if (std::is_same<T, upstream_msg::ack_batch>::value) {
                        // Make sure the other actor does not falsely believe us a source.
                        inbound_path::emit_irregular_shutdown(this, slots, current_sender(), sec::invalid_upstream);
                    }
                    return;
                }
                ACTOR_ASSERT(i->second != nullptr);
                auto ptr = i->second;
                ptr->handle(slots, x);
                if (ptr->done()) {
                    ACTOR_LOG_DEBUG("done sending:" << ACTOR_ARG(slots));
                    ptr->stop();
                    erase_stream_manager(ptr);
                } else if (ptr->out().path(slots.receiver) == nullptr) {
                    ACTOR_LOG_DEBUG("done sending on path:" << ACTOR_ARG(slots.receiver));
                    erase_stream_manager(slots.receiver);
                }
            }

            /// @cond PRIVATE

            // -- utility functions for invoking default handler -------------------------

            /// Utility function that swaps `f` into a temporary before calling it
            /// and restoring `f` only if it has not been replaced by the user.
            template<class F, class... Ts>
            auto call_handler(F &f, Ts &&... xs) ->
                typename std::enable_if<!std::is_same<decltype(f(std::forward<Ts>(xs)...)), void>::value,
                                        decltype(f(std::forward<Ts>(xs)...))>::type {
                using std::swap;
                F g;
                swap(f, g);
                auto res = g(std::forward<Ts>(xs)...);
                if (!f)
                    swap(g, f);
                return res;
            }

            template<class F, class... Ts>
            auto call_handler(F &f, Ts &&... xs) ->
                typename std::enable_if<std::is_same<decltype(f(std::forward<Ts>(xs)...)), void>::value>::type {
                using std::swap;
                F g;
                swap(f, g);
                g(std::forward<Ts>(xs)...);
                if (!f)
                    swap(g, f);
            }

            void call_error_handler(error &err) {
                call_handler(error_handler_, this, err);
            }

            // -- timeout management -----------------------------------------------------

            /// Requests a new timeout and returns its ID.
            uint64_t set_timeout(std::string type, actor_clock::time_point x);

            // -- stream processing ------------------------------------------------------

            /// Returns a currently unused slot.
            stream_slot next_slot();

            /// Assigns slot `x` to `mgr`, i.e., adds a new entry to `stream_managers_`.
            void assign_slot(stream_slot x, stream_manager_ptr mgr);

            /// Assigns slot `x` to the pending manager `mgr`, i.e., adds a new entry to
            /// `pending_stream_managers_`.
            void assign_pending_slot(stream_slot x, stream_manager_ptr mgr);

            /// Convenience function for calling `assign_slot(next_slot(), mgr)`.
            stream_slot assign_next_slot_to(stream_manager_ptr mgr);

            /// Convenience function for calling `assign_slot(next_slot(), mgr)`.
            stream_slot assign_next_pending_slot_to(stream_manager_ptr mgr);

            /// Adds a new stream manager to the actor and starts cycle management if
            /// needed.
            bool add_stream_manager(stream_slot id, stream_manager_ptr ptr);

            /// Removes the stream manager mapped to `id` in `O(log n)`.
            void erase_stream_manager(stream_slot id);

            /// Removes the stream manager mapped to `id` in `O(log n)`.
            void erase_pending_stream_manager(stream_slot id);

            /// Removes all entries for `mgr` in `O(n)`.
            void erase_stream_manager(const stream_manager_ptr &mgr);

            /// Processes a stream handshake.
            /// @pre `x.content().match_elements<open_stream_msg>()`
            invoke_message_result handle_open_stream_msg(mailbox_element &x);

            /// Advances credit and batch timeouts and returns the timestamp when to call
            /// this function again.
            actor_clock::time_point advance_streams(actor_clock::time_point now);

            // -- properties -------------------------------------------------------------

            /// Returns `true` if the actor has a behavior, awaits responses, or
            /// participates in streams.
            /// @private
            inline bool alive() const noexcept {
                return !bhvr_stack_.empty() || !awaited_responses_.empty() || !multiplexed_responses_.empty() ||
                       !stream_managers_.empty() || !pending_stream_managers_.empty();
            }

            /// @endcond

        protected:
            // -- member variables -------------------------------------------------------

            /// Stores incoming messages.
            mailbox_type mailbox_;

            /// Stores user-defined callbacks for message handling.
            detail::behavior_stack bhvr_stack_;

            /// Identifies the timeout messages we are currently waiting for.
            uint64_t timeout_id_;

            /// Stores callbacks for awaited responses.
            std::forward_list<pending_response> awaited_responses_;

            /// Stores callbacks for multiplexed responses.
            detail::unordered_flat_map<message_id, behavior> multiplexed_responses_;

            /// Customization point for setting a default `message` callback.
            default_handler default_handler_;

            /// Customization point for setting a default `error` callback.
            error_handler error_handler_;

            /// Customization point for setting a default `down_msg` callback.
            down_handler down_handler_;

            /// Customization point for setting a default `down_msg` callback.
            node_down_handler node_down_handler_;

            /// Customization point for setting a default `exit_msg` callback.
            exit_handler exit_handler_;

            /// Stores stream managers for established streams.
            stream_manager_map stream_managers_;

            /// Stores stream managers for pending streams, i.e., streams that have not
            /// yet received an ACK.
            stream_manager_map pending_stream_managers_;

            /// Controls batch and credit timeouts.
            detail::tick_emitter stream_ticks_;

            /// Number of ticks per batch delay.
            size_t max_batch_delay_ticks_;

            /// Number of ticks of each credit round.
            size_t credit_round_ticks_;

            /// Pointer to a private thread object associated with a detached actor.
            detail::private_thread *private_thread_;

#ifndef ACTOR_NO_EXCEPTIONS
            /// Customization point for setting a default exception callback.
            exception_handler exception_handler_;
#endif    // ACTOR_NO_EXCEPTIONS

            /// @endcond
        };

    }    // namespace actor
}    // namespace nil
