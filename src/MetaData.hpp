#pragma once

#include <string>
#include <vector>
#include <variant>
#include <any>

namespace ReflectHLSL {
    struct LiteralValue {
        std::variant<int, uint32_t, float, bool, LiteralTree> value;
        std::string format() const;
    };
    
    struct LiteralTree {
        std::vector<LiteralValue> children;

        std::string format() const;
        std::string generateLiteral() const;
    };

    using LiteralList = std::vector<LiteralTree>;

    struct Semantic {
        std::string semanticName;
        std::string inParens;

        std::string format() const;
        std::string generateLiteral() const;
    };

    struct ArrayQual {
        bool present = false;
        std::string value;

        std::string format() const;
        std::string generateLiteral() const;
    };

    struct VarDeclaration {
        std::string storage;
        std::string type;
        std::string templateParam;
        std::string name;
        ArrayQual arrayQual;
        Semantic semantic;
        bool hasDefault = false;
        LiteralTree defaultValue;

        std::string format() const;
        std::string generateLiteral() const;
    };

    struct StructDeclaration {
        std::string type; //cbuffer or struct, presumably
        std::string name;
        Semantic semantic;
        std::vector<std::variant<VarDeclaration, StructDeclaration>> declarations;

        std::string format(int tabs = 0) const;
        std::string generateLiteral() const;
    };

    using AnyDeclaration = std::variant<VarDeclaration, StructDeclaration>;
    using DeclarationList = std::vector<AnyDeclaration>;
    using IDList = std::vector<std::string>;

    struct ProgramDeclaration {
        DeclarationList declarations;
        std::string format() const;
    };
}