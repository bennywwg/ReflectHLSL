#pragma once

#include <set>
#include <iostream>
#include <functional>
#include <map>
#include <variant>
#include <tuple>
#include <cxxabi.h>

#include <parsegen.hpp>

#include "MetaData.hpp"

namespace ReflectHLSL {
    using namespace std;
    
    std::string demangle(const char* mangled) {
        int status;
        std::unique_ptr<char[], void (*)(void*)> result(
            abi::__cxa_demangle(mangled, 0, 0, &status), std::free);
        return result.get() ? std::string(result.get()) : "error occurred";
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
            }

            return norm[name];
        }

        template<size_t I, typename Tuple>
        auto any_get(vector<any>& anys) {
            return std::any_cast<decltype(std::get<I>(Tuple()))>(anys[I]);
        }

        template<typename Function, typename Tuple, size_t ... I>
        auto call3(Function f, Tuple*, vector<any>& anys, std::index_sequence<I ...>) {
            return f(any_get<I, Tuple>(anys) ...);
        }

        template<typename Function, typename Tuple, typename ...Args>
        auto call2(Function f, vector<any>& anys) {
            return call3(
                f,
                (Tuple*)(nullptr),
                anys,
                std::make_index_sequence<sizeof...(Args)>{}
            );
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
                //return call2<function<R(Args...)> const&, tuple<Args...>, Args...>(func, anys);
            };

            productions.push_back({ lhs, rhs });
        }

        template<typename F>
        void Rule(F lambda) { AddRule_Internal(function(lambda)); }

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
        void Token(string const& regex) { AddToken_Internal(function([](string&) { return T { }; }), regex); }
        
        // Unused
        /*
        void MakeTokens() {
            Token<TPlus>("+");
            Token<TMinus>("-");
            Token<TNot>("!");
            Token<TMul>("*");
            Token<TDiv>("/");
            Token<TMod>("%");
            Rule([](TL, TL) { return TShiftL { }; });
            Rule([](TG, TG) { return TShiftR { }; });
            Token<TAssign>("=");
            Token<TL>("<");
            Rule([](TL, TAssign) { return TLEQ { }; });
            Token<TG>(">");
            Rule([](TG, TAssign) { return TGEQ { }; });
            Rule([](TAssign, TAssign) { return TEQ { }; });
            Rule([](TNot, TAssign) { return TNEQ { }; });
            Token<TBitAND>("&");
            Token<TBitXOR>("^");
            Token<TBitOR>("|");
            Token<TBitNOT>("~");
            Rule([](TBitAND, TBitAND) { return TAnd { }; });
            Rule([](TBitOR, TBitOR) { return TOr { }; });
            Token<TQuestion>("?");
            Token<TColon>(":");
            Token<TSemicolon>(";");
            Token<TComma>(",");
            Token<TLParen>("(");
            Token<TLParen>(")");
            Token<TLBrack>("[");
            Token<TRBrack>("]");
            Token<TLBrace>("{");
            Token<TRBrace>("}");

            Token<TSpace>(parsegen::regex::whitespace());
            Token([](string& text) {
                return TInt { strtoll(text.data(), nullptr, 10) };
            }, parsegen::regex::unsigned_integer());
            Token([](string& text) {
                return TFloat { strtod(tex.data(), nullptr) };
            }, parsegen::regex::unsigned_floating_point_not_integer());
            Token([](string& text) {
                return TID { text }; 
            }, parsegen::regex::identifier());

            Rule([]() { return MaybeSpace { };});
            Rule([](TSpace) { return MaybeSpace { };});

            Rule([](TLParen, TID const& id, TRParen) {
                return Cast { id.ID };
            });
        }*/

        void InitRules() {

            // Token
            struct Space { };
            struct Semicolon { };
            struct Equals { };
            struct Colon { };
            struct Comma { };
            struct ID { string Val; };
            struct Float { string Val; };
            struct Int { string Val; };
            struct Op { string Val; };
            struct LBrace { };
            struct RBrace { };
            struct LBrack { };
            struct RBrack { };
            struct LParen { };
            struct RParen { };

            // Nonterminals
            using Literal = variant<LiteralValue, LiteralTree>;
            using LiteralList = std::vector<Literal>;
            struct Program { DeclarationList Declarations; };
            struct FunctionScope { };
            struct ArrayLiteral { LiteralList Literals; };
            struct MaybeSpace { };
            struct Scope { };
            struct AnyOp { };
            struct Any { };
            struct AnyList { };
            struct FDecl { };
            struct Param { };
            struct ParamList { };
            struct Default { Literal Val; };
            struct ArrayQual { };
            using MaybeArrayQual = optional<ArrayQual>;
            struct Semantic {
                ID id;
            };
            using MaybeSemantic = optional<Semantic>;
            struct SemanticParens { 
                ID id;
            };
            using MaybeSemanticParens = optional<SemanticParens>;
            struct StructBody { };
            using DeclMode = optional<variant<Default, StructBody>>;
            struct VarDecl {
                IDList ids;
                MaybeArrayQual arrayQual;
                MaybeSemantic semantic;
                DeclMode mode;
            };

            Rule([]() { return Semantic { ID { "" }}; });
            Rule([]() { return Semantic { ID { "" }}; });

            Rule([]() { return OptSemanticParens(std::nullopt); });
            Rule([](LParen, MaybeSpace, ID id, MaybeSpace, RParen, MaybeSpace){ return MaybeSemanticParens { SemanticParens { id } }; });

            Rule([](LBrace, MaybeSpace, MaybeDecList, RBrace, MaybeSpace){ return StructBody { }; });

            Rule([]() { return MaybeDecList(std::nullopt); });
            Rule([](DeclarationList decList) { return MaybeDecList(decList); });

            Rule([](){ return DeclMode(std::nullopt); });
            Rule([](Default def){ return DeclMode(def); });
            Rule([](StructBody){ return DeclMode(StructBody {}); });

            Rule([](
                IDList ids,
                MaybeSpace,
                MaybeArrayQual arr,
                MaybeSemantic sem,
                DeclMode mode,
                Semicolon,
                MaybeSpace
            ) {
                return VarDecl { ids, arr, sem, mode };
            });

            Rule([](IDList) { return Param { };});
            Rule([](IDList, MaybeSpace, Colon, MaybeSpace, ID) { return Param { };});
            Rule([](MaybeSpace) { return ParamList { };});
            Rule([](Param) { return ParamList { };});
            Rule([](ParamList, AnyOp, MaybeSpace, Param) { return ParamList { };});

            Rule([](IDList, MaybeSpace, LParen, ParamList, RParen, MaybeSpace, Scope, MaybeSpace){return FDecl { }; });
            Rule([](IDList, MaybeSpace, LParen, ParamList, RParen, MaybeSpace, Colon, MaybeSpace, ID, MaybeSpace, Scope, MaybeSpace){return FDecl { }; });

            Rule([](Op) { return AnyOp { };});
            Rule([](Comma) { return AnyOp { };});

            Rule([](LParen) { return Any { };});
            Rule([](RParen) { return Any { };});
            Rule([](LBrack) { return Any { };});
            Rule([](RBrack) { return Any { };});
            Rule([](Equals) { return Any { };});
            Rule([](Semicolon) { return Any { };});
            Rule([](Colon) { return Any { };});
            Rule([](AnyOp) { return Any { };});
            Rule([](ID) { return Any { };});
            Rule([](Float) { return Any { };});
            Rule([](Int) { return Any { };});
            Rule([](Scope) { return Any { };});
            Rule([](Space) { return Any { };});

            Rule([](Any) { return AnyList { };});
            Rule([](AnyList, Any) { return AnyList { };});

            Rule([](){ return MaybeSpace { };});
            Rule([](Space){ return MaybeSpace { };});

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

        inline virtual string denormalize_name(string const& normalized) const override {
            for (auto it : norm) {
                if (it.second == normalized) return it.first;
            }
            throw std::runtime_error("Couldn't denormalize name");
        }

        HLSL() {
            InitRules();

            tables = build_parser_tables(*this);
        }
    };

    class Parser : public parsegen::parser {
    public:
        HLSL lang;

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