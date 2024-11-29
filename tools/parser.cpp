#include "parser.h"

#include <boost/algorithm/string.hpp>
#include <primitives/exceptions.h>
#include <primitives/overload.h>
#include <pystring.h>

#include <iostream>
#include <regex>

// https://core.telegram.org/bots/api

static String prepare_type(const String &t) {
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

static String extract_return_type_sentence(const String &desc) {
    static std::vector<std::regex> rs{
        std::regex{"An (.*?) is returned\\."},
        std::regex{"On success, (.*?)\\."},
        std::regex{"On success, returns a (.*?)\\."},
        std::regex{"On success, returns an (.*?)\\."},
        std::regex{"returns a (.*?)\\."},
        std::regex{"returns an (.*?)\\."},
        std::regex{"Returns (.*?)\\."},
    };

    for (auto &&r: rs) {
        std::smatch m;
        if (std::regex_search(desc, m, r))
            return m[1].str();
    }
    throw SW_RUNTIME_ERROR("Return type sentence not found: " + desc);
}

static Field extract_return_type(const String &desc) {
    auto sent = extract_return_type_sentence(desc);

    // find words starting with capitals
    Field f;
    std::vector<String> capital_words;
    for (auto &&w: split_string(sent, " ")) {
        if (!isupper(w[0]))
            continue;
        if (w == "Array")
            f.array++;
        else
            capital_words.push_back(w);
    }

    if (capital_words.empty()) {
        auto p = desc.find(sent);
        if (p == -1) {
            throw SW_LOGIC_ERROR("check capitals logic for: " + sent);
        }
        return extract_return_type(desc.substr(p + sent.size()));
    }

    f.types.push_back(*capital_words.begin());
    f.types[0] = prepare_type(f.types[0]);
    return f;
}

static String getAllText1(xmlNode *in) {
    String s;
    if (getName(in) == "text") {
        s += getContent(in);
    }
    if (getName(in) == "img") {
        if (in->properties && in->properties->children && (const char *)in->properties->children->content == "emoji"s) {
            s += (const char *)in->properties->next->next->next->next->children->content;
        }
    }
    if (in->children)
        s += getAllText1(in->children);
    if (in->next)
        s += getAllText1(in->next);
    return s;
}
String getAllText(xmlNode *in) {
    auto text = getAllText1(in);
    // we can't do all replacements here because we rely on the in different places
    return text;
}

static void parseTypeOneOf(auto &t, xmlNode *ul) {
    while (ul = getNext(ul, "li")) {
        if (ul->children)
            t.oneof.push_back(getAllText(ul->children));
    }
}

static void parseType(auto &t, xmlNode *tb) {
    tb = getNext(tb, "tbody");
    for (auto b = getNext(tb, "tr"); b; b = getNext(b, "tr")) {
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
        while (tt.find(ao) == 0) {
            f.array++;
            tt = tt.substr(strlen(ao));
        }
        pystring::split(tt, f.types, " or ");
        if (f.types.size() == 1)
            pystring::split(tt, f.types, " and ");
        for (auto &t: f.types)
            t = prepare_type(t);

        auto comment = getNext(field_type, "td");
        checkNullptr(comment);
        overload(
            [&](Type &t) {
                if (comment->children && getName(comment->children) == "em" && comment->children->children &&
                    getName(comment->children->children) == "text" &&
                    getContent(comment->children->children) == "Optional")
                    f.optional = true;
                if (comment->children) {
                    f.description.parse(comment->children);
                }
            },
            [&](Method &t) {
                if (comment->children && getName(comment->children) == "text" && comment->children->content &&
                    getContent(comment->children) == "Optional")
                    f.optional = true;
                auto desc = getNext(comment, "td");
                if (desc && desc->children) {
                    f.description.parse(desc->children);
                }
            })(t);

        if (!f.description.text.empty()) {
            auto &text = f.description.text;
            prepare_desc(text);

            auto p = text.find(" one of \"");
            if (p == text.npos) {
                p = text.find("One of \"");
            }
            if (p != text.npos) {
                auto end = text.find(".", p);
                auto sub = text.substr(p);
                p = 0;
                while (1) {
                    p = sub.find("\"", p);
                    if (p == sub.npos)
                        break;
                    end = sub.find("\"", p + 1);
                    auto s = sub.substr(p + 1, end - p - 1);
                    if (!s.empty()) {
                        boost::replace_all(s, "/", "_");
                        auto enum_key = s;
                        auto enum_value = f.description.enum_values_map.empty() ? s : f.description.enum_values_map[s];
                        if (enum_key == "static")
                            enum_key += "_";
                        f.enum_values[enum_key] = enum_value;
                    }
                    p = end + 1;
                }
            }

            std::regex r{", always \"(.*?)\""};
            std::smatch m;
            if (std::regex_search(text, m, r)) {
                f.always = m[1].str();
            }

            // also add 'defaults to'
        }

        if (f.name == "parse_mode") {
            f.types[0] = "ParseMode";
            f.enum_ = true;
        } else if (
                t.name == "Dice" && f.name == "emoji" ||
                t.name == "sendDice" && f.name == "emoji"
                ) {
            f.types[0] = "DiceEmoji";
            f.enum_ = true;
        }

        t.fields.push_back(f);
    }
}


Parser::Parser(const String &s) {
    ctx = htmlNewParserCtxt();
    auto doc = htmlCtxtReadMemory(ctx, s.c_str(), s.size(), nullptr, nullptr,
                                  XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    root = xmlDocGetRootElement(doc);
    if (!root)
        throw SW_RUNTIME_ERROR("Invalid document");
}

Parser::~Parser() {
    htmlFreeParserCtxt(ctx);
}

void Parser::enumerateSectionChildren(const String &name) {
    enumerateSectionChildren(root, name);
}

void Parser::enumerateSectionChildren(xmlNode *in, const String &name) {
    auto n = getSection(in, name);
    checkNullptr(n);
    do {
        SCOPE_EXIT{n = n->next;};
        auto good_node = getName(n) == "h4" && n->children && n->children->next && n->children->next->type == XML_TEXT_NODE;
        if (!good_node) {
            continue;
        }
        auto name = getContent(n->children->next);
        auto is_type = isupper(name[0]);
        // skip other headers
        if (name.find(" ") != name.npos) {
            continue;
        }
        auto nt = n->next;
        decltype(nt) tb = nullptr;
        decltype(nt) ul = nullptr;
        decltype(nt) p = nullptr;
        while (nt) {
            if (getName(nt) == "h4") {
                break;
            } else if (getName(nt) == "table") {
                tb = nt;
                break;
            } else if (getName(nt) == "ul") {
                ul = nt;
                break;
            } else if (!p && getName(nt) == "p") {
                p = nt;
            }
            nt = nt->next;
        }

        if (is_type) {
            Type t;
            t.name = name;
            if (p)
                t.description = getAllText(p->children);
            if (ul)
                parseTypeOneOf(t, ul->children);
            if (tb)
                parseType(t, tb);
            if (t.fields.empty() && t.name == "InputFile") {
                Field fn;
                fn.name = "file_name";
                fn.types.push_back("String");
                fn.description.text = "Local file name of file to upload.";
                t.fields.push_back(fn);

                Field mt;
                mt.name = "mime_type";
                mt.types.push_back("String");
                mt.description.text = "MIME type of file to upload.";
                t.fields.push_back(mt);
            }
            prepare_desc(t.description);
            types.push_back(t);
        } else {
            Method t;
            t.name = name;
            if (p)
                t.description = getAllText(p->children);
            t.return_type = extract_return_type(t.description);
            if (tb)
                parseType(t, tb);
            // sometimes optional fields won't be last in api spec
            std::stable_sort(t.fields.begin(), t.fields.end(), [](auto &&f1, auto &&f2){
                return f1.optional < f2.optional;
            });
            prepare_desc(t.description);
            methods.push_back(t);
        }
    } while (n && getName(n) != "h3");
}

xmlNode *Parser::getSection(xmlNode *in, const String &name) const {
    auto n = getNext(in, "h3");
    checkNullptr(n);
    auto a = getNext(n, "a");
    if (a) {
        auto p = a->properties;
        while (p) {
            if (getName(p) == "name" &&
                p->children && getName(p->children) == "text" &&
                getContent(p->children) == name)
                return n;
            p = p->next;
        }
    }
    return getSection(n, name);
}
