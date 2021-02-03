#include <nil/actor/core/posix.hh>
#include <nil/actor/detail/read_first_line.hh>

namespace nil { namespace actor {

    sstring read_first_line(std::filesystem::path sys_file) {
        auto file = file_desc::open(sys_file.string(), O_RDONLY | O_CLOEXEC);
        sstring buf;
        size_t n = 0;
        do {
            // try to avoid allocations
            sstring tmp = uninitialized_string(8);
            auto ret = file.read(tmp.data(), 8ul);
            if (!ret) {    // EAGAIN
                continue;
            }
            n = *ret;
            if (n > 0) {
                buf += tmp;
            }
        } while (n != 0);
        auto end = buf.find('\n');
        auto value = buf.substr(0, end);
        file.close();
        return value;
    }

}}
