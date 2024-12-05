//---------------------------------------------------------------------------//
// Copyright (c) 2024 Elena Tatuzova <e.tatuzova@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#pragma once

#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>
#include <nil/crypto3/algebra/curves/vesta.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/vesta.hpp>
#include <nil/crypto3/algebra/random_element.hpp>

#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/crypto3/hash/keccak.hpp>

#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>

#include <nil/blueprint/blueprint/plonk/circuit.hpp>
#include <nil/blueprint/blueprint/plonk/assignment.hpp>

#include <nil/blueprint/zkevm_bbf/input_generators/opcode_tester.hpp>
#include <nil/blueprint/zkevm_bbf/input_generators/opcode_tester_input_generator.hpp>

#include <nil/crypto3/zk/snark/systems/plonk/placeholder/prover.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/verifier.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/preprocessor.hpp>

#include "../test_l1_wrapper.hpp"

using namespace nil::crypto3;
using namespace nil::blueprint;
using namespace nil::blueprint::bbf;

class zkEVMOpcodeTestFixture: public BBFTestFixture {
public:
    zkEVMOpcodeTestFixture():BBFTestFixture(){}

    template<typename BlueprintFieldType>
    void complex_opcode_test(
        const zkevm_opcode_tester                       &opcode_tester,
        const l1_size_restrictions                      &max_sizes
    ){
        nil::blueprint::bbf::zkevm_opcode_tester_input_generator circuit_inputs(opcode_tester);

        using integral_type = typename BlueprintFieldType::integral_type;
        using value_type = typename BlueprintFieldType::value_type;

        integral_type base16 = integral_type(1) << 16;

        std::size_t max_keccak_blocks = max_sizes.max_keccak_blocks;
        std::size_t max_bytecode = max_sizes.max_bytecode;
        std::size_t max_mpt = max_sizes.max_mpt;
        std::size_t max_rw = max_sizes.max_rw;
        std::size_t max_copy = max_sizes.max_copy;
        std::size_t max_zkevm_rows = max_sizes.max_zkevm_rows;

        typename nil::blueprint::bbf::copy<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::ASSIGNMENT>::input_type copy_assignment_input;
        typename nil::blueprint::bbf::copy<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::CONSTRAINTS>::input_type copy_constraint_input;
        copy_assignment_input.rlc_challenge = 7;
        copy_assignment_input.bytecodes = circuit_inputs.bytecodes();
        copy_assignment_input.keccak_buffers = circuit_inputs.keccaks();
        copy_assignment_input.rw_operations = circuit_inputs.rw_operations();
        copy_assignment_input.copy_events = circuit_inputs.copy_events();

        typename nil::blueprint::bbf::zkevm<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::ASSIGNMENT>::input_type zkevm_assignment_input;
        typename nil::blueprint::bbf::zkevm<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::CONSTRAINTS>::input_type zkevm_constraint_input;
        zkevm_assignment_input.rlc_challenge = 7;
        zkevm_assignment_input.bytecodes = circuit_inputs.bytecodes();
        zkevm_assignment_input.keccak_buffers = circuit_inputs.keccaks();
        zkevm_assignment_input.rw_operations = circuit_inputs.rw_operations();
        zkevm_assignment_input.copy_events = circuit_inputs.copy_events();
        zkevm_assignment_input.zkevm_states = circuit_inputs.zkevm_states();
        zkevm_assignment_input.exponentiations = circuit_inputs.exponentiations();

        typename nil::blueprint::bbf::rw<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::ASSIGNMENT>::input_type rw_assignment_input = circuit_inputs.rw_operations();
        typename nil::blueprint::bbf::rw<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::CONSTRAINTS>::input_type rw_constraint_input;

        typename nil::blueprint::bbf::keccak<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::ASSIGNMENT>::input_type keccak_assignment_input;
        typename nil::blueprint::bbf::keccak<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::CONSTRAINTS>::input_type keccak_constraint_input;
        keccak_assignment_input.private_input = 12345;

        typename nil::blueprint::bbf::bytecode<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::ASSIGNMENT>::input_type bytecode_assignment_input;
        typename nil::blueprint::bbf::bytecode<BlueprintFieldType,nil::blueprint::bbf::GenerationStage::CONSTRAINTS>::input_type bytecode_constraint_input;
        bytecode_assignment_input.rlc_challenge = 7;
        bytecode_assignment_input.bytecodes = circuit_inputs.bytecodes();
        bytecode_assignment_input.keccak_buffers = circuit_inputs.keccaks();
        bool result;

        // Max_rows, max_bytecode, max_rw
        result = test_bbf_component<BlueprintFieldType, nil::blueprint::bbf::zkevm>(
            "zkevm",
            {}, zkevm_assignment_input, zkevm_constraint_input,
            max_zkevm_rows,
            max_copy,
            max_rw,
            max_keccak_blocks,
            max_bytecode
        );
        BOOST_CHECK(result);
        std::cout << std::endl;

        // Max_bytecode, max_bytecode
        std::cout << "Bytecode circuit" << std::endl;
        result = test_bbf_component<BlueprintFieldType, nil::blueprint::bbf::bytecode>(
            "bytecode",
            {7}, bytecode_assignment_input, bytecode_constraint_input, max_bytecode, max_keccak_blocks
        );
        BOOST_CHECK(result);
        std::cout << std::endl;

        // Max_rw, Max_mpt
        std::cout << "RW circuit" << std::endl;
        result = test_bbf_component<BlueprintFieldType, nil::blueprint::bbf::rw>(
            "rw",
            {}, rw_assignment_input, rw_constraint_input, max_rw, max_mpt
        );
        BOOST_CHECK(result);
        std::cout << std::endl;

        // Max_copy, Max_rw, Max_keccak, Max_bytecode
        result =test_bbf_component<BlueprintFieldType, nil::blueprint::bbf::copy>(
            "copy",
            {7}, copy_assignment_input, copy_constraint_input,
            max_copy, max_rw, max_keccak_blocks, max_bytecode
        );
        BOOST_CHECK(result);
        std::cout << std::endl;

        // Max_keccak
        result = test_bbf_component<BlueprintFieldType, nil::blueprint::bbf::keccak>(
            "keccak",
            {}, keccak_assignment_input , keccak_constraint_input
        );
        BOOST_CHECK(result);
        std::cout << std::endl;
    }
};