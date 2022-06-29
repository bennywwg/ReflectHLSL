#pragma once

#include "../parsegen-cpp/src/parsegen.hpp"
#include <set>

#include <iostream>
#include <functional>
#include <map>
#include <variant>

struct LiteralTree {
  bool isValue;
  std::string value;
  std::vector<LiteralTree> children;

  std::string format() const {
    if (isValue) {
      return value;
    } else {
      std::string res;
      res += "{ ";
      for(size_t i = 0; i < children.size(); ++i) {
        res += children[i].format();
        if ((i + 1) != (children.size())) res += ", ";
      }
      res += " }";
      return res;
    }
  }
};

using LiteralList = std::vector<LiteralTree>;

struct Semantic {
  std::string semanticName;
  std::string inParens;

  std::string format() const {
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
};

struct ArrayQual {
  bool present;
  std::string value;

  std::string format() const {
    if (!present) return "";
    std::string res;
    res += "[";
    res += value;
    res += "]";
    return res;
  }
};

struct VarDeclaration {
  std::string storage;
  std::string type;
  std::string name;
  ArrayQual arrayQual;
  Semantic semantic;
  bool hasDefault;
  LiteralTree defaultValue;

  std::string format() const {
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
};

struct StructDeclaration {
  std::string type; //cbuffer or struct, presumably
  std::string name;
  Semantic semantic;
  std::vector<std::variant<VarDeclaration, StructDeclaration>> declarations;

  std::string format(int tabs = 0) const {
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
        res += std::string(tabs + 1, '\t');
        res += std::get<VarDeclaration>(v).format();
      } else {
        res += std::get<StructDeclaration>(v).format(tabs + 1);
      }
    }
    res += std::string(tabs, '\t');
    res += "};\n";
    return res;
  }
};

using AnyDeclaration = std::variant<VarDeclaration, StructDeclaration>;
using DeclarationList = std::vector<AnyDeclaration>;
using IDList = std::vector<std::string>;

struct ProgramDeclaration {
  DeclarationList declarations;
  std::string format() {
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
};

struct LangTree {
  std::string text;
  std::vector<LangTree> children;
  std::any data;
};

struct HLSL {
  using ProductionCallback = std::function<std::any(LangTree const&)>;
  using TokenCallback = std::function<std::any(std::string const&)>;

  std::map<int, ProductionCallback> prodCallbacks;
  std::map<int, TokenCallback> tokenCallbacks;

  parsegen::parser_tables_ptr tables;
  parsegen::language lang;

  void r(std::string const& lhs, std::vector<std::string> const& rhs) {
    lang.productions.push_back({lhs, rhs});
  }
  void r(std::string const& lhs, std::vector<std::string> const& rhs, ProductionCallback const& cb) {
    r(lhs, rhs);
    prodCallbacks[static_cast<int>(lang.productions.size() - 1)]  = cb;
  }

  void t(std::string const& tok, std::string const& reg) {
    lang.tokens.push_back({tok, reg});
  }
  void t(std::string const& tok, std::string const& reg, TokenCallback const& cb) {
    t(tok, reg);
    tokenCallbacks[static_cast<int>(lang.tokens.size() - 1)] = cb;
  }

