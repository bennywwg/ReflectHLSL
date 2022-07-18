#pragma once

#include <string>
#include <vector>
#include <variant>
#include <any>
#include <memory>
#include <optional>

#include "Generator.hpp"

namespace ReflectHLSL {
    struct LiteralValue;

    struct LiteralTree {
        std::vector<std::shared_ptr<LiteralValue>> children;

        std::string format() const;
        std::string generateLiteral() const;
    };

    struct LiteralValue {
        std::variant<std::string, LiteralTree> value;
        std::string format() const;
    };

    // Token
    struct Space { };
    struct Semicolon { };
    struct Equals { };
    struct Colon { };
    struct Comma { };
    struct ID { std::string Val; };
    struct Float { std::string Val; };
    struct Int { std::string Val; };
    struct Op { std::string Val; };
    struct LBrace { };
    struct RBrace { };
    struct LBrack { };
    struct RBrack { };
    struct LParen { };
    struct RParen { };
    struct Less { };
    struct Great { };

    // Nonterminals
    using LiteralList = std::vector<std::shared_ptr<LiteralValue>>;
    struct LiteralListEncapsulation {
        LiteralList list;
    };
    struct TemplateID {
        ID id;
        LiteralList inTemplate;
    };
    using IDList = std::vector<TemplateID>;
    struct MaybeSpace { };
    struct Scope { };
    struct AnyOp { };
    struct Any { };
    struct AnyList { };
    struct FunctionAttrib {
        ID id;
        LiteralList literals;
    };
    struct FDecl {
        ID name;
    };
    struct Param { };
    struct ParamList { };
    struct Default { LiteralValue Val; };
    struct ArrayQual2 {
        std::string Size;
    };
    struct ArrayQuals {
        std::vector<std::string> Sizes;
    };
    using MaybeArrayQual = std::optional<ArrayQuals>;
    struct SemanticParens {
        ID id;
    };
    using MaybeSemanticParens = std::optional<SemanticParens>;
    struct Semantic {
        ID id;
        MaybeSemanticParens parens;

        std::string GetGeneration();
    };
    using MaybeSemantic = std::optional<Semantic>;
    struct MaybeDecList;
    struct StructBody {
        std::shared_ptr<MaybeDecList> Val;
    };
    using DeclMode = std::optional<std::variant<Default, StructBody>>;
    struct VarDecl {
        IDList ids;
        MaybeArrayQual arrayQual;
        MaybeSemantic semantic;
        DeclMode mode;

        void GetGeneration(GenerationContext& ctx, int tabs);
    };
    using AnyDecl = std::variant<VarDecl, FDecl, FunctionAttrib>;
    struct DeclarationList {
        std::vector<AnyDecl> Val;
    };
    struct MaybeDecList {
        std::optional<DeclarationList> Val;
    };
    struct Program {
        DeclarationList Val;
    };
}