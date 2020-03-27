/*
c++: 17
deps:
    - pub.egorpugin.primitives.emitter-master
    - pub.egorpugin.primitives.http-master
    - pub.egorpugin.primitives.xml-master
    - pub.egorpugin.primitives.sw.main-master
    - org.sw.demo.nlohmann.json
    - org.sw.demo.imageworks.pystring
*/

#include "api_tool.h"

#include <primitives/sw/main.h>
#include <primitives/sw/cl.h>
#include <pystring.h>

#include <iostream>
#include <regex>

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

    if (optional)
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
        if (optional)
            ctx.addText(">");
    }
    else
    {
        auto t = types[0];
        auto simple = is_simple();
        t = prepare_type(t);
        ctx.addText((simple ? "" : "this_namespace::Ptr<") + t + (simple ? "" : ">"));
        auto a = array;
        while (a--)
            ctx.addText(">");
        if (optional)
            ctx.addText(">");
    }
}

bool Field::is_simple() const
{
    auto &t = types[0];
    return t == "String" || t == "Integer" || t == "Int" || t == "Boolean" || t == "Float" || t == "Float number" ||
           t == "True" || t == "False";
}

String Field::prepare_type(String t)
{
    if (t == "True" || t == "False")
        t = "Boolean";
    if (t == "Float number")
        t = "Float";
    if (t == "Int")
        t = "Integer";
    if (t == "Messages")
        t = "Message";
    return t;
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

void Type::emitCreateType(primitives::CppEmitter &ctx) const
{
    ctx.beginFunction("static Ptr<" + name + "> create" + name + "(const nlohmann::json &j)");
    ctx.addLine("auto r = createPtr<" + name + ">();");
    for (auto &f : fields)
    {
        if (f.is_simple() && f.array == 0)
            ctx.addLine("GET_SIMPLE_FIELD(" + f.name + ");");
        else if (f.array)
        {
            //if (f.optional)
                ctx.addLine("throw std::runtime_error(\"ni\"); // array: " + f.name);
            //else
                //ctx.addLine("GET_ARRAY(" + f.name + ", " + f.prepare_type(f.types[0]) + ");");
        }
        else if (f.types.size() > 1)
            ctx.addLine("throw std::runtime_error(\"ni\"); // variant: " + f.name);
        else
            ctx.addLine("GET_FIELD(" + f.name + ", " + f.prepare_type(f.types[0]) + ");");
    }
    ctx.addLine("return r;");
    ctx.endFunction();
    ctx.emptyLines();
}

void Type::emitMethod(const Emitter &e, primitives::CppEmitter &h, primitives::CppEmitter &cpp) const
{
    h.addLine();
    cpp.addLine();

    if (!return_type.types.empty())
    {
        return_type.emitFieldType(h);
        return_type.emitFieldType(cpp);
    }

    auto get_parameters = [this](auto &ctx, bool defaults, int last_non_optional = -1)
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
        }
        if (!fields.empty())
            ctx.trimEnd(1);
        return last_non_optional;
    };

    cpp.addText(" Api::" + name + "(");
    cpp.increaseIndent();
    auto lno = get_parameters(cpp, false);
    cpp.decreaseIndent();
    cpp.addLine(") const");

    h.addText(" " + name + "(");
    h.increaseIndent();
    get_parameters(h, true, lno);
    h.decreaseIndent();
    h.addLine(") const;");
    h.emptyLines();

    cpp.beginBlock();
    cpp.addLine("std::vector<HttpReqArg> args;");
    cpp.addLine("args.reserve(" + std::to_string(fields.size()) + ");");
    for (auto &f : fields)
    {
        if (f.optional)
            cpp.increaseIndent("if (" + f.name + ")");
        if (f.types.size() > 1)
            cpp.addLine("throw std::runtime_error(\"ni\"); // variant");
        else
        {
            if (f.array)
                cpp.addLine("throw std::runtime_error(\"ni\"); // array");
            else
                cpp.addLine("args.emplace_back(\"" + f.name + "\", " + (f.optional ? "*" : "") + f.name + ");");
        }
        if (f.optional)
            cpp.decreaseIndent();
    }
    cpp.addLine("auto j = sendRequest(\"" + name + "\", args);");
    cpp.addLine();
    return_type.emitFieldType(cpp);
    cpp.addText(" r");
    if (return_type.array == 0)
    {
        cpp.addText(" = ");
        if (return_type.is_simple())
            cpp.addText("{}");
        else
            cpp.addText("create" + return_type.prepare_type(return_type.types[0]) + "(j)");
    }
    cpp.addText(";");
    if (return_type.array)
        cpp.addLine("GET_RETURN_VALUE_ARRAY(" + return_type.prepare_type(return_type.types[0]) + ");");
    cpp.addLine("return r;");
    cpp.endBlock();
    cpp.emptyLines();
}

