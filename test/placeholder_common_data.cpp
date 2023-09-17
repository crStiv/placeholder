#define BOOST_TEST_MODULE crypto3_marshalling_placeholder_common_data_test

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>

#include <nil/marshalling/status_type.hpp>
#include <nil/marshalling/field_type.hpp>
#include <nil/marshalling/endianness.hpp>

#include <nil/crypto3/multiprecision/cpp_int.hpp>
#include <nil/crypto3/multiprecision/number.hpp>

#include <nil/crypto3/algebra/random_element.hpp>
#include <nil/crypto3/algebra/curves/bls12.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/bls12.hpp>
#include <nil/crypto3/algebra/curves/alt_bn128.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/alt_bn128.hpp>
#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>
#include <nil/crypto3/algebra/random_element.hpp>

#include <nil/crypto3/hash/type_traits.hpp>
#include <nil/crypto3/hash/sha2.hpp>
#include <nil/crypto3/hash/keccak.hpp>

#include <nil/crypto3/random/algebraic_random_device.hpp>

#include <nil/crypto3/marshalling/zk/types/placeholder/common_data.hpp>

#include <nil/crypto3/zk/snark/systems/plonk/placeholder/preprocessor.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/prover.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/verifier.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/params.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/profiling.hpp>

#include <nil/crypto3/math/algorithms/unity_root.hpp>
#include <nil/crypto3/math/polynomial/lagrange_interpolation.hpp>
#include <nil/crypto3/math/algorithms/calculate_domain_set.hpp>

#include "./detail/circuits.hpp"


using namespace nil::crypto3;
using namespace nil::crypto3::zk;
using namespace nil::crypto3::zk::snark;


inline std::vector<std::size_t> generate_random_step_list(const std::size_t r, const int max_step) {
    using dist_type = std::uniform_int_distribution<int>;
    static std::random_device random_engine;

    std::vector<std::size_t> step_list;
    std::size_t steps_sum = 0;
    while (steps_sum != r) {
        if (r - steps_sum <= max_step) {
            while (r - steps_sum != 1) {
                step_list.emplace_back(r - steps_sum - 1);
                steps_sum += step_list.back();
            }
            step_list.emplace_back(1);
            steps_sum += step_list.back();
        } else {
            step_list.emplace_back(dist_type(1, max_step)(random_engine));
            steps_sum += step_list.back();
        }
    }
    return step_list;
}

template<typename fri_type, typename FieldType>
typename fri_type::params_type create_fri_params(std::size_t degree_log, const int max_step = 1) {
    typename fri_type::params_type params;
    math::polynomial<typename FieldType::value_type> q = {0, 0, 1};

    constexpr std::size_t expand_factor = 4;

    std::size_t r = degree_log - 1;

    std::vector<std::shared_ptr<math::evaluation_domain<FieldType>>> domain_set =
            math::calculate_domain_set<FieldType>(degree_log + expand_factor, r);

    params.r = r;
    params.D = domain_set;
    params.max_degree = (1 << degree_log) - 1;
    params.step_list = generate_random_step_list(r, max_step);

    return params;
}

template<typename CommonDataType>
void test_placeholder_common_data(CommonDataType common_data) {
    using Endianness = nil::marshalling::option::big_endian;
    using TTypeBase = nil::marshalling::field_type<Endianness>;

    auto filled_common_data = nil::crypto3::marshalling::types::fill_placeholder_common_data<Endianness, CommonDataType>(common_data);
    auto _common_data = nil::crypto3::marshalling::types::make_placeholder_common_data<Endianness,CommonDataType>(filled_common_data);
    BOOST_CHECK(common_data == _common_data);

    std::vector<std::uint8_t> cv;
    cv.resize(filled_common_data.length(), 0x00);
    auto write_iter = cv.begin();
    nil::marshalling::status_type status = filled_common_data.write(write_iter, cv.size());

    nil::crypto3::marshalling::types::placeholder_common_data<TTypeBase, CommonDataType> test_val_read;
    auto read_iter = cv.begin();
    status = test_val_read.read(read_iter, cv.size());
    auto constructed_val_read = nil::crypto3::marshalling::types::make_placeholder_common_data<Endianness, CommonDataType>(
            test_val_read);
    BOOST_CHECK(common_data == constructed_val_read);
}

