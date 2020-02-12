//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2018-2019 Nil Foundation AG
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt for Boost License or
// http://opensource.org/licenses/BSD-3-Clause for BSD 3-Clause License
//---------------------------------------------------------------------------//

#pragma once

#include <atomic>

#include <nil/actor/fwd.hpp>
#include <nil/actor/message_id.hpp>

namespace nil {
    namespace actor {

        /// Interface for all message receivers. * This interface describes an
        /// entity that can receive messages and is implemented by {@link actor}
        /// and {@link group}.
        class abstract_channel {
        public:
            friend class abstract_actor;
            friend class abstract_group;

            virtual ~abstract_channel();

            /// Enqueues a new message without forwarding stack to the channel.
            virtual void enqueue(strong_actor_ptr sender, message_id mid, message content,
                                 execution_unit *host = nullptr) = 0;

            static constexpr int is_abstract_actor_flag = 0x01000000;

            static constexpr int is_abstract_group_flag = 0x02000000;

            static constexpr int is_actor_bind_decorator_flag = 0x04000000;

            static constexpr int is_actor_dot_decorator_flag = 0x08000000;

            static constexpr int is_actor_decorator_mask = 0x0C000000;

            static constexpr int is_hidden_flag = 0x10000000;

            inline bool is_abstract_actor() const {
                return static_cast<bool>(flags() & is_abstract_actor_flag);
            }

            inline bool is_abstract_group() const {
                return static_cast<bool>(flags() & is_abstract_group_flag);
            }

            inline bool is_actor_decorator() const {
                return static_cast<bool>(flags() & is_actor_decorator_mask);
            }

        protected:
            // note: *both* operations use relaxed memory order, this is because
            // only the actor itself is granted write access while all access
            // from other actors or threads is always read-only; further, only
            // flags that are considered constant after an actor has launched are
            // read by others, i.e., there is no acquire/release semantic between
            // setting and reading flags
            inline int flags() const {
                return flags_.load(std::memory_order_relaxed);
            }

            inline void flags(int new_value) {
                flags_.store(new_value, std::memory_order_relaxed);
            }

        private:
            // can only be called from abstract_actor and abstract_group
            abstract_channel(int fs);

            // Accumulates several state and type flags. Subtypes may use only the
            // first 20 bits, i.e., the bitmask 0xFFF00000 is reserved for
            // channel-related flags.
            std::atomic<int> flags_;
        };
    }    // namespace actor
}    // namespace nil
