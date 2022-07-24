#include <cmath>
#include <cstdlib>
#include <array>

#include "HLSL.hpp"

// Replace comments with spaces
std::string removeComments(std::string input) {
    std::string res = input;
    bool inCommentLine = false;
    bool inCommentBlock = false;

    auto check = [&](size_t i, char c) {return i < input.size() && input[i] == c; };

    for (size_t i = 0; i < input.size(); ++i) {
        if (check(i, '/')) {
            if (check(i + 1, '/')) {
                inCommentLine = true;
            }

            if (!inCommentLine && check(i + 1, '*')) {
                inCommentBlock = true;
            }
        }

        if (inCommentBlock && i >= 2 && check(i - 2, '*') && check(i - 1, '/')) {
            inCommentBlock = false;
        }
        if (check(i, '\n')) {
            inCommentLine = false;
        }

        if (inCommentLine || inCommentBlock) {
            res[i] = ' ';
        }
    }

    return res;
}

std::string removeDefines(ReflectHLSL::DefinesContext& ctx, std::string input) {
    std::string res = input;
    std::string currentMacro;
    bool inMacroLine = false;
    bool escapingNewline = false;

    auto check = [&](int i, char c) { return i >= 0 && i < static_cast<int>(input.size()) && input[i] == c; };

    for (int i = 0; i < static_cast<int>(input.size()); ++i) {
        if (check(i, '#')) {
            inMacroLine = true;
        } else if (!escapingNewline && check(i, '\\')) {
            escapingNewline = true;
        } else if (!escapingNewline && check(i, '\n')) {
            if (inMacroLine) {
                ctx.Defines.push_back(currentMacro);
                currentMacro.clear();
            }
            inMacroLine = false;
        } else {
            escapingNewline = false;
        }

        if (inMacroLine) {
            currentMacro.push_back(res[i]);
            res[i] = ' ';
        }
    }

    return res;
}

int main(int argc, char** argv) {
    std::string input = argc >= 2 ? std::string(argv[1]) : std::string();
    
    input = R"(./test/shaders2.hlsl)";

    if (input.empty()) {
        std::cerr << "No filename given" << std::endl;
        return 1;
    }

    try {
        parsegen::Parser<ReflectHLSL::HLSL> parse;

        ReflectHLSL::DefinesContext dctx;

        std::string s = loadFile(input);
        s = removeDefines(dctx, s);
        s = removeComments(s);
        ReflectHLSL::Program p = parse.Parse(s);

        ReflectHLSL::GenerationContext ctx;
       

        std::string currentInvokeSize;
        std::vector<ReflectHLSL::VarDecl> structuredVariables;

        for (auto d : p.Val.Val) {
            if (d.index() == 0) {
                ReflectHLSL::VarDecl v = std::get<ReflectHLSL::VarDecl>(d);
                v.GetGeneration(ctx, 2);
                
                if (v.GetTypename() == "StructuredBuffer" ||
                    v.GetTypename() == "RWStructuredBuffer")
                {
                    structuredVariables.push_back(v);
                }
            } else if (d.index() == 1) {
                const std::string name = std::get<ReflectHLSL::FDecl>(d).name.Val;

                if (!name.empty()) {
                    ctx.Output += "\n\t\tstatic constexpr uint3 " /*+ name +*/ "InvokeSize = " + currentInvokeSize;
                    currentInvokeSize.clear();
                }
            } else if (d.index() == 2) {
                currentInvokeSize = "uint3(" +
                    std::get<ReflectHLSL::FunctionAttrib>(d).literals[0]->format() + ", " +
                    std::get<ReflectHLSL::FunctionAttrib>(d).literals[1]->format() + ", " +
                    std::get<ReflectHLSL::FunctionAttrib>(d).literals[2]->format() + ");\n";
            }
        }

        ctx.Output += "\n\t\tinline Program(Context& ctx)\n";
        for (size_t i = 0; i < structuredVariables.size(); ++i) {
            if (i == 0) {
                ctx.Output += "\t\t: ";
            } else {
                ctx.Output += "\t\t, ";
            }

            std::string shaderProfile;
            std::string resourceType;
            std::string resourceIndex;

            if(structuredVariables[i].semantic.has_value()) {
                auto semantic = *structuredVariables[i].semantic;
                if (semantic.parens.has_value()) {
                    auto params = semantic.parens->params;
                    shaderProfile = params[0].id.Val;

                    if (params.size() >= 2) {
                        resourceType = params[1].id.Val;

                        if (params[1].arr.has_value()) {
                            auto sizes = params[1].arr->Sizes;
                            resourceIndex = sizes[0];
                        }
                    }
                }
            }

            ctx.Output += structuredVariables[i].GetName() + "(ctx, \"" + structuredVariables[i].GetName() + "\"";
            
            if (!shaderProfile.empty()) {
                ctx.Output += ", \"" + shaderProfile + "\"";
            }

            if (!resourceType.empty()) {
                ctx.Output += ", \"" + resourceType + "\"";
            }

            if (!resourceIndex.empty()) {
                ctx.Output += ", " + resourceIndex;
            }

            ctx.Output += ")\n";
        }
        ctx.Output += "\t\t{ }\n";

        writeFile(R"(./test/Out.inl)", ReflectHLSL::Generate(ctx, dctx));
    } catch (parsegen::parse_error const& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    } catch (std::exception const& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
