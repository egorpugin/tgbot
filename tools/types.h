#pragma once

#include <nlohmann/json.hpp>
#include <primitives/emitter.h>
#include <primitives/xml.h>

struct Emitter;

String getAllText(xmlNode *in);

void prepare_desc(auto &text) {
    boost::replace_all(text, "“", "\"");
    boost::replace_all(text, "”", "\"");
}

struct Field {
    struct desc {
        xmlNode *node;
        std::string text;
        std::string display_text;
        std::map<String, String> enum_values_map;
        std::set<String> enum_values_keys;
        int idx{};

        String getAllText(xmlNode *in) {
            String s;
            if (getName(in) == "text") {
                s += getContent(in);
            }
            if (getName(in) == "img") {
                if (in->properties && in->properties->children &&
                    (const char *)in->properties->children->content == "emoji"s) {
                    s += (const char *)in->properties->next->next->next->next->children->content;
                    if (std::ranges::any_of(s, [](auto c){return c < 32 || c > 127;})) {
                        if (enum_values_keys.insert(s).second) {
                            auto val = s;
                            s = std::format("value_{}", ++idx);
                            enum_values_map[s] = val;
                        } else {
                            s = enum_values_map[s];
                        }
                    }
                }
            }
            if (in->children)
                s += getAllText(in->children);
            if (in->next)
                s += getAllText(in->next);
            return s;
        }
        void parse(xmlNode *in) {
            text = getAllText(in);
            display_text = ::getAllText(in);

            prepare_desc(text);
            prepare_desc(display_text);
        }
    };

    String name;
    std::vector<String> types;
    desc description;
    bool optional = false;
    int array = 0;
    std::map<String, String> enum_values;
    bool enum_ = false;
    String always;

    void save(nlohmann::json &j) const;
    void emitField(primitives::CppEmitter &ctx, bool emitoptional = false) const;
    void emitFieldType(primitives::CppEmitter &ctx, bool emitoptional = false, bool return_type = false, const String &parent_type = {}) const;
    std::vector<String> get_dependent_types() const;
    bool is_enum() const;
    String get_enum_name() const;
    String get_enum_type(const String &parent_type = {}) const;
};

struct basic_info {
    String name;
    std::vector<Field> fields;
    String description;

    void save(nlohmann::json &j) const {
        if (!name.empty())
            j["name"] = name;
        if (!description.empty())
            j["description"] = description;
        for (auto &f: fields) {
            nlohmann::json jf;
            f.save(jf);
            j["fields"].push_back(jf);
        }
    }
    void emitEnums(primitives::CppEmitter &ctx) const;
};

struct Type : basic_info {
    std::vector<String> oneof;

    void save(nlohmann::json &j) const {
        basic_info::save(j);
        for (auto &f: oneof)
            j["oneof"].push_back(f);
    }

    bool is_oneof() const { return !oneof.empty(); }
    bool is_received_variant(const std::map<String, Type> &types) const {
        std::map<String, std::vector<std::pair<const Type *, const Field *>>> fields;
        for (auto &tn : oneof) {
            auto &t2 = types.find(tn)->second;
            for (auto &f : t2.fields) {
                if (!f.always.empty())
                    fields[f.name].push_back({ &t2, &f });
            }
        }
        return !fields.empty();
    }

    std::vector<String> get_dependent_types() const;

    void emit(primitives::CppEmitter &ctx) const;
    void emitFwdDecl(primitives::CppEmitter &ctx) const;
};

struct Method : basic_info {
    Field return_type;

    void save(nlohmann::json &j) const {
        basic_info::save(j);
        return_type.save(j["return_type"]);
    }

    auto cpp_name() const { return name + "Request"; }

    void emit(const Emitter &e, primitives::CppEmitter &h) const;
    void emitRequestType(primitives::CppEmitter &ctx) const;
};