void Type::emitFwdDecl(primitives::CppEmitter &ctx) const
{
    if (!is_oneof())
        ctx.addLine("struct " + name + ";");
    else
        emitType(ctx);
}

Parser::Parser(const String &s)
{
    ctx = htmlNewParserCtxt();
    auto doc = htmlCtxtReadMemory(ctx, s.c_str(), s.size(), nullptr, nullptr,
        XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    root = xmlDocGetRootElement(doc);
    if (!root)
        throw SW_RUNTIME_ERROR("Invalid document");
}

Parser::~Parser()
{
    htmlFreeParserCtxt(ctx);
}

void Parser::enumerateSectionChildren(const String &name)
{
    enumerateSectionChildren(root, name);
}

void Parser::enumerateSectionChildren(xmlNode *in, const String &name)
{
    auto n = getSection(in, name);
    checkNullptr(n);
    do
    {
        if (getName(n) == "h4" && n->children && n->children->next && n->children->next->type == XML_TEXT_NODE)
        {
            Type t;
            t.name = getContent(n->children->next);
            if (t.name.find(" ") == t.name.npos) // skip other headers
            {
                auto nt = n->next;
                decltype(nt) tb = nullptr;
                decltype(nt) ul = nullptr;
                decltype(nt) p = nullptr;
                while (nt)
                {
                    if (getName(nt) == "h4")
                        break;
                    else if (getName(nt) == "table")
                    {
                        tb = nt;
                        break;
                    }
                    else if (getName(nt) == "ul")
                    {
                        ul = nt;
                        break;
                    }
                    else if (!p && getName(nt) == "p")
                    {
                        p = nt;
                    }
                    nt = nt->next;
                }
                checkNullptr(p); // there is return type in desc
                t.description = getAllText(p->children);
                if (ul)
                    parseTypeOneOf(t, ul->children);
                if (tb)
                    parseType(t, tb);

                // return type
                if (!t.is_type())
                {
                    static std::regex r1("On success.*?(\\w+) object is returned");
                    static std::regex r2("On success.*?(\\w+) is returned");
                    static std::regex r3("[Rr]eturns.*?(\\w+) object");
                    static std::regex r4("[Rr]eturns.*?(\\w+) on success");
                    static std::regex r5(".*?Array of (\\w+) objects");
                    static std::regex r6("[Rr]eturns.*?of a (\\w+) object");
                    static std::regex r7("On success.*?returns the edited (\\w+)");
                    static std::regex r8("On success, the stopped (\\w+)");
                    std::smatch m;
                    if (0
                        || std::regex_search(t.description, m, r8)
                        || std::regex_search(t.description, m, r1)
                        || std::regex_search(t.description, m, r2)
                        || std::regex_search(t.description, m, r3)
                        || std::regex_search(t.description, m, r4)
                        || std::regex_search(t.description, m, r5)
                        || std::regex_search(t.description, m, r6)
                        || std::regex_search(t.description, m, r7)
                        )
                    {
                        t.return_type.types.push_back(m[1].str());
                        std::smatch m2;
                        if (std::regex_search(t.description, m2, r5))
                            t.return_type.array++;
                    }
                }

                (t.is_type() ? types : methods).push_back(t);
            }
        }
        n = n->next;
    } while (n && getName(n) != "h3");
}

xmlNode *Parser::getSection(xmlNode *in, const String &name) const
{
    auto n = getNext(in, "h3");
    checkNullptr(n);
    auto a = getNext(n, "a");
    if (a)
    {
        auto p = a->properties;
        while (p)
        {
            if (getName(p) == "name" &&
                p->children && getName(p->children) == "text" &&
                getContent(p->children) == name)
                return n;
            p = p->next;
        }
    }
    return getSection(n, name);
}

String Parser::getAllText(xmlNode *in) const
{
    String s;
    if (getName(in) == "text")
        s += getContent(in);
    if (in->children)
        s += getAllText(in->children);
    if (in->next)
        s += getAllText(in->next);
    return s;
}

void Parser::parseTypeOneOf(Type &t, xmlNode *ul) const
{
    while (ul = getNext(ul, "li"))
    {
        if (ul->children)
            t.oneof.push_back(getAllText(ul->children));
    }
}

void Parser::parseType(Type &t, xmlNode *tb) const
{
    tb = getNext(tb, "tbody");
    for (auto b = getNext(tb, "tr"); b; b = getNext(b, "tr"))
    {
        Field f;

        auto field_name = getNext(b, "td");
        checkNullptr(field_name);
        checkNullptr(field_name->children->content);
        f.name = getContent(field_name->children);

        auto field_type = getNext(field_name, "td");
        checkNullptr(field_type);
        checkNullptr(field_type->children);
        auto tt = getAllText(field_type->children);
        auto ao = "Array of ";
        while (tt.find(ao) == 0)
        {
            f.array++;
            tt = tt.substr(strlen(ao));
        }
        pystring::split(tt, f.types, " or ");
        if (f.types.size() == 1)
            pystring::split(tt, f.types, " and ");

        auto comment = getNext(field_type, "td");
        checkNullptr(comment);
        if (t.is_type())
        {
            if (comment->children && getName(comment->children) == "em" &&
                comment->children->children && getName(comment->children->children) == "text" &&
                getContent(comment->children->children) == "Optional")
                f.optional = true;
            if (comment->children)
                f.description = getAllText(comment->children);
        }
        else
        {
            if (comment->children && getName(comment->children) == "text" &&
                comment->children->content && getContent(comment->children) == "Optional")
                f.optional = true;
            auto desc = getNext(comment, "td");
            if (desc && desc->children)
                f.description = getAllText(desc->children);
        }

        t.fields.push_back(f);
    }
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
    return ctx.getText();
}

void Emitter::emitMethods() const
{
    primitives::CppEmitter h, cpp;
    for (auto &[n, t] : types)
        cpp.addLine("static Ptr<" + t.name + "> create" + t.name + "(const nlohmann::json &j);");
    cpp.emptyLines();
    for (auto &[n, t] : types)
        t.emitCreateType(cpp);
    cpp.emptyLines();
    for (auto &[n,m] : methods)
        m.emitMethod(*this, h, cpp);
    write_file("methods.inl.h", h.getText());
    write_file("methods.inl.cpp", cpp.getText());
}

int main(int argc, char **argv)
{
    cl::opt<String> target(cl::Positional, cl::Required, cl::desc("Html file or url to parse"));
    cl::opt<path> json("j", cl::desc("Output api as json file"));
    cl::ParseCommandLineOptions(argc, argv);

    std::clog << "Trying to parse " << target << "\n";
    Parser p(read_file_or_download_file(target));

    p.enumerateSectionChildren("getting-updates");
    p.enumerateSectionChildren("available-types");
    p.enumerateSectionChildren("available-methods");
    p.enumerateSectionChildren("updating-messages");
    p.enumerateSectionChildren("stickers");
    p.enumerateSectionChildren("inline-mode");
    p.enumerateSectionChildren("payments");
    p.enumerateSectionChildren("telegram-passport");
    p.enumerateSectionChildren("games");

    // json
    {
        nlohmann::json j;
        for (auto &t : p.types)
            t.save(j["types"][t.name]);
        for (auto &t : p.methods)
            t.save(j["methods"][t.name]);
        if (!json.empty())
            write_file(json, j.dump(2));
        //else
        //std::cout << j.dump(2) << "\n";
    }

    Emitter e(p);

    write_file("types.inl.h", e.emitTypes());
    e.emitMethods();

    return 0;
}
