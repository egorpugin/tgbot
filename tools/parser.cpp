#include "parser.h"

#include <boost/algorithm/string.hpp>
#include <primitives/exceptions.h>
#include <pystring.h>

#include <iostream>
#include <regex>

// https://core.telegram.org/bots/api

static String prepare_type(const String &t)
{
    if (t == "True" || t == "False")
        return "Boolean";
    if (t == "Float number")
        return "Float";
    if (t == "Int")
        return "Integer";
    if (t == "Messages")
        return "Message";
    return t;
}

static String extract_return_type_sentence(const String &desc)
{
    static std::vector<std::regex> rs{
        std::regex{"An (.*?) is returned\\."},
        std::regex{"Returns (.*?)\\."},
        std::regex{"On success, (.*?)\\."}
    };

    for (auto &&r : rs)
    {
        std::smatch m;
        if (std::regex_search(desc, m, r))
            return m[1].str();
    }
    throw SW_RUNTIME_ERROR("Return type sentence not found: " + desc);
}

static Field extract_return_type(const String &desc)
{
    auto sent = extract_return_type_sentence(desc);

    // find words starting with capitals
    Field f;
    std::vector<String> capital_words;
    for (auto &&w : split_string(sent, " "))
    {
        if (!isupper(w[0]))
            continue;
        if (w == "Array")
            f.array++;
        else
            capital_words.push_back(w);
    }

    if (capital_words.empty())
        throw SW_LOGIC_ERROR("check capitals logic for: " + sent);

    f.types.push_back(*capital_words.begin());
    f.types[0] = prepare_type(f.types[0]);
    return f;
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
                    t.return_type = extract_return_type(t.description);

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

                // chat_id field is always string in form @chat_id
                if (t.name == "BotCommandScopeChat" || t.name == "BotCommandScopeChatAdministrators" || t.name == "BotCommandScopeChatMember")
                {
                    t.fields[1].types = {"String"};
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

        if (!f.description.empty())
        {
            boost::replace_all(f.description, "\xe2\x80\x9c", "\"");
            boost::replace_all(f.description, "\xe2\x80\x9d", "\"");

            auto p = f.description.find(" one of ");
            if (p == f.description.npos)
                p = f.description.find("One of ");
            if (p != f.description.npos)
            {
                auto end = f.description.find(".", p);
                auto sub = f.description.substr(p, end - p);
                p = 0;
                while (1)
                {
                    p = sub.find("\"", p);
                    if (p == sub.npos)
                        break;
                    end = sub.find("\"", p + 1);
                    auto s = sub.substr(p + 1, end - p - 1);
                    if (!s.empty())
                    {
                        boost::replace_all(s, "/", "_");
                        f.enum_values[s] = s;
                    }
                    p = end + 1;
                }
            }

            std::regex r{ ", always \"(.*?)\"" };
            std::smatch m;
            if (std::regex_search(f.description, m, r)) {
                f.always = m[1].str();
            }
        }

        if (f.name == "parse_mode")
        {
            f.types[0] = "ParseMode";
            f.enum_ = true;
        }
        else if (
            t.name == "Dice" && f.name == "emoji" ||
            t.name == "sendDice" && f.name == "emoji"
            )
        {
            f.types[0] = "DiceEmoji";
            f.enum_ = true;
        }

        t.fields.push_back(f);
    }
}
