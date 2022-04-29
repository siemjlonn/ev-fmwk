#include <everest/interface.hpp>
#include <everest/utils/helpers.hpp>
#include <everest/utils/schema.hpp>

namespace everest {

bool arguments_match_command_type(const Arguments& arguments, const schema::CommandType& cmd_type) {
    for (auto& arg : cmd_type.arguments) {
        auto it = arguments.find(arg.first);
        if (it == arguments.end()) {
            // missing argument
            return false;
        }

        auto validation_result = arg.second.validate(it.value());
        if (!validation_result) {
            // FIXME (aw): handle validation error
            return false;
        }
    }

    return true;
}

schema::ValidationResult value_matches_variable_type(const Value& value, const schema::VariableType& variable_type) {
    return variable_type.validate(value);
}

schema::ValidationResult return_value_matches_command_type(const Value& value, const schema::CommandType& cmd_type) {
    // FIXME (aw): what if empty?
    return cmd_type.return_type.validate(value);
}

} // namespace everest
