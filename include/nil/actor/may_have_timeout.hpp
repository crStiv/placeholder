//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2017-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#pragma once

namespace nil {
    namespace actor {

        template<class F>
        struct timeout_definition;

        class behavior;

        template<class T>
        struct may_have_timeout {
            static constexpr bool value = false;
        };

        template<>
        struct may_have_timeout<behavior> {
            static constexpr bool value = true;
        };

        template<class F>
        struct may_have_timeout<timeout_definition<F>> {
            static constexpr bool value = true;
        };

    }    // namespace actor
}    // namespace nil
