#pragma once

#include <set>
#include <iostream>
#include <functional>
#include <map>
#include <variant>
#include <tuple>

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

                Rule([](AnyDecl decl) -> DeclarationList {
                    return { { decl } };
                });
                Rule([](DeclarationList list, AnyDecl decl) -> DeclarationList {
                    list.Val.push_back(decl);
                    return list;
                });

                Rule([](FunctionAttrib attrib) -> AnyDecl { return attrib; });
                Rule([](FDecl decl) -> AnyDecl { return decl; });
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
                    return { val.Val };
                });
                Rule([](Float val) -> LiteralValue {
                    return { val.Val };
                });
                Rule([](ID val) -> LiteralValue {
                    return { val.Val };
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
                    Rule([](ArrayQuals Quals) -> MaybeArrayQual { return Quals; });
                    Rule([](ArrayQual2 Qual) -> ArrayQuals { return { { Qual.Size } }; });
                    Rule([](ArrayQuals Quals, ArrayQual2 Qual) -> ArrayQuals {
                        Quals.Sizes.push_back(Qual.Size);
                        return Quals;
                    });
                    Rule([](LBrack, MaybeSpace, ID id, MaybeSpace, RBrack, MaybeSpace) -> ArrayQual2 { return { id.Val }; });
                    Rule([](LBrack, MaybeSpace, Int val, MaybeSpace, RBrack, MaybeSpace) -> ArrayQual2 { return { val.Val }; });

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
                    Rule([](LBrace, MaybeSpace, MaybeDecList decList, RBrace, MaybeSpace) -> StructBody {
                        return { std::make_shared<MaybeDecList>(decList) };
                    });

                    // Example:            vvvvvv
                    //          struct B { int x; };
                    Rule([]() { return MaybeDecList(std::nullopt); });
                    Rule([](DeclarationList decList) { return MaybeDecList(decList); });
                }

                // Determine whether the declaration is a variable or a struct
                Rule([]() { return DeclMode(std::nullopt); });
                Rule([](Default def) -> DeclMode { return def; });
                Rule([](StructBody body) -> DeclMode { return body; });
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
                Rule([](ID id) -> TemplateID { return { id, { } }; });
                Rule([](ID id, Less, ID templateId, Great) -> TemplateID {
                    auto lit = std::make_shared<LiteralValue>(templateId.Val);
                    return { id, { lit } };
                });

                Rule([](TemplateID id) -> IDList { return { id }; });
                Rule([](IDList list, MaybeSpace, TemplateID id) -> IDList {
                    list.push_back(id);
                    return list;
                });

                Rule([](IDList) { return Param{ }; });
                Rule([](IDList, MaybeSpace, Colon, MaybeSpace, ID) { return Param{ }; });
                Rule([](MaybeSpace) { return ParamList{ }; });
                Rule([](Param) { return ParamList{ }; });
                Rule([](ParamList, Comma, MaybeSpace, Param) { return ParamList{ }; });

                Rule([](LBrack, MaybeSpace, ID id, MaybeSpace, LParen, MaybeSpace, LiteralList literals, RParen, MaybeSpace, RBrack, MaybeSpace) -> FunctionAttrib {
                    return { id, literals };
                });

                Rule([](IDList ids, MaybeSpace, LParen, ParamList, RParen, MaybeSpace, Scope, MaybeSpace) { return FDecl{ ids[1].id }; });
                Rule([](IDList ids, MaybeSpace, LParen, ParamList, RParen, MaybeSpace, Colon, MaybeSpace, ID, MaybeSpace, Scope, MaybeSpace) { return FDecl{ ids[1].id }; });

                Rule([](Op) { return AnyOp{ }; });
                Rule([](Comma) { return AnyOp{ }; });
                Rule([](Less) { return AnyOp{ }; });
                Rule([](Great) { return AnyOp{ }; });

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
                Token<Op>("[\\.\\*\\/\\|\\+\\-&\\?]");
                Token<LBrace>("\\{");
                Token<RBrace>("\\}");
                Token<LBrack>("\\[");
                Token<RBrack>("\\]");
                Token<LParen>("\\(");
                Token<RParen>("\\)");
                Token<Less>("<");
                Token<Great>(">");
            }
        }
    
        HLSL() = default;
    };
}