//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2017-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/detail/thread_safe_actor_clock.hpp>

#include <nil/actor/actor_control_block.hpp>
#include <nil/actor/logger.hpp>
#include <nil/actor/sec.hpp>
#include <nil/actor/system_messages.hpp>

namespace nil::actor::detail {

    void thread_safe_actor_clock::set_ordinary_timeout(time_point t,
                                                       abstract_actor *self,
                                                       std::string type,
                                                       uint64_t id) {
        push(new ordinary_timeout(t, self->ctrl(), type, id));
    }

    void thread_safe_actor_clock::set_request_timeout(time_point t, abstract_actor *self, message_id id) {
        push(new request_timeout(t, self->ctrl(), id));
    }

    void thread_safe_actor_clock::set_multi_timeout(time_point t, abstract_actor *self, std::string type, uint64_t id) {
        push(new multi_timeout(t, self->ctrl(), type, id));
    }

    void thread_safe_actor_clock::cancel_ordinary_timeout(abstract_actor *self, std::string type) {
        push(new ordinary_timeout_cancellation(self->id(), type));
    }

    void thread_safe_actor_clock::cancel_request_timeout(abstract_actor *self, message_id id) {
        push(new request_timeout_cancellation(self->id(), id));
    }

    void thread_safe_actor_clock::cancel_timeouts(abstract_actor *self) {
        push(new timeouts_cancellation(self->id()));
    }

    void thread_safe_actor_clock::schedule_message(time_point t,
                                                   strong_actor_ptr receiver,
                                                   mailbox_element_ptr content) {
        push(new actor_msg(t, std::move(receiver), std::move(content)));
    }

    void thread_safe_actor_clock::schedule_message(time_point t, group target, strong_actor_ptr sender,
                                                   message content) {
        auto ptr = new group_msg(t, std::move(target), std::move(sender), std::move(content));
        push(ptr);
    }

    void thread_safe_actor_clock::cancel_all() {
        push(new drop_all);
    }

    void thread_safe_actor_clock::run_dispatch_loop() {
        for (;;) {
            // Wait until queue is non-empty.
            if (schedule_.empty()) {
                queue_.wait_nonempty();
            } else {
                auto t = schedule_.begin()->second->due;
                if (!queue_.wait_nonempty(t)) {
                    // Handle timeout by shipping timed-out events and starting anew.
                    trigger_expired_timeouts();
                    continue;
                }
            }
            // Push all elements from the queue to the events buffer.
            auto i = events_.begin();
            auto e = queue_.get_all(i);
            for (; i != e; ++i) {
                auto &x = *i;
                ACTOR_ASSERT(x != nullptr);
                switch (x->subtype) {
                    case ordinary_timeout_cancellation_type: {
                        handle(static_cast<ordinary_timeout_cancellation &>(*x));
                        break;
                    }
                    case request_timeout_cancellation_type: {
                        handle(static_cast<request_timeout_cancellation &>(*x));
                        break;
                    }
                    case timeouts_cancellation_type: {
                        handle(static_cast<timeouts_cancellation &>(*x));
                        break;
                    }
                    case drop_all_type: {
                        schedule_.clear();
                        actor_lookup_.clear();
                        break;
                    }
                    case shutdown_type: {
                        schedule_.clear();
                        actor_lookup_.clear();
                        // Call it a day.
                        return;
                    }
                    case ordinary_timeout_type: {
                        auto dptr = static_cast<ordinary_timeout *>(x.release());
                        add_schedule_entry(std::unique_ptr<ordinary_timeout> {dptr});
                        break;
                    }
                    case multi_timeout_type: {
                        auto dptr = static_cast<multi_timeout *>(x.release());
                        add_schedule_entry(std::unique_ptr<multi_timeout> {dptr});
                        break;
                    }
                    case request_timeout_type: {
                        auto dptr = static_cast<request_timeout *>(x.release());
                        add_schedule_entry(std::unique_ptr<request_timeout> {dptr});
                        break;
                    }
                    case actor_msg_type: {
                        auto dptr = static_cast<actor_msg *>(x.release());
                        add_schedule_entry(std::unique_ptr<actor_msg> {dptr});
                        break;
                    }
                    case group_msg_type: {
                        auto dptr = static_cast<group_msg *>(x.release());
                        add_schedule_entry(std::unique_ptr<group_msg> {dptr});
                        break;
                    }
                    default: {
                        ACTOR_LOG_ERROR("unexpected event type");
                        break;
                    }
                }
                x.reset();
            }
        }
    }

    void thread_safe_actor_clock::cancel_dispatch_loop() {
        push(new shutdown);
    }

    void thread_safe_actor_clock::push(event *ptr) {
        queue_.push_back(unique_event_ptr {ptr});
    }

}    // namespace nil::actor::detail
