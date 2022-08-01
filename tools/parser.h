#pragma once

#include "types.h"

#include <libxml/HTMLparser.h>
#include <primitives/xml.h>

struct Parser {
    std::vector<Type> types;
    std::vector<Method> methods;

    Parser(const String &s);
    ~Parser();
    void enumerateSectionChildren(const String &name);

private:
    xmlParserCtxtPtr ctx;
    xmlNodePtr root;

    void enumerateSectionChildren(xmlNode *in, const String &name);
    xmlNode *getSection(xmlNode *in, const String &name) const;
};
