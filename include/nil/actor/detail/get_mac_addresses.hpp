//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt for Boost License or
// http://opensource.org/licenses/BSD-3-Clause for BSD 3-Clause License
//---------------------------------------------------------------------------//

#pragma once

#include <string>
#include <vector>
#include <utility>

namespace nil {
    namespace actor {
        namespace detail {

            using iface_info = std::pair<std::string /* interface name */, std::string /* interface address */>;

            std::vector<iface_info> get_mac_addresses();

        }    // namespace detail
    }        // namespace actor
}    // namespace nil
