#include <cmath>
#include <cstdlib>
#include <array>

#include "HLSL.hpp"

static std::filesystem::file_time_type lastWriteTime;

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

int ProcessFile(std::filesystem::path input, std::filesystem::path output = "") {
    if (output.empty()) {
        output = input;
        output += ".inl";
    }

    std::filesystem::path spvPath = input;
    spvPath += ".spv";

    // Early return if file is not out of date
    if (std::filesystem::exists(output)) {
        auto inputTime = std::filesystem::last_write_time(input);
        auto outputTime = std::filesystem::last_write_time(output);

        if (inputTime < outputTime && lastWriteTime < outputTime) {
            return 0;
        }
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
            }
            else if (d.index() == 1) {
                const ReflectHLSL::FDecl func = std::get<ReflectHLSL::FDecl>(d);

                const std::string returnType = func.returnType.Val;
                const std::string name = func.name.Val;

                ctx.Output += "\n\t\t// " + returnType;

                // Output all the parameter types
                for (auto param : func.params) {
                    ctx.Output += "\n\t\t// - " + param.typeName.Val + " " + param.name.Val;
                }

                if (!name.empty() && !currentInvokeSize.empty()) {
                    ctx.Output += "\n\t\tstatic constexpr uint3 " /*+ name +*/ "InvokeSize = " + currentInvokeSize;
                    currentInvokeSize.clear();
                }
            }
            else if (d.index() == 2) {
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
            }
            else {
                ctx.Output += "\t\t, ";
            }

            std::string shaderProfile;
            std::string resourceType;
            std::string resourceIndex;

            if (structuredVariables[i].semantic.has_value()) {
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

        // Try and load bytecode from the .spv file
        {
            std::vector<uint8_t> bytecode;
            if (std::filesystem::exists(spvPath)) {
				bytecode = loadFileBytes(spvPath);
			}

            const size_t size = bytecode.size();
            const size_t roundedUpSize = (size + 7) & ~7;

            // Pad with zeros to make it 8 byte aligned
            if (size != roundedUpSize) {
				bytecode.resize(roundedUpSize);
            }

            uint64_t* const bytecode64 = reinterpret_cast<uint64_t*>(bytecode.data());
            const size_t numU64s = roundedUpSize / 8;
            std::stringstream stream;

            for (size_t i = 0; i < numU64s; ++i) {
                if (i != 0) {
					stream << ", ";
                    if (i % 8 == 0) { // Visually nicer
                        stream << "\n\t\t\t";
                    }
				}
                stream << "0x" << std::hex << std::setw(16) << std::setfill('0') << bytecode64[i];// << "ull";
			}

            ctx.Output += "\n\t\tstatic constexpr size_t BytecodeSize = " + std::to_string(size) + ";";
            ctx.Output += "\n\t\tstatic constexpr uint8_t Bytecode[] = {\n\t\t\t" + stream.str() + "\n\t\t};\n";
        }

        writeFile(output, ReflectHLSL::Generate(ctx, dctx));

        std::cout << output.string() << std::endl;

        return 0;
    }
    catch (parsegen::parse_error const& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
    catch (std::exception const& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}

int ScanDir(std::filesystem::path scanDirectory) {
    // Process all files in the current directory
    int anyError = 0;
    for (auto& p : std::filesystem::recursive_directory_iterator(scanDirectory)) {
        const std::string extension = p.path().extension().string();
        if (p.is_regular_file() && (extension == ".vert" || extension == ".frag" || extension == ".comp")) {
            if (ProcessFile(p.path()))
            {
                std::cerr << "Failed to process " << p.path().string() << std::endl;
                anyError = 1;
            }
        }
    }

    return anyError;
}

int main(int argc, char** argv) {
    // Get time point for when this executable was updated
    {
        const std::filesystem::path executablePath = argv[0];
        lastWriteTime = std::filesystem::last_write_time(executablePath);
    }

    // If -scan is passed, scan the directory for files to process
    if (argc >= 2 && std::string(argv[1]) == "-scan") {
		std::filesystem::path scanDirectory = argc >= 3 ? std::string(argv[2]) : std::string();
        if (scanDirectory.empty()) {
            std::cerr << "No directory specified for -scan" << std::endl;
            return 1;
        }

        return ScanDir(scanDirectory);
	} else if (argc >= 2 && std::string(argv[1]) == "-file") {
        std::filesystem::path filePath = argc >= 3 ? std::string(argv[2]) : std::string();
        if (filePath.empty()) {
            std::cerr << "No file specified for -file" << std::endl;
			return 1;
        }
        return ProcessFile(filePath);
    }
    else
    {
        return ScanDir(std::filesystem::current_path());
    }
}
