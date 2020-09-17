//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//
// @file Declaration of interfaces for the Merkle tree check update gadget.
//
// The gadget checks the following: given two roots R1 and R2, address A, two
// values V1 and V2, and authentication path P, check that
// - P is a valid authentication path for the value V1 as the A-th leaf in a Merkle tree with root R1, and
// - P is a valid authentication path for the value V2 as the A-th leaf in a Merkle tree with root R2.
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_ZK_MERKLE_TREE_CHECK_UPDATE_GADGET_HPP_
#define CRYPTO3_ZK_MERKLE_TREE_CHECK_UPDATE_GADGET_HPP_

#include <nil/crypto3/zk/snark/merkle_tree.hpp>
#include <nil/crypto3/zk/snark/gadget.hpp>
#include <nil/crypto3/zk/snark/gadgets/hashes/crh_gadget.hpp>
#include <nil/crypto3/zk/snark/gadgets/hashes/digest_selector_gadget.hpp>
#include <nil/crypto3/zk/snark/gadgets/hashes/hash_io.hpp>
#include <nil/crypto3/zk/snark/gadgets/merkle_tree/merkle_authentication_path_variable.hpp>

namespace nil {
    namespace crypto3 {
        namespace zk {
            namespace snark {

                template<typename FieldType, typename Hash>
                class merkle_tree_check_update_gadget : public gadget<FieldType> {
                private:
                    std::vector<Hash> prev_hashers;
                    std::vector<block_variable<FieldType>> prev_hasher_inputs;
                    std::vector<digest_selector_gadget<FieldType>> prev_propagators;
                    std::vector<digest_variable<FieldType>> prev_internal_output;

                    std::vector<Hash> next_hashers;
                    std::vector<block_variable<FieldType>> next_hasher_inputs;
                    std::vector<digest_selector_gadget<FieldType>> next_propagators;
                    std::vector<digest_variable<FieldType>> next_internal_output;

                    std::shared_ptr<digest_variable<FieldType>> computed_next_root;
                    std::shared_ptr<bit_vector_copy_gadget<FieldType>> check_next_root;

                public:
                    const std::size_t digest_size;
                    const std::size_t tree_depth;

                    pb_variable_array<FieldType> address_bits;
                    digest_variable<FieldType> prev_leaf_digest;
                    digest_variable<FieldType> prev_root_digest;
                    merkle_authentication_path_variable<FieldType, Hash> prev_path;
                    digest_variable<FieldType> next_leaf_digest;
                    digest_variable<FieldType> next_root_digest;
                    merkle_authentication_path_variable<FieldType, Hash> next_path;
                    pb_linear_combination<FieldType> update_successful;

                    /* Note that while it is necessary to generate R1CS constraints
                       for prev_path, it is not necessary to do so for next_path. See
                       comment in the implementation of generate_r1cs_constraints() */

                    merkle_tree_check_update_gadget(
                        protoboard<FieldType> &pb,
                        const std::size_t tree_depth,
                        const pb_variable_array<FieldType> &address_bits,
                        const digest_variable<FieldType> &prev_leaf_digest,
                        const digest_variable<FieldType> &prev_root_digest,
                        const merkle_authentication_path_variable<FieldType, Hash> &prev_path,
                        const digest_variable<FieldType> &next_leaf_digest,
                        const digest_variable<FieldType> &next_root_digest,
                        const merkle_authentication_path_variable<FieldType, Hash> &next_path,
                        const pb_linear_combination<FieldType> &update_successful);

                    void generate_r1cs_constraints();
                    void generate_r1cs_witness();

                    static std::size_t root_size_in_bits();
                    /* for debugging purposes */
                    static std::size_t expected_constraints(const std::size_t tree_depth);
                };

                template<typename FieldType, typename Hash>
                void test_merkle_tree_check_update_gadget();

