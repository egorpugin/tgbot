#include "emitter.h"

#include <primitives/exceptions.h>
#include <primitives/http.h>

static bool is_simple(const String &t)
{
    return 0
        || t == "String"
        || t == "Integer"
        || t == "Int"
        || t == "Boolean"
        || t == "Float"
        || t == "Float number"
        || t == "True"
        || t == "False"
        ;
}

void Field::save(nlohmann::json &j) const
{
    j["name"] = name;
    for (auto &t : types)
        j["type"].push_back(t);
    if (optional)
        j["optional"] = optional;
    if (array)
        j["array"] = array;
    j["description"] = description;
}

void Field::emitField(primitives::CppEmitter &ctx) const
{
    ctx.addLine("// " + description);
    ctx.addLine();
    emitFieldType(ctx);
    ctx.addText(" " + name + ";");
    ctx.emptyLines();
}

void Field::emitFieldType(primitives::CppEmitter &ctx) const
{
    if (types.empty())
        throw SW_RUNTIME_ERROR("Empty types");

    auto t = types[0];
    auto simple = is_simple(t);
    auto opt = optional && (simple || types.size() != 1);

    if (opt) // we do not need Optional since we have Ptr already
        ctx.addText("Optional<");
    auto a = array;
    while (a--)
        ctx.addText("Vector<");
    if (types.size() > 1)
    {
        ctx.addText("Variant<");
        for (auto &f : types)
            ctx.addText(f + ", ");
        ctx.trimEnd(2);
        ctx.addText(">");
        auto a = array;
        while (a--)
            ctx.addText(">");
    }
    else
    {
        ctx.addText((simple ? "" : "this_namespace::Ptr<") + t + (simple ? "" : ">"));
        auto a = array;
        while (a--)
            ctx.addText(">");
    }
    if (opt)
        ctx.addText(">");
}

void Type::save(nlohmann::json &j) const
{
    j["name"] = name;
    for (auto &f : fields)
    {
        nlohmann::json jf;
        f.save(jf);
        j["fields"].push_back(jf);
    }
    for (auto &f : oneof)
        j["oneof"].push_back(f);
    //if (is_type())
        //j["return_type"] = return_type;
}

void Type::emitType(primitives::CppEmitter &ctx) const
{
    ctx.addLine("// " + description);
    if (!is_oneof())
    {
        ctx.beginBlock("struct " + name);
        ctx.addLine("using Ptr = this_namespace::Ptr<" + name + ">;");
        ctx.emptyLines();
        for (auto &f : fields)
            f.emitField(ctx);
        ctx.endBlock(true);
    }
    else
    {
        ctx.increaseIndent("using " + name + " = Variant<");
        for (auto &f : oneof)
            ctx.addLine(f + ",");
        ctx.trimEnd(1);
        ctx.decreaseIndent(">;");
    }
    ctx.emptyLines();
}

void Type::emitMethodRequestType(primitives::CppEmitter &ctx) const
{
    if (fields.empty())
        return;
    ctx.beginBlock("struct " + name + "Request");
    for (auto &f : fields)
        f.emitField(ctx);
    ctx.endBlock(true);
    ctx.emptyLines();
}

void Type::emitCreateType(primitives::CppEmitter &ctx) const
{
    // from json
    ctx.addLine("template <>");
    ctx.beginFunction(name + " from_json(const nlohmann::json &j)");
    ctx.addLine(name + " v;");
    for (auto &f : fields)
        ctx.addLine("FROM_JSON(" + f.name + ");");
    ctx.addLine("return v;");
    ctx.endFunction();
    ctx.emptyLines();

    // to json
    ctx.addLine("template <>");
    ctx.beginFunction("nlohmann::json to_json(const " + name + " &r)");
    ctx.addLine("nlohmann::json j;");
    for (auto &f : fields)
        ctx.addLine("TO_JSON(" + f.name + ", r." + f.name + ");");
    ctx.addLine("return j;");
    ctx.endFunction();
    ctx.emptyLines();
}

