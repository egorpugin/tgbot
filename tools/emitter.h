#pragma once

#include "parser.h"

struct Emitter
{
    std::map<String, Type> types;
    std::map<String, Type> methods;

    Emitter(const Parser &p);

    void emitTypesHeader();
    void emitTypesCpp();
    void emitTypesSeparate();
    void emitMethods() const;
    void emitReflection() const;
};