  HLSL() {
    r("program", {"s?", "declist"},
    [](LangTree const& tree) {
      DeclarationList list = std::any_cast<DeclarationList>(tree.children[1].data);
      ProgramDeclaration prog = { list };
      std::cout << prog.format();
      return prog;
    });

    r("scope", {"lb", "rb"});
    r("scope", {"lb", "anylist", "rb"});

    r("declist", {"decl"},
    [](LangTree const& tree) {
      return DeclarationList { std::any_cast<AnyDeclaration>(tree.children[0].data) };
    });
    r("declist", {"declist", "decl"},
    [](LangTree const& tree) {
      DeclarationList list = std::any_cast<DeclarationList>(tree.children[0].data);
      try {
        list.push_back(std::any_cast<AnyDeclaration>(tree.children[1].data));
      } catch (std::bad_any_cast const&) { }
      return list;
    });

    r("decl", {"vardecl"});
    r("decl", {"allfdecl"});

    r("idlist", {"id"},
    [](LangTree const& tree) { return IDList { tree.text }; });
    r("idlist", {"idlist", "space", "id"},
    [](LangTree const& tree) {
      IDList res = std::any_cast<IDList>(tree.children[0].data);
      res.push_back(tree.children[2].text);
      return res;
    });

    r("literallist", {"literal"},
    [](LangTree const& tree) {
      return LiteralList { std::any_cast<LiteralTree>(tree.children[0].data) };
    });
    r("literallist", {"literallist", "s?", ",", "s?", "literal"},
    [](LangTree const& tree) {
      LiteralList literals = std::any_cast<LiteralList>(tree.children[0].data);
      literals.push_back(std::any_cast<LiteralTree>(tree.children[4].data));
      return literals;
    });

    r("arrayliteral", {"lb", "s?", "literallist", "s?", "rb"},
    [](LangTree const& tree) {
      return LiteralTree { false, "", std::any_cast<LiteralList>(tree.children[2].data) };
    });

    r("valueliteral", {"float"});
    r("valueliteral", {"int"});
    r("valueliteral", {"id"});
    r("literal", {"valueliteral"},
    [](LangTree const& tree) {
      return LiteralTree { true, tree.children[0].children[0].text, { }};
    });
    r("literal", {"arrayliteral"});

    r("default", {"=", "s?", "literal", "s?"});

    r("arrayqual", {},
    [](LangTree const&) { return ArrayQual { false, "" }; });
    r("arrayqual", {"lbr", "s?", "int", "s?", "rbr", "s?"},
    [](LangTree const& tree) { return ArrayQual { true, tree.children[2].text }; });
    r("arrayqual", {"lbr", "s?", "id", "s?", "rbr", "s?"},
    [](LangTree const& tree) { return ArrayQual { true, tree.children[2].text }; });

    r("optsemantic", {},
    [](LangTree const& tree) {
      return Semantic { "", "" };
    });
    r("optsemantic", {":", "s?", "id", "s?", "optsemanticparens"},
    [](LangTree const& tree) {
      return Semantic { tree.children[2].text, std::any_cast<std::string>(tree.children[4].data) };
    });
    r("optsemanticparens", {},
    [](LangTree const& tree) {
      return std::string("");
    });
    r("optsemanticparens", {"lp", "s?", "id", "s?", "rp", "s?"},
    [](LangTree const& tree) {
      return tree.children[2].text;
    });

    r("structbody", {"lb", "s?", "declist?", "rb", "s?"});
    r("declist?", {},
    [](LangTree const& tree) {
      return DeclarationList { };
    });
    r("declist?", {"declist"});

    r("declmode", {});
    r("declmode", {"default"});
    r("declmode", {"structbody"});

    r("vardecl", {"idlist", "s?", "arrayqual", "optsemantic", "declmode", ";", "s?"},
    [](LangTree const& tree) {
      IDList ids = std::any_cast<IDList>(tree.children[0].data);

      try {
        StructDeclaration res;
        res.declarations = std::any_cast<DeclarationList>(tree.children[4].data);
        res.name = ids.back();
        res.type = ids.front();
        res.semantic = std::any_cast<Semantic>(tree.children[3].data);

        return AnyDeclaration(res);
      } catch (std::bad_any_cast const& ex) {
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
        } catch (std::bad_any_cast const&) {
          res.hasDefault = false;
        }

        return AnyDeclaration(res);
      } catch (std::bad_any_cast const& ex) {
        // Not a struct or regular declaration... this is actually bad
        throw std::runtime_error("Declaration isn't valid");
      }
    });

    r("param", {"idlist"});
    r("param", {"idlist", "s?", ":", "s?", "id"});
    r("paramlist", {"s?"});
    r("paramlist", {"param"});
    r("paramlist", {"paramlist", "anyop", "s?", "param"});

    r("fdecl", {"idlist", "s?", "lp", "paramlist", "rp", "s?", "scope", "s?"});
    r("fdecl", {"idlist", "s?", "lp", "paramlist", "rp", "s?", ":", "s?", "id", "s?", "scope", "s?"});
    r("allfdecl", {"fdecl"});

    r("anyop", {"op"});
    r("anyop", {","});

    r("any", {"lp"});
    r("any", {"rp"});
    r("any", {"lbr"});
    r("any", {"rbr"});
    r("any", {"="});
    r("any", {";"});
    r("any", {":"});
    r("any", {"anyop"});
    r("any", {"id"});
    r("any", {"float"});
    r("any", {"int"});
    r("any", {"scope"});
    r("any", {"space"});

    r("anylist", {"any"});
    r("anylist", {"anylist", "any"});

    r("s?", {});
    r("s?", {"space"});

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

class HLSLParser  : public parsegen::parser {
public:
  HLSL lang;

  HLSLParser(HLSL lang)
  : parsegen::parser(lang.tables)
  , lang(lang)
  { }
  virtual ~HLSLParser() = default;

protected:
  virtual std::any shift(int token, std::string& text) override {
    LangTree res = LangTree { text, {} };
    auto it = lang.tokenCallbacks.find(token);
    if (it != lang.tokenCallbacks.end()) {
      res.data = it->second(text);
    }
    return res;
  }
  virtual std::any reduce(int prod, std::vector<std::any>& rhs) override {
    std::string res = "";
    std::vector<LangTree> children;
    std::any unitary;
    bool unitaryError = false;
    for (int i = 0; i < rhs.size(); ++i) {
      LangTree t = std::any_cast<LangTree>(rhs[i]);
      res += t.text;
      if (t.data.has_value()) {
        if (unitary.has_value()) unitaryError = true;
        else unitary = t.data;
      }
      children.push_back(t);
    }
    LangTree resTree = { res, children };

    auto it = lang.prodCallbacks.find(prod);
    if (it != lang.prodCallbacks.end()) {
      resTree.data = it->second(resTree);
    } else if (unitaryError) {
      //throw std::runtime_error("Only one any can be propogated automatically");
    } else {
      resTree.data = unitary;
    }

    return resTree;
  }
};