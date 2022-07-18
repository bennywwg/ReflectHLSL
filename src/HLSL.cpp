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
    
    input = R"(C:\Users\benny\Build\ReflectHLSL\test\shaders2.hlsl)";

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

        for (auto d : p.Val.Val) {

            if (d.index() == 0) {
                std::get<ReflectHLSL::VarDecl>(d).GetGeneration(ctx, 2);
            } else if (d.index() == 1) {
                const std::string name = std::get<ReflectHLSL::FDecl>(d).name.Val;

                if (!name.empty()) {
                    ctx.Output += "\t\tstatic constexpr uint3 " + name + "InvokeSize = " + currentInvokeSize;
                    currentInvokeSize.clear();
                }
            } else if (d.index() == 2) {
                currentInvokeSize = "uint3(" +
                    std::get<ReflectHLSL::FunctionAttrib>(d).literals[0]->format() + ", " +
                    std::get<ReflectHLSL::FunctionAttrib>(d).literals[1]->format() + ", " +
                    std::get<ReflectHLSL::FunctionAttrib>(d).literals[2]->format() + ");\n";
            }
        }

        writeFile(R"(C:\Users\benny\Build\ReflectHLSL\test\Out.inl)", ReflectHLSL::Generate(ctx, dctx));
    } catch (parsegen::parse_error const& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    } catch (std::exception const& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