                template<typename FieldType, typename Hash>
                merkle_tree_check_update_gadget<FieldType, Hash>::merkle_tree_check_update_gadget(
                    protoboard<FieldType> &pb,
                    const std::size_t tree_depth,
                    const pb_variable_array<FieldType> &address_bits,
                    const digest_variable<FieldType> &prev_leaf_digest,
                    const digest_variable<FieldType> &prev_root_digest,
                    const merkle_authentication_path_variable<FieldType, Hash> &prev_path,
                    const digest_variable<FieldType> &next_leaf_digest,
                    const digest_variable<FieldType> &next_root_digest,
                    const merkle_authentication_path_variable<FieldType, Hash> &next_path,
                    const pb_linear_combination<FieldType> &update_successful) :
                    gadget<FieldType>(pb),
                    digest_size(Hash::get_digest_len()), tree_depth(tree_depth), address_bits(address_bits),
                    prev_leaf_digest(prev_leaf_digest), prev_root_digest(prev_root_digest), prev_path(prev_path),
                    next_leaf_digest(next_leaf_digest), next_root_digest(next_root_digest), next_path(next_path),
                    update_successful(update_successful) {
                    assert(tree_depth > 0);
                    assert(tree_depth == address_bits.size());

                    for (std::size_t i = 0; i < tree_depth - 1; ++i) {
                        prev_internal_output.emplace_back(digest_variable<FieldType>(pb, digest_size));
                        next_internal_output.emplace_back(digest_variable<FieldType>(pb, digest_size));
                    }

                    computed_next_root.reset(new digest_variable<FieldType>(pb, digest_size));

                    for (std::size_t i = 0; i < tree_depth; ++i) {
                        block_variable<FieldType> prev_inp(pb, prev_path.left_digests[i], prev_path.right_digests[i]);
                        prev_hasher_inputs.emplace_back(prev_inp);
                        prev_hashers.emplace_back(Hash(pb, 2 * digest_size, prev_inp,
                                                        (i == 0 ? prev_root_digest : prev_internal_output[i - 1])));

                        block_variable<FieldType> next_inp(pb, next_path.left_digests[i], next_path.right_digests[i]);
                        next_hasher_inputs.emplace_back(next_inp);
                        next_hashers.emplace_back(Hash(pb, 2 * digest_size, next_inp,
                                                        (i == 0 ? *computed_next_root : next_internal_output[i - 1])));
                    }

                    for (std::size_t i = 0; i < tree_depth; ++i) {
                        prev_propagators.emplace_back(digest_selector_gadget<FieldType>(
                            pb, digest_size, i < tree_depth - 1 ? prev_internal_output[i] : prev_leaf_digest,
                            address_bits[tree_depth - 1 - i], prev_path.left_digests[i], prev_path.right_digests[i]));
                        next_propagators.emplace_back(digest_selector_gadget<FieldType>(
                            pb, digest_size, i < tree_depth - 1 ? next_internal_output[i] : next_leaf_digest,
                            address_bits[tree_depth - 1 - i], next_path.left_digests[i], next_path.right_digests[i]));
                    }

                    check_next_root.reset(new bit_vector_copy_gadget<FieldType>(
                        pb, computed_next_root->bits, next_root_digest.bits, update_successful, FieldType::capacity()));
                }