BOOST_AUTO_TEST_SUITE(placeholder_circuit1)
    using Endianness = nil::marshalling::option::big_endian;
    using TTypeBase = nil::marshalling::field_type<Endianness>;

    using curve_type = algebra::curves::pallas;
    using field_type = typename curve_type::base_field_type;
    using merkle_hash_type = hashes::keccak_1600<512>;
    using transcript_hash_type = hashes::keccak_1600<512>;
    constexpr static const std::size_t table_rows_log = 4;

    struct placeholder_test_params {
        constexpr static const std::size_t table_rows = 1 << table_rows_log;
        constexpr static const std::size_t permutation_size = 4;
        constexpr static const std::size_t usable_rows = (1 << table_rows_log) - 3;


        constexpr static const std::size_t witness_columns = witness_columns_1;
        constexpr static const std::size_t public_input_columns = public_columns_1;
        constexpr static const std::size_t constant_columns = constant_columns_1;
        constexpr static const std::size_t selector_columns = selector_columns_1;

        using arithmetization_params =
            plonk_arithmetization_params<witness_columns, public_input_columns, constant_columns, selector_columns>;

        constexpr static const std::size_t lambda = 40;
        constexpr static const std::size_t r = table_rows_log - 1;
        constexpr static const std::size_t m = 2;
    };
    typedef placeholder_circuit_params<field_type, typename placeholder_test_params::arithmetization_params> circuit_params;
    using transcript_type = typename transcript::fiat_shamir_heuristic_sequential<transcript_hash_type>;

    using lpc_params_type = commitments::list_polynomial_commitment_params<        
        merkle_hash_type,
        transcript_hash_type, 
        placeholder_test_params::lambda, 
        placeholder_test_params::r,
        placeholder_test_params::m,
        true
    >;

    using lpc_type = commitments::list_polynomial_commitment<field_type, lpc_params_type>;
    using lpc_scheme_type = typename commitments::lpc_commitment_scheme<lpc_type>;
    using lpc_placeholder_params_type = nil::crypto3::zk::snark::placeholder_params<circuit_params, lpc_scheme_type>;
    using policy_type = zk::snark::detail::placeholder_policy<field_type, lpc_placeholder_params_type>;

