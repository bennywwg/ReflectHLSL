#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <array>

#include "hlslp.hpp"

#include "Generator.hpp"

void writeFile(std::string path, std::string text) {
    std::ofstream file(path);
    file << text;
}

std::string loadFile(std::string path) {
    std::ifstream t(path);
    return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

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

std::string removeDefines(std::string input) {
    std::string res = input;
    bool inMacroLine = false;

    auto check = [&](size_t i, char c) {return i < input.size() && input[i] == c; };

    for (size_t i = 0; i < input.size(); ++i) {
        if (check(i, '#')) {
            inMacroLine = true;
        }
        else if (check(i, '\n')) {
            inMacroLine = false;
        }

        if (inMacroLine) {
            res[i] = ' ';
        }
    }

    return res;
}

int main(int argc, char** argv) {
    std::string input = argc >= 2 ? std::string(argv[1]) : std::string();

    if (input.empty()) {
        std::cerr << "No filename given" << std::endl;
        return 1;
    }

    ReflectHLSL::Parser parser = ReflectHLSL::Parser();
    try {
        std::string s = loadFile(input);
        s = removeDefines(s);
        s = removeComments(s);
        parser.parse_string(s, "input");
    }
    catch (std::exception const& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
