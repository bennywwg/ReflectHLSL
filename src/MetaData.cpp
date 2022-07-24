#include "MetaData.hpp"
#include <stdexcept>

namespace ReflectHLSL {
#define RH_ASSERT(exp, info) {if(!(exp)) throw std::runtime_error(info);}

    std::string LiteralTree::format() const {
        std::string res;
        res += "{ ";
        for (size_t i = 0; i < children.size(); ++i) {
            res += children[i]->format();
            if ((i + 1) != (children.size())) res += ", ";
        }
        res += " }";
        return res;
    }

    std::string LiteralValue::format() const {
        if (value.index() == 0) {
            return std::get<std::string>(value);
        } else if (value.index() == 1) {
            return std::get<LiteralTree>(value).format();
        }
        else {
            throw std::runtime_error("index was something that didn't exist");
        }
    };

    void VarDecl::GetGeneration(GenerationContext& ctx, int tabs) {
        const bool IsStruct = mode.has_value() && mode->index() == 1;

        std::string& res = ctx.Output;

        { // Setup name and qualifier
            res += std::string(tabs, '\t');

            if (IsStruct) {
                StructBody body = std::get<StructBody>(*mode);

                RH_ASSERT(ids.size() >= 1 && \
                    ids.size() <= 2 && \
                    (ids[0].id.Val == "struct" || ids[0].id.Val == "cbuffer"), \
                    "Invalid struct declaration");

                res += "struct " + ids[1].id.Val;
            } else {
                for (auto ID : ids) {
                    res += ID.id.Val + (ID.inTemplate.empty() ? std::string() : "<" + ID.inTemplate[0]->format() + ">") + " ";
                }

                res.pop_back();
            }
        }

        const std::string Identifier = IsStruct ? (ids[1].id.Val) : ids.back().id.Val;

        { // Array decls
            if (arrayQual.has_value()) {
                for (auto size : arrayQual->Sizes) {
                    res += "[" + size + "]";
                }
            }
        }

        std::string semanticComment;

        { // Semantic
            if (semantic.has_value()) {
                semanticComment = " // : " + semantic->GetGeneration();

                if (IsStruct) {
                    //ctx.CBufferRegisterMap[Identifier] = semantic->parens->id.Val;
                } else if (semantic->parens.has_value()) {
                    //ctx.VarRegisterMap[Identifier] = semantic->parens->id.Val;
                }
            }
        }
        
        if (IsStruct) { // Is a struct
            StructBody body = std::get<StructBody>(*mode);

            res += " {" + semanticComment + "\n";
            if (body.Val->Val.has_value()) {
                for (auto Decl : body.Val->Val->Val) {
                    if (Decl.index() == 0) {
                        std::get<VarDecl>(Decl).GetGeneration(ctx, tabs + 1);
                    }
                }
            }

            res += std::string(tabs, '\t') + "}";
        } else {
            if (mode.has_value()) {
                Default def = std::get<Default>(*mode);

                res += " = " + def.Val.format();
            }
        }

        res += ";";

        if (!IsStruct) {
            res += semanticComment;
        }

        res += "\n";
    }

    inline std::string Semantic::GetGeneration() {
        return id.Val;// + (parens.has_value() ? ("(" + parens->id.Val + ")") : std::string());
    }

    /*
    std::string Semantic::format() const {
         if (semanticName.empty()) return "";
         std::string res;
         res += " : ";
         res += semanticName;
         if (!inParens.empty()) {
             res += "(";
             res += inParens;
             res += ")";
         }
         return res;
     }

    std::string ArrayQual::format() const {
         if (!present) return "";
         std::string res;
         res += "[";
         res += value;
         res += "]";
         return res;
     }

    std::string VarDeclaration::format() const {
         std::string res;
         res += storage;
         if (!storage.empty()) res += " ";
         res += type;
         res += " ";
         res += name;
         res += arrayQual.format();
         res += semantic.format();
         if (hasDefault) {
             res += " = ";
             res += defaultValue.format();
         }
         return res + ";\n";
     }

    std::string StructDeclaration::format(int tabs) const {
         std::string res;
         res += std::string(tabs, '\t');
         res += type;
         res += " ";
         res += name;
         res += semantic.format();
         if (declarations.empty()) return res + " { };\n";
         res += " {\n";
         for (auto v : declarations) {
             if (v.index() == 0) {
                 res += std::string(static_cast<size_t>(tabs) + 1, '\t');
                 res += std::get<VarDeclaration>(v).format();
             } else {
                 res += std::get<StructDeclaration>(v).format(tabs + 1);
             }
         }
         res += std::string(tabs, '\t');
         res += "};\n";
         return res;
     }

    std::string ProgramDeclaration::format() const {
        std::string res;
        for (auto v : declarations) {
            if (v.index() == 0) {
                res += std::get<VarDeclaration>(v).format();
            } else {
                res += std::get<StructDeclaration>(v).format();
            }
            res += "\n";
        }
        return res;
    }
    */
}