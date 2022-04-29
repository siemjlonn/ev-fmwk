#include <everest/types.hpp>

static const int JSON_DUMP_INDENT = 2;

namespace everest {
    
std::string format_value(Value value) {
    return value.dump(JSON_DUMP_INDENT);
}

std::string format_arguments(Arguments arguments) {
    // FIXME (aw): could be done better
    return arguments.dump(JSON_DUMP_INDENT);
}

} // namespace everest