                template<typename FieldType, typename Hash>
                void merkle_tree_check_update_gadget<FieldType, Hash>::generate_r1cs_constraints() {
                    /* ensure correct hash computations */
                    for (std::size_t i = 0; i < tree_depth; ++i) {
                        prev_hashers[i].generate_r1cs_constraints(
                            false);    // we check root outside and prev_left/prev_right above
                        next_hashers[i].generate_r1cs_constraints(true);    // however we must check right side hashes
                    }

                    /* ensure consistency of internal_left/internal_right with internal_output */
                    for (std::size_t i = 0; i < tree_depth; ++i) {
                        prev_propagators[i].generate_r1cs_constraints();
                        next_propagators[i].generate_r1cs_constraints();
                    }

                    /* ensure that prev auxiliary input and next auxiliary input match */
                    for (std::size_t i = 0; i < tree_depth; ++i) {
                        for (std::size_t j = 0; j < digest_size; ++j) {
                            /*
                              addr * (prev_left - next_left) + (1 - addr) * (prev_right - next_right) = 0
                              addr * (prev_left - next_left - prev_right + next_right) = next_right - prev_right
                            */
                            this->pb.add_r1cs_constraint(r1cs_constraint<FieldType>(
                                address_bits[tree_depth - 1 - i],
                                prev_path.left_digests[i].bits[j] - next_path.left_digests[i].bits[j] -
                                    prev_path.right_digests[i].bits[j] + next_path.right_digests[i].bits[j],
                                next_path.right_digests[i].bits[j] - prev_path.right_digests[i].bits[j]));
                        }
                    }

                    /* Note that while it is necessary to generate R1CS constraints
                       for prev_path, it is not necessary to do so for next_path.

                       This holds, because { next_path.left_inputs[i],
                       next_path.right_inputs[i] } is a pair { hash_output,
                       auxiliary_input }. The bitness for hash_output is enforced
                       above by next_hashers[i].generate_r1cs_constraints.

                       Because auxiliary input is the same for prev_path and next_path
                       (enforced above), we have that auxiliary_input part is also
                       constrained to be boolean, because prev_path is *all*
                       constrained to be all boolean. */

                    check_next_root->generate_r1cs_constraints(false, false);
                }

                template<typename FieldType, typename Hash>
                void merkle_tree_check_update_gadget<FieldType, Hash>::generate_r1cs_witness() {
                    /* do the hash computations bottom-up */
                    for (int i = tree_depth - 1; i >= 0; --i) {
                        /* ensure consistency of prev_path and next_path */
                        if (this->pb.val(address_bits[tree_depth - 1 - i]) == FieldType::one()) {
                            next_path.left_digests[i].generate_r1cs_witness(prev_path.left_digests[i].get_digest());
                        } else {
                            next_path.right_digests[i].generate_r1cs_witness(prev_path.right_digests[i].get_digest());
                        }

                        /* propagate previous input */
                        prev_propagators[i].generate_r1cs_witness();
                        next_propagators[i].generate_r1cs_witness();

                        /* compute hash */
                        prev_hashers[i].generate_r1cs_witness();
                        next_hashers[i].generate_r1cs_witness();
                    }

                    check_next_root->generate_r1cs_witness();
                }

                template<typename FieldType, typename Hash>
                std::size_t merkle_tree_check_update_gadget<FieldType, Hash>::root_size_in_bits() {
                    return Hash::get_digest_len();
                }

                template<typename FieldType, typename Hash>
                std::size_t
                    merkle_tree_check_update_gadget<FieldType, Hash>::expected_constraints(const std::size_t tree_depth) {
                    /* NB: this includes path constraints */
                    const std::size_t prev_hasher_constraints = tree_depth * Hash::expected_constraints(false);
                    const std::size_t next_hasher_constraints = tree_depth * Hash::expected_constraints(true);
                    const std::size_t prev_authentication_path_constraints = 2 * tree_depth * Hash::get_digest_len();
                    const std::size_t prev_propagator_constraints = tree_depth * Hash::get_digest_len();
                    const std::size_t next_propagator_constraints = tree_depth * Hash::get_digest_len();
                    const std::size_t check_next_root_constraints =
                        3 * (Hash::get_digest_len() + (FieldType::capacity()) - 1) / FieldType::capacity();
                    const std::size_t aux_equality_constraints = tree_depth * Hash::get_digest_len();

                    return (prev_hasher_constraints + next_hasher_constraints + prev_authentication_path_constraints +
                            prev_propagator_constraints + next_propagator_constraints + check_next_root_constraints +
                            aux_equality_constraints);
                }

