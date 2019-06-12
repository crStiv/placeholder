//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nilfoundation.org>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_BLAKE2B_H_
#define CRYPTO3_BLAKE2B_H_

#include <nil/crypto3/hash/detail/blake2b/blake2b_functions.hpp>

#include <nil/crypto3/hash/detail/haifa_construction.hpp>
#include <nil/crypto3/hash/detail/haifa_state_preprocessor.hpp>

namespace nil {
    namespace crypto3 {
        namespace hash {
            template<std::size_t DigestBits>
            class blake2b_compressor {
            protected:
                typedef detail::blake2b_functions<DigestBits> policy_type;

                typedef typename policy_type::byte_type byte_type;

            public:

                typedef typename policy_type::iv_generator iv_generator;

                constexpr static const std::size_t word_bits = policy_type::word_bits;
                typedef typename policy_type::word_type word_type;

                constexpr static const std::size_t state_bits = policy_type::state_bits;
                constexpr static const std::size_t state_words = policy_type::state_words;
                typedef typename policy_type::state_type state_type;

                constexpr static const std::size_t block_bits = policy_type::block_bits;
                constexpr static const std::size_t block_words = policy_type::block_words;
                typedef typename policy_type::block_type block_type;

                constexpr static const std::size_t salt_bits = policy_type::salt_bits;
                typedef typename policy_type::salt_type salt_type;
                constexpr static const salt_type salt_value = policy_type::salt_value;

                inline void operator()(state_type &state, const block_type &block, typename state_type::value_type seen,
                                       typename state_type::value_type finalizator = 0) {
                    this->process_block(state, block, seen, finalizator);
                }

            protected:
                static inline void process_block(state_type &state, const block_type &block,
                                                 typename state_type::value_type seen,
                                                 typename state_type::value_type finalizator) {
                    state_type v, M = {0};

                    std::copy(state.begin(), state.end(), v.end());
                    std::copy(iv_generator()().begin(), iv_generator()().end(), v.end());

                    std::array<typename state_type::value_type, 2> s = {seen, 0x00};

                    v[12] ^= s[0];
                    v[13] ^= s[1];

                    s = {finalizator, 0};

                    v[14] ^= s[0];
                    v[15] ^= s[1];

                    s.fill(0);

                    policy_type::template round<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15>(v, M);
                    policy_type::template round<14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3>(v, M);
                    policy_type::template round<11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4>(v, M);
                    policy_type::template round<7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8>(v, M);
                    policy_type::template round<9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13>(v, M);
                    policy_type::template round<2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9>(v, M);
                    policy_type::template round<12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11>(v, M);
                    policy_type::template round<13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10>(v, M);
                    policy_type::template round<6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5>(v, M);
                    policy_type::template round<10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0>(v, M);
                    policy_type::template round<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15>(v, M);
                    policy_type::template round<14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3>(v, M);

                    for (size_t i = 0; i < 8; i++) {
                        state[i] ^= v[i] ^ v[i + 8];
                    }
                }
            };

            /*!
             * @brief Blake2b. A recently designed hash function. Very fast on 64-bit processors.
             * Can output a hash of any length between 1 and 64 bytes, this is specified by passing
             * a value to the constructor with the desired length.
             *
             * @ingroup hash
             * @tparam DigestBits
             */
            template<std::size_t DigestBits>
            class blake2b {
                typedef detail::blake2b_policy<DigestBits> policy_type;
            public:
                typedef haifa_construction<stream_endian::little_octet_big_bit, policy_type::digest_bits,
                                           typename policy_type::iv_generator,
                                           blake2b_compressor<DigestBits>> block_hash_type_;
#ifdef CRYPTO3_HASH_NO_HIDE_INTERNAL_TYPES
                typedef block_hash_type_ block_hash_type;
#else
                struct block_hash_type : block_hash_type_ {
                };
#endif
                template<typename StateAccumulator, std::size_t ValueBits>
                struct stream_processor {
                    typedef haifa_state_preprocessor<block_hash_type, StateAccumulator,
                                                     stream_endian::little_octet_big_bit, ValueBits,
                                                     block_hash_type::word_bits> type_;
#ifdef CRYPTO3_HASH_NO_HIDE_INTERNAL_TYPES
                    typedef type_ type;
#else
                    struct type : type_ {
                    };
#endif
                };
                typedef typename block_hash_type::digest_type digest_type;
            };
        }
    }
}

#endif
