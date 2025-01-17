//---------------------------------------------------------------------------//
// Copyright (c) 2024 Alexey Yashunsky <a.yashunsky@nil.foundation>
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
        class zkevm_err0_operation : public zkevm_operation<BlueprintFieldType> {
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

            zkevm_err0_operation(){
                this->gas_cost = 0;
            }

            std::map<gate_class, std::pair<
                std::vector<std::pair<std::size_t, constraint_type>>,
                std::vector<std::pair<std::size_t, lookup_constraint_type>>
            >> generate_gates(zkevm_circuit_type &zkevm_circuit) override {
                const auto &state = zkevm_circuit.get_state();
                using circuit_integral_type = typename BlueprintFieldType::integral_type;

                std::vector<std::pair<std::size_t, constraint_type>> constraints;
                std::vector<std::pair<std::size_t, lookup_constraint_type>> lookup_constraints;

                constexpr const std::size_t chunk_amount = 16;
                const std::vector<std::size_t> &witness_cols = zkevm_circuit.get_opcode_cols();
                auto var_gen = [&witness_cols](std::size_t i, int32_t offset = 0) {
                    return zkevm_operation<BlueprintFieldType>::var_gen(witness_cols, i, offset);
                };

                // Table layout                                             Row #
                // +-----+-----+------+------------------+------------------+
                // | d3  | g| b|      All these cells are used by the       | 1
                // +--+--+--+--+------+------------------+------------------+
                // |d1|si|d2|so|      erroneous opcode virtual selector     | 0
                // +--+--+--+--+------+------------------+------------------+
                //
                //    g = insufficient gas
                //   si = not enough values on stack for input
                //   so = stack output exceeds maximum size
                //    b = parity bit for virtual selector
                //
                // stack_read, stack_write, cost: defined by constraints based on virtual selector
                // stack_size, gas: taken from state
                //
                // stack_input + d1 = stack_size + si*2^16   <=>   si = [stack_input > stack_size]
                // stack_size + stack_output + d2 = stack_input + 1024 + so*2^16  <=>  so = [stack_size + stack_output > 1024 + stack_input]
                // cost + d3 = gas + g*2^32   <=>   g = [cost > gas]
                //
                // Ensure there is an error: (si-1)(so-1)(g-1) = 0

                std::size_t position = 0;
                var si_var = var_gen(0, -1),
                    so_var = var_gen(1, -1),
                    opcode_var = var_gen(2, -1),
                    d_var = var_gen(0),
                    b_var   = var_gen(1),
                    real_opcode_var = var_gen(2);

                constraints.push_back({position, si_var * (1 - si_var)});
                constraints.push_back({position, so_var * (1 - so_var)});
                constraints.push_back({0, b_var * (1 - b_var)});
                constraints.push_back({0, si_var + so_var - 1});

                var stack_size_var = zkevm_circuit.get_state().stack_size;

                constraint_type stack_input,
                                stack_output,
                                opcode,
                                real_opcode;

                for(auto [opcode_mnemo, i] : zkevm_circuit.get_opcodes_info().opcode_to_number_map) {
                    if (zkevm_circuit.get_opcodes_info().get_opcode_value(opcode_mnemo) < 256) { // only use true opcodes
                        std::size_t b = i % 2,
                                    n = i / 2;
                        constraint_type opcode_selector = (b ? b_var : 1 - b_var) * var_gen(4 + (n % 44), -1 + (n/44));
                        stack_input += zkevm_circuit.get_opcodes_info().get_opcode_stack_input(opcode_mnemo) * opcode_selector;
                        stack_output += zkevm_circuit.get_opcodes_info().get_opcode_stack_output(opcode_mnemo) * opcode_selector;
                        opcode += opcode_selector * i;
                        real_opcode += zkevm_circuit.get_opcodes_info().get_opcode_value(opcode_mnemo) * opcode_selector;
                    }
                }
                // stack_input + d1 = stack_size + si*2^16
                constraints.push_back({position, si_var * (d_var - (stack_input - stack_size_var))});
                constraints.push_back({position, so_var * (d_var - (stack_size_var + stack_output - stack_input - 1024))});
                constraints.push_back({position, opcode  - opcode_var});
                constraints.push_back({position, real_opcode  - real_opcode_var});

                lookup_constraints.push_back({ position, {zkevm_circuit.get_bytecode_table_id(), {
                    real_opcode_var - real_opcode_var + 1,
                    state.pc(),
                    real_opcode_var,
                    real_opcode_var - real_opcode_var + 1,
                    state.bytecode_hash_hi(),
                    state.bytecode_hash_lo()
                }}});
/*
                // stack_size + stack_output + d2 = stack_input + 1024 + so*2^16
                constraints.push_back({position, stack_size_var + stack_output + d2_var - stack_input - 1024 - so_var*65536});

                // cost + d3 = gas + g*2^32   <=>   g = [cost > gas]
                constraints.push_back({position, cost + d30_var + 65536*d31_var - gas_var - g_var*(circuit_integral_type(1) << 32)});

                constraints.push_back({position, (si_var - 1)*(so_var - 1)*(g_var - 1)});*/
                return {{gate_class::MIDDLE_OP,  {constraints, lookup_constraints}}};
            }

            void generate_assignments(zkevm_table_type &zkevm_table, const zkevm_machine_interface &machine,
                                      zkevm_word_type additional_input) {
                using word_type = typename zkevm_stack::word_type;
                using integral_type = boost::multiprecision::number<boost::multiprecision::backends::cpp_int_modular_backend<257>>;
                using circuit_integral_type = typename BlueprintFieldType::integral_type;
                zkevm_opcode opcode_mnemo = machine.error_opcode();
                std::size_t opcode_num = zkevm_table.get_opcodes_info().get_opcode_number(opcode_mnemo);
                std::size_t opcode_value = zkevm_table.get_opcodes_info().get_opcode_value(opcode_mnemo);
                std::cout << "\topcode = " << opcode_to_string(opcode_mnemo) << std::endl;

                BOOST_ASSERT(opcode_num < 176);
                // Do something with it.
                // 176 is the maximum we can handle by using a "virtual selector" with 2 rows 44 columns and a dedicated parity cell

                std::size_t b = static_cast<std::size_t>(opcode_num % 2),
                            n = static_cast<std::size_t>(opcode_num / 2),
                            col = n % 44,
                            row = n / 44;
                value_type  stack_size = machine.stack_size();

                std::size_t stack_input = zkevm_table.get_opcodes_info().get_opcode_stack_input(opcode_mnemo),
                            stack_output = zkevm_table.get_opcodes_info().get_opcode_stack_output(opcode_mnemo);

                value_type si = (stack_input > circuit_integral_type(stack_size.data)),
                           so = (circuit_integral_type(stack_size.data) + stack_output > 1024 + stack_input),
                           d = (si == 1)? stack_input  - stack_size: stack_size + stack_output - stack_input - 1024;
                std::cout << "\topcode = " << std::hex << opcode_num << std::dec << std::endl;
                std::cout << "\tstack_input = " << stack_input << std::endl;
                std::cout << "\tstack_size = " << circuit_integral_type(stack_size.data) << std::endl;
                std::cout << "\tsi = " << si << std::endl;
                std::cout << "\tso = " << so << std::endl;
                std::cout << "\td = " << d << std::endl;

                const std::vector<std::size_t> &witness_cols = zkevm_table.get_opcode_cols();
                assignment_type &assignment = zkevm_table.get_assignment();
                const std::size_t curr_row = zkevm_table.get_current_row();

                assignment.witness(witness_cols[0], curr_row) = si;
                assignment.witness(witness_cols[1], curr_row) = so;
                assignment.witness(witness_cols[2], curr_row) = zkevm_table.get_opcodes_info().get_opcode_number(opcode_mnemo);
                assignment.witness(witness_cols[0], curr_row+1) = d;
                assignment.witness(witness_cols[1], curr_row+1) = b;
                assignment.witness(witness_cols[2], curr_row+1) = zkevm_table.get_opcodes_info().get_opcode_value(opcode_mnemo);
                std::cout << "Error opcode is " << opcode_to_string(opcode_mnemo) << std::endl;

                for(std::size_t i = 0; i < 44; i++) {
                    for(std::size_t j = 0; j < 2; j++) {
                        assignment.witness(witness_cols[4 + i], curr_row + j) = (i == col) && (j == row);
                    }
                }
            }

            void generate_assignments(zkevm_table_type &zkevm_table, const zkevm_machine_interface &machine) override {
                 generate_assignments(zkevm_table, machine, 0); // just to have a default
            }

            std::size_t rows_amount() override {
                return 2;
            }

            constraint_type stack_size_transition(const zkevm_circuit_type &zkevm_circuit) override {
                constraint_type c;
                return c;
            }
            constraint_type gas_transition(const zkevm_circuit_type &zkevm_circuit) override {
                constraint_type c;
                return c;
            }
            constraint_type pc_transition(const zkevm_circuit_type &zkevm_circuit) override {
                constraint_type c;
                return c;
            }
        };
    }   // namespace blueprint
}   // namespace nil
