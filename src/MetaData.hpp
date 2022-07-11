#pragma once

#include <string>
#include <vector>
#include <variant>
#include <any>
#include <memory>
#include <optional>

namespace ReflectHLSL {
    struct LiteralValue;

    struct LiteralTree {
        std::vector<std::shared_ptr<LiteralValue>> children;

        std::string format() const;
        std::string generateLiteral() const;
    };

    struct LiteralValue {
        std::variant<int, float, bool, LiteralTree> value;
        std::string format() const;
    };

    using std::string;
    using std::vector;
    using std::optional;
    using std::variant;

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
    using IDList = vector<ID>;
    using LiteralList = std::vector<std::shared_ptr<LiteralValue>>;
    struct LiteralListEncapsulation {
        LiteralList list;
    };
    struct FunctionScope { };
    struct MaybeSpace { };
    struct Scope { };
    struct AnyOp { };
    struct Any { };
    struct AnyList { };
    struct FDecl { };
    struct Param { };
    struct ParamList { };
    struct Default { LiteralValue Val; };
    struct ArrayQual {
        string size;
    };
    using MaybeArrayQual = optional<ArrayQual>;
    struct SemanticParens {
        ID id;
    };
    using MaybeSemanticParens = optional<SemanticParens>;
    struct Semantic {
        ID id;
        MaybeSemanticParens parens;
    };
    using MaybeSemantic = optional<Semantic>;
    struct StructBody { };
    using DeclMode = optional<variant<Default, StructBody>>;
    struct VarDecl {
        IDList ids;
        MaybeArrayQual arrayQual;
        MaybeSemantic semantic;
        DeclMode mode;
    };
    using AnyDecl = optional<VarDecl>; // We only care about vardecls, no data collected for functions
    using DeclarationList = vector<AnyDecl>;
    using MaybeDecList = optional<DeclarationList>;
    struct Program { DeclarationList Declarations; };
}