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

#include <frontend.hpp>

#include "MetaData.hpp"

namespace ReflectHLSL {
    
    class HLSL : public parsegen::frontend {
    public:
        using ReturnType = Program;

        virtual void InitRules() override {
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
                    return { std::make_shared<LiteralValue>(val) };
                });
                Rule([](LiteralList list, MaybeSpace, Comma, MaybeSpace, LiteralValue val) -> LiteralList {
                    list.push_back(std::make_shared<LiteralValue>(val));
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
    
        HLSL() = default;
    };
}