void Type::emitMethod(const Emitter &e, primitives::CppEmitter &h, primitives::CppEmitter &cpp) const
{
    bool has_input_file = false;
    auto get_parameters = [this, &has_input_file](auto &ctx, bool defaults, int last_non_optional = -1)
    {
        for (const auto &[i,f] : enumerate(fields))
        {
            ctx.addLine("const ");
            f.emitFieldType(ctx);
            ctx.addText(" &");
            ctx.addText(f.name);
            if (f.optional && defaults && i > last_non_optional)
                ctx.addText(" = {}");
            ctx.addText(",");
            if (!f.optional && !defaults)
                last_non_optional = i;
            has_input_file |= std::find(f.types.begin(), f.types.end(), "InputFile") != f.types.end();
        }
        if (!fields.empty())
            ctx.trimEnd(1);
        return last_non_optional;
    };

    // before h.
    cpp.addLine();
    return_type.emitFieldType(cpp);
    cpp.addText(" Api::" + name + "(");
    cpp.increaseIndent();
    auto lno = get_parameters(cpp, false);
    cpp.decreaseIndent();
    cpp.addLine(") const");

    // usual call
    h.addLine("// " + description); // second desc to make intellisense happy
    h.addLine();
    return_type.emitFieldType(h);
    h.addText(" " + name + "(");
    if (!fields.empty())
    {
        h.increaseIndent();
        get_parameters(h, true, lno);
        h.decreaseIndent();
        h.addLine();
    }
    h.addText(") const;");
    h.emptyLines();
    // with request struct
    if (!fields.empty())
    {
        h.addLine("// " + description);
        h.addLine();
        return_type.emitFieldType(h);
        h.addText(" " + name + "(const " + name + "Request &) const;");
        h.emptyLines();
    }

    //
    cpp.beginBlock();
    if (has_input_file)
    {
        cpp.addLine("HttpRequestArguments args;");
        cpp.addLine("args.reserve(" + std::to_string(fields.size()) + ");");
        for (auto &f : fields)
            cpp.addLine("TO_REQUEST_ARG(" + f.name + ");");
        cpp.addLine("auto j = SEND_REQUEST(" + name + ", args);");
    }
    else
    {
        cpp.addLine("nlohmann::json j;");
        for (auto &f : fields)
            cpp.addLine("TO_JSON(" + f.name + ", " + f.name + ");");
        cpp.addLine("j = SEND_REQUEST(" + name + ", j.dump());");
    }
    cpp.addLine();
    cpp.addText("return from_json<");
    return_type.emitFieldType(cpp);
    cpp.addLine(">(j);");
    cpp.endBlock();
    cpp.emptyLines();

    // with request struct
    if (!fields.empty())
    {
        cpp.addLine();
        return_type.emitFieldType(cpp);
        cpp.addText(" Api::" + name + "(const " + name + "Request &r) const");
        cpp.beginBlock();
        cpp.addLine("return " + name + "(");
        cpp.increaseIndent();
        for (const auto &f : fields)
            cpp.addLine("r." + f.name + ",");
        cpp.trimEnd(1);
        cpp.decreaseIndent();
        cpp.addLine(");");
        cpp.endBlock();
        cpp.emptyLines();
    }
}

void Type::emitFwdDecl(primitives::CppEmitter &ctx) const
{
    if (!is_oneof())
        ctx.addLine("struct " + name + ";");
    else
        emitType(ctx);
}

Emitter::Emitter(const Parser &p)
{
    for (auto &t : p.types)
        types[t.name] = t;
    for (auto &t : p.methods)
        methods[t.name] = t;
}

String Emitter::emitTypes()
{
    primitives::CppEmitter ctx;
    for (auto &[n, t] : types)
    {
        if (!t.is_oneof())
            t.emitFwdDecl(ctx);
    }
    ctx.emptyLines();
    for (auto &[n, t] : types)
    {
        if (t.is_oneof())
            t.emitFwdDecl(ctx);
    }
    ctx.emptyLines();
    for (auto &[n, t] : types)
    {
        if (!t.is_oneof())
            t.emitType(ctx);
    }
    ctx.emptyLines();
    ctx.addLine("// requests");
    ctx.emptyLines();
    for (auto &[n, m] : methods)
        m.emitMethodRequestType(ctx);
    return ctx.getText();
}

void Emitter::emitMethods() const
{
    primitives::CppEmitter h, cpp;
    for (auto &[n, t] : types)
    {
        cpp.addLine("template <> " + t.name + " from_json<" + t.name + ">(const nlohmann::json &);");
        cpp.addLine("template <> nlohmann::json to_json(const " + t.name + " &);");
        //cpp.emptyLines();
    }
    cpp.emptyLines();
    for (auto &[n, t] : types)
        t.emitCreateType(cpp);
    cpp.emptyLines();
    for (auto &[n,m] : methods)
        m.emitMethod(*this, h, cpp);
    write_file("methods.inl.h", h.getText());
    write_file("methods.inl.cpp", cpp.getText());
}
