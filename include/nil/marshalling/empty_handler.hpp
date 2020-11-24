//---------------------------------------------------------------------------//
// Copyright (c) 2017-2020 Mikhail Komarov <nemo@nil.foundation>
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

/// @file
/// Contains definition of @ref nil::marshalling::empty_handler class

#ifndef MARSHALLING_EMPTY_HANDLER_HPP
#define MARSHALLING_EMPTY_HANDLER_HPP

namespace nil {
    namespace marshalling {

        /// @brief Empty message handler, does nothing.
        /// @details May be used in nil::marshalling::option::handler_type option to force
        ///     existence of "nil::marshalling::Message::dispatch()" member function.
        class empty_handler {
        public:
            template<typename TMessage>
            void handle(TMessage &) {
            }
        };

    }    // namespace marshalling
}    // namespace nil
#endif    // MARSHALLING_EMPTY_HANDLER_HPP
