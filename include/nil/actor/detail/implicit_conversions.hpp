//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2018-2019 Nil Foundation AG
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt for Boost License or
// http://opensource.org/licenses/BSD-3-Clause for BSD 3-Clause License
//---------------------------------------------------------------------------//

#pragma once

#include <string>
#include <type_traits>

#include <nil/actor/fwd.hpp>
#include <nil/actor/actor_marker.hpp>

#include <nil/actor/detail/type_list.hpp>
#include <nil/actor/detail/type_traits.hpp>

namespace nil {
    namespace actor {
        namespace detail {

            template<class T, bool IsDyn = std::is_base_of<dynamically_typed_actor_base, T>::value,
                     bool IsStat = std::is_base_of<statically_typed_actor_base, T>::value>
            struct implicit_actor_conversions {
                using type = T;
            };

            template<class T>
            struct implicit_actor_conversions<T, true, false> {
                using type = actor;
            };

            template<class T>
            struct implicit_actor_conversions<T, false, true> {
                using type = typename detail::tl_apply<typename T::signatures, typed_actor>::type;
            };

            template<>
            struct implicit_actor_conversions<actor_control_block, false, false> {
                using type = strong_actor_ptr;
            };

            template<class T>
            struct implicit_conversions {
                using type = typename std::conditional<std::is_convertible<T, error>::value, error, T>::type;
            };

            template<class T>
            struct implicit_conversions<T *> : implicit_actor_conversions<T> {};

            template<>
            struct implicit_conversions<char *> {
                using type = std::string;
            };

            template<size_t N>
            struct implicit_conversions<char[N]> : implicit_conversions<char *> {};

            template<>
            struct implicit_conversions<const char *> : implicit_conversions<char *> {};

            template<size_t N>
            struct implicit_conversions<const char[N]> : implicit_conversions<char *> {};

            template<>
            struct implicit_conversions<char16_t *> {
                using type = std::u16string;
            };

            template<size_t N>
            struct implicit_conversions<char16_t[N]> : implicit_conversions<char16_t *> {};

            template<>
            struct implicit_conversions<const char16_t *> : implicit_conversions<char16_t *> {};

            template<size_t N>
            struct implicit_conversions<const char16_t[N]> : implicit_conversions<char16_t *> {};

            template<>
            struct implicit_conversions<char32_t *> {
                using type = std::u16string;
            };

            template<size_t N>
            struct implicit_conversions<char32_t[N]> : implicit_conversions<char32_t *> {};

            template<>
            struct implicit_conversions<const char32_t *> : implicit_conversions<char32_t *> {};

            template<size_t N>
            struct implicit_conversions<const char32_t[N]> : implicit_conversions<char32_t *> {};

            template<>
            struct implicit_conversions<scoped_actor> {
                using type = actor;
            };

            template<class T>
            using implicit_conversions_t = typename implicit_conversions<T>::type;

            template<class T>
            struct strip_and_convert {
                using type = typename implicit_conversions<
                    typename std::remove_const<typename std::remove_reference<T>::type>::type>::type;
            };

            template<class T>
            using strip_and_convert_t = typename strip_and_convert<T>::type;

        }    // namespace detail
    }        // namespace actor
}    // namespace nil