                template<typename FieldType, typename Hash>
                void test_merkle_tree_check_update_gadget() {
                    /* prepare test */
                    const std::size_t digest_len = Hash::get_digest_len();

                    const std::size_t tree_depth = 16;
                    std::vector<merkle_authentication_node> prev_path(tree_depth);

                    std::vector<bool> prev_load_hash(digest_len);
                    std::generate(prev_load_hash.begin(), prev_load_hash.end(), [&]() { return std::rand() % 2; });
                    std::vector<bool> prev_store_hash(digest_len);
                    std::generate(prev_store_hash.begin(), prev_store_hash.end(), [&]() { return std::rand() % 2; });

                    std::vector<bool> loaded_leaf = prev_load_hash;
                    std::vector<bool> stored_leaf = prev_store_hash;

                    std::vector<bool> address_bits;

                    std::size_t address = 0;
                    for (long level = tree_depth - 1; level >= 0; --level) {
                        const bool computed_is_right = (std::rand() % 2);
                        address |= (computed_is_right ? 1ul << (tree_depth - 1 - level) : 0);
                        address_bits.push_back(computed_is_right);
                        std::vector<bool> other(digest_len);
                        std::generate(other.begin(), other.end(), [&]() { return std::rand() % 2; });

                        std::vector<bool> load_block = prev_load_hash;
                        load_block.insert(computed_is_right ? load_block.begin() : load_block.end(), other.begin(),
                                          other.end());
                        std::vector<bool> store_block = prev_store_hash;
                        store_block.insert(computed_is_right ? store_block.begin() : store_block.end(), other.begin(),
                                           other.end());

                        std::vector<bool> load_h = Hash::get_hash(load_block);
                        std::vector<bool> store_h = Hash::get_hash(store_block);

                        prev_path[level] = other;

                        prev_load_hash = load_h;
                        prev_store_hash = store_h;
                    }

                    std::vector<bool> load_root = prev_load_hash;
                    std::vector<bool> store_root = prev_store_hash;

                    /* execute the test */
                    protoboard<FieldType> pb;
                    pb_variable_array<FieldType> address_bits_va;
                    address_bits_va.allocate(pb, tree_depth);
                    digest_variable<FieldType> prev_leaf_digest(pb, digest_len);
                    digest_variable<FieldType> prev_root_digest(pb, digest_len);
                    merkle_authentication_path_variable<FieldType, Hash> prev_path_var(pb, tree_depth);
                    digest_variable<FieldType> next_leaf_digest(pb, digest_len);
                    digest_variable<FieldType> next_root_digest(pb, digest_len);
                    merkle_authentication_path_variable<FieldType, Hash> next_path_var(pb, tree_depth);
                    merkle_tree_check_update_gadget<FieldType, Hash> mls(
                        pb, tree_depth, address_bits_va, prev_leaf_digest, prev_root_digest, prev_path_var,
                        next_leaf_digest, next_root_digest, next_path_var, pb_variable<FieldType>(0));

                    prev_path_var.generate_r1cs_constraints();
                    mls.generate_r1cs_constraints();

                    address_bits_va.fill_with_bits(pb, address_bits);
                    assert(address_bits_va.get_field_element_from_bits(pb).as_ulong() == address);
                    prev_leaf_digest.generate_r1cs_witness(loaded_leaf);
                    prev_path_var.generate_r1cs_witness(address, prev_path);
                    next_leaf_digest.generate_r1cs_witness(stored_leaf);
                    address_bits_va.fill_with_bits(pb, address_bits);
                    mls.generate_r1cs_witness();

                    /* make sure that update check will check for the right things */
                    prev_leaf_digest.generate_r1cs_witness(loaded_leaf);
                    next_leaf_digest.generate_r1cs_witness(stored_leaf);
                    prev_root_digest.generate_r1cs_witness(load_root);
                    next_root_digest.generate_r1cs_witness(store_root);
                    address_bits_va.fill_with_bits(pb, address_bits);
                    assert(pb.is_satisfied());

                    const std::size_t num_constraints = pb.num_constraints();
                    const std::size_t expected_constraints =
                        merkle_tree_check_update_gadget<FieldType, Hash>::expected_constraints(tree_depth);
                    assert(num_constraints == expected_constraints);
                }

            }    // namespace snark
        }        // namespace zk
    }            // namespace crypto3
}    // namespace nil

#endif    // MERKLE_TREE_CHECK_UPDATE_GADGET_HPP_
