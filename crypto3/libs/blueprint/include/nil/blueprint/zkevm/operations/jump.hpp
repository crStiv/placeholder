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

#include <numeric>
#include <algorithm>

#include <nil/blueprint/zkevm/zkevm_word.hpp>
#include <nil/blueprint/zkevm/zkevm_operation.hpp>

namespace nil {
    namespace blueprint {
        template<typename BlueprintFieldType>
        class zkevm_operation;

        template<typename BlueprintFieldType>
        class zkevm_table;

        template<typename BlueprintFieldType>
        class zkevm_jump_operation : public zkevm_operation<BlueprintFieldType> {
        public:
            using op_type = zkevm_operation<BlueprintFieldType>;
            using gate_class = typename op_type::gate_class;
            using constraint_type = typename op_type::constraint_type;
            using lookup_constraint_type = crypto3::zk::snark::plonk_lookup_constraint<BlueprintFieldType>;
            using zkevm_circuit_type = typename op_type::zkevm_circuit_type;
            using zkevm_table_type = typename op_type::zkevm_table_type;
            using assignment_type = typename op_type::assignment_type;
            using value_type = typename BlueprintFieldType::value_type;
            using var = typename op_type::var;
            using state_var = state_variable<BlueprintFieldType>;

            zkevm_jump_operation() {
                this->stack_input = 1;
                this->stack_output = 0;
                this->gas_cost = 8;
            }

            std::map<gate_class, std::pair<
                std::vector<std::pair<std::size_t, constraint_type>>,
                std::vector<std::pair<std::size_t, lookup_constraint_type>>
            >> generate_gates(zkevm_circuit_type &zkevm_circuit) override {
                std::vector<std::pair<std::size_t, lookup_constraint_type>> lookup_constraints;
                const std::vector<std::size_t> &witness_cols = zkevm_circuit.get_opcode_cols();
                var destination = zkevm_operation<BlueprintFieldType>::var_gen(witness_cols, 0, 0);
                const auto &state = zkevm_circuit.get_state();

                std::size_t position = 0;
                lookup_constraints.push_back({ position, {zkevm_circuit.get_bytecode_table_id(), {
                    destination - destination + 1, // Should be a constant
                    destination,
                    destination - destination + zkevm_circuit.get_opcodes_info().get_opcode_value(zkevm_opcode::JUMPDEST), // Should be a constant
                    destination - destination + 1, // Should be a constant
                    state.bytecode_hash_hi(),
                    state.bytecode_hash_lo()
                }}});

                // TODO:
                // Lookup to RW stack
                return {{gate_class::MIDDLE_OP, {{}, lookup_constraints}}};
            }

            void generate_assignments(zkevm_table_type &zkevm_table, const zkevm_machine_interface &machine) override {
                std::cout << "Generate assignments for JUMP" << std::endl;
                assignment_type &assignment = zkevm_table.get_assignment();
                const std::size_t curr_row = zkevm_table.get_current_row();
                auto witness_cols = zkevm_table.get_opcode_cols();

                zkevm_word_type dest = machine.stack_top();
                std::cout << "JUMP assign destination = " << dest << std::endl;
                assignment.witness(witness_cols[0], curr_row) = w_lo<BlueprintFieldType>(dest);
            }

            virtual constraint_type pc_transition(const zkevm_circuit_type &zkevm_circuit) override{
                auto witness_cols = zkevm_circuit.get_opcode_cols();
                const auto &state = zkevm_circuit.get_state();
                constraint_type c;

                c = state.pc.next() - var(witness_cols[0], -1);
                return c;
            }

            std::size_t rows_amount() override {
                return 1;
            }
        };

        template<typename BlueprintFieldType>
        class zkevm_jumpdest_operation : public zkevm_operation<BlueprintFieldType> {
        public:
            using op_type = zkevm_operation<BlueprintFieldType>;
            using gate_class = typename op_type::gate_class;
            using constraint_type = typename op_type::constraint_type;
            using lookup_constraint_type = crypto3::zk::snark::plonk_lookup_constraint<BlueprintFieldType>;
            using zkevm_circuit_type = typename op_type::zkevm_circuit_type;
            using zkevm_table_type = typename op_type::zkevm_table_type;
            using assignment_type = typename op_type::assignment_type;
            using value_type = typename BlueprintFieldType::value_type;
            using var = typename op_type::var;

            zkevm_jumpdest_operation() {
                this->gas_cost = 1;
            }

            std::map<gate_class, std::pair<
                std::vector<std::pair<std::size_t, constraint_type>>,
                std::vector<std::pair<std::size_t, lookup_constraint_type>>
                >>
                generate_gates(zkevm_circuit_type &zkevm_circuit) override {
                // No gates. Just state transition
                return {{gate_class::MIDDLE_OP, {{}, {}}}};
            }

            void generate_assignments(zkevm_table_type &zkevm_table, const zkevm_machine_interface &machine) override {
                // No assignments. Just state transition
            }

            std::size_t rows_amount() override {
                return 1;
            }
        };
    }   // namespace blueprint
}   // namespace nil
