#pragma once

#include <nlohmann/json.hpp>
#include <primitives/emitter.h>

struct Emitter;

struct Field
{
    String name;
    std::vector<String> types;
    String description;
    bool optional = false;
    int array = 0;
    std::map<String, String> enum_values;
    bool enum_ = false;
    String always;

    void save(nlohmann::json &j) const;
    void emitField(primitives::CppEmitter &ctx) const;
    void emitFieldType(primitives::CppEmitter &ctx, bool emitoptional = false, bool return_type = false, const String &parent_type = {}) const;
    std::vector<String> get_dependent_types() const;
    bool is_enum() const;
    String get_enum_name() const;
    String get_enum_type(const String &parent_type = {}) const;
};

struct Type
{
    String name;
    Field return_type;
    std::vector<Field> fields;
    std::vector<String> oneof;
    String description;

    void save(nlohmann::json &j) const;

    bool is_type() const { return isupper(name[0]); }
    bool is_oneof() const { return !oneof.empty(); }
    bool is_received_variant(const std::map<String, Type> &types) const {
        std::map<String, std::vector<std::pair<const Type *, const Field *>>> fields;
        for (auto &tn : oneof) {
            auto &t2 = types.find(tn)->second;
            for (auto &f : t2.fields) {
                if (!f.always.empty())
                    fields[f.name].push_back({ &t2, &f });
            }
        }
        return !fields.empty();
    }

    String get_file_name() const { return name + "_type.inl.h"; }
    String get_request_file_name() const { return name + "Request_type.inl.h"; }
    std::vector<String> get_dependent_types() const;

    void emitType(primitives::CppEmitter &ctx) const;
    void emitTypeCpp(primitives::CppEmitter &ctx) const;
    void emitMethod(const Emitter &e, primitives::CppEmitter &h, primitives::CppEmitter &cpp) const;
    void emitMethodRequestType(primitives::CppEmitter &ctx) const;
    void emitFwdDecl(primitives::CppEmitter &ctx) const;
    void emitEnums(primitives::CppEmitter &ctx) const;
};
