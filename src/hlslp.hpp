#pragma once

#include <set>
#include <iostream>
#include <functional>
#include <map>
#include <variant>
#include <tuple>

#include <parsegen.hpp>

#include "MetaData.hpp"

namespace ReflectHLSL {
    using namespace std;

    struct AutoTyper {
        
    };

    struct HLSL {
        using ProductionCallback = std::function<std::any(LangTree const&)>;
        using TokenCallback = std::function<std::any(std::string const&)>;

        std::map<int, ProductionCallback> prodCallbacks;
        std::map<int, TokenCallback> tokenCallbacks;

        parsegen::parser_tables_ptr tables;
        parsegen::language lang;

        map<string, string> norm;
        map<int, function<any(vector<any>&)>> rules;
        uint32_t nextID = 0;

        string ToAlpha(int value) {
            string res = to_string(value);
            for (size_t i = 0; i < res.size(); ++i) {
                res[i] += ('a' - '0');
            }
            return res;
        }

        string GetType(string name) {
            if (norm.find(name) == norm.end()) {
                ++nextID;
                norm[name] = ToAlpha(nextID);
                std::cout << "Added " << name << " as " << norm[name] << "\n";
            }

            return norm[name];
        }

        template<typename Function, typename Tuple, size_t ... I>
        auto call(Function f, Tuple t, std::index_sequence<I ...>) {
            return f(std::get<I>(t) ...);
        }

        template<typename Function, typename Tuple>
        auto call(Function f, Tuple t) {
            static constexpr auto size = std::tuple_size<Tuple>::value;
            return call(f, t, std::make_index_sequence<size>{});
        }

        template<int Index, typename ...Args>
        typename std::enable_if<Index == 0, void>::type
        PackAnys(tuple<Args...>& args, vector<any>& anys) {
            get<0>(args) = any_cast<decltype(get<0>(args))>(anys[0]);
        }

        template<int Index, typename ...Args>
        typename std::enable_if<Index != 0, void>::type
        PackAnys(tuple<Args...>& args, vector<any>& anys) {
            PackAnys<Index - 1, Args...>(args, anys);
            get<Index>(args) = any_cast<decltype(get<Index>(args))>(anys[Index]);
        }

        template<typename R, typename ...Args>
        typename std::enable_if<sizeof...(Args) == 0, void>::type
        RegTypes(vector<string>& res) {
            res[0] = GetType(typeid(R).name());
        };

        template<typename R, typename ...Args>
        typename std::enable_if<sizeof...(Args) != 0, void>::type
        RegTypes(vector<string>& res) {
            RegTypes<Args...>(res);
            res[sizeof...(Args)] = GetType(typeid(R).name());
        };

        template<typename R, typename ...Args>
        void AddRule_Internal(function<R(Args...)> const& func) {
            vector<string> rhs; rhs.resize(sizeof...(Args));
            RegTypes<Args...>(rhs);

            const string lhs = GetType(typeid(R).name());

            rules[static_cast<int>(lang.productions.size())] = [this, func](vector<any>& anys) -> any {
                tuple<Args...> args;
                PackAnys<sizeof...(Args) - 1, Args...>(args, anys);
                return call(func, args);
            };

            lang.productions.push_back({ lhs, rhs });
        }

        template<typename F>
        void Rule(F lambda) { AddRule_Internal(function(lambda)); }

        void r(std::string const& lhs, std::vector<std::string> const& rhs) {
            lang.productions.push_back({ lhs, rhs });
        }
        void r(std::string const& lhs, std::vector<std::string> const& rhs, ProductionCallback const& cb) {
            r(lhs, rhs);
            prodCallbacks[static_cast<int>(lang.productions.size() - 1)] = cb;
        }

        void t(std::string const& tok, std::string const& reg) {
            lang.tokens.push_back({ tok, reg });
        }
        void t(std::string const& tok, std::string const& reg, TokenCallback const& cb) {
            t(tok, reg);
            tokenCallbacks[static_cast<int>(lang.tokens.size() - 1)] = cb;
        }

