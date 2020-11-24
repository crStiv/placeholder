//---------------------------------------------------------------------------//
// Copyright (c) 2017-2020 Mikhail Komarov <nemo@nil.foundation>
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

#ifndef MARSHALLING_PROTOCOL_LAYERS_ACCESS_HPP
#define MARSHALLING_PROTOCOL_LAYERS_ACCESS_HPP

#include <nil/marshalling/detail/macro_common.hpp>
#include <nil/marshalling/detail/reverse_macro_args.hpp>

#define MARSHALLING_LAYER_TYPE_1 MARSHALLING_EXPAND(::this_layer_type)
#define MARSHALLING_LAYER_TYPE_2 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_1
#define MARSHALLING_LAYER_TYPE_3 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_2
#define MARSHALLING_LAYER_TYPE_4 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_3
#define MARSHALLING_LAYER_TYPE_5 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_4
#define MARSHALLING_LAYER_TYPE_6 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_5
#define MARSHALLING_LAYER_TYPE_7 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_6
#define MARSHALLING_LAYER_TYPE_8 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_7
#define MARSHALLING_LAYER_TYPE_9 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_8
#define MARSHALLING_LAYER_TYPE_10 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_9
#define MARSHALLING_LAYER_TYPE_11 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_10
#define MARSHALLING_LAYER_TYPE_12 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_11
#define MARSHALLING_LAYER_TYPE_13 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_12
#define MARSHALLING_LAYER_TYPE_14 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_13
#define MARSHALLING_LAYER_TYPE_15 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_14
#define MARSHALLING_LAYER_TYPE_16 MARSHALLING_EXPAND(::next_layer_type) MARSHALLING_LAYER_TYPE_15

#define MARSHALLING_DO_LAYER_TYPE_INTERNAL_(N, B_) typename B_ MARSHALLING_EXPAND(MARSHALLING_LAYER_TYPE_##N)
#define MARSHALLING_DO_LAYER_TYPE_INTERNAL(N, B_) MARSHALLING_EXPAND(MARSHALLING_DO_LAYER_TYPE_INTERNAL_(N, B_))
#define MARSHALLING_DO_LAYER_TYPE(B_, ...) \
    MARSHALLING_EXPAND(MARSHALLING_DO_LAYER_TYPE_INTERNAL(MARSHALLING_NUM_ARGS(__VA_ARGS__), B_))

#define MARSHALLING_LAYER_CALL_1 MARSHALLING_EXPAND(.this_layer())
#define MARSHALLING_LAYER_CALL_2 MARSHALLING_EXPAND(.next_layer())
#define MARSHALLING_LAYER_CALL_3 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_2
#define MARSHALLING_LAYER_CALL_4 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_3
#define MARSHALLING_LAYER_CALL_5 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_4
#define MARSHALLING_LAYER_CALL_6 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_5
#define MARSHALLING_LAYER_CALL_7 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_6
#define MARSHALLING_LAYER_CALL_8 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_7
#define MARSHALLING_LAYER_CALL_9 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_8
#define MARSHALLING_LAYER_CALL_10 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_9
#define MARSHALLING_LAYER_CALL_11 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_10
#define MARSHALLING_LAYER_CALL_12 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_11
#define MARSHALLING_LAYER_CALL_13 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_12
#define MARSHALLING_LAYER_CALL_14 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_13
#define MARSHALLING_LAYER_CALL_15 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_14
#define MARSHALLING_LAYER_CALL_16 MARSHALLING_EXPAND(.next_layer()) MARSHALLING_LAYER_CALL_15

#define MARSHALLING_DO_LAYER_CALL_INTERNAL_(N, B_) MARSHALLING_EXPAND(B_) MARSHALLING_EXPAND(MARSHALLING_LAYER_CALL_##N)
#define MARSHALLING_DO_LAYER_CALL_INTERNAL(N, B_) MARSHALLING_EXPAND(MARSHALLING_DO_LAYER_CALL_INTERNAL_(N, B_))
#define MARSHALLING_DO_LAYER_CALL(B_, ...) \
    MARSHALLING_EXPAND(MARSHALLING_DO_LAYER_CALL_INTERNAL(MARSHALLING_NUM_ARGS(__VA_ARGS__), B_))

