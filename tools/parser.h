#pragma once

#include "types.h"

#include <libxml/HTMLparser.h>
#include <nlohmann/json.hpp>
#include <primitives/emitter.h>
#include <primitives/exceptions.h>
#include <primitives/http.h>
#include <primitives/templates.h>
#include <primitives/xml.h>

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
