#pragma once

#include "parser.h"

struct Emitter
{
    std::map<String, Type> types;
    std::map<String, Type> methods;

    Emitter(const Parser &p);

    String emitTypes();
    void emitMethods() const;
};
