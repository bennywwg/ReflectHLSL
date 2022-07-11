#pragma once

#include <set>
#include <iostream>
#include <functional>
#include <map>
#include <variant>
#include <tuple>

#ifdef __unix__
#include <cxxabi.h>
#endif

#include <parsegen.hpp>

#include "MetaData.hpp"

#define STRINGIFY(x) #x

namespace ReflectHLSL {
    using namespace std;
    
    std::string demangle(const char* mangled) {
#ifdef __unix__
        int status;
        std::unique_ptr<char[], void (*)(void*)> result(
            abi::__cxa_demangle(mangled, 0, 0, &status), std::free);
        return result.get() ? std::string(result.get()) : "error occurred";
#else
        return mangled;
#endif
    }

    template<class T>
    string DemangleName() {
        return demangle(typeid(T).name());
    }

    string DemangleName(any const& a) {
        return demangle(a.type().name());
    }

    struct HLSL : public parsegen::language {
        parsegen::parser_tables_ptr tables;

        map<string, string> norm; // map from typeid name to normalized name suitable for parsegen
        map<int, function<any(vector<any>&)>> productionCallbacks;
        map<int, function<any(string&)>> tokenCallbacks;
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
                denormalize_map[norm[name]] = name;
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
        PackAnys(tuple<Args...>&, vector<any>&) { }

        template<int Index, typename ...Args>
        typename std::enable_if<Index == 1, void>::type
        PackAnys(tuple<Args...>& args, vector<any>& anys) {
            get<0>(args) = any_cast<decltype(get<0>(args))>(anys[0]);
        }

        template<int Index, typename ...Args>
        typename std::enable_if<(Index > 1), void>::type
        PackAnys(tuple<Args...>& args, vector<any>& anys) {
            PackAnys<Index - 1, Args...>(args, anys);
            get<Index - 1>(args) = any_cast<decltype(get<Index - 1>(args))>(anys[Index - 1]);
        }

        template<typename ...Args>
        typename std::enable_if<sizeof...(Args) == 0, void>::type
        RegTypes(vector<string>&) { };

        template<typename R, typename ...Args>
        typename std::enable_if<sizeof...(Args) == 0, void>::type
        RegTypes(vector<string>& res) {
            res.back() = GetType(DemangleName<R>());
        };

        template<typename R, typename ...Args>
        typename std::enable_if<sizeof...(Args) != 0, void>::type
        RegTypes(vector<string>& res) {
            RegTypes<Args...>(res);
            res[res.size() - sizeof...(Args) - 1] = GetType(DemangleName<R>());
        };

        template<typename R, typename ...Args>
        void AddRule_Internal(function<R(Args...)> const& func) {
            vector<string> rhs; rhs.resize(sizeof...(Args));
            RegTypes<Args...>(rhs);

            const string lhs = GetType(DemangleName<R>());

            productionCallbacks[static_cast<int>(productions.size())] = [this, func](vector<any>& anys) -> any {
                tuple<Args...> args;
                PackAnys<sizeof...(Args), Args...>(args, anys);
                return call(func, args);
            };

            productions.push_back({ lhs, rhs });
        }

        template<typename F>
        void RuleF(F lambda) { AddRule_Internal(function(lambda)); }

#define Rule production_names[static_cast<int>(productions.size())] = (__FILE__ + string(":") + std::to_string(__LINE__)), RuleF

        template<typename R, typename ...Args>
        void AddToken_Internal(function<R(string&)> const& func, string const& regex) {
            const string lhs = GetType(DemangleName<R>());

            tokenCallbacks[static_cast<int>(tokens.size())] = [this, func](string& val) -> any {
                return func(val);
            };

            tokens.push_back( { lhs, regex });
        }

        template<typename F>
        void Token(F lambda, string const& regex) { AddToken_Internal(function(lambda), regex); }

        template<typename T>
        typename std::enable_if<!std::is_constructible<T, string&>::value, void>::type
        Token(string const& regex) { AddToken_Internal(function([](string&) { return T { }; }), regex); }

        template<typename T>
        typename std::enable_if<std::is_constructible<T, string&>::value, void>::type
        Token(string const& regex) { AddToken_Internal(function([](string& text) { return T { text }; }), regex); }

