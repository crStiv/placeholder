//---------------------------------------------------------------------------//
// Copyright (c) 2019 Nil Foundation AG
// Copyright (c) 2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_KDF_SP800_56A_HPP
#define CRYPTO3_KDF_SP800_56A_HPP

#include <nil/crypto3/detail/type_traits.hpp>

#include <nil/crypto3/mac/hmac.hpp>

#include <nil/crypto3/kdf/detail/sp800_56a/sp800_56a_policy.hpp>

namespace nil {
    namespace crypto3 {
        namespace kdf {
            /*!
             * @brief KDF defined in NIST SP 800-56a revision 2 (Single-step key-derivation function)
             * @tparam Construction
             */
            template<typename Construction, typename = void>
            class sp800_56a {};

            /*!
             * @brief Hash version of SP 800-56a KDF.
             * @tparam Hash
             */
            template<typename Hash>
            class sp800_56a<Hash, typename std::enable_if<is_hash<Hash>::value>::type> {
            public:
                typedef Hash hash_type;
            };

            /*!
             * @brief MAC version of SP 800-56a KDF.
             * @tparam MessageAuthenticationCode
             */
            template<typename MessageAuthenticationCode>
            class sp800_56a<MessageAuthenticationCode,
                            typename std::enable_if<is_mac<MessageAuthenticationCode>::value>::type> {
                typedef detail::sp800_56a_policy<MessageAuthenticationCode> policy_type;

            public:
                typedef typename policy_type::hash_type hash_type;
                typedef typename policy_type::mac_type mac_type;

                constexpr static const std::size_t min_key_bits = policy_type::min_key_bits;
                constexpr static const std::size_t max_key_bits = policy_type::max_key_bits;
                typedef typename policy_type::key_type key_type;

                static void process(const key_type &key) {
                    mac_type mac(key);

                    const uint64_t kRepsUpperBound = (1ULL << 32U);

                    const size_t digest_len = auxfunc.output_length();

                    const size_t reps = key.size() / digest_len + ((key.size() % digest_len) ? 1 : 0);

                    if (reps >= kRepsUpperBound) {
                        // See SP-800-56A, point 5.8.1
                        throw std::invalid_argument("SP800-56A KDF requested output too large");
                    }

                    uint32_t counter = 1;
                    secure_vector<uint8_t> result;
                    for (size_t i = 0; i < reps; i++) {
                        auxfunc.update_be(counter++);
                        auxfunc.update(secret, secret_len);
                        auxfunc.update(label, label_len);
                        auxfunc.final(result);

                        const size_t offset = digest_len * i;
                        const size_t len = std::min(result.size(), key_len - offset);
                        copy_mem(&key[offset], result.data(), len);
                    }

                    return key_len;
                }
            };

            /*!
             * @brief Strictly standard-compliant SP 800-56a version
             * @tparam Hash
             */
            template<typename Hash>
            class sp800_56a<mac::hmac<Hash>, typename std::enable_if<is_mac<mac::hmac<Hash>>::value>::type> {
                typedef detail::sp800_56a_policy<mac::hmac<Hash>> policy_type;

            public:
                typedef typename policy_type::hash_type hash_type;
                typedef typename policy_type::mac_type mac_type;

                constexpr static const std::size_t min_key_bits = policy_type::min_key_bits;
                constexpr static const std::size_t max_key_bits = policy_type::max_key_bits;
                typedef typename policy_type::key_type key_type;

                static void process(const key_type &key) {
                    mac_type mac(key);

                    const uint64_t kRepsUpperBound = (1ULL << 32U);

                    const size_t digest_len = auxfunc.output_length();

                    const size_t reps = key.size() / digest_len + ((key.size() % digest_len) ? 1 : 0);

                    if (reps >= kRepsUpperBound) {
                        // See SP-800-56A, point 5.8.1
                        throw std::invalid_argument("SP800-56A KDF requested output too large");
                    }

                    uint32_t counter = 1;
                    secure_vector<uint8_t> result;
                    for (size_t i = 0; i < reps; i++) {
                        auxfunc.update_be(counter++);
                        auxfunc.update(secret, secret_len);
                        auxfunc.update(label, label_len);
                        auxfunc.final(result);

                        const size_t offset = digest_len * i;
                        const size_t len = std::min(result.size(), key_len - offset);
                        copy_mem(&key[offset], result.data(), len);
                    }

                    return key_len;
                }
            };
        }    // namespace kdf
    }        // namespace crypto3
}    // namespace nil

#endif
