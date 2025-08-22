#include "utils.h"
#include <spdlog/spdlog.h>

namespace yw {
    namespace utils {
        void print_hello() {
            spdlog::info("Hello, World!");
        }
    }
}