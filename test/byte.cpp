//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2017-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE byte

#include <nil/actor/byte.hpp>

#include <nil/actor/test/dsl.hpp>

#include <cstdint>

#include <nil/actor/detail/parser/add_ascii.hpp>

using namespace nil::actor;

namespace boost {
    namespace test_tools {
        namespace tt_detail {
            template<>
            struct print_log_value<nil::actor::byte> {
                void operator()(std::ostream &, nil::actor::byte const &) {
                }
            };
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

namespace {

    byte operator"" _b(const char *str, size_t n) {
        size_t consumed = 0;
        uint8_t result = 0;
        for (size_t i = 0; i < n; ++i) {
            if (str[i] != '\'') {
                if (!detail::parser::add_ascii<2>(result, str[i]))
                    throw std::logic_error("invalid character or over-/underflow");
                else
                    ++consumed;
            }
        }
        if (consumed != 8)
            throw std::logic_error("too few digits, expected exactly 8");
        return static_cast<byte>(result);
    }

    struct fixture {
        fixture() {
            // Sanity checks.
            if ("0001'1100"_b != static_cast<byte>(0x1C))
                BOOST_FAIL("operator \"\"_b broken");
            if ("1000'0001"_b != static_cast<byte>(0x81))
                BOOST_FAIL("operator \"\"_b broken");
        }
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(byte_tests, fixture)

BOOST_AUTO_TEST_CASE(to_integer_test) {
    BOOST_CHECK_EQUAL(to_integer<int>("0110'1001"_b), 0x69);
}

BOOST_AUTO_TEST_CASE(left_shift) {
    auto x = "0000'0001"_b;
    x <<= 1;
    BOOST_CHECK_EQUAL(x, "0000'0010"_b);
    BOOST_CHECK_EQUAL("0000'0010"_b << 1, "0000'0100"_b);
    BOOST_CHECK_EQUAL("0000'0010"_b << 2, "0000'1000"_b);
    BOOST_CHECK_EQUAL("0000'0010"_b << 3, "0001'0000"_b);
    BOOST_CHECK_EQUAL("0000'0010"_b << 4, "0010'0000"_b);
    BOOST_CHECK_EQUAL("0000'0010"_b << 5, "0100'0000"_b);
    BOOST_CHECK_EQUAL("0000'0010"_b << 6, "1000'0000"_b);
    BOOST_CHECK_EQUAL("0000'0010"_b << 7, "0000'0000"_b);
}

BOOST_AUTO_TEST_CASE(right_shift) {
    auto x = "0100'0000"_b;
    x >>= 1;
    BOOST_CHECK_EQUAL(x, "0010'0000"_b);
    BOOST_CHECK_EQUAL("0100'0000"_b >> 1, "0010'0000"_b);
    BOOST_CHECK_EQUAL("0100'0000"_b >> 2, "0001'0000"_b);
    BOOST_CHECK_EQUAL("0100'0000"_b >> 3, "0000'1000"_b);
    BOOST_CHECK_EQUAL("0100'0000"_b >> 4, "0000'0100"_b);
    BOOST_CHECK_EQUAL("0100'0000"_b >> 5, "0000'0010"_b);
    BOOST_CHECK_EQUAL("0100'0000"_b >> 6, "0000'0001"_b);
    BOOST_CHECK_EQUAL("0100'0000"_b >> 7, "0000'0000"_b);
}

BOOST_AUTO_TEST_CASE(bitwise_or) {
    auto x = "0001'1110"_b;
    x |= "0111'1000"_b;
    BOOST_CHECK_EQUAL(x, "0111'1110"_b);
    BOOST_CHECK_EQUAL("0001'1110"_b | "0111'1000"_b, "0111'1110"_b);
}

BOOST_AUTO_TEST_CASE(bitwise_and) {
    auto x = "0001'1110"_b;
    x &= "0111'1000"_b;
    BOOST_CHECK_EQUAL(x, "0001'1000"_b);
    BOOST_CHECK_EQUAL("0001'1110"_b & "0111'1000"_b, "0001'1000"_b);
}

BOOST_AUTO_TEST_CASE(bitwise_xor) {
    auto x = "0001'1110"_b;
    x ^= "0111'1000"_b;
    BOOST_CHECK_EQUAL(x, "0110'0110"_b);
    BOOST_CHECK_EQUAL("0001'1110"_b ^ "0111'1000"_b, "0110'0110"_b);
}

BOOST_AUTO_TEST_CASE(bitwise_not) {
    BOOST_CHECK_EQUAL(~"0111'1110"_b, "1000'0001"_b);
}

BOOST_AUTO_TEST_SUITE_END()
