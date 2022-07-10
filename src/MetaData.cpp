#include "MetaData.hpp"

namespace ReflectHLSL {
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

    std::string LiteralValue::format() const {
        if (value.index() == 0) {
            return std::to_string(std::get<int>(value));
        } else if (value.index() == 1) {
            return std::to_string(std::get<uint32_t>(value));
        } else if (value.index() == 2) {
            return std::to_string(std::get<float>(value));
        } else if (value.index() == 3) {
            return std::get<bool>(value) ? "true" : "false";
        } else if (value.index() == 4) {
            return std::get<LiteralTree>(value).format();
        } else {
            throw std::runtime_error("index was something that didn't exist");
        }
    };
}