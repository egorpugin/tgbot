#pragma once

#include <libxml/HTMLparser.h>
#include <nlohmann/json.hpp>
#include <primitives/emitter.h>
#include <primitives/exceptions.h>
#include <primitives/http.h>
#include <primitives/templates.h>
#include <primitives/xml.h>

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
    void emitFieldType(primitives::CppEmitter &ctx) const;
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

    void emitType(primitives::CppEmitter &ctx) const;
    void emitCreateType(primitives::CppEmitter &ctx) const;
    void emitMethod(const Emitter &e, primitives::CppEmitter &h, primitives::CppEmitter &cpp) const;
    void emitFwdDecl(primitives::CppEmitter &ctx) const;
    void emitProtobuf(primitives::CppEmitter &ctx) const;
};

struct Parser
{
    std::vector<Type> types;
    std::vector<Type> methods;

    Parser(const String &s);
    ~Parser();

    void enumerateSectionChildren(const String &name);

private:
    xmlParserCtxtPtr ctx;
    xmlNodePtr root;

    void enumerateSectionChildren(xmlNode *in, const String &name);
    xmlNode *getSection(xmlNode *in, const String &name) const;
    String getAllText(xmlNode *in) const;
    void parseTypeOneOf(Type &t, xmlNode *ul) const;
    void parseType(Type &t, xmlNode *tb) const;
};

struct Emitter
{
    std::map<String, Type> types;
    std::map<String, Type> methods;

    Emitter(const Parser &p);

    String emitTypes();
    void emitMethods() const;
    String emitProtobuf() const;
};
