#pragma once

#include <map>
#include <vector>
#include <string>
#include <format>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>

#include <glm/glm.hpp>

static void writeFile(std::filesystem::path path, std::string text) {
	std::ofstream file(path);
	file << text;
}

static std::string loadFile(std::filesystem::path path) {
	std::ifstream t(path);
	return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

static std::vector<uint8_t> loadFileBytes(std::filesystem::path path) {
	std::ifstream t(path, std::ios::binary);
	return std::vector<uint8_t>((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

namespace ReflectHLSL {
	struct DefinesContext {
		std::vector<std::string> Defines;

		void ReplaceDefine(std::string& inout) const;

		std::string GetDefs() const;

		std::string GetUndefs() const;
	};


	struct TestProgram {
		// Begin Generated Section
		
		//constexpr static ProgramDeclaration Program = {
		//	StructDeclaration { "struct", "BlahType", { "", "" }, {
		//		VarDeclaration { "", "vec4", "", "abc",		{ false, "" }, { "", "" }, false, { true, "0.0", { }}},
		//		VarDeclaration { "", "vec2", "", "uv",		{ false, "" }, { "", "" }, false, { true, "0.0", { }}},
		//		VarDeclaration { "", "float", "", "field",	{ false, "" }, { "", "" }, false, { true, "0.0", { }}},
		//		VarDeclaration { "", "int", "", "val",		{ false, "" }, { "", "" }, false, { true, "0",   { }}}
		//	} },
		//	VarDeclaration { "", "StructuredBuffer", "", "MyBlahs", { false, "" }, { "", "" }, false, { true, "0.0", { }}}
		//};
		
		constexpr static uint8_t Bytecode[3427] = { 0 };
		
		//struct BlahType {
		//	vec4 abc;
		//	vec2 uv;
		//	float field;
		//	int val;
		//};

		//struct Dispatch {
		//	StructuredBuffer<BlahType> MyBlahs;
		//};
		//
		// End Generated Section
	};
	
	struct GLMVectorConfig {
		template<int L,			typename T> struct Vector { using Type = glm::vec<L,    T>;	};
		template<int C, int R,	typename T> struct Matrix { using Type = glm::mat<C, R, T>;	};
	};

	struct BufferConfig {
		template<typename T> struct Buffer { struct Type {}; };
		template<typename T> struct RWBuffer { struct Type {}; };
	};

	struct GenerationContext {
		std::map<std::string, std::string> CBufferRegisterMap;
		std::map<std::string, std::string> VarRegisterMap;

		std::string Output;
	};

	std::string Generate(GenerationContext const& ctx, DefinesContext const& dctx);
}