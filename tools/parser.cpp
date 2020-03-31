#include "parser.h"

#include <pystring.h>

#include <iostream>
#include <regex>

// https://core.telegram.org/bots/api

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

                if (t.fields.empty() && t.name == "InputFile")
                {
                    Field fn;
                    fn.name = "file_name";
                    fn.types.push_back("String");
                    fn.description = "Local file name of file to upload.";
                    t.fields.push_back(fn);

                    Field mt;
                    mt.name = "mime_type";
                    mt.types.push_back("String");
                    mt.description = "MIME type of file to upload.";
                    t.fields.push_back(mt);
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
