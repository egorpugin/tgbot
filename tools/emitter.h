#pragma once

#include "parser.h"

struct Emitter {
    std::map<String, Type> types;
    std::map<String, Method> methods;
    std::map<String, std::map<String, String>> enums;

    Emitter(const Parser &p);

    void emitTypesHeader();
    String emitReflection() const;
    void emitTypesCpp();
    String emitMethods() const;
};