#ifdef MARSHALLING_MUST_DEFINE_BASE

#define MARSHALLING_ACCESS_LAYER_FUNC(c_, n_) \
    MARSHALLING_DO_LAYER_TYPE_INTERNAL(c_, Base) & MARSHALLING_CONCATENATE(layer_, n_)()
#define MARSHALLING_ACCESS_LAYER_CONST_FUNC(c_, n_) \
    const MARSHALLING_DO_LAYER_TYPE_INTERNAL(c_, Base) & MARSHALLING_CONCATENATE(layer_, n_)() const

#else    // #ifdef MARSHALLING_MUST_DEFINE_BASE
#define MARSHALLING_ACCESS_LAYER_FUNC(c_, n_)                         \
    FUNC_AUTO_REF_RETURN(MARSHALLING_CONCATENATE(layer_, n_),         \
                         decltype(MARSHALLING_DO_LAYER_CALL_INTERNAL( \
                             c_, MARSHALLING_EXPAND(nil::marshalling::protocol::to_protocol_layer_base(*this)))))
#define MARSHALLING_ACCESS_LAYER_CONST_FUNC(c_, n_)  \
    FUNC_AUTO_REF_RETURN_CONST(                      \
        MARSHALLING_CONCATENATE(layer_, n_),         \
        decltype(MARSHALLING_DO_LAYER_CALL_INTERNAL( \
            c_, MARSHALLING_EXPAND(nil::marshalling::protocol::to_protocol_layer_base(*this)))))
#endif    // #ifdef MARSHALLING_MUST_DEFINE_BASE

#define MARSHALLING_ACCESS_LAYER_ACC_FUNC(c_, n_)                                                                 \
    MARSHALLING_ACCESS_LAYER_FUNC(c_, n_) {                                                                       \
        return MARSHALLING_DO_LAYER_CALL_INTERNAL(c_, nil::marshalling::protocol::to_protocol_layer_base(*this)); \
    }                                                                                                             \
    MARSHALLING_ACCESS_LAYER_CONST_FUNC(c_, n_) {                                                                 \
        return MARSHALLING_DO_LAYER_CALL_INTERNAL(c_, nil::marshalling::protocol::to_protocol_layer_base(*this)); \
    }

#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_1(n_) MARSHALLING_ACCESS_LAYER_ACC_FUNC(1, n_)
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_2(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(2, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_1(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_3(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(3, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_2(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_4(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(4, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_3(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_5(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(5, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_4(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_6(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(6, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_5(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_7(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(7, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_6(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_8(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(8, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_7(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_9(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(9, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_8(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_10(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(10, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_9(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_11(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(11, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_10(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_12(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(12, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_11(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_13(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(13, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_12(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_14(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(14, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_13(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_15(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(15, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_14(__VA_ARGS__))
#define MARSHALLING_ACCESS_LAYER_ACC_FUNC_16(n_, ...) \
    MARSHALLING_ACCESS_LAYER_ACC_FUNC(16, n_)         \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_15(c_, __VA_ARGS__))

#define MARSHALLING_CHOOSE_ACCESS_LAYER_ACC_FUNC_(N, ...) \
    MARSHALLING_EXPAND(MARSHALLING_ACCESS_LAYER_ACC_FUNC_##N(__VA_ARGS__))
#define MARSHALLING_CHOOSE_ACCESS_LAYER_ACC_FUNC(N, ...) \
    MARSHALLING_EXPAND(MARSHALLING_CHOOSE_ACCESS_LAYER_ACC_FUNC_(N, __VA_ARGS__))
#define MARSHALLING_DO_ACCESS_LAYER_ACC_FUNC(...) \
    MARSHALLING_EXPAND(MARSHALLING_CHOOSE_ACCESS_LAYER_ACC_FUNC(MARSHALLING_NUM_ARGS(__VA_ARGS__), __VA_ARGS__))
#endif    // MARSHALLING_PROTOCOL_LAYERS_ACCESS_HPP