BOOST_AUTO_TEST_CASE(prover_test) {
    auto circuit = circuit_test_1<field_type>();

    plonk_table_description<field_type, typename circuit_params::arithmetization_params> desc;

    desc.rows_amount = placeholder_test_params::table_rows;
    desc.usable_rows_amount = placeholder_test_params::usable_rows;

    typename policy_type::constraint_system_type constraint_system(circuit.gates, circuit.copy_constraints, circuit.lookup_gates);
    typename policy_type::variable_assignment_type assignments = circuit.table;

    std::vector<std::size_t> columns_with_copy_constraints = {0, 1, 2, 3};


    typename lpc_type::fri_type::params_type fri_params = create_fri_params<typename lpc_type::fri_type, field_type>(table_rows_log);
    lpc_scheme_type lpc_scheme(fri_params);
    transcript_type lpc_transcript;

    typename placeholder_public_preprocessor<field_type, lpc_placeholder_params_type>::preprocessed_data_type
        lpc_preprocessed_public_data = placeholder_public_preprocessor<field_type, lpc_placeholder_params_type>::process(
            constraint_system, assignments.public_table(), desc, lpc_scheme, columns_with_copy_constraints.size(), lpc_transcript
        );

    test_placeholder_common_data<placeholder_public_preprocessor<field_type, lpc_placeholder_params_type>::preprocessed_data_type::common_data_type>(
        lpc_preprocessed_public_data.common_data
    );
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(placeholder_circuit2)
    using Endianness = nil::marshalling::option::big_endian;
    using TTypeBase = nil::marshalling::field_type<Endianness>;

    using curve_type = algebra::curves::bls12<381>;
    using field_type = typename curve_type::scalar_field_type;

    constexpr static const std::size_t table_rows_log = 4;
    constexpr static const std::size_t table_rows = 1 << table_rows_log;
    constexpr static const std::size_t permutation_size = 4;
    constexpr static const std::size_t usable_rows = (1 << table_rows_log) - 3;

    struct placeholder_test_params {
        using merkle_hash_type = hashes::keccak_1600<512>;
        using transcript_hash_type = hashes::keccak_1600<512>;

        constexpr static const std::size_t witness_columns = 3;
        constexpr static const std::size_t public_input_columns = 1;
        constexpr static const std::size_t constant_columns = 0;
        constexpr static const std::size_t selector_columns = 2;

        using arithmetization_params =
            plonk_arithmetization_params<witness_columns, public_input_columns, constant_columns, selector_columns>;

        constexpr static const std::size_t lambda = 1;
        constexpr static const std::size_t r = table_rows_log - 1;
        constexpr static const std::size_t m = 2;
    };
    using circuit_t_params = placeholder_circuit_params<
        field_type, 
        typename placeholder_test_params::arithmetization_params
    >;

    using transcript_type = typename transcript::fiat_shamir_heuristic_sequential<typename placeholder_test_params::transcript_hash_type>;

    using lpc_params_type = commitments::list_polynomial_commitment_params<        
        typename placeholder_test_params::merkle_hash_type,
        typename placeholder_test_params::transcript_hash_type, 
        placeholder_test_params::lambda, 
        placeholder_test_params::r,
        placeholder_test_params::m
    >;

    using lpc_type = commitments::list_polynomial_commitment<field_type, lpc_params_type>;
    using lpc_scheme_type = typename commitments::lpc_commitment_scheme<lpc_type>;
    using lpc_placeholder_params_type = nil::crypto3::zk::snark::placeholder_params<circuit_t_params, lpc_scheme_type>;

    using policy_type = zk::snark::detail::placeholder_policy<field_type, circuit_t_params>;
    
BOOST_AUTO_TEST_CASE(common_data_marshalling_test) {
    auto pi0 = nil::crypto3::algebra::random_element<field_type>();
    auto circuit = circuit_test_t<field_type>(pi0);

    plonk_table_description<field_type, typename circuit_t_params::arithmetization_params> desc;
    desc.rows_amount = table_rows;
    desc.usable_rows_amount = usable_rows;

    typename policy_type::constraint_system_type constraint_system(circuit.gates, circuit.copy_constraints, circuit.lookup_gates);
    typename policy_type::variable_assignment_type assignments = circuit.table;

    std::vector<std::size_t> columns_with_copy_constraints = {0, 1, 2, 3};

    bool verifier_res;

    // LPC commitment scheme
    typename lpc_type::fri_type::params_type fri_params = create_fri_params<typename lpc_type::fri_type, field_type>(table_rows_log);
    lpc_scheme_type lpc_scheme(fri_params);
    transcript_type lpc_transcript;

    typename placeholder_public_preprocessor<field_type, lpc_placeholder_params_type>::preprocessed_data_type
        lpc_preprocessed_public_data = placeholder_public_preprocessor<field_type, lpc_placeholder_params_type>::process(
            constraint_system, assignments.public_table(), desc, lpc_scheme, columns_with_copy_constraints.size(), lpc_transcript
        );
    test_placeholder_common_data<placeholder_public_preprocessor<field_type, lpc_placeholder_params_type>::preprocessed_data_type::common_data_type>(
        lpc_preprocessed_public_data.common_data
    );
}
BOOST_AUTO_TEST_SUITE_END()
