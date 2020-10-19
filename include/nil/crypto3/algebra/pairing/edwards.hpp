//---------------------------------------------------------------------------//
// Copyright (c) 2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
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

#ifndef CRYPTO3_ALGEBRA_PAIRING_EDWARDS_POLICY_HPP
#define CRYPTO3_ALGEBRA_PAIRING_EDWARDS_POLICY_HPP

#include <nil/crypto3/algebra/pairing/detail/edwards/functions.hpp>
#include <nil/crypto3/algebra/pairing/policy.hpp>

#include <nil/crypto3/algebra/curves/edwards.hpp>

namespace nil {
    namespace crypto3 {
        namespace algebra {
            namespace curves {
                template<std::size_t ModulusBits, std::size_t GeneratorBits>
                struct edwards;
            }    // namespace curves
            namespace pairing {

                using namespace nil::crypto3::algebra;

                template<std::size_t ModulusBits, std::size_t GeneratorBits>
                class pairing_policy<curves::edwards<ModulusBits, GeneratorBits>> : public detail::edwards_pairing_functions<ModulusBits, GeneratorBits>{
                    using policy_type = detail::edwards_pairing_functions<ModulusBits, GeneratorBits>;
                    using basic_policy = detail::edwards_basic_policy<ModulusBits, GeneratorBits>;
                public:

                    using number_type = typename basic_policy::number_type;

                    constexpr static const typename basic_policy::number_type pairing_loop_count = basic_policy::ate_loop_count;

                    using Fp_type = typename basic_policy::Fp_field;
                    using G1_type = typename basic_policy::g1;
                    using G2_type = typename basic_policy::g2;
                    using Fq_type = typename basic_policy::Fq_field;
                    using Fqe_type = typename basic_policy::Fqe_field;
                    using Fqk_type = typename basic_policy::Fqk_field;
                    using GT_type = typename basic_policy::gt;

                    using G1_precomp = typename policy_type::g1_precomp;
                    using G2_precomp = typename policy_type::g2_precomp;

                    using policy_type::precompute_g1;
                    using policy_type::precompute_g2;

                    using policy_type::reduced_pairing;
                    using policy_type::pairing;

                    using policy_type::miller_loop;
                    using policy_type::double_miller_loop;
                    using policy_type::final_exponentiation;
                };

                template<std::size_t ModulusBits, std::size_t GeneratorBits>
                constexpr typename pairing_policy<curves::edwards<ModulusBits, GeneratorBits>>::number_type 
                    const pairing_policy<curves::edwards<ModulusBits, GeneratorBits>>::pairing_loop_count;
            }    // namespace pairing
        }        // namespace algebra
    }            // namespace crypto3
}    // namespace nil
#endif    // CRYPTO3_ALGEBRA_PAIRING_EDWARDS_POLICY_HPP