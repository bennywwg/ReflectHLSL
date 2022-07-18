#pragma once

#include <map>
#include <vector>
#include <string>
#include <format>
#include <sstream>
#include <fstream>
#include <iostream>

#include <glm/glm.hpp>

static void writeFile(std::string path, std::string text) {
	std::ofstream file(path);
	file << text;
}

static std::string loadFile(std::string path) {
	std::ifstream t(path);
	return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

namespace ReflectHLSL {
	struct DefinesContext {
		std::vector<std::string> Defines;

		void ReplaceDefine(std::string& inout) const;

		std::string GetDefs() const;

		std::string GetUndefs() const;
	};

	template<
		typename VectorConfig,
		typename BufferConfig
	>
	struct Generator {
		using uint		= uint32_t;

		using float1	= float;
		using float2	= VectorConfig::template Vector<2,    float>::Type;
		using float3	= VectorConfig::template Vector<3,    float>::Type;
		using float4	= VectorConfig::template Vector<4,    float>::Type;
		using int1		= int;
		using int2		= VectorConfig::template Vector<2,  int32_t>::Type;
		using int3		= VectorConfig::template Vector<3,  int32_t>::Type;
		using int4		= VectorConfig::template Vector<4,  int32_t>::Type;
		using uint1		= uint;
		using uint2		= VectorConfig::template Vector<2, uint32_t>::Type;
		using uint3		= VectorConfig::template Vector<3, uint32_t>::Type;
		using uint4		= VectorConfig::template Vector<4, uint32_t>::Type;
		using double1	= double;
		using double2	= VectorConfig::template Vector<2,   double>::Type;
		using double3	= VectorConfig::template Vector<3,   double>::Type;
		using double4	= VectorConfig::template Vector<4,   double>::Type;
		using float4x4	= VectorConfig::template Matrix<4,	  float>::Type;

		template<typename T>
		using StructuredBuffer = BufferConfig::template Buffer<T>::Type;

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