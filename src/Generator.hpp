#pragma once

#include "MetaData.hpp"

#include <glm/glm.hpp>

namespace ReflectHLSL {
	template<
		typename VectorConfig,
		typename BufferConfig
	>
	struct Generator {
		using vec2	= VectorConfig::template Vector<2,    float>::Type;
		using vec3	= VectorConfig::template Vector<3,    float>::Type;
		using vec4	= VectorConfig::template Vector<4,    float>::Type;
		using ivec2	= VectorConfig::template Vector<2,  int32_t>::Type;
		using ivec3	= VectorConfig::template Vector<3,  int32_t>::Type;
		using ivec4	= VectorConfig::template Vector<4,  int32_t>::Type;
		using uvec2	= VectorConfig::template Vector<2, uint32_t>::Type;
		using uvec3	= VectorConfig::template Vector<3, uint32_t>::Type;
		using uvec4	= VectorConfig::template Vector<4, uint32_t>::Type;
		using dvec2	= VectorConfig::template Vector<2,   double>::Type;
		using dvec3	= VectorConfig::template Vector<3,   double>::Type;
		using dvec4	= VectorConfig::template Vector<4,   double>::Type;
		using mat4	= VectorConfig::template Matrix<4,	  float>::Type;

		template<typename T>
		using StructuredBuffer = BufferConfig::template Buffer<T>::Type;

		struct TestProgram {
			// Begin Generated Section
			
			constexpr static ProgramDeclaration Program = {
				StructDeclaration { "struct", "BlahType", { "", "" }, {
					VarDeclaration { "", "vec4", "", "abc",		{ false, "" }, { "", "" }, false, { true, "0.0", { }}},
					VarDeclaration { "", "vec2", "", "uv",		{ false, "" }, { "", "" }, false, { true, "0.0", { }}},
					VarDeclaration { "", "float", "", "field",	{ false, "" }, { "", "" }, false, { true, "0.0", { }}},
					VarDeclaration { "", "int", "", "val",		{ false, "" }, { "", "" }, false, { true, "0",   { }}}
				} },
				VarDeclaration { "", "StructuredBuffer", "", "MyBlahs", { false, "" }, { "", "" }, false, { true, "0.0", { }}}
			};
			
			constexpr static uint8_t Bytecode[3427] = { 0 };
			
			struct BlahType {
				vec4 abc;
				vec2 uv;
				float field;
				int val;
			};

			struct Dispatch {
				StructuredBuffer<BlahType> MyBlahs;
			};

			// End Generated Section
		};
	};

	struct GLMVectorConfig {
		template<int L,			typename T> struct Vector { using Type = glm::vec<L,    T>;	};
		template<int C, int R,	typename T> struct Matrix { using Type = glm::mat<C, R, T>;	};
	};

	struct BufferConfig {
		template<typename T> struct Buffer { struct Type {}; };
	};
}