        HLSL() {
            struct Space { };
            struct MaybeSpace { };
            struct LeftBrace { };
            struct RightBrace { };
            struct Any { string text; };
            struct AnyList { vector<Any> text; };
            struct Scope { };
            struct FunctionDecl { };

            Rule([](MaybeSpace, DeclarationList list) { return ProgramDeclaration { list }; });

            Rule([](LeftBrace, RightBrace) { return Scope { }; });
            Rule([](LeftBrace, AnyList, RightBrace) { return Scope { }; }); // TODO: Include text from the any list

            Rule([](AnyDeclaration decl) { return DeclarationList{ decl }; });

            Rule([](DeclarationList list, AnyDeclaration decl) {
                list.push_back(decl);
                return list;
            });

            Rule([](Declaration var) { return AnyDeclaration{ var }; });

            //r("program", { "s?", "declist" },
            //    [](LangTree const& tree) {
            //        DeclarationList list = std::any_cast<DeclarationList>(tree.children[1].data);
            //        ProgramDeclaration prog = { list };
            //        std::cout << prog.format();
            //        return prog;
            //    });

            //r("scope", { "lb", "rb" });
            //r("scope", { "lb", "anylist", "rb" });

            //r("declist", { "decl" },
            //    [](LangTree const& tree) {
            //        return DeclarationList{ std::any_cast<AnyDeclaration>(tree.children[0].data) };
            //    }
            //);
            //r("declist", { "declist", "decl" },
            //    [](LangTree const& tree) {
            //        DeclarationList list = std::any_cast<DeclarationList>(tree.children[0].data);
            //        try {
            //            list.push_back(std::any_cast<AnyDeclaration>(tree.children[1].data));
            //        }
            //        catch (std::bad_any_cast const&) {}
            //        return list;
            //    });

            r("decl", { "vardecl" });
            r("decl", { "allfdecl" });

            Rule([](string id) { return IDList{ id }; });
            Rule([](IDList& ids, Space, string id) {
                ids.push_back(id);
                return std::move(ids);
            });
            //r("idlist", { "id" },
            //    [](LangTree const& tree) { return IDList{ tree.text }; });
            //r("idlist", { "idlist", "space", "id" },
            //    [](LangTree const& tree) {
            //        IDList res = std::any_cast<IDList>(tree.children[0].data);
            //        res.push_back(tree.children[2].text);
            //        return res;
            //    });

            r("literallist", { "literal" },
                [](LangTree const& tree) {
                    return LiteralList{ std::any_cast<LiteralTree>(tree.children[0].data) };
                });
            r("literallist", { "literallist", "s?", ",", "s?", "literal" },
                [](LangTree const& tree) {
                    LiteralList literals = std::any_cast<LiteralList>(tree.children[0].data);
                    literals.push_back(std::any_cast<LiteralTree>(tree.children[4].data));
                    return literals;
                });

            r("arrayliteral", { "lb", "s?", "literallist", "s?", "rb" },
                [](LangTree const& tree) {
                    return LiteralTree{ false, "", std::any_cast<LiteralList>(tree.children[2].data) };
                });

            r("valueliteral", { "float" });
            r("valueliteral", { "int" });
            r("valueliteral", { "id" });
            r("literal", { "valueliteral" },
                [](LangTree const& tree) {
                    return LiteralTree{ true, tree.children[0].children[0].text, { } };
                });
            r("literal", { "arrayliteral" });

            r("default", { "=", "s?", "literal", "s?" });

            r("arrayqual", {},
                [](LangTree const&) { return ArrayQual{ false, "" }; });
            r("arrayqual", { "lbr", "s?", "int", "s?", "rbr", "s?" },
                [](LangTree const& tree) { return ArrayQual{ true, tree.children[2].text }; });
            r("arrayqual", { "lbr", "s?", "id", "s?", "rbr", "s?" },
                [](LangTree const& tree) { return ArrayQual{ true, tree.children[2].text }; });

            r("optsemantic", {},
                [](LangTree const& tree) {
                    return Semantic{ "", "" };
                });
            r("optsemantic", { ":", "s?", "id", "s?", "optsemanticparens" },
                [](LangTree const& tree) {
                    return Semantic{ tree.children[2].text, std::any_cast<std::string>(tree.children[4].data) };
                });
            r("optsemanticparens", {},
                [](LangTree const& tree) {
                    return std::string("");
                });
            r("optsemanticparens", { "lp", "s?", "id", "s?", "rp", "s?" },
                [](LangTree const& tree) {
                    return tree.children[2].text;
                });

            r("structbody", { "lb", "s?", "declist?", "rb", "s?" });
            r("declist?", {},
                [](LangTree const& tree) {
                    return DeclarationList{ };
                });
            r("declist?", { "declist" });

            r("declmode", {});
            r("declmode", { "default" });
            r("declmode", { "structbody" });

            r("vardecl", { "idlist", "s?", "arrayqual", "optsemantic", "declmode", ";", "s?" },
                [](LangTree const& tree) {
                    IDList ids = std::any_cast<IDList>(tree.children[0].data);

                    try {
                        StructDeclaration res;
                        res.declarations = std::any_cast<DeclarationList>(tree.children[4].data);
                        res.name = ids.back();
                        res.type = ids.front();
                        res.semantic = std::any_cast<Semantic>(tree.children[3].data);

                        return AnyDeclaration(res);
                    }
                    catch (std::bad_any_cast const&) {
                        // Not a struct... OK
                    }

                    try {
                        VarDeclaration res;
                        res.name = ids.back();
                        res.type = ids[ids.size() - 2];
                        if (ids.size() > 2) {
                            res.storage = ids[0];
                        }
                        res.arrayQual = std::any_cast<ArrayQual>(tree.children[2].data);
                        res.semantic = std::any_cast<Semantic>(tree.children[3].data);

                        try {
                            res.defaultValue = std::any_cast<LiteralTree>(tree.children[4].data);
                            res.hasDefault = true;
                        }
                        catch (std::bad_any_cast const&) {
                            res.hasDefault = false;
                        }

                        return AnyDeclaration(res);
                    }
                    catch (std::bad_any_cast const&) {
                        // Not a struct or regular declaration... this is actually bad
                        throw std::runtime_error("Declaration isn't valid");
                    }
                });

            r("param", { "idlist" });
            r("param", { "idlist", "s?", ":", "s?", "id" });
            r("paramlist", { "s?" });
            r("paramlist", { "param" });
            r("paramlist", { "paramlist", "anyop", "s?", "param" });

            r("fdecl", { "idlist", "s?", "lp", "paramlist", "rp", "s?", "scope", "s?" });
            r("fdecl", { "idlist", "s?", "lp", "paramlist", "rp", "s?", ":", "s?", "id", "s?", "scope", "s?" });
            r("allfdecl", { "fdecl" });

            r("anyop", { "op" });
            r("anyop", { "," });

            r("any", { "lp" });
            r("any", { "rp" });
            r("any", { "lbr" });
            r("any", { "rbr" });
            r("any", { "=" });
            r("any", { ";" });
            r("any", { ":" });
            r("any", { "anyop" });
            r("any", { "id" });
            r("any", { "float" });
            r("any", { "int" });
            r("any", { "scope" });
            r("any", { "space" });

            r("anylist", { "any" });
            r("anylist", { "anylist", "any" });

            r("s?", {});
            r("s?", { "space" });

            t("space", parsegen::regex::whitespace());
            t(";", ";");
            t("=", "=");
            t(":", ":");
            t(",", ",");
            t("id", parsegen::regex::identifier());
            t("float", parsegen::regex::signed_floating_point());//, [](std:;string const& text) { return std::atof(text); });
            t("int", parsegen::regex::signed_integer());//, [](std:;string const& text) { return std::atoi(text); });
            t("op", "[\\.\\*\\/\\|\\+\\-<>&\\?]");
            t("lb", "\\{");
            t("rb", "\\}");
            t("lbr", "\\[");
            t("rbr", "\\]");
            t("lp", "\\(");
            t("rp", "\\)");

            tables = build_parser_tables(lang);
        }
    };

    class Parser : public parsegen::parser {
    public:
        HLSL lang;

        Parser()
            : parsegen::parser(lang.tables)
            , lang()
        { }
        virtual ~Parser() = default;

    protected:
        virtual std::any shift(int token, std::string& text) override {
            LangTree res = LangTree{ text, {} };
            auto it = lang.tokenCallbacks.find(token);
            if (it != lang.tokenCallbacks.end()) {
                res.data = it->second(text);
            }
            return res;
        }
        virtual std::any reduce(int prod, std::vector<std::any>& rhs) override {
            auto it = lang.rules.find(prod);
            if (it != lang.rules.end()) return it->second(rhs);
            throw std::runtime_error("Internal error, rule that should exist wasn't found");
        }
    };
}