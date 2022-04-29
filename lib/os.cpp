#include <everest/utils/os.hpp>

#include <cstdlib>
#include <iostream>

namespace everest::os {

// GCOVR_EXCL_START - std::abort can't be easily tested
void unrecoverable_error(const std::string& message) {
    std::cerr << "Unrecoverable error:\n" << message << "Terminating\n";
    std::abort();
}
// GCOVR_EXCL_STOP

std::string get_environment_variable(const std::string& key) {
    // NOLINTNEXTLINE(concurrency-mt-unsafe): not problematic that this function is not threadsafe here
    return std::getenv(key.c_str());
}


} // namespace everest::os
