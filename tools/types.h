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

    void save(nlohmann::json &j) const;
    void emitField(primitives::CppEmitter &ctx) const;
    void emitFieldType(primitives::CppEmitter &ctx, bool emitoptional = false) const;
    std::vector<String> get_dependent_types() const;
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

    String get_file_name() const { return name + "_type.inl.h"; }
    String get_request_file_name() const { return name + "Request_type.inl.h"; }
    std::vector<String> get_dependent_types() const;

    void emitType(primitives::CppEmitter &ctx) const;
    void emitMethod(const Emitter &e, primitives::CppEmitter &h, primitives::CppEmitter &cpp) const;
    void emitMethodRequestType(primitives::CppEmitter &ctx) const;
    void emitFwdDecl(primitives::CppEmitter &ctx) const;
};
