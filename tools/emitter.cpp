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

std::vector<String> Field::get_dependent_types() const
{
    std::vector<String> t;
    for (auto &type : types)
    {
        if (!is_simple(type))
            t.push_back(type);
    }
    return t;
}

void Field::emitField(primitives::CppEmitter &ctx) const
{
    ctx.addLine("// " + description);
    ctx.addLine();
    emitFieldType(ctx);
    ctx.addText(" " + name + ";");
    ctx.emptyLines();
}

void Field::emitFieldType(primitives::CppEmitter &ctx, bool emitoptional, bool return_type) const
{
    if (types.empty())
        throw SW_RUNTIME_ERROR("Empty types");

    auto t = types[0];
    auto simple = is_simple(t);
    auto opt = optional && (simple || types.size() != 1);

    // we cannot replace all ptrs with optionals, because we have type deps
    // we need separate headers or something like this for deps

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
        auto type = emitoptional ? "Optional<" : "this_namespace::Ptr<";
        ctx.addText((simple || return_type ? "" : type) + t + (simple || return_type ? "" : ">"));
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
    j["description"] = description;
    return_type.save(j["return_type"]);
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

std::vector<String> Type::get_dependent_types() const
{
    std::vector<String> t;
    for (auto &f : fields)
    {
        auto dt = f.get_dependent_types();
        t.insert(t.end(), dt.begin(), dt.end());
    }
    return t;
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
        ctx.emptyLines();
        ctx.addLine("//");
        ctx.addLine(name + "() = default;");
        ctx.addLine(name + "(const std::string &); // from json string");
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

void Type::emitTypeCpp(primitives::CppEmitter &ctx) const
{
    ctx.addLine(name + "::" + name + "(const std::string &s) : " +
        name + "{from_json<" + name + ">(nlohmann::json::parse(s))} {}");
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

void Type::emitMethod(const Emitter &e, primitives::CppEmitter &h, primitives::CppEmitter &cpp) const
{
    bool has_input_file = false;
    auto get_parameters = [this, &has_input_file](auto &ctx, bool defaults, int last_non_optional)
    {
        for (const auto &[i,f] : enumerate(fields))
        {
            ctx.addLine("const ");
            f.emitFieldType(ctx);
            ctx.addText(" &");
            ctx.addText(f.name);
            if (f.optional && static_cast<int>(i) > last_non_optional && defaults)
                ctx.addText(" = {}");
            ctx.addText(",");
            has_input_file |= std::find(f.types.begin(), f.types.end(), "InputFile") != f.types.end();
        }
        if (!fields.empty())
            ctx.trimEnd(1);
    };

    int last_non_optional = -1;
    for (const auto &[i,f] : enumerate(fields))
    {
        if (!f.optional)
            last_non_optional = i;
    }

    auto emit_header = [this, &h, &get_parameters, &last_non_optional](const String &suffix = {}) {
        // usual call
        h.addLine("// " + description); // second desc to make intellisense happy
        h.addLine();
        return_type.emitFieldType(h, true, true);
        h.addText(" " + name + suffix + "(");
        if (!fields.empty())
        {
            h.increaseIndent();
            get_parameters(h, true, last_non_optional);
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
            return_type.emitFieldType(h, true, true);
            h.addText(" " + name + suffix + "(const " + name + "Request &) const;");
            h.emptyLines();
        }
    };
    emit_header();
    //emit_header("Raw");

    // usual call
    cpp.addLine();
    return_type.emitFieldType(cpp, true, true);
    cpp.addText(" api::" + name + "(");
    cpp.increaseIndent();
    get_parameters(cpp, false, last_non_optional);
    cpp.decreaseIndent();
    cpp.addLine(") const");
    cpp.beginBlock();
    if (has_input_file)
    {
        cpp.addLine("std::array<http_request_argument, " + std::to_string(fields.size()) + "> args;");
        cpp.addLine("auto i = args.begin();");
        for (auto &f : fields)
            cpp.addLine("TO_REQUEST_ARG(" + f.name + ");");
        cpp.addLine("auto j = SEND_REQUEST2(" + name + ");");
    }
    else
    {
        cpp.addLine("nlohmann::json j;");
        for (auto &f : fields)
            cpp.addLine("TO_JSON2(" + f.name + ");");
        cpp.addLine("j = SEND_REQUEST(" + name + ", j.dump());");
    }
    cpp.addLine();
    cpp.addText("return from_json<");
    return_type.emitFieldType(cpp, true, true);
    cpp.addText(">(j);");
    cpp.endBlock();
    cpp.emptyLines();

    // with request struct
    if (!fields.empty())
    {
        cpp.addLine();
        return_type.emitFieldType(cpp, true, true);
        cpp.addText(" api::" + name + "(const " + name + "Request &r) const");
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

void Emitter::emitTypesHeader()
{
    primitives::CppEmitter ctx;
    ctx.addLine("#pragma once");
    ctx.addLine();
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
    write_file("types.inl.h", ctx.getText());
}

void Emitter::emitTypesCpp()
{
    primitives::CppEmitter ctx;
    for (auto &[n, t] : types)
    {
        if (
            n == "InputMediaAnimation" ||
            n == "InputMediaAudio" ||
            n == "InputMediaDocument" ||
            n == "InputMediaVideo"
            )
            continue;
        if (!t.is_oneof())
            t.emitTypeCpp(ctx);
    }
    write_file("types.inl.cpp", ctx.getText());
}

void Emitter::emitTypesSeparate()
{
    primitives::CppEmitter tctx;
    for (auto &[n, t] : types)
    {
        if (!t.is_oneof())
            t.emitFwdDecl(tctx);
    }
    tctx.emptyLines();
    for (auto &[n, t] : types)
    {
        if (t.is_oneof())
            t.emitFwdDecl(tctx);
    }
    tctx.emptyLines();
    //
    for (auto &[n, t] : types)
    {
        primitives::CppEmitter ctx;
        ctx.addLine("#pragma once");
        ctx.addLine();
        /*if (auto dtypes = t.get_dependent_types(); !dtypes.empty())
        {
            for (auto &dt : dtypes)
                ctx.addLine("#include \"" + types[dt].get_file_name() + "\"");
            ctx.addLine();
        }*/
        if (!t.is_oneof())
            t.emitType(ctx);
        ctx.emptyLines();
        write_file(t.get_file_name(), ctx.getText());
        tctx.addLine("#include \"" + t.get_file_name() + "\"");
    }
    tctx.emptyLines();
    for (auto &[n, m] : methods)
    {
        primitives::CppEmitter ctx;
        ctx.addLine("#pragma once");
        ctx.addLine();
        m.emitMethodRequestType(ctx);
        write_file(m.get_request_file_name(), ctx.getText());
        tctx.addLine("#include \"" + m.get_request_file_name() + "\"");
    }
    write_file("types.inl.h", tctx.getText());
}

void Emitter::emitMethods() const
{
    primitives::CppEmitter h, cpp;
    for (auto &[n,m] : methods)
        m.emitMethod(*this, h, cpp);
    write_file("methods.inl.h", h.getText());
    write_file("methods.inl.cpp", cpp.getText());
}

void Emitter::emitReflection() const
{
    primitives::CppEmitter ctx;
    ctx.addLine("#pragma once");
    ctx.addLine();
    ctx.addLine("namespace tgbot {");
    ctx.addLine();
    ctx.addLine("template <typename T> struct refl;");
    ctx.addLine();
    // refl
    for (auto &[n, t] : types)
    {
        ctx.addLine("template <> struct refl<" + t.name + "> {");
        ctx.increaseIndent();
        ctx.addLine("using T = " + t.name + ";");
        ctx.addLine();
        //ctx.addLine("static constexpr auto size() { return " + std::to_string(t.fields.size()) + "; }");
        //ctx.addLine();
        ctx.addLine("template <typename F>");
        ctx.addLine("static void for_each(F &&f) {");
        ctx.increaseIndent();
        for (auto &f : t.fields)
            ctx.addLine("f(\"" + f.name + "\", &T::" + f.name + ");");
        ctx.endBlock();
        ctx.endBlock(true);
        ctx.emptyLines();
    }
    ctx.endNamespace("tgbot");
    write_file("reflection.inl.h", ctx.getText());
}
