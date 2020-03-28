#include "api_tool.h"

#include <primitives/sw/main.h>
#include <primitives/sw/cl.h>
#include <pystring.h>

#include <iostream>
#include <regex>

static String prepare_type(String t)
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

static String get_pb_type(String t, bool optional)
{
    //if (!optional)
    {
        if (t == "Integer")
            return "int32";
        if (t == "Boolean")
            return "bool";
        if (t == "Float")
            return "float";
        if (t == "String")
            return "string";
    }
    return t;
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
    auto opt = optional && simple;

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

void Type::emitCreateType(primitives::CppEmitter &ctx) const
{
    // from json
    ctx.addLine("template <>");
    ctx.beginFunction("void fromJson(const nlohmann::json &j, " + name + " &v)");
    for (auto &f : fields)
        ctx.addLine("FROM_JSON(" + f.name + ", v." + f.name + ");");
    ctx.endFunction();
    ctx.emptyLines();

    // to json
    ctx.addLine("template <>");
    ctx.beginFunction("nlohmann::json toJson(const " + name + " &r)");
    ctx.addLine("nlohmann::json j;");
    for (auto &f : fields)
        ctx.addLine("TO_JSON(" + f.name + ", r." + f.name + ");");
    ctx.addLine("return j;");
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
    cpp.addLine("nlohmann::json j;");
    for (auto &f : fields)
        cpp.addLine("TO_JSON(" + f.name + ", " + f.name + ");");
    cpp.addLine("j = SEND_REQUEST(" + name + ");");
    cpp.addLine();
    return_type.emitFieldType(cpp);
    cpp.addText(" r;");
    cpp.addLine("fromJson(j, r);");
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

void Type::emitProtobuf(primitives::CppEmitter &ctx) const
{
    bool is_method = !is_type();

    ctx.increaseIndent("message " + name + (is_method ? "Request" : "") + " {");
    int i = 0;
    for (const auto &f : fields)
    {
        if (f.types.size() > 1)
        {
            ctx.increaseIndent("oneof " + f.name + " {");
            for (auto &t : f.types)
            {
                auto t2 = get_pb_type(t, false);
                ctx.addLine(t2 + " " + f.name + "_" + t2 + " = " + std::to_string(i + 1) + ";");
                i++;
            }
            ctx.decreaseIndent("}");
        }
        else
        {
            auto t = get_pb_type(f.types[0], f.optional);

            auto a = f.array;
            bool ar = a > 1;
            while (a > 1)
            {
                a--;
                auto t2 = t + "_Repeated" + std::to_string(a);
                ctx.increaseIndent("message " + t2 + " {");
                ctx.addLine("repeated " + t + " " + f.name + " = 1;");
                ctx.decreaseIndent("}");
            }

            ctx.addLine();
            if (a > 0 || ar)
            {
                ctx.addText("repeated ");
                if (ar)
                    t = t + "_Repeated1";
            }
            ctx.addText(t + " " + f.name + " = " + std::to_string(i + 1) + ";");
            i++;
        }
    }
    ctx.decreaseIndent("}");
    ctx.emptyLines();
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
                        t.return_type.types[0] = prepare_type(t.return_type.types[0]);
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
        for (auto &t : f.types)
            t = prepare_type(t);

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
    {
        cpp.addLine("template <>");
        cpp.addLine("void fromJson<" + t.name + ">(const nlohmann::json &, " + t.name + " &);");
        cpp.addLine("template <>");
        cpp.addLine("nlohmann::json toJson(const " + t.name + " &);");
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

String Emitter::emitProtobuf() const
{
    primitives::CppEmitter ctx;
    ctx.addLine("syntax = \"proto3\";");
    ctx.addLine();
    ctx.addLine("package TgBot.api;");
    ctx.addLine();

    // add builtin types
    ctx.addLine(R"(message Integer {
    int32 integer = 1;
}

message Float {
    float float = 1;
}

message Boolean {
    bool boolean = 1;
}

message String {
    string string = 1;
}
)");

    for (auto &[n, t] : types)
        t.emitProtobuf(ctx);
    for (auto &[n, t] : methods)
        t.emitProtobuf(ctx);
    return ctx.getText();
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

    // pb impl won't work because of vector<vector<>> types :(
    write_file("tgapi.proto", e.emitProtobuf());

    return 0;
}
