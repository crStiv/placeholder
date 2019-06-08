//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nilfoundation.org>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_HASH_STATE_HPP
#define CRYPTO3_HASH_STATE_HPP

#include <boost/accumulators/framework/accumulator_set.hpp>

#include <nil/crypto3/hash/accumulators/hash.hpp>

namespace nil {
    namespace crypto3 {
        namespace hash {
            /*!
             * @brief Hash state managing container
             *
             * Meets the requirements of HashStateContainer, ConceptContainer, SequenceContainer, Container
             *
             * @tparam Hasher
             * @tparam Endian
             * @tparam ValueBits
             * @tparam LengthBits
             */
            template<typename ProcessingMode>
            struct hash_accumulator {
                typedef boost::accumulators::accumulator_set<hash::static_digest<ProcessingMode::output_block_bits>,
                                                             boost::accumulators::features<
                                                                     accumulators::tag::hash<ProcessingMode>>> type;

                typedef ProcessingMode mode_type;
            };
        }
    }
} // namespace nil

#endif // CRYPTO3_BLOCK_STREAM_PREPROCESSOR_HPP