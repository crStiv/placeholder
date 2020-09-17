//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//
// @file Declaration of interfaces for a RAM example, as well as functions to sample
// RAM examples with prescribed parameters (according to some distribution).
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_ZK_RAM_EXAMPLES_HPP_
#define CRYPTO3_ZK_RAM_EXAMPLES_HPP_

#include <nil/crypto3/zk/snark/relations/ram_computations/rams/ram_params.hpp>

namespace nil {
    namespace crypto3 {
        namespace zk {
            namespace snark {

                template<typename ramT>
                struct ram_example {
                    ram_architecture_params<ramT> ap;
                    std::size_t boot_trace_size_bound;
                    std::size_t time_bound;
                    ram_boot_trace<ramT> boot_trace;
                    ram_input_tape<ramT> auxiliary_input;
                };

                /**
                 * For now: only specialized to TinyRAM
                 */
                template<typename ramT>
                ram_example<ramT> gen_ram_example_simple(const ram_architecture_params<ramT> &ap,
                                                         std::size_t boot_trace_size_bound, std::size_t time_bound,
                                                         bool satisfiable = true) {
                    const std::size_t program_size = boot_trace_size_bound / 2;
                    const std::size_t input_size = boot_trace_size_bound - program_size;

                    ram_example<ramT> result;

                    result.ap = ap;
                    result.boot_trace_size_bound = boot_trace_size_bound;
                    result.time_bound = time_bound;

                    tinyram_program prelude;
                    prelude.instructions = generate_tinyram_prelude(ap);

                    std::size_t boot_pos = 0;
                    for (std::size_t i = 0; i < prelude.instructions.size(); ++i) {
                        result.boot_trace.set_trace_entry(boot_pos++,
                                                          std::make_pair(i, prelude.instructions[i].as_dword(ap)));
                    }

                    result.boot_trace[boot_pos] = std::make_pair(
                        boot_pos++, tinyram_instruction(tinyram_opcode_ANSWER, true, 0, 0, satisfiable ? 0 : 1)
                                        .as_dword(ap)); /* answer 0/1 depending on satisfiability */

                    while (boot_pos < program_size) {
                        result.boot_trace.set_trace_entry(boot_pos++, random_tinyram_instruction(ap).as_dword(ap));
                    }

                    for (std::size_t i = 0; i < input_size; ++i) {
                        result.boot_trace.set_trace_entry(
                            boot_pos++,
                            std::make_pair((1ul << (ap.dwaddr_len() - 1)) + i, std::rand() % (1ul << (2 * ap.w))));
                    }

                    BOOST_CHECK(boot_pos == boot_trace_size_bound);
                    return result;
                }

                /**
                 * For now: only specialized to TinyRAM
                 */
                template<typename ramT>
                ram_example<ramT> gen_ram_example_complex(const ram_architecture_params<ramT> &ap,
                                                          std::size_t boot_trace_size_bound, std::size_t time_bound,
                                                          bool satisfiable = true) {
                    const std::size_t program_size = boot_trace_size_bound / 2;
                    const std::size_t input_size = boot_trace_size_bound - program_size;

                    BOOST_CHECK(2 * ap.w / 8 * program_size < 1ul << (ap.w - 1));
                    BOOST_CHECK(ap.w / 8 * input_size < 1ul << (ap.w - 1));

                    ram_example<ramT> result;

                    result.ap = ap;
                    result.boot_trace_size_bound = boot_trace_size_bound;
                    result.time_bound = time_bound;

                    tinyram_program prelude;
                    prelude.instructions = generate_tinyram_prelude(ap);

                    std::size_t boot_pos = 0;
                    for (std::size_t i = 0; i < prelude.instructions.size(); ++i) {
                        result.boot_trace.set_trace_entry(boot_pos++,
                                                          std::make_pair(i, prelude.instructions[i].as_dword(ap)));
                    }

                    const std::size_t prelude_len = prelude.instructions.size();
                    const std::size_t instr_addr = (prelude_len + 4) * (2 * ap.w / 8);
                    const std::size_t input_addr =
                        (1ul << (ap.w - 1)) + (ap.w / 8);    // byte address of the first input word

                    result.boot_trace.set_trace_entry(
                        boot_pos,
                        std::make_pair(boot_pos,
                                       tinyram_instruction(tinyram_opcode_LOADB, true, 1, 0, instr_addr).as_dword(ap)));
                    ++boot_pos;
                    result.boot_trace.set_trace_entry(
                        boot_pos,
                        std::make_pair(boot_pos,
                                       tinyram_instruction(tinyram_opcode_LOADW, true, 2, 0, input_addr).as_dword(ap)));
                    ++boot_pos;
                    result.boot_trace.set_trace_entry(
                        boot_pos,
                        std::make_pair(boot_pos, tinyram_instruction(tinyram_opcode_SUB, false, 1, 1, 2).as_dword(ap)));
                    ++boot_pos;
                    result.boot_trace.set_trace_entry(
                        boot_pos,
                        std::make_pair(
                            boot_pos, tinyram_instruction(tinyram_opcode_STOREB, true, 1, 0, instr_addr).as_dword(ap)));
                    ++boot_pos;
                    result.boot_trace.set_trace_entry(
                        boot_pos,
                        std::make_pair(boot_pos,
                                       tinyram_instruction(tinyram_opcode_ANSWER, true, 0, 0, 1).as_dword(ap)));
                    ++boot_pos;

                    while (boot_pos < program_size) {
                        result.boot_trace.set_trace_entry(
                            boot_pos, std::make_pair(boot_pos, random_tinyram_instruction(ap).as_dword(ap)));
                        ++boot_pos;
                    }

                    result.boot_trace.set_trace_entry(
                        boot_pos++, std::make_pair(1ul << (ap.dwaddr_len() - 1), satisfiable ? 1ul << ap.w : 0));

                    for (std::size_t i = 1; i < input_size; ++i) {
                        result.boot_trace.set_trace_entry(
                            boot_pos++,
                            std::make_pair((1ul << (ap.dwaddr_len() - 1)) + i + 1, std::rand() % (1ul << (2 * ap.w))));
                    }

                    return result;
                }
            }    // namespace snark
        }        // namespace zk
    }            // namespace crypto3
}    // namespace nil

#endif    // RAM_EXAMPLES_HPP_