        void InitRules() {

            {
                Rule([](MaybeSpace, DeclarationList list) -> Program {
                    return { list };
                });

                Rule([](DeclarationList list, AnyDecl decl) -> DeclarationList {
                    list.push_back(decl);
                    return list;
                });
                Rule([](AnyDecl decl) -> DeclarationList {
                    return { decl };
                });

                Rule([](FDecl) -> AnyDecl { return std::nullopt; });
                Rule([](VarDecl decl) -> AnyDecl { return decl; });
            }

            // Creating Literals, output is LiteralValue
            {
                Rule([](LiteralValue val) -> LiteralList {
                    return { make_shared<LiteralValue>(val) };
                });
                Rule([](LiteralList list, MaybeSpace, Comma, MaybeSpace, LiteralValue val) -> LiteralList {
                    list.push_back(make_shared<LiteralValue>(val));
                    return list;
                });

                Rule([](LBrace, MaybeSpace, LiteralList list, MaybeSpace, RBrace) -> LiteralListEncapsulation {
                    return { list };
                });

                Rule([](Int val) -> LiteralValue {
                    return { std::stoi(val.Val) };
                });
                Rule([](Float val) -> LiteralValue {
                    return { std::stof(val.Val) };
                });
                Rule([](ID val) -> LiteralValue {
                    if (val.Val == "true") return LiteralValue{ true };
                    if (val.Val == "false")  return LiteralValue{ false };
                    throw std::runtime_error("Expected true or false in literal, but got an arbitrary string");
                });
                Rule([](LiteralListEncapsulation val) -> LiteralValue {
                    return { LiteralTree { val.list } };
                });
            }

            // Variable and Struct declaration
            {
                // Variables
                {
                    // Example:      vvv
                    //          int a[4] : register(b0) = { 0 };
                    Rule([]() -> MaybeArrayQual { return std::nullopt; });
                    Rule([](LBrack, MaybeSpace, ID id, MaybeSpace, RBrack, MaybeSpace) -> MaybeArrayQual { return ArrayQual{ id.Val }; });
                    Rule([](LBrack, MaybeSpace, Int val, MaybeSpace, RBrack, MaybeSpace) -> MaybeArrayQual { return ArrayQual{ val.Val }; });

                    // Example:          vvvvvvvvvvvvvv
                    //          int a[4] : register(b0) = { 0 };
                    Rule([]() -> MaybeSemantic { return std::nullopt; });
                    Rule([](Colon, MaybeSpace, ID id, MaybeSpace, MaybeSemanticParens parens) -> MaybeSemantic {
                        return Semantic{ id, parens };
                    });

                    // Example:                    vvvv
                    //          int a[4] : register(b0) = { 0 };
                    Rule([]() { return MaybeSemanticParens(std::nullopt); });
                    Rule([](LParen, MaybeSpace, ID id, MaybeSpace, RParen, MaybeSpace) { return MaybeSemanticParens{ { id } }; });

                    // Example:                         vvvvvvv
                    //          int a[4] : register(b0) = { 0 };
                    Rule([](Equals, MaybeSpace, LiteralValue val, MaybeSpace) -> Default { return { val }; });
                }

                // Structs
                {
                    // Example:          vvvvvvvvvv
                    //          struct B { int x; };
                    Rule([](LBrace, MaybeSpace, MaybeDecList, RBrace, MaybeSpace) -> StructBody { return { }; });

                    // Example:            vvvvvv
                    //          struct B { int x; };
                    Rule([]() { return MaybeDecList(std::nullopt); });
                    Rule([](DeclarationList decList) { return MaybeDecList(decList); });
                }

                // Determine whether the declaration is a variable or a struct
                Rule([]() { return DeclMode(std::nullopt); });
                Rule([](Default def) { return DeclMode(def); });
                Rule([](StructBody) { return DeclMode(StructBody{}); });
                Rule([](
                    IDList ids,
                    MaybeSpace,
                    MaybeArrayQual arr,
                    MaybeSemantic sem,
                    DeclMode mode,
                    Semicolon,
                    MaybeSpace
                ) {
                        return VarDecl{ ids, arr, sem, mode };
                });
            }

            // Various odds and ends
            {
                Rule([](ID id) -> IDList { return { id }; });
                Rule([](IDList list, MaybeSpace, ID id) -> IDList {
                    list.push_back(id);
                    return list;
                });

                Rule([](IDList) { return Param{ }; });
                Rule([](IDList, MaybeSpace, Colon, MaybeSpace, ID) { return Param{ }; });
                Rule([](MaybeSpace) { return ParamList{ }; });
                Rule([](Param) { return ParamList{ }; });
                Rule([](ParamList, AnyOp, MaybeSpace, Param) { return ParamList{ }; });

                Rule([](IDList, MaybeSpace, LParen, ParamList, RParen, MaybeSpace, Scope, MaybeSpace) { return FDecl{ }; });
                Rule([](IDList, MaybeSpace, LParen, ParamList, RParen, MaybeSpace, Colon, MaybeSpace, ID, MaybeSpace, Scope, MaybeSpace) { return FDecl{ }; });

                Rule([](Op) { return AnyOp{ }; });
                Rule([](Comma) { return AnyOp{ }; });

                Rule([]() { return MaybeSpace{ }; });
                Rule([](Space) { return MaybeSpace{ }; });

                // Any and AnyList
                {
                    Rule([](LBrace, RBrace) -> Scope { return { }; });
                    Rule([](LBrace, AnyList, RBrace) -> Scope { return { }; });

                    Rule([](LParen) { return Any{ }; });
                    Rule([](RParen) { return Any{ }; });
                    Rule([](LBrack) { return Any{ }; });
                    Rule([](RBrack) { return Any{ }; });
                    Rule([](Equals) { return Any{ }; });
                    Rule([](Semicolon) { return Any{ }; });
                    Rule([](Colon) { return Any{ }; });
                    Rule([](AnyOp) { return Any{ }; });
                    Rule([](ID) { return Any{ }; });
                    Rule([](Float) { return Any{ }; });
                    Rule([](Int) { return Any{ }; });
                    Rule([](Scope) { return Any{ }; });
                    Rule([](Space) { return Any{ }; });

                    Rule([](Any) { return AnyList{ }; });
                    Rule([](AnyList, Any) { return AnyList{ }; });
                }
            }

            // Tokens
            {
                Token<Space>(parsegen::regex::whitespace());
                Token<Semicolon>(";");
                Token<Equals>("=");
                Token<Colon>(":");
                Token<Comma>(",");
                Token<ID>(parsegen::regex::identifier());
                Token<Float>(parsegen::regex::signed_floating_point());
                Token<Int>(parsegen::regex::signed_integer());
                Token<Op>("[\\.\\*\\/\\|\\+\\-<>&\\?]");
                Token<LBrace>("\\{");
                Token<RBrace>("\\}");
                Token<LBrack>("\\[");
                Token<RBrack>("\\]");
                Token<LParen>("\\(");
                Token<RParen>("\\)");
            }
        }

