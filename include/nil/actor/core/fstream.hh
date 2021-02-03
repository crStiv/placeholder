//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
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

/// \file

// File <-> streams adapters
//
// =nil; Actor files are block-based due to the reliance on DMA - you must read
// on sector boundaries.  The adapters in this file provide a byte stream
// interface to files, while retaining the zero-copy characteristics of
// seastar files.

#include <nil/actor/core/file.hh>
#include <nil/actor/core/iostream.hh>
#include <nil/actor/core/shared_ptr.hh>
#include <nil/actor/core/detail/api-level.hh>

namespace nil {
    namespace actor {

        class file_input_stream_history {
            static constexpr uint64_t window_size = 4 * 1024 * 1024;
            struct window {
                uint64_t total_read = 0;
                uint64_t unused_read = 0;
            };
            window current_window;
            window previous_window;
            unsigned read_ahead = 1;

            friend class file_data_source_impl;
        };

        /// Data structure describing options for opening a file input stream
        struct file_input_stream_options {
            size_t buffer_size = 8192;    ///< I/O buffer size
            unsigned read_ahead = 0;      ///< Maximum number of extra read-ahead operations
            ::nil::actor::io_priority_class io_priority_class = default_priority_class();
            lw_shared_ptr<file_input_stream_history> dynamic_adjustments =
                {};    ///< Input stream history, if null dynamic adjustments are disabled
        };

        /// \brief Creates an input_stream to read a portion of a file.
        ///
        /// \param file File to read; multiple streams for the same file may coexist
        /// \param offset Starting offset to read from (no alignment restrictions)
        /// \param len Maximum number of bytes to read; the stream will stop at end-of-file
        ///            even if `offset + len` is beyond end-of-file.
        /// \param options A set of options controlling the stream.
        ///
        /// \note Multiple input streams may exist concurrently for the same file.
        input_stream<char>
            make_file_input_stream(file file, uint64_t offset, uint64_t len, file_input_stream_options options = {});

        // Create an input_stream for a given file, with the specified options.
        // Multiple fibers of execution (continuations) may safely open
        // multiple input streams concurrently for the same file.
        input_stream<char> make_file_input_stream(file file, uint64_t offset, file_input_stream_options = {});

        // Create an input_stream for reading starting at a given position of the
        // given file. Multiple fibers of execution (continuations) may safely open
        // multiple input streams concurrently for the same file.
        input_stream<char> make_file_input_stream(file file, file_input_stream_options = {});

        struct file_output_stream_options {
            // For small files, setting preallocation_size can make it impossible for XFS to find
            // an aligned extent. On the other hand, without it, XFS will divide the file into
            // file_size/buffer_size extents. To avoid fragmentation, we set the default buffer_size
            // to 64k (so each extent will be a minimum of 64k) and preallocation_size to 0 (to avoid
            // extent allocation problems).
            //
            // Large files should increase both buffer_size and preallocation_size.
            unsigned buffer_size = 65536;
            unsigned preallocation_size = 0;    ///< Preallocate extents. For large files, set to a large number (a few
                                                ///< megabytes) to reduce fragmentation
            unsigned write_behind = 1;          ///< Number of buffers to write in parallel
            ::nil::actor::io_priority_class io_priority_class = default_priority_class();
        };

        SEASTAR_INCLUDE_API_V2 namespace api_v2 {

            /// Create an output_stream for writing starting at the position zero of a
            /// newly created file.
            /// NOTE: flush() should be the last thing to be called on a file output stream.
            [[deprecated("use =nil; Actor_API_LEVEL=3 instead")]] output_stream<char> make_file_output_stream(
                file file, uint64_t buffer_size = 8192);

            /// Create an output_stream for writing starting at the position zero of a
            /// newly created file.
            /// NOTE: flush() should be the last thing to be called on a file output stream.
            [[deprecated("use =nil; Actor_API_LEVEL=3 instead")]] output_stream<char> make_file_output_stream(
                file file, file_output_stream_options options);

            /// Create a data_sink for writing starting at the position zero of a
            /// newly created file.
            [[deprecated("use =nil; Actor_API_LEVEL=3 instead")]] data_sink make_file_data_sink(
                file, file_output_stream_options);
        }

        SEASTAR_INCLUDE_API_V3 namespace api_v3 {
            inline namespace and_newer {

                /// Create an output_stream for writing starting at the position zero of a
                /// newly created file.
                /// NOTE: flush() should be the last thing to be called on a file output stream.
                /// Closes the file if the stream creation fails.
                future<output_stream<char>> make_file_output_stream(file file, uint64_t buffer_size = 8192) noexcept;

                /// Create an output_stream for writing starting at the position zero of a
                /// newly created file.
                /// NOTE: flush() should be the last thing to be called on a file output stream.
                /// Closes the file if the stream creation fails.
                future<output_stream<char>> make_file_output_stream(file file,
                                                                    file_output_stream_options options) noexcept;

                /// Create a data_sink for writing starting at the position zero of a
                /// newly created file.
                /// Closes the file if the sink creation fails.
                future<data_sink> make_file_data_sink(file, file_output_stream_options) noexcept;

            }    // namespace and_newer
        }
    }    // namespace actor
}    // namespace nil