        HLSL() {
            InitRules();

            try {
                tables = build_parser_tables(*this);
            } catch (parsegen::parse_error const& er) {
                throw std::runtime_error(er.what());
            }
        }
    };

    class Parser : public parsegen::parser {
    public:
        HLSL lang;

        Program GetProgram(string const& text) {
            auto res = parse_string(text, "input");
            try {
                return std::any_cast<Program>(res);
            } catch (std::bad_any_cast const& ex) {
                throw std::runtime_error("parse_string completed but the return type was incorrect");
            }
        }

        Parser(HLSL l) : parsegen::parser(l.tables), lang(l) { }
        virtual ~Parser() = default;

    public:
        inline virtual string denormalize_name(string const& normalized) const override {
            return lang.denormalize_name(normalized);
        }

    protected:
        inline virtual std::any shift(int token, std::string& text) override {
            auto it = lang.tokenCallbacks.find(token);
            if (it != lang.tokenCallbacks.end()) return it->second(text);
            throw std::runtime_error("Internal error, token that should exist wasn't found");
        }
        inline virtual std::any reduce(int prod, std::vector<std::any>& rhs) override {
            auto it = lang.productionCallbacks.find(prod);
            if (it != lang.productionCallbacks.end()) return it->second(rhs);
            throw std::runtime_error("Internal error, rule that should exist wasn't found");
        }
    };